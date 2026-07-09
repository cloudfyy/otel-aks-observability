# otel-ui

React dashboard that calls the `.NET` and `Python` sample APIs and displays their live weather responses side by side.

## Local development

```bash
npm install
npm run dev
```

Defaults in development:

- `.NET API`: `http://localhost:5041/weatherforecast`
- `Python API`: `http://localhost:8000/weatherforecast`
- OTEL trace export: `/otlp/v1/traces` proxied by Vite to `http://localhost:4318`

You can override them with:

- `VITE_DOTNET_API_BASE_URL`
- `VITE_PYTHON_API_BASE_URL`

## Production build behavior

When `VITE_DOTNET_API_BASE_URL` and `VITE_PYTHON_API_BASE_URL` are not set, the production build targets ingress-relative paths:

- `.NET API`: `/dotnet/weatherforecast`
- `Python API`: `/python/weatherforecast`

This matches the AKS ingress manifests in `dev/` and `prod/apps/`.

The UI also emits browser spans through the same-origin `/otlp/v1/traces` endpoint. In AKS, that path is reverse-proxied to the Collector by the dedicated OTLP ingress manifests.

Optional OTEL env var:

- `VITE_OTEL_EXPORTER_OTLP_ENDPOINT`: override the browser trace export endpoint if needed
- `VITE_OTEL_SERVICE_NAME`: override `service.name` (default: `otel-ui`)
- `VITE_OTEL_SERVICE_VERSION`: override `service.version` (default: `1.0.4`)
- `VITE_OTEL_ENVIRONMENT_NAME`: override `deployment.environment.name` (default uses `MODE`)
- `VITE_OTEL_IGNORE_URLS`: comma-separated regex list for ignored URLs (default: `^/otlp/v1/traces`)
- `VITE_OTEL_PROPAGATE_TRACE_HEADER_URLS`: comma-separated regex list for trace header propagation

Telemetry defaults are centralized in `src/telemetry.config.js` so endpoint, resource attributes,
and instrumentation URL rules can be changed without editing telemetry bootstrap logic.

## Container image

Build and push to ACR:

```powershell
./build-push-acr.ps1 -AcrLoginServer qiqiacr.azurecr.io -ImageTag 1.0.5
```

For explicit OTEL environment labeling during build:

```powershell
./build-push-acr.ps1 -AcrLoginServer qiqiacr.azurecr.io -ImageTag 1.0.5 -OtelEnvironmentName dev
```

```bash
./build-push-acr.sh --acr-login-server qiqiacr.azurecr.io --image-tag 1.0.5
```

```bash
./build-push-acr.sh --acr-login-server qiqiacr.azurecr.io --image-tag 1.0.5 --otel-environment-name dev
```
