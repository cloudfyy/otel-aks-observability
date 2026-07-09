param(
  [Parameter(Mandatory = $true)]
  [string]$AcrLoginServer,

  [Parameter(Mandatory = $true)]
  [string]$ImageTag,

  [string]$OtelEnvironmentName
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$acrName = $AcrLoginServer.Split('.')[0]
$image = "otel-ui:${ImageTag}"

Write-Host "Building and pushing image ${AcrLoginServer}/${image} with az acr build"
$buildArgs = @(
  "--registry", $acrName,
  "--source-acr-auth-id", "[caller]",
  "--image", $image,
  "--file", (Join-Path $scriptDir "Dockerfile")
)

if ($OtelEnvironmentName) {
  $buildArgs += @("--build-arg", "VITE_OTEL_ENVIRONMENT_NAME=$OtelEnvironmentName")
}

$buildArgs += $scriptDir

az acr build @buildArgs
if ($LASTEXITCODE -ne 0) {
  throw "az acr build failed with exit code $LASTEXITCODE."
}

Write-Host "Done: ${AcrLoginServer}/${image}"