param(
  [string]$Namespace = "apps-dev",
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
  throw "Set ACR_LOGIN_SERVER, pass -AcrLoginServer, or create dev/.env.local from .env.example."
}

$manifestFiles = @(
  "otelapidemo-dotnet.yaml",
  "otelapidemo-python.yaml",
  "otelapidemo-cpp.yaml",
  "otel-ui.yaml",
  "otelapidemo-ingress.yaml",
  "otel-ui-ingress.yaml",
  "otel-ui-otlp-service.yaml",
  "otel-ui-otlp-ingress.yaml"
)

$rendered = ($manifestFiles | ForEach-Object {
  $path = Join-Path $PSScriptRoot $_
  Get-Content $path -Raw
}) -join "`n"

$rendered.Replace("<ACR_LOGIN_SERVER>", $AcrLoginServer) | kubectl apply -n $Namespace -f -
if ($LASTEXITCODE -ne 0) {
  throw "kubectl apply failed with exit code $LASTEXITCODE."
}