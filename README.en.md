# OTel Project Overview

[中文](README.md)

## What This Project Does

This directory contains an end-to-end OpenTelemetry implementation on AKS, from development validation to production deployment.

Main capabilities:

- Data pipeline: receives application traces, metrics, and logs through OTLP (gRPC/HTTP)
- Auto-instrumentation: injects .NET/Python instrumentation with OpenTelemetry Operator and Instrumentation CRDs
- Export target: sends telemetry to Azure Monitor / Application Insights
- Production hardening: provides agent + gateway layered architecture, mTLS certificates, network policies, and scaling/alert guidance
- Operations governance: provides version ledger, upgrade pre-checks, and validation commands

## Directory Structure

- dev/
  - Development and integration examples (primarily single Collector)
  - Includes .NET/Python Instrumentation, sample app manifests, and collector values
  - Reference: dev/README.dev.md
- prod/
  - Production baseline configuration (agent + gateway)
  - Includes NetworkPolicy, cert-manager certificates, production values, alerts, and version ledger
  - Reference: prod/README.prod.md
- otelapidemo/
  - Sample application project for validating auto-instrumentation and end-to-end telemetry flow

## Recommended Reading Order

1. Start with dev/README.dev.md to validate the single-collector setup.
2. Continue with prod/README.prod.md to deploy the production baseline (agent + gateway).
3. Before upgrades/changes, update prod/version-baseline.current.md and run Upgrade Pre-Checks.

## Key Files

- Dev collector values: dev/otle-gateway-myvalues.yaml
- Dev .NET Instrumentation CRD: dev/inst-crd-dotnet.yaml
- Dev Python Instrumentation CRD: dev/inst-crd-python.yaml
- Prod gateway values: prod/gateway-values.prod.yaml
- Prod agent values: prod/agent-values.prod.yaml
- Prod network policy: prod/networkpolicy.prod.yaml
- Prod certificate config: prod/collector-tls.prod.yaml
- Prod version ledger: prod/version-baseline.current.md

## Notes

- Dev configuration is optimized for observability and troubleshooting, and may include additional debug settings.
- Prod configuration is optimized for stability and security, with least privilege, retry/queue controls, certificates, and network isolation.
- Pin image and chart versions; avoid using latest.
