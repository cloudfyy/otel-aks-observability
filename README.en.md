# OTel Project Overview

[中文](README.md)

## What This Project Does

This directory contains an end-to-end OpenTelemetry implementation on AKS, from development validation to production deployment.

Main capabilities:

- Data pipeline: receives application traces, metrics, and logs through OTLP (gRPC/HTTP)
- Auto-instrumentation: injects .NET/Python instrumentation with OpenTelemetry Operator and Instrumentation CRDs
- Export target: sends telemetry to Azure Monitor / Application Insights
- Production hardening: provides agent + gateway layered architecture, gateway tail sampling, trace ID routing, mTLS certificates, network policies, and scaling/alert guidance
- Operations governance: provides version ledger, upgrade pre-checks, and validation commands

## Directory Structure

- [dev/](dev/)
  - Development and integration examples (primarily single Collector)
  - Includes .NET/Python Instrumentation, sample app manifests, and collector values
  - Reference: [dev/README.dev.en.md](dev/README.dev.en.md)
- [prod/](prod/)
  - Production baseline configuration (agent + gateway)
  - Includes NetworkPolicy, cert-manager certificates, production values, sample app deployment scripts, alerts, and version ledger
  - Reference: [prod/README.prod.en.md](prod/README.prod.en.md)
- [otelapipy/](otelapipy/)
  - Python Web API sample project (aligned with .NET weatherforecast behavior)
  - Includes containerization files and dev/prod auto-instrumentation deployment manifests
  - Reference: [otelapipy/README.md](otelapipy/README.md)
- [otelapidemo/](otelapidemo/)
  - Sample application project for validating auto-instrumentation and end-to-end telemetry flow

## Recommended Reading Order

1. Start with [dev/README.dev.en.md](dev/README.dev.en.md) to validate the single-collector setup.
2. Continue with [prod/README.prod.en.md](prod/README.prod.en.md) to deploy the production baseline (agent + gateway).
3. Before upgrades/changes, update [prod/version-baseline.current.md](prod/version-baseline.current.md) and run Upgrade Pre-Checks.

## Key Files

- Dev collector values: [dev/otel-gateway-myvalues.yaml](dev/otel-gateway-myvalues.yaml)
- Dev .NET Instrumentation CRD: [dev/inst-crd-dotnet.yaml](dev/inst-crd-dotnet.yaml)
- Dev Python Instrumentation CRD: [dev/inst-crd-python.yaml](dev/inst-crd-python.yaml)
- Prod gateway values: [prod/gateway-values.prod.yaml](prod/gateway-values.prod.yaml)
- Prod agent values: [prod/agent-values.prod.yaml](prod/agent-values.prod.yaml)
- Prod ingress-nginx values: [prod/ingress-nginx-values.prod.yaml](prod/ingress-nginx-values.prod.yaml)
- Prod gateway headless Service (tail sampling trace ID routing): [prod/otel-gateway-headless.prod.yaml](prod/otel-gateway-headless.prod.yaml)
- Prod agent OTLP entry Service: [prod/otel-agent-service.prod.yaml](prod/otel-agent-service.prod.yaml)
- Prod agent RBAC (k8sattributes permissions): [prod/otel-agent-rbac.prod.yaml](prod/otel-agent-rbac.prod.yaml)
- Prod .NET sample app manifest: [prod/apps/otelapidemo-dotnet.yaml](prod/apps/otelapidemo-dotnet.yaml)
- Prod Python sample app manifest: [prod/apps/otelapidemo-python.yaml](prod/apps/otelapidemo-python.yaml)
- Prod sample app Kustomize entrypoint: [prod/apps/kustomization.yaml](prod/apps/kustomization.yaml)
- Prod sample app deployment script (PowerShell): [prod/apps/deploy-apps.ps1](prod/apps/deploy-apps.ps1)
- Prod sample app deployment script (bash): [prod/apps/deploy-apps.sh](prod/apps/deploy-apps.sh)
- Dev sample app deployment script (PowerShell): [dev/deploy-apps.ps1](dev/deploy-apps.ps1)
- Dev sample app deployment script (bash): [dev/deploy-apps.sh](dev/deploy-apps.sh)
- Prod sample app local ACR config template: [prod/apps/.env.example](prod/apps/.env.example)
- Shared production Ingress for sample apps: [prod/apps/otelapidemo-ingress.prod.yaml](prod/apps/otelapidemo-ingress.prod.yaml)
- Python API source entry: [otelapipy/app/main.py](otelapipy/app/main.py)
- Python API container build file: [otelapipy/Dockerfile](otelapipy/Dockerfile)
- Python API dev deployment manifest: [dev/otelapidemo-python.yaml](dev/otelapidemo-python.yaml)
- Python API prod deployment manifest: [prod/apps/otelapidemo-python.yaml](prod/apps/otelapidemo-python.yaml)
- Prod network policy: [prod/networkpolicy.prod.yaml](prod/networkpolicy.prod.yaml)
- Prod certificate config: [prod/collector-tls.prod.yaml](prod/collector-tls.prod.yaml)
- Prod alert query guidance: [prod/alerts-kql.prod.md](prod/alerts-kql.prod.md)
- App Insights dashboard guide (English): [prod/appinsights-dashboard.prod.en.md](prod/appinsights-dashboard.prod.en.md)
- Prod version ledger: [prod/version-baseline.current.md](prod/version-baseline.current.md)

## Notes

- Dev configuration is optimized for observability and troubleshooting, and may include additional debug settings.
- Prod configuration is optimized for stability and security, with least privilege, retry/queue controls, certificates, and network isolation.
- Prod sample app manifests keep the `<ACR_LOGIN_SERVER>` placeholder; the real ACR is passed to deployment scripts through an ignored local file or environment variable and is not committed.
- The production guide includes troubleshooting steps, PowerShell/bash scripts, and final App Insights KQL verification (wait 3-10 minutes for ingestion).
- Pin image and chart versions; avoid using latest.
