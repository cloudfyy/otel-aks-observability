# OTel Version Baseline (Current Test)

Update this file before and after each upgrade test.

## Static Config Targets (from repo)

- Helm chart version target: `0.162.0`
- Gateway collector image target: `otel/opentelemetry-collector-contrib:0.154.0`
- Agent collector image target: `otel/opentelemetry-collector-contrib:0.154.0`
- Dotnet instrumentation CRD: `dotnet-auto-prod` (sampling `0.1`)
- Python instrumentation CRD: `python-auto-prod` (sampling `0.1`)

## Config Version Ledger

- Config version: `v1.0.2`
- Config updated date: `2026-07-05`
- Version marker format:
	- Helm values files: top-of-file comments `# config-version` and `# config-updated`
	- Kubernetes manifests: `metadata.annotations` with `otel.config/version` and `otel.config/updated`

### Files with version markers

- `./prod/gateway-values.prod.yaml`
- `./prod/agent-values.prod.yaml`
- `./prod/otel-agent-service.prod.yaml`
- `./prod/otel-agent-rbac.prod.yaml`
- `./prod/inst-crd-dotnet.prod.yaml`
- `./prod/inst-crd-python.prod.yaml`
- `./prod/networkpolicy.prod.yaml`
- `./prod/collector-tls.prod.yaml`

## Runtime Snapshot (fill from cluster)

- Snapshot time (UTC):
- Cluster/context:
- Namespace: `observability`

### Helm Releases

- otel-gateway chart/app version:
- otel-agent chart/app version:

### Running Images

- gateway deployment image:
- agent daemonset image:
- opentelemetry-operator image:
- cert-manager controller image:

### Kubernetes / Tooling

- Kubernetes server version:
- kubectl client version:
- helm version:

## Capture Commands (PowerShell)

```powershell
# Context and versions
kubectl config current-context
kubectl version --short
helm version

# Helm releases
helm list -n observability | Select-String -Pattern 'otel-gateway|otel-agent'

# Running images
kubectl get deploy -n observability otel-gateway-opentelemetry-collector -o jsonpath='{.spec.template.spec.containers[0].image}{"`n"}'
kubectl get ds -n observability otel-agent-opentelemetry-collector -o jsonpath='{.spec.template.spec.containers[0].image}{"`n"}'
kubectl get deploy -n opentelemetry-operator-system opentelemetry-operator -o jsonpath='{.spec.template.spec.containers[0].image}{"`n"}'
kubectl get deploy -n cert-manager cert-manager -o jsonpath='{.spec.template.spec.containers[0].image}{"`n"}'
```

## Capture Commands (bash)

```bash
# Context and versions
kubectl config current-context
kubectl version --short
helm version

# Helm releases
helm list -n observability | grep -E 'otel-gateway|otel-agent'

# Running images
kubectl get deploy -n observability otel-gateway-opentelemetry-collector -o jsonpath='{.spec.template.spec.containers[0].image}{"\n"}'
kubectl get ds -n observability otel-agent-opentelemetry-collector -o jsonpath='{.spec.template.spec.containers[0].image}{"\n"}'
kubectl get deploy -n opentelemetry-operator-system opentelemetry-operator -o jsonpath='{.spec.template.spec.containers[0].image}{"\n"}'
kubectl get deploy -n cert-manager cert-manager -o jsonpath='{.spec.template.spec.containers[0].image}{"\n"}'
```
