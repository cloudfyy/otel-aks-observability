param(
  [Parameter(Mandatory = $true)]
  [string]$AcrLoginServer,

  [Parameter(Mandatory = $true)]
  [string]$ImageTag,

  [switch]$SkipDeps
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$manifestPath = Join-Path $scriptDir "vcpkg.json"
$acrName = $AcrLoginServer.Split('.')[0]
$resolvedAcrLoginServer = (az acr show --name $acrName --query loginServer --output tsv).Trim()
if ([string]::IsNullOrWhiteSpace($resolvedAcrLoginServer)) {
  throw "Unable to resolve ACR login server for registry '$acrName'."
}

$image = "otelapicpp:${ImageTag}"
$manifestHash = (Get-FileHash -Algorithm SHA256 -Path $manifestPath).Hash.Substring(0, 12).ToLowerInvariant()
$depsImage = "otelapicpp-deps:${manifestHash}"

function Get-AcrImageDigest {
  param(
    [Parameter(Mandatory = $true)]
    [string]$RegistryName,
    [Parameter(Mandatory = $true)]
    [string]$ImageName
  )

  $digest = az acr repository show --name $RegistryName --image $ImageName --query digest --output tsv 2>$null
  if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($digest)) {
    return $null
  }

  return $digest.Trim()
}

function Write-LatestAcrRunSummary {
  param(
    [Parameter(Mandatory = $true)]
    [string]$RegistryName
  )

  $runJson = az acr task list-runs --registry $RegistryName --top 1 --output json | ConvertFrom-Json
  if ($runJson -and $runJson.Count -gt 0) {
    $run = $runJson[0]
    Write-Host "Latest run: id=$($run.runId) status=$($run.status) duration=$($run.runDuration)"
  }
}

Write-Host "ACR registry: ${resolvedAcrLoginServer} (name: ${acrName})"
Write-Host "Manifest hash (vcpkg.json): ${manifestHash}"
Write-Host "Deps image candidate: ${resolvedAcrLoginServer}/${depsImage}"

$existingDepsDigest = Get-AcrImageDigest -RegistryName $acrName -ImageName $depsImage
if ($existingDepsDigest) {
  Write-Host "Deps image cache hit: digest=${existingDepsDigest}"
}
else {
  Write-Host "Deps image cache miss: ${depsImage} not found in registry"
}

if ($SkipDeps -and -not $existingDepsDigest) {
  throw "-SkipDeps was set, but deps image '${resolvedAcrLoginServer}/${depsImage}' does not exist."
}

if (-not $SkipDeps -and -not $existingDepsDigest) {
  Write-Host "Building deps image ${resolvedAcrLoginServer}/${depsImage} with az acr build"
  $depsTimer = [System.Diagnostics.Stopwatch]::StartNew()
  az acr build --registry $acrName --source-acr-auth-id "[caller]" --image $depsImage --file (Join-Path $scriptDir "Dockerfile.deps") $scriptDir
  if ($LASTEXITCODE -ne 0) {
    throw "deps az acr build failed with exit code $LASTEXITCODE."
  }
  $depsTimer.Stop()
  Write-Host ("Deps build elapsed: {0:n1}s" -f $depsTimer.Elapsed.TotalSeconds)
  Write-LatestAcrRunSummary -RegistryName $acrName

  $existingDepsDigest = Get-AcrImageDigest -RegistryName $acrName -ImageName $depsImage
  if ($existingDepsDigest) {
    Write-Host "Deps image published: digest=${existingDepsDigest}"
  }
}
elseif (-not $SkipDeps -and $existingDepsDigest) {
  Write-Host "Skipping deps build due to cache hit"
}
else {
  Write-Host "Skipping deps build due to -SkipDeps"
}

Write-Host "Building and pushing image ${AcrLoginServer}/${image} with az acr build"
$appTimer = [System.Diagnostics.Stopwatch]::StartNew()
az acr build --registry $acrName --source-acr-auth-id "[caller]" --image $image --build-arg "DEPS_IMAGE=${resolvedAcrLoginServer}/${depsImage}" --file (Join-Path $scriptDir "Dockerfile") $scriptDir
if ($LASTEXITCODE -ne 0) {
  throw "az acr build failed with exit code $LASTEXITCODE."
}
$appTimer.Stop()
Write-Host ("App build elapsed: {0:n1}s" -f $appTimer.Elapsed.TotalSeconds)
Write-LatestAcrRunSummary -RegistryName $acrName

$appDigest = Get-AcrImageDigest -RegistryName $acrName -ImageName $image
if ($appDigest) {
  Write-Host "App image digest: ${appDigest}"
}

Write-Host "Done: ${resolvedAcrLoginServer}/${image}"
