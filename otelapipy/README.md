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
docker build -t <ACR_LOGIN_SERVER>/otelapipy:latest .
docker run --rm -p 8000:8000 <ACR_LOGIN_SERVER>/otelapipy:latest
```

## Kubernetes Manifests

- `deploy/otelapi-py.dev.yaml`: uses `observability/python-auto` annotation.
- `deploy/otelapi-py.prod.yaml`: uses `observability/python-auto-prod` annotation.

Both manifests use `<ACR_LOGIN_SERVER>` as image placeholder. Replace it before deployment.
