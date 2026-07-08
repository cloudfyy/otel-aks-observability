param(
  [Parameter(Mandatory = $true)]
  [string]$AcrLoginServer,

  [Parameter(Mandatory = $true)]
  [string]$ImageTag
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$acrName = $AcrLoginServer.Split('.')[0]
$image = "otelapipy:${ImageTag}"

Write-Host "Building and pushing image ${AcrLoginServer}/${image} with az acr build"
az acr build --registry $acrName --source-acr-auth-id "[caller]" --image $image --file (Join-Path $scriptDir "Dockerfile") $scriptDir
if ($LASTEXITCODE -ne 0) {
  throw "az acr build failed with exit code $LASTEXITCODE."
}

Write-Host "Done: ${AcrLoginServer}/${image}"
