# OTel Development Deployment

[中文入口](../README.md) | [English Home](../README.en.md) | [中文文档名](README.dev.md)

## Files

- otle-gateway-myvalues.yaml: primary Collector values for development (single-collector).
- current-values.yaml: historical development values (rollback/comparison reference).
- myvalues.yaml: custom development values sample for experiments.
- inst-crd-dotnet.yaml: .NET auto-instrumentation CRD.
- inst-crd-python.yaml: Python auto-instrumentation CRD.
- otelapidemo-dotnet.yaml: .NET sample app manifest.
- otelapidemo-python.yaml: Python sample app manifest template (example-only for now; further validation required).
- certmgr-test.yaml: cert-manager test manifest for development validation.
- README.dev.md: Chinese development deployment guide.
- README.dev.en.md: this English development deployment guide.

## Prerequisites

1. Access to AKS cluster with kubectl and helm configured.
2. Namespace observability exists.
3. Application namespace exists (example: apps-dev).
4. If you use a private registry image for auto-instrumentation, configure imagePullSecrets in the application namespace.

## Deploy Order

1. Create and label application namespace.
2. Install or upgrade OpenTelemetry Operator and verify it is healthy.
3. Check whether `connection_string` in `otle-gateway-myvalues.yaml` is still a placeholder, and replace it with a real value first.
4. Deploy or upgrade single collector (development mode).
5. Apply Instrumentation CRDs.
6. Check whether `<ACR_LOGIN_SERVER>` in demo app yaml is still a placeholder, and replace it with a real value first.
7. Deploy test applications.
8. Verify collector pipeline counters and telemetry ingestion.

## Commands (bash)

```bash
# 1) Create and label app namespace
kubectl create namespace apps-dev --dry-run=client -o yaml | kubectl apply -f -
kubectl label namespace apps-dev otel-client=true --overwrite

# 2) Install or upgrade OpenTelemetry Operator (release name: opentelemetry-operator)
helm upgrade --install opentelemetry-operator open-telemetry/opentelemetry-operator \
  -n opentelemetry-operator-system --create-namespace
kubectl get pods -n opentelemetry-operator-system

# 3) Check whether connection_string is still a placeholder; replace it before deployment if prompted
grep -q 'connection_string: "<APP_INSIGHTS_CONNECTION_STRING>"' ./dev/otle-gateway-myvalues.yaml && echo "Replace <APP_INSIGHTS_CONNECTION_STRING> in ./dev/otle-gateway-myvalues.yaml before deployment" || echo "connection_string looks set"

# 4) Deploy single collector (release name: otel-collector)
helm upgrade --install otel-collector open-telemetry/opentelemetry-collector \
  -n observability --create-namespace \
  -f ./dev/otle-gateway-myvalues.yaml

# 5) Apply Instrumentation CRDs
kubectl apply -f ./dev/inst-crd-dotnet.yaml
kubectl apply -f ./dev/inst-crd-python.yaml

# 6) Check whether <ACR_LOGIN_SERVER> is still a placeholder in demo app yaml
grep -Eq 'image:[[:space:]]*<ACR_LOGIN_SERVER>/otelapidemo:latest' ./dev/otelapidemo-dotnet.yaml && echo "Replace <ACR_LOGIN_SERVER> in ./dev/otelapidemo-dotnet.yaml before deployment" || echo "dotnet demo image login server looks set"

# 7) Deploy sample apps
kubectl apply -n apps-dev -f ./dev/otelapidemo-dotnet.yaml
# Optional: Python manifest is example-only for now; enable after validation
# kubectl apply -n apps-dev -f ./dev/otelapidemo-python.yaml

# 8) Verify basic status
kubectl get pods -n observability
kubectl get deploy -n observability
kubectl get instrumentation -n observability
kubectl get pods -n apps-dev
```

## Commands (PowerShell)

```powershell
# 1) Create and label app namespace
kubectl create namespace apps-dev --dry-run=client -o yaml | kubectl apply -f -
kubectl label namespace apps-dev otel-client=true --overwrite

# 2) Install or upgrade OpenTelemetry Operator (release name: opentelemetry-operator)
helm upgrade --install opentelemetry-operator open-telemetry/opentelemetry-operator `
  -n opentelemetry-operator-system --create-namespace
kubectl get pods -n opentelemetry-operator-system

# 3) Check whether connection_string is still a placeholder; replace it before deployment if prompted
if (Select-String -Path ./dev/otle-gateway-myvalues.yaml -Pattern 'connection_string:\s*"<APP_INSIGHTS_CONNECTION_STRING>"' -Quiet) { Write-Host "Replace <APP_INSIGHTS_CONNECTION_STRING> in ./dev/otle-gateway-myvalues.yaml before deployment" } else { Write-Host "connection_string looks set" }

# 4) Deploy single collector (release name: otel-collector)
helm upgrade --install otel-collector open-telemetry/opentelemetry-collector `
  -n observability --create-namespace `
  -f ./dev/otle-gateway-myvalues.yaml

# 5) Apply Instrumentation CRDs
kubectl apply -f ./dev/inst-crd-dotnet.yaml
kubectl apply -f ./dev/inst-crd-python.yaml

# 6) Check whether <ACR_LOGIN_SERVER> is still a placeholder in demo app yaml
if (Select-String -Path ./dev/otelapidemo-dotnet.yaml -Pattern 'image:\s*<ACR_LOGIN_SERVER>/otelapidemo:latest' -Quiet) { Write-Host "Replace <ACR_LOGIN_SERVER> in ./dev/otelapidemo-dotnet.yaml before deployment" } else { Write-Host "dotnet demo image login server looks set" }

# 7) Deploy sample apps
kubectl apply -n apps-dev -f ./dev/otelapidemo-dotnet.yaml
# Optional: Python manifest is example-only for now; enable after validation
# kubectl apply -n apps-dev -f ./dev/otelapidemo-python.yaml

# 8) Verify basic status
kubectl get pods -n observability
kubectl get deploy -n observability
kubectl get instrumentation -n observability
kubectl get pods -n apps-dev

# 9) Collector pipeline counters (single collector)
$pod = kubectl get pods -n observability -l app.kubernetes.io/instance=otel-collector -o jsonpath='{.items[0].metadata.name}'
kubectl get --raw "/api/v1/namespaces/observability/pods/${pod}:8888/proxy/metrics" |
  Select-String -Pattern "otelcol_receiver_accepted_spans|otelcol_exporter_sent_spans|otelcol_receiver_accepted_log_records|otelcol_exporter_sent_log_records|otelcol_receiver_accepted_metric_points|otelcol_exporter_sent_metric_points"
```

## Annotation Examples

```yaml
metadata:
  annotations:
    instrumentation.opentelemetry.io/inject-dotnet: "observability/dotnet-auto"
```

```yaml
metadata:
  annotations:
    instrumentation.opentelemetry.io/inject-python: "observability/python-auto"
```

## Notes

- This development baseline uses a single collector deployment.
- Current dev values include debug and azuremonitor exporters for troubleshooting.
- If logs are not visible in Azure Monitor, first verify app-side log generation and collector sent/failed counters.
- `current-values.yaml` and `myvalues.yaml` are kept as historical/alternate values and are not referenced by default commands.
- Image fields in `otelapidemo-*.yaml` use the `<ACR_LOGIN_SERVER>` placeholder; inject the real ACR through local temporary replacement or environment variables during deployment, and do not write it back into committed manifests.
- `otelapidemo-python.yaml` is currently an example template only and has not completed full validation; validate in an isolated environment before enabling.
- For better CRD reuse, keep service-specific OTEL_SERVICE_NAME in application Deployment, not in shared Instrumentation CRD.
