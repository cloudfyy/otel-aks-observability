param(
  [string]$Namespace = "apps-prod",
  [string]$AcrLoginServer
)

$ErrorActionPreference = "Stop"

# Keep the real ACR login server out of committed manifests. The value can come
# from an explicit parameter, the environment, or the ignored local env file.
if (-not $AcrLoginServer -and $env:ACR_LOGIN_SERVER) {
  $AcrLoginServer = $env:ACR_LOGIN_SERVER
}

$localEnvPath = Join-Path $PSScriptRoot ".env.local"
if (-not $AcrLoginServer -and (Test-Path $localEnvPath)) {
  Get-Content $localEnvPath | ForEach-Object {
    if ($_ -match '^\s*ACR_LOGIN_SERVER\s*=\s*(.+?)\s*$') {
      $AcrLoginServer = $Matches[1]
    }
  }
}

if (-not $AcrLoginServer) {
  throw "Set ACR_LOGIN_SERVER, pass -AcrLoginServer, or create prod/apps/.env.local from .env.example."
}

# Render the shared Kustomize base, then replace the image placeholder only in
# the stream sent to kubectl. The repository files keep <ACR_LOGIN_SERVER>.
$rendered = kubectl kustomize $PSScriptRoot
if ($LASTEXITCODE -ne 0) {
  throw "kubectl kustomize failed with exit code $LASTEXITCODE."
}

$rendered.Replace("<ACR_LOGIN_SERVER>", $AcrLoginServer) | kubectl apply -n $Namespace -f -
if ($LASTEXITCODE -ne 0) {
  throw "kubectl apply failed with exit code $LASTEXITCODE."
}