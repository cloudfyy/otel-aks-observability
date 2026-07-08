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
4. Apply the RBAC required for the collector to read Kubernetes metadata.
5. Deploy or upgrade single collector (development mode).
6. Apply Instrumentation CRDs.
7. Check whether `<ACR_LOGIN_SERVER>` in demo app yaml is still a placeholder, and replace it with a real value first.
8. Deploy test applications.
9. Run a short load test against both the .NET and Python sample apps to generate traces, logs, and metrics.
10. Verify collector pipeline counters, Kubernetes resource attributes, and telemetry ingestion.

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

# 4) Apply RBAC required by k8sattributes
kubectl apply -f ./dev/otel-collector-k8sattributes-rbac.yaml

# 5) Deploy single collector (release name: otel-collector)
helm upgrade --install otel-collector open-telemetry/opentelemetry-collector \
  -n observability --create-namespace \
  -f ./dev/otle-gateway-myvalues.yaml

# 6) Apply Instrumentation CRDs
kubectl apply -f ./dev/inst-crd-dotnet.yaml
kubectl apply -f ./dev/inst-crd-python.yaml

# 7) Inject ACR and deploy sample apps through the deployment script (recommended)
export ACR_LOGIN_SERVER="myacr.azurecr.io"
./dev/deploy-apps.sh

# 9) Run a short load test (first target the .NET sample app)
demo_host=$(kubectl get svc -n apps-dev otelapidemo -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
if [ -z "$demo_host" ]; then
  demo_host=$(kubectl get svc -n apps-dev otelapidemo -o jsonpath='{.status.loadBalancer.ingress[0].hostname}')
fi
seq 1 200 | xargs -I{} -P 20 curl -fsS "http://${demo_host}/weatherforecast" > /dev/null

# 9b) Run the same load test against the Python sample app (only if it is deployed)
python_demo_host=$(kubectl get svc -n apps-dev otelapidemo-python -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
if [ -z "$python_demo_host" ]; then
  python_demo_host=$(kubectl get svc -n apps-dev otelapidemo-python -o jsonpath='{.status.loadBalancer.ingress[0].hostname}')
fi
seq 1 200 | xargs -I{} -P 20 curl -fsS "http://${python_demo_host}/weatherforecast" > /dev/null

# 10) Verify basic status
kubectl get pods -n observability
kubectl get deploy -n observability
kubectl get instrumentation -n observability
kubectl get pods -n apps-dev

# 11) Collector pipeline counters (single collector)
pod=$(kubectl get pods -n observability -l app.kubernetes.io/instance=otel-collector -o jsonpath='{.items[0].metadata.name}')
kubectl get --raw "/api/v1/namespaces/observability/pods/${pod}:8888/proxy/metrics" |
  grep -E "otelcol_receiver_accepted_spans|otelcol_exporter_sent_spans|otelcol_receiver_accepted_log_records|otelcol_exporter_sent_log_records|otelcol_receiver_accepted_metric_points|otelcol_exporter_sent_metric_points"
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

# 4) Apply RBAC required by k8sattributes
kubectl apply -f ./dev/otel-collector-k8sattributes-rbac.yaml

# 5) Deploy single collector (release name: otel-collector)
helm upgrade --install otel-collector open-telemetry/opentelemetry-collector `
  -n observability --create-namespace `
  -f ./dev/otle-gateway-myvalues.yaml

# 6) Apply Instrumentation CRDs
kubectl apply -f ./dev/inst-crd-dotnet.yaml
kubectl apply -f ./dev/inst-crd-python.yaml

# 7) Inject ACR and deploy sample apps through the deployment script (recommended)
$env:ACR_LOGIN_SERVER = "myacr.azurecr.io"
./dev/deploy-apps.ps1

# 9) Run a short load test (first target the .NET sample app)
$demoHost = kubectl get svc -n apps-dev otelapidemo -o jsonpath='{.status.loadBalancer.ingress[0].ip}'
if ([string]::IsNullOrWhiteSpace($demoHost)) {
  $demoHost = kubectl get svc -n apps-dev otelapidemo -o jsonpath='{.status.loadBalancer.ingress[0].hostname}'
}
1..200 | ForEach-Object { Invoke-WebRequest -Uri "http://${demoHost}/weatherforecast" -UseBasicParsing | Out-Null }

# 9b) Run the same load test against the Python sample app (only if it is deployed)
$pythonDemoHost = kubectl get svc -n apps-dev otelapidemo-python -o jsonpath='{.status.loadBalancer.ingress[0].ip}'
if ([string]::IsNullOrWhiteSpace($pythonDemoHost)) {
  $pythonDemoHost = kubectl get svc -n apps-dev otelapidemo-python -o jsonpath='{.status.loadBalancer.ingress[0].hostname}'
}
1..200 | ForEach-Object { Invoke-WebRequest -Uri "http://${pythonDemoHost}/weatherforecast" -UseBasicParsing | Out-Null }

# 10) Verify basic status
kubectl get pods -n observability
kubectl get deploy -n observability
kubectl get instrumentation -n observability
kubectl get pods -n apps-dev

# 11) Collector pipeline counters (single collector)
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
- The current dev collector enables the `k8sattributes` processor to append `k8s.*` resource attributes automatically, while application-side `OTEL_RESOURCE_ATTRIBUTES` continues to hold static labels such as environment.
- The load-test commands now cover both the .NET and Python sample apps on the `/weatherforecast` endpoint; skip the Python step if that sample app is not deployed.
- If logs are not visible in Azure Monitor, first verify app-side log generation and collector sent/failed counters.
- `current-values.yaml` and `myvalues.yaml` are kept as historical/alternate values and are not referenced by default commands.
- Image fields in `otelapidemo-*.yaml` use the `<ACR_LOGIN_SERVER>` placeholder; use `./dev/deploy-apps.ps1` or `./dev/deploy-apps.sh` to inject the real ACR during deployment, and do not write it back into committed manifests.
- `otelapidemo-python.yaml` is currently an example template only and has not completed full validation; validate in an isolated environment before enabling.
- For better CRD reuse, keep service-specific OTEL_SERVICE_NAME in application Deployment, not in shared Instrumentation CRD.

## KQL Validation

The following examples assume the Application Insights-compatible `traces` table with the `timestamp` column. If your environment uses workspace-based tables, replace `traces` with `AppTraces` and `timestamp` with `TimeGenerated`.

```kusto
// 1) Check whether k8s resource attributes are present in the last hour
traces
| where timestamp > ago(1h)
| extend
  service = tostring(customDimensions["service.name"]),
  ns = tostring(customDimensions["k8s.namespace.name"]),
  pod = tostring(customDimensions["k8s.pod.name"]),
  deployment = tostring(customDimensions["k8s.deployment.name"]),
  node = tostring(customDimensions["k8s.node.name"])
| where isnotempty(ns) or isnotempty(pod) or isnotempty(deployment) or isnotempty(node)
| project timestamp, service, ns, pod, deployment, node, message
| order by timestamp desc
| take 50
```

```kusto
// 2) Count which k8s fields already have values
traces
| where timestamp > ago(1h)
| summarize
  ns_count = countif(isnotempty(tostring(customDimensions["k8s.namespace.name"]))),
  pod_count = countif(isnotempty(tostring(customDimensions["k8s.pod.name"]))),
  deployment_count = countif(isnotempty(tostring(customDimensions["k8s.deployment.name"]))),
  container_count = countif(isnotempty(tostring(customDimensions["k8s.container.name"]))),
  node_count = countif(isnotempty(tostring(customDimensions["k8s.node.name"])))
```

```kusto
// 3) Check k8s metadata coverage by service
traces
| where timestamp > ago(1h)
| extend
  service = tostring(customDimensions["service.name"]),
  ns = tostring(customDimensions["k8s.namespace.name"]),
  pod = tostring(customDimensions["k8s.pod.name"])
| summarize
  total = count(),
  with_ns = countif(isnotempty(ns)),
  with_pod = countif(isnotempty(pod))
  by service
| order by total desc
```

```kusto
// 4) Confirm that the development environment label is dev
traces
| where timestamp > ago(1h)
| extend
  service = tostring(customDimensions["service.name"]),
  env = tostring(customDimensions["deployment.environment.name"])
| summarize count() by service, env
| order by count_ desc
```
