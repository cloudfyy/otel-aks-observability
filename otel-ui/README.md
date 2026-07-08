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

You can override them with:

- `VITE_DOTNET_API_BASE_URL`
- `VITE_PYTHON_API_BASE_URL`

## Production build behavior

When `VITE_DOTNET_API_BASE_URL` and `VITE_PYTHON_API_BASE_URL` are not set, the production build targets ingress-relative paths:

- `.NET API`: `/dotnet/weatherforecast`
- `Python API`: `/python/weatherforecast`

This matches the AKS ingress manifests in `dev/` and `prod/apps/`.

## Container image

Build and push to ACR:

```powershell
./build-push-acr.ps1 -AcrLoginServer qiqiacr.azurecr.io -ImageTag 1.0.1
```

```bash
./build-push-acr.sh --acr-login-server qiqiacr.azurecr.io --image-tag 1.0.1
```
