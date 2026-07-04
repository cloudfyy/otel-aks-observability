# OTel 生产部署

[中文首页](../README.md) | [English Home](../README.en.md) | [English Doc](README.prod.en.md)

## 文件清单

- networkpolicy.prod.yaml：生产网络策略（默认拒绝 + 必要放通）。
- collector-tls.prod.yaml：cert-manager 证书与 Issuer 资源（gateway/agent mTLS）。
- gateway-values.prod.yaml：gateway Collector Helm values（导出与扩展策略）。
- agent-values.prod.yaml：agent Collector Helm values（节点侧接入与转发）。
- ingress-nginx-values.prod.yaml：ingress-nginx Helm values（AKS Load Balancer TCP 健康探针）。
- otel-agent-service.prod.yaml：agent OTLP 稳定入口 Service（应用上报目标）。
- otel-agent-rbac.prod.yaml：agent 所需 RBAC（k8sattributes 读取权限）。
- inst-crd-dotnet.prod.yaml：生产 .NET 自动注入 Instrumentation CRD。
- inst-crd-python.prod.yaml：生产 Python 自动注入 Instrumentation CRD。
- otelapidemo-dotnet.yaml：生产 .NET 示例应用清单（已内置生产注解）。
- otelapidemo-python.yaml：生产 Python 示例应用清单（已内置生产注解）。
- otelapidemo-ingress.prod.yaml：生产示例应用共享 Ingress（.NET 与 Python 共用一个 Ingress 资源）。
- alerts-kql.prod.md：生产告警与 KQL 建议。
- version-baseline.current.md：生产版本基线台账与变更记录。
- README.prod.md：当前中文生产部署说明。
- README.prod.en.md：英文生产部署说明。

## 前置条件

1. 已具备 AKS 集群访问权限，并已正确配置 `kubectl` 与 `helm`。
2. 集群中 `cert-manager` 已安装且状态正常。
3. 必要命名空间已存在（`observability`、`apps-prod`），且应用命名空间已标记 `otel-client=true`。
4. `observability` 命名空间中已存在 App Insights 连接串密钥（`appinsights-conn`）。
5. RBAC 权限允许在 `observability` 命名空间读取并更新 release。
6. 集群已安装 NGINX Ingress Controller，且存在 `nginx` IngressClass；`otelapidemo-ingress.prod.yaml` 依赖 NGINX rewrite 注解将 `/dotnet/*` 与 `/python/*` 改写到后端原始路径。在 AKS 上建议将 ingress-nginx Service 的 80 端口健康探针设置为 TCP，避免 Azure Load Balancer 使用 HTTP `/` 探针导致公网访问超时。

## 部署顺序

1. 标记客户端命名空间并应用 NetworkPolicy。
2. 创建或更新 Application Insights 连接串密钥。
3. 应用 cert-manager TLS 清单，生成 gateway 与 agent 证书。
4. 部署 gateway collector（Deployment，多副本）。
5. 部署 agent collector（DaemonSet）。
6. 应用 agent Service 清单（为应用提供稳定 OTLP 入口）。
7. 应用 agent 的 RBAC 清单（k8sattributes 元数据提取权限）。
8. 应用 Instrumentation CRD。
9. 部署 otelapidemo 示例应用（.NET 与 Python Service 均使用 `ClusterIP`）。
10. 应用共享 Ingress，将 .NET 与 Python 放在同一个 Ingress 资源后。
11. 验证基础状态。

## 命令

```bash
# 1) 创建应用命名空间并放通 OTel 客户端流量
kubectl create namespace apps-prod --dry-run=client -o yaml | kubectl apply -f -
kubectl label namespace apps-prod otel-client=true --overwrite
kubectl apply -f ./prod/networkpolicy.prod.yaml

# 2) 创建密钥
kubectl create secret generic appinsights-conn \
  -n observability \
  --from-literal=connection_string="<APP_INSIGHTS_CONNECTION_STRING>" \
  --dry-run=client -o yaml | kubectl apply -f -

# 3) 通过 cert-manager 创建 TLS 证书与密钥
kubectl apply -f ./prod/collector-tls.prod.yaml

# 4) 部署 Gateway（release 名称：otel-gateway）
helm upgrade --install otel-gateway open-telemetry/opentelemetry-collector \
  --version 0.162.0 \
  -n observability --create-namespace \
  -f ./prod/gateway-values.prod.yaml

# 5) 部署 Agent（release 名称：otel-agent）
helm upgrade --install otel-agent open-telemetry/opentelemetry-collector \
  --version 0.162.0 \
  -n observability --create-namespace \
  -f ./prod/agent-values.prod.yaml

# 6) 应用 agent Service（稳定 OTLP 入口）
kubectl apply -f ./prod/otel-agent-service.prod.yaml

# 7) 应用 agent RBAC（k8sattributes 权限）
kubectl apply -f ./prod/otel-agent-rbac.prod.yaml

# 8) 应用 Instrumentation
kubectl apply -f ./prod/inst-crd-dotnet.prod.yaml
kubectl apply -f ./prod/inst-crd-python.prod.yaml

# 9) 部署 otelapidemo 示例应用（prod 清单，Service 使用 ClusterIP）
kubectl apply -n apps-prod -f ./prod/otelapidemo-dotnet.yaml
kubectl apply -n apps-prod -f ./prod/otelapidemo-python.yaml

# 9.5) 安装或更新 NGINX Ingress Controller（AKS 使用 TCP 健康探针）
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm repo update ingress-nginx
helm upgrade --install ingress-nginx ingress-nginx/ingress-nginx \
  --namespace ingress-nginx --create-namespace \
  -f ./prod/ingress-nginx-values.prod.yaml

# 10) 应用共享 Ingress（基于路径转发：/dotnet 与 /python）
kubectl apply -n apps-prod -f ./prod/otelapidemo-ingress.prod.yaml

# 11) 验证
kubectl get pods -n observability
kubectl get deploy,ds -n observability
kubectl get svc -n observability otel-agent-opentelemetry-collector
kubectl get certificate -n observability
kubectl get pods -n apps-prod
kubectl get svc -n apps-prod otelapidemo otelapidemo-python
kubectl get ingress -n apps-prod otelapidemo
```

## 命令（PowerShell）

```powershell
# 1) 创建应用命名空间并放通 OTel 客户端流量
kubectl create namespace apps-prod --dry-run=client -o yaml | kubectl apply -f -
kubectl label namespace apps-prod otel-client=true --overwrite
kubectl apply -f ./prod/networkpolicy.prod.yaml

# 2) 创建密钥
kubectl create secret generic appinsights-conn `
  -n observability `
  --from-literal=connection_string="<APP_INSIGHTS_CONNECTION_STRING>" `
  --dry-run=client -o yaml | kubectl apply -f -

# 3) 通过 cert-manager 创建 TLS 证书与密钥
kubectl apply -f ./prod/collector-tls.prod.yaml

# 4) 部署 Gateway（release 名称：otel-gateway）
helm upgrade --install otel-gateway open-telemetry/opentelemetry-collector `
  --version 0.162.0 `
  -n observability --create-namespace `
  -f ./prod/gateway-values.prod.yaml

# 5) 部署 Agent（release 名称：otel-agent）
helm upgrade --install otel-agent open-telemetry/opentelemetry-collector `
  --version 0.162.0 `
  -n observability --create-namespace `
  -f ./prod/agent-values.prod.yaml

# 6) 应用 agent Service（稳定 OTLP 入口）
kubectl apply -f ./prod/otel-agent-service.prod.yaml

# 7) 应用 agent RBAC（k8sattributes 权限）
kubectl apply -f ./prod/otel-agent-rbac.prod.yaml

# 8) 应用 Instrumentation
kubectl apply -f ./prod/inst-crd-dotnet.prod.yaml
kubectl apply -f ./prod/inst-crd-python.prod.yaml

# 9) 部署 otelapidemo 示例应用（prod 清单，Service 使用 ClusterIP）
kubectl apply -n apps-prod -f ./prod/otelapidemo-dotnet.yaml
kubectl apply -n apps-prod -f ./prod/otelapidemo-python.yaml

# 9.5) 安装或更新 NGINX Ingress Controller（AKS 使用 TCP 健康探针）
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm repo update ingress-nginx
helm upgrade --install ingress-nginx ingress-nginx/ingress-nginx `
  --namespace ingress-nginx --create-namespace `
  -f ./prod/ingress-nginx-values.prod.yaml

# 10) 应用共享 Ingress（基于路径转发：/dotnet 与 /python）
kubectl apply -n apps-prod -f ./prod/otelapidemo-ingress.prod.yaml

# 11) 验证
kubectl get pods -n observability
kubectl get deploy,ds -n observability
kubectl get svc -n observability otel-agent-opentelemetry-collector
kubectl get instrumentation -n observability
kubectl get certificate -n observability
kubectl get pods -n apps-prod
kubectl get svc -n apps-prod otelapidemo otelapidemo-python
kubectl get ingress -n apps-prod otelapidemo

# 12) Collector 管道计数器（gateway）
$pod = kubectl get pods -n observability -l app.kubernetes.io/instance=otel-gateway -o jsonpath='{.items[0].metadata.name}'
kubectl get --raw "/api/v1/namespaces/observability/pods/${pod}:8888/proxy/metrics" |
  Select-String -Pattern "otelcol_receiver_accepted_spans|otelcol_exporter_sent_spans|otelcol_receiver_accepted_log_records|otelcol_exporter_sent_log_records|otelcol_receiver_accepted_metric_points|otelcol_exporter_sent_metric_points"

# 13) （可选）如你是从旧版 dev 清单迁移，才需要手工 patch 注解并重启
# 新的 ./prod/otelapidemo-*.yaml 已内置生产注解，无需执行该步骤
```

## 应用注解示例

```yaml
metadata:
  annotations:
    instrumentation.opentelemetry.io/inject-dotnet: "observability/dotnet-auto-prod"
```

```yaml
metadata:
  annotations:
    instrumentation.opentelemetry.io/inject-python: "observability/python-auto-prod"
```

## 说明

- 当前生产基线已关闭 debug exporter，仅保留 azuremonitor。
- 采样率设置为 10%（`0.1`），用于生产成本控制。
- 应用通过服务 DNS `otel-agent-opentelemetry-collector.observability.svc.cluster.local:4317` 上报到 agent，再由 agent 转发至 gateway。
- 生产示例应用不直接暴露 `LoadBalancer` Service；`.NET` 与 Python Service 均为 `ClusterIP`，外部访问统一通过 `otelapidemo-ingress.prod.yaml` 中的共享 Ingress。
- 共享 Ingress 基于路径转发：`/dotnet/*` 转发到 `.NET` Service，`/python/*` 转发到 Python Service。NGINX rewrite 会去掉前缀，后端应用仍使用原始路径 `/weatherforecast`，无需修改应用代码。
- 相同的 agent/gateway 架构同样适用于 Python 负载；差异仅在 Instrumentation CRD 与应用注解。
- `otelapidemo-*.yaml` 中的镜像地址使用占位符 `<ACR_LOGIN_SERVER>`；部署前请替换为你的实际 ACR 登录地址。
- 对 Python 来说，业务日志仍需应用主动输出；自动注入可开启 OTLP 日志导出，但不会自动产生日志内容。

## 排查步骤（访问应用后 AI 无数据）

1. 检查核心组件状态：`observability` 下 agent/gateway Pod 必须全部 `Running`。
2. 检查 OTLP 入口服务：`otel-agent-opentelemetry-collector` 必须存在且有 Endpoints。
3. 检查自动注入：.NET 应用 Pod 注解应为 `observability/dotnet-auto-prod`，并存在 `opentelemetry-auto-instrumentation-dotnet` initContainer；Python 应用 Pod 注解应为 `observability/python-auto-prod`，并存在 Python auto-instrumentation 注入相关的 initContainer/env。
4. 检查 Instrumentation：`dotnet-auto-prod` 与 `python-auto-prod` 的 endpoint/sampler 配置正确，且 sampler 与预期一致。Python 应使用 OTLP HTTP/protobuf 和 `4318` 端口。
5. 发送测试流量：对业务接口连续压测 50-200 次，避免低采样率下偶发“全空”。
6. 检查 Collector 自监控：查看 agent/gateway 的 exporter queue 与 error 日志。
7. 验证 App Insights：先用无过滤 KQL 看近 30-60 分钟总量，再按 `cloud_RoleName` 或 `service.name` 过滤。

### 常见问题

#### Ingress 有公网 IP，但访问 `/dotnet/weatherforecast` 或 `/python/weatherforecast` 超时

现象：`kubectl get ingress -n apps-prod otelapidemo` 已显示公网地址，Ingress 规则和 Service Endpoints 都正常；从集群内部访问 `ingress-nginx-controller` Service 或直接访问后端 Service 返回 `200`，但从本机访问公网 IP 的 80 端口超时。

本次排查结论：AKS 为 ingress-nginx `LoadBalancer` Service 创建的 Azure Load Balancer 健康探针默认使用 HTTP `/`。ingress-nginx 在根路径 `/` 没有匹配规则时返回 `404`，Azure Load Balancer 会认为后端不健康，从而导致公网访问超时。后端应用、Ingress rewrite 与 Service Endpoints 本身并没有问题。

快速确认：

```powershell
kubectl get svc -n ingress-nginx ingress-nginx-controller -o wide
kubectl get ingress -n apps-prod otelapidemo -o wide

# 集群内部验证 Ingress Service 是否能转发到后端
kubectl run ingress-test --rm -i --restart=Never --image=curlimages/curl:8.11.1 -- curl -i --max-time 10 http://ingress-nginx-controller.ingress-nginx.svc.cluster.local/dotnet/weatherforecast

# 查看 Azure Load Balancer 探针协议与路径
$nodeRg = az aks show -g <AKS_RESOURCE_GROUP> -n <AKS_CLUSTER_NAME> --query nodeResourceGroup -o tsv
az network lb probe list -g $nodeRg --lb-name kubernetes --query "[].{name:name, protocol:protocol, port:port, requestPath:requestPath}" -o table
```

修复方式：使用 `prod/ingress-nginx-values.prod.yaml` 安装或升级 ingress-nginx，将 80 端口健康探针改为 TCP。

```bash
helm upgrade --install ingress-nginx ingress-nginx/ingress-nginx \
  --namespace ingress-nginx --create-namespace \
  -f ./prod/ingress-nginx-values.prod.yaml
```

期望的 Helm values：

```yaml
controller:
  service:
    type: LoadBalancer
    annotations:
      service.beta.kubernetes.io/port_80_health-probe_protocol: Tcp
```

修复后验证：

```powershell
$ingressAddress = kubectl get ingress -n apps-prod otelapidemo -o jsonpath='{.status.loadBalancer.ingress[0].ip}'
Test-NetConnection $ingressAddress -Port 80
curl.exe -i --max-time 10 "http://$ingressAddress/dotnet/weatherforecast"
curl.exe -i --max-time 10 "http://$ingressAddress/python/weatherforecast"
```

#### Python 自动注入后 App Insights 没有请求、依赖或日志

现象：应用接口可访问，Pod 日志中也能看到业务日志，但 App Insights 中按 Python 服务名查不到 `requests`、`dependencies` 或 `traces`。

常见原因：Python auto-instrumentation 镜像可能未安装 OTLP gRPC exporter 包。可在应用 Pod 中验证：

```powershell
$pod = kubectl get pods -n apps-prod -l app=otelapidemo-python -o jsonpath='{.items[0].metadata.name}'

kubectl exec -n apps-prod $pod -c otelapidemo-python -- python -m pip show opentelemetry-exporter-otlp-proto-grpc
kubectl exec -n apps-prod $pod -c otelapidemo-python -- python -m pip show opentelemetry-exporter-otlp-proto-http

kubectl exec -n apps-prod $pod -c otelapidemo-python -- python -c "import importlib.util; names=['opentelemetry.exporter.otlp.proto.grpc','opentelemetry.exporter.otlp.proto.http']; [print(name + ': ' + ('INSTALLED' if importlib.util.find_spec(name) else 'NOT INSTALLED')) for name in names]"
```

如果输出类似如下，说明当前 Python 自动注入环境不支持 OTLP gRPC，但支持 OTLP HTTP/protobuf：

```text
WARNING: Package(s) not found: opentelemetry-exporter-otlp-proto-grpc
Name: opentelemetry-exporter-otlp-proto-http
opentelemetry.exporter.otlp.proto.grpc: NOT INSTALLED
opentelemetry.exporter.otlp.proto.http: INSTALLED
```

修复方式：将 Python Instrumentation 改为 OTLP HTTP/protobuf，并使用 `4318` 端口：

```yaml
spec:
  exporter:
    endpoint: http://otel-agent-opentelemetry-collector.observability.svc.cluster.local:4318
  python:
    env:
      - name: OTEL_EXPORTER_OTLP_PROTOCOL
        value: http/protobuf
```

应用 Instrumentation 后重启 Python Deployment，再发送 50-200 次测试请求。App Insights 中的 `cloud_RoleName` 通常为 `apps-prod.otelapidemo-python`。

### App Insights 最终核验 KQL（30 分钟）

- 在发送测试流量、重启 Pod、或调整 Collector/Instrumentation 配置后，建议先等待 3-10 分钟再查询，以避免摄取延迟导致误判。
- Azure Monitor / App Insights 中的 `cloud_RoleName` 通常由 Kubernetes namespace 与服务名组合而成，例如 `.NET` 为 `apps-prod.otelapidemo`，Python 为 `apps-prod.otelapidemo-python`。
- 如果不同环境中的 `cloud_RoleName` 映射有差异，可使用 `customDimensions["service.namespace"]` 和 `customDimensions["service.name"]` 作为兜底过滤条件。

```kql
union requests, dependencies, traces
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
  or (
    tostring(customDimensions["service.namespace"]) =~ "apps-prod"
    and tostring(customDimensions["service.name"]) in~ ("otelapidemo", "otelapidemo-python")
  )
| order by timestamp desc
```

### 快速排查脚本（PowerShell）

```powershell
$nsObs = "observability"
$nsApp = "apps-prod"
$dotnetApp = "otelapidemo"
$pythonApp = "otelapidemo-python"
$svc = "otel-agent-opentelemetry-collector"
$nsIngress = "ingress-nginx"
$ingressName = "otelapidemo"
$ingressSvc = "ingress-nginx-controller"

Write-Host "== 1) 组件状态 =="
kubectl get pods -n $nsObs -o wide
kubectl get pods -n $nsApp -l app=$dotnetApp -o wide
kubectl get pods -n $nsApp -l app=$pythonApp -o wide

Write-Host "== 2) OTLP 入口服务 =="
kubectl get svc -n $nsObs $svc -o wide
kubectl get endpoints -n $nsObs $svc -o wide

Write-Host "== 3) Instrumentation 与应用注解 =="
kubectl get instrumentation -n $nsObs dotnet-auto-prod -o yaml
kubectl get instrumentation -n $nsObs python-auto-prod -o yaml
kubectl get deploy -n $nsApp $dotnetApp -o jsonpath='{.spec.template.metadata.annotations.instrumentation\.opentelemetry\.io/inject-dotnet}{"\n"}'
kubectl get deploy -n $nsApp $pythonApp -o jsonpath='{.spec.template.metadata.annotations.instrumentation\.opentelemetry\.io/inject-python}{"\n"}'
kubectl get pods -n $nsApp -l app=$dotnetApp -o jsonpath='{range .items[*]}{.metadata.name}{" initContainers="}{range .spec.initContainers[*]}{.name}{","}{end}{"\n"}{end}'
kubectl get pods -n $nsApp -l app=$pythonApp -o jsonpath='{range .items[*]}{.metadata.name}{" initContainers="}{range .spec.initContainers[*]}{.name}{","}{end}{"\n"}{end}'

Write-Host "== 4) 生产 Ingress 状态 =="
kubectl get ingressclass nginx
kubectl get pods -n $nsIngress -l app.kubernetes.io/component=controller -o wide
kubectl get svc -n $nsIngress $ingressSvc -o wide
kubectl get svc -n $nsApp $dotnetApp $pythonApp -o wide
kubectl get endpoints -n $nsApp $dotnetApp $pythonApp -o wide
kubectl get ingress -n $nsApp $ingressName -o wide
kubectl describe ingress -n $nsApp $ingressName
kubectl get svc -n $nsIngress $ingressSvc -o jsonpath='{.metadata.annotations.service\.beta\.kubernetes\.io/port_80_health-probe_protocol}{"\n"}'
kubectl run ingress-test --rm -i --restart=Never --image=curlimages/curl:8.11.1 -- sh -c "curl -i --max-time 10 http://$ingressSvc.$nsIngress.svc.cluster.local/dotnet/weatherforecast; echo; curl -i --max-time 10 http://$ingressSvc.$nsIngress.svc.cluster.local/python/weatherforecast"

Write-Host "== 5) 打测试流量 =="
$ingressAddress = kubectl get ingress -n $nsApp $ingressName -o jsonpath='{.status.loadBalancer.ingress[0].ip}'
if ([string]::IsNullOrEmpty($ingressAddress)) {
  $ingressAddress = kubectl get ingress -n $nsApp $ingressName -o jsonpath='{.status.loadBalancer.ingress[0].hostname}'
}
if ([string]::IsNullOrEmpty($ingressAddress)) {
  Write-Host "Ingress address not ready"
} else {
  Test-NetConnection $ingressAddress -Port 80
  1..100 | ForEach-Object {
    try { Invoke-WebRequest -Uri ("http://{0}/dotnet/weatherforecast" -f $ingressAddress) -UseBasicParsing -TimeoutSec 5 | Out-Null } catch {}
    try { Invoke-WebRequest -Uri ("http://{0}/python/weatherforecast" -f $ingressAddress) -UseBasicParsing -TimeoutSec 5 | Out-Null } catch {}
  }
  Write-Host "Traffic sent to http://$ingressAddress/dotnet/weatherforecast and /python/weatherforecast"
}

Write-Host "== 6) Collector 关键日志（近10分钟） =="
kubectl logs -n $nsObs -l app.kubernetes.io/instance=otel-agent --since=10m | Select-String -Pattern "forbidden|error|failed|otlp|gateway" 
kubectl logs -n $nsObs -l app.kubernetes.io/instance=otel-gateway --since=10m | Select-String -Pattern "error|failed|azuremonitor|export|401|403|404|429|5[0-9][0-9]"
```

### 快速排查脚本（bash）

```bash
set -euo pipefail

NS_OBS="observability"
NS_APP="apps-prod"
DOTNET_APP="otelapidemo"
PYTHON_APP="otelapidemo-python"
SVC="otel-agent-opentelemetry-collector"
NS_INGRESS="ingress-nginx"
INGRESS_NAME="otelapidemo"
INGRESS_SVC="ingress-nginx-controller"

echo "== 1) 组件状态 =="
kubectl get pods -n "$NS_OBS" -o wide
kubectl get pods -n "$NS_APP" -l app="$DOTNET_APP" -o wide
kubectl get pods -n "$NS_APP" -l app="$PYTHON_APP" -o wide

echo "== 2) OTLP 入口服务 =="
kubectl get svc -n "$NS_OBS" "$SVC" -o wide
kubectl get endpoints -n "$NS_OBS" "$SVC" -o wide

echo "== 3) Instrumentation 与应用注解 =="
kubectl get instrumentation -n "$NS_OBS" dotnet-auto-prod -o yaml
kubectl get instrumentation -n "$NS_OBS" python-auto-prod -o yaml
kubectl get deploy -n "$NS_APP" "$DOTNET_APP" -o jsonpath='{.spec.template.metadata.annotations.instrumentation\.opentelemetry\.io/inject-dotnet}{"\n"}'
kubectl get deploy -n "$NS_APP" "$PYTHON_APP" -o jsonpath='{.spec.template.metadata.annotations.instrumentation\.opentelemetry\.io/inject-python}{"\n"}'
kubectl get pods -n "$NS_APP" -l app="$DOTNET_APP" -o jsonpath='{range .items[*]}{.metadata.name}{" initContainers="}{range .spec.initContainers[*]}{.name}{","}{end}{"\n"}{end}'
kubectl get pods -n "$NS_APP" -l app="$PYTHON_APP" -o jsonpath='{range .items[*]}{.metadata.name}{" initContainers="}{range .spec.initContainers[*]}{.name}{","}{end}{"\n"}{end}'

echo "== 4) 生产 Ingress 状态 =="
kubectl get ingressclass nginx
kubectl get pods -n "$NS_INGRESS" -l app.kubernetes.io/component=controller -o wide
kubectl get svc -n "$NS_INGRESS" "$INGRESS_SVC" -o wide
kubectl get svc -n "$NS_APP" "$DOTNET_APP" "$PYTHON_APP" -o wide
kubectl get endpoints -n "$NS_APP" "$DOTNET_APP" "$PYTHON_APP" -o wide
kubectl get ingress -n "$NS_APP" "$INGRESS_NAME" -o wide
kubectl describe ingress -n "$NS_APP" "$INGRESS_NAME"
kubectl get svc -n "$NS_INGRESS" "$INGRESS_SVC" -o jsonpath='{.metadata.annotations.service\.beta\.kubernetes\.io/port_80_health-probe_protocol}{"\n"}'
kubectl run ingress-test --rm -i --restart=Never --image=curlimages/curl:8.11.1 -- sh -c "curl -i --max-time 10 http://${INGRESS_SVC}.${NS_INGRESS}.svc.cluster.local/dotnet/weatherforecast; echo; curl -i --max-time 10 http://${INGRESS_SVC}.${NS_INGRESS}.svc.cluster.local/python/weatherforecast"

echo "== 5) 打测试流量 =="
INGRESS_ADDRESS=$(kubectl get ingress -n "$NS_APP" "$INGRESS_NAME" -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
if [ -z "${INGRESS_ADDRESS}" ]; then
  INGRESS_ADDRESS=$(kubectl get ingress -n "$NS_APP" "$INGRESS_NAME" -o jsonpath='{.status.loadBalancer.ingress[0].hostname}')
fi
if [ -n "${INGRESS_ADDRESS}" ]; then
  for i in $(seq 1 100); do
    curl -sS "http://${INGRESS_ADDRESS}/dotnet/weatherforecast" >/dev/null || true
    curl -sS "http://${INGRESS_ADDRESS}/python/weatherforecast" >/dev/null || true
  done
  echo "Traffic sent to http://${INGRESS_ADDRESS}/dotnet/weatherforecast and /python/weatherforecast"
else
  echo "Ingress address not ready"
fi

echo "== 6) Collector 关键日志（近10分钟） =="
kubectl logs -n "$NS_OBS" -l app.kubernetes.io/instance=otel-agent --since=10m | egrep -i "forbidden|error|failed|otlp|gateway" || true
kubectl logs -n "$NS_OBS" -l app.kubernetes.io/instance=otel-gateway --since=10m | egrep -i "error|failed|azuremonitor|export|401|403|404|429|5[0-9][0-9]" || true
```

## Collector 架构（生产）

```mermaid
flowchart LR
  subgraph AKS[AKS Cluster]
    subgraph Apps[Application Workloads]
      A1[App Pod 1\nOTel Auto-Instrumentation]
      A2[App Pod 2\nOTel Auto-Instrumentation]
      A3[App Pod N\nOTel Auto-Instrumentation]
    end

    subgraph Agent[OTel Agent Layer - DaemonSet]
      AG1[Agent on Node 1\nreceivers: otlp\nprocessors: memory_limiter,k8sattributes,batch]
      AG2[Agent on Node 2\nreceivers: otlp\nprocessors: memory_limiter,k8sattributes,batch]
      AGN[Agent on Node N\nreceivers: otlp\nprocessors: memory_limiter,k8sattributes,batch]
    end

    subgraph Gateway[OTel Gateway Layer - Deployment]
      GW1[Gateway Pod 1\nqueue+retry+batch]
      GW2[Gateway Pod 2\nqueue+retry+batch]
      GW3[Gateway Pod 3\nqueue+retry+batch]
      SVC[(otel-gateway Service\nClusterIP)]
    end

    HPA[HPA\nscale gateway replicas]
    PDB[PDB\nprotect availability]
  end

  AI[(Azure Monitor / Application Insights)]

  A1 --> AG1
  A2 --> AG2
  A3 --> AGN

  AG1 --> SVC
  AG2 --> SVC
  AGN --> SVC

  SVC --> GW1
  SVC --> GW2
  SVC --> GW3

  GW1 --> AI
  GW2 --> AI
  GW3 --> AI

  HPA -. manages .-> GW1
  HPA -. manages .-> GW2
  HPA -. manages .-> GW3
  PDB -. protects .-> GW1
  PDB -. protects .-> GW2
  PDB -. protects .-> GW3
```

## 证书关系

```mermaid
flowchart LR
  SS[Issuer: otel-selfsigned]
  RCA[Certificate: otel-root-ca\nSecret: otel-root-ca\nRole: Root CA]
  CAI[Issuer: otel-ca-issuer\nuses Secret: otel-root-ca]
  GWC[Certificate: otel-gateway-tls\nSecret: otel-gateway-tls\nUsage: server auth]
  AGC[Certificate: otel-agent-client-tls\nSecret: otel-agent-client-tls\nUsage: client auth]
  GW[Gateway Collector\nmounts /etc/otel/tls]
  AG[Agent Collector\nmounts /etc/otel/certs]

  SS --> RCA
  RCA --> CAI
  CAI --> GWC
  CAI --> AGC
  GWC --> GW
  AGC --> AG

  AG -. validates gateway cert with CA .-> RCA
  GW -. validates agent cert with CA .-> RCA
```

## Collector 告警阈值建议

1. `otelcol_exporter_send_failed_* > 0` 持续 5 分钟：Sev2。
2. `otelcol_receiver_refused_* > 0` 持续 5 分钟：Sev2。
3. accepted 与 sent 计数器差值持续增长 10 分钟：Sev2。
4. exporter 队列使用率 > 70% 持续 10 分钟：Sev3。
5. exporter 队列使用率 > 90% 持续 5 分钟：Sev2。
6. Collector Pod 在 10 分钟内重启 >= 2 次：Sev2。
7. HPA 长时间（15 分钟以上）处于最大副本：Sev3（容量预警）。
8. 导出延迟 P95 > 5 秒持续 10 分钟：Sev3。

## 升级前检查（Upgrade Pre-Checks）

在执行任何 OTel 升级前，请先固化当前状态，确保回滚可确定执行。

0. 开始升级前，先在 `./prod/version-baseline.current.md` 更新当前测试软件版本（chart/image/operator/cert-manager/k8s/helm）。

1. 导出当前 release values（即第 2 项所需基线数据）：将集群当前生效配置保存为回滚基线。

```bash
mkdir -p ./prod/upgrade-baseline
helm get values otel-gateway -n observability -o yaml > ./prod/upgrade-baseline/otel-gateway.values.current.yaml
helm get values otel-agent -n observability -o yaml > ./prod/upgrade-baseline/otel-agent.values.current.yaml
```

```powershell
New-Item -ItemType Directory -Force -Path ./prod/upgrade-baseline | Out-Null
helm get values otel-gateway -n observability -o yaml | Out-File -Encoding utf8 ./prod/upgrade-baseline/otel-gateway.values.current.yaml
helm get values otel-agent -n observability -o yaml | Out-File -Encoding utf8 ./prod/upgrade-baseline/otel-agent.values.current.yaml
```

2. 记录当前 chart 版本 / 镜像 tag / operator 版本（第 3 项）。

```bash
# Chart 版本
helm list -n observability | grep -E 'otel-gateway|otel-agent'

# 当前运行的 Collector 镜像 tag
kubectl get deploy -n observability otel-gateway-opentelemetry-collector -o jsonpath='{.spec.template.spec.containers[0].image}{"\n"}'
kubectl get ds -n observability otel-agent-opentelemetry-collector -o jsonpath='{.spec.template.spec.containers[0].image}{"\n"}'

# Operator 版本（deployment 镜像）
kubectl get deploy -n opentelemetry-operator-system opentelemetry-operator -o jsonpath='{.spec.template.spec.containers[0].image}{"\n"}'
```

```powershell
# Chart 版本
helm list -n observability | Select-String -Pattern 'otel-gateway|otel-agent'

# 当前运行的 Collector 镜像 tag
kubectl get deploy -n observability otel-gateway-opentelemetry-collector -o jsonpath='{.spec.template.spec.containers[0].image}{"`n"}'
kubectl get ds -n observability otel-agent-opentelemetry-collector -o jsonpath='{.spec.template.spec.containers[0].image}{"`n"}'

# Operator 版本（deployment 镜像）
kubectl get deploy -n opentelemetry-operator-system opentelemetry-operator -o jsonpath='{.spec.template.spec.containers[0].image}{"`n"}'
```

3. 升级前执行一次基线验证：包括 traces、metrics、logs 管道计数器、App Insights 入库、collector 自监控指标，以及 HPA/PDB 状态。

