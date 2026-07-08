# otelapi-py

Python Web API sample that mirrors the .NET weather forecast behavior in `otelapidemo` and is ready for OpenTelemetry auto-instrumentation in AKS.

## Endpoints

- `GET /weatherforecast`: returns 5 forecast records with random temperatures and summaries.

## Local Run

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
uvicorn app.main:app --host 0.0.0.0 --port 8000
```

PowerShell:

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
uvicorn app.main:app --host 0.0.0.0 --port 8000
```

## Docker

```bash
docker build -t <ACR_LOGIN_SERVER>/otelapipy:1.0.2 .
docker run --rm -p 8000:8000 <ACR_LOGIN_SERVER>/otelapipy:1.0.2
```

Build and push to ACR scripts:

```powershell
./build-push-acr.ps1 -AcrLoginServer qiqiacr.azurecr.io -ImageTag 1.0.2
```

```bash
./build-push-acr.sh --acr-login-server qiqiacr.azurecr.io --image-tag 1.0.2
```

## Kubernetes

- `../dev/otelapidemo-python.yaml`: development sample manifest.
- `../prod/apps/otelapidemo-python.yaml`: production sample manifest.

Development deployment should use `../dev/deploy-apps.ps1` or `../dev/deploy-apps.sh`.

Production deployment should use `../prod/apps/deploy-apps.ps1` or `../prod/apps/deploy-apps.sh`.

Both script sets inject the real ACR login server locally so it is not committed to manifests.
