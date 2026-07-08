param(
  [Parameter(Mandatory = $true)]
  [string]$AcrLoginServer,

  [string]$ImageTag = "1.0.1"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildContext = Split-Path -Parent $scriptDir
$acrName = $AcrLoginServer.Split('.')[0]
$image = "otelapidemo:${ImageTag}"

Write-Host "Building and pushing image ${AcrLoginServer}/${image} with az acr build"
az acr build --registry $acrName --source-acr-auth-id "[caller]" --image $image --file (Join-Path $scriptDir "Dockerfile") $buildContext
if ($LASTEXITCODE -ne 0) {
  throw "az acr build failed with exit code $LASTEXITCODE."
}

Write-Host "Done: ${AcrLoginServer}/${image}"
