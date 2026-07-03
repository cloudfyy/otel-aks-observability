# OTel Development Deployment

## Files

- otle-gateway-myvalues.yaml
- inst-crd-dotnet.yaml
- inst-crd-python.yaml
- otelapidemo-dotnet.yaml
- otelapidemo-python.yaml
- certmgr-test.yaml

## Prerequisites

1. Access to AKS cluster with kubectl and helm configured.
2. Namespace observability exists.
3. Application namespace exists (example: apps-dev).
4. OpenTelemetry Operator is installed and healthy.
5. If you use a private registry image for auto-instrumentation, configure imagePullSecrets in the application namespace.

## Deploy Order

1. Create and label application namespace.
2. Deploy or upgrade single collector (development mode).
3. Apply Instrumentation CRDs.
4. Deploy test applications.
5. Verify collector pipeline counters and telemetry ingestion.

## Commands (bash)

```bash
# 1) Create and label app namespace
kubectl create namespace apps-dev --dry-run=client -o yaml | kubectl apply -f -
kubectl label namespace apps-dev otel-client=true --overwrite

# 2) Deploy single collector (release name: otel-collector)
helm upgrade --install otel-collector open-telemetry/opentelemetry-collector \
  -n observability --create-namespace \
  -f otel/dev/otle-gateway-myvalues.yaml

# 3) Apply Instrumentation CRDs
kubectl apply -f otel/dev/inst-crd-dotnet.yaml
kubectl apply -f otel/dev/inst-crd-python.yaml

# 4) Deploy sample apps
kubectl apply -n apps-dev -f otel/dev/otelapidemo-dotnet.yaml
kubectl apply -n apps-dev -f otel/dev/otelapidemo-python.yaml

# 5) Verify basic status
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

# 2) Deploy single collector (release name: otel-collector)
helm upgrade --install otel-collector open-telemetry/opentelemetry-collector `
  -n observability --create-namespace `
  -f otel/dev/otle-gateway-myvalues.yaml

# 3) Apply Instrumentation CRDs
kubectl apply -f otel/dev/inst-crd-dotnet.yaml
kubectl apply -f otel/dev/inst-crd-python.yaml

# 4) Deploy sample apps
kubectl apply -n apps-dev -f otel/dev/otelapidemo-dotnet.yaml
kubectl apply -n apps-dev -f otel/dev/otelapidemo-python.yaml

# 5) Verify basic status
kubectl get pods -n observability
kubectl get deploy -n observability
kubectl get instrumentation -n observability
kubectl get pods -n apps-dev

# 6) Collector pipeline counters (single collector)
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
- For better CRD reuse, keep service-specific OTEL_SERVICE_NAME in application Deployment, not in shared Instrumentation CRD.
