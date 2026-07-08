# OTel 生产部署

[中文首页](../README.md) | [English Home](../README.en.md) | [English Doc](README.prod.en.md)

## 文件清单

- networkpolicy.prod.yaml：生产网络策略（默认拒绝 + 必要放通）。
- collector-tls.prod.yaml：cert-manager 证书与 Issuer 资源（gateway/agent mTLS）。
- gateway-values.prod.yaml：gateway Collector Helm values（导出与扩展策略）。
- agent-values.prod.yaml：agent Collector Helm values（节点侧接入与转发）。
- ingress-nginx-values.prod.yaml：ingress-nginx Helm values（AKS Load Balancer TCP 健康探针）。
- otel-gateway-headless.prod.yaml：gateway headless Service（agent 按 traceID 路由至 gateway Pod）。
- otel-agent-service.prod.yaml：agent OTLP 稳定入口 Service（应用上报目标）。
- otel-agent-rbac.prod.yaml：agent 所需 RBAC（k8sattributes 读取权限）。
- inst-crd-dotnet.prod.yaml：生产 .NET 自动注入 Instrumentation CRD。
- inst-crd-python.prod.yaml：生产 Python 自动注入 Instrumentation CRD。
- apps/otelapidemo-dotnet.yaml：生产 .NET 示例应用清单（已内置生产注解）。
- apps/otelapidemo-python.yaml：生产 Python 示例应用清单（已内置生产注解）。
- apps/otel-ui.yaml：生产 React UI 清单。
- apps/kustomization.yaml：生产示例应用 Kustomize 入口（替换 ACR 镜像地址）。
- apps/otelapidemo-ingress.prod.yaml：生产 API 路由 Ingress（`/dotnet/*` 与 `/python/*`）。
- apps/otel-ui-ingress.prod.yaml：生产 UI 根路径 Ingress（`/`）。
- alerts-kql.prod.md：生产告警与 KQL 建议。
- appinsights-dashboard.prod.md：Application Insights 仪表盘（程序运行 + OTel 常用指标）建议。
- appinsights-dashboard.prod.en.md：Application Insights dashboard 英文说明。
- version-baseline.current.md：生产版本基线台账与变更记录。
- README.prod.md：当前中文生产部署说明。
- README.prod.en.md：英文生产部署说明。

## 前置条件

1. 已具备 AKS 集群访问权限，并已正确配置 `kubectl` 与 `helm`。
2. 集群中 `cert-manager` 已安装且状态正常。
3. 必要命名空间已存在（`observability`、`apps-prod`），且应用命名空间已标记 `otel-client=true`。
4. `observability` 命名空间中已存在 App Insights 连接串密钥（`appinsights-conn`）。
5. RBAC 权限允许在 `observability` 命名空间读取并更新 release。
6. 集群已安装 NGINX Ingress Controller，且存在 `nginx` IngressClass；`apps/otelapidemo-ingress.prod.yaml` 依赖 NGINX rewrite 注解将 `/dotnet/*` 与 `/python/*` 改写到后端原始路径。在 AKS 上建议将 ingress-nginx Service 的 80 端口健康探针设置为 TCP，避免 Azure Load Balancer 使用 HTTP `/` 探针导致公网访问超时。
7. 部署前请将 `gateway-values.prod.yaml` 与 `agent-values.prod.yaml` 中的 `<AKS_CLUSTER_NAME>` 替换为实际 AKS 集群名称，用于标准化 `k8s.cluster.name` 资源属性。

## 部署顺序

1. 标记客户端命名空间并应用 NetworkPolicy。
2. 安装或升级 OpenTelemetry Operator，并确认状态正常。
3. 创建或更新 Application Insights 连接串密钥。
4. 应用 cert-manager TLS 清单，生成 gateway 与 agent 证书。
5. 部署 gateway collector（Deployment，多副本）。
6. 部署 agent collector（DaemonSet）。
7. 应用 agent Service 清单（为应用提供稳定 OTLP 入口）。
8. 应用 agent 的 RBAC 清单（k8sattributes 元数据提取权限）。
9. 应用 Instrumentation CRD。
10. 部署 `.NET`、Python 与 React UI 示例应用（Service 均使用 `ClusterIP`）。
11. 应用 API 路由 Ingress 与 UI 根路径 Ingress。
12. 验证统一入口、基础状态与 Collector 指标。

## 命令（bash）

```bash
# 1) 创建应用命名空间并放通 OTel 客户端流量
kubectl create namespace apps-prod --dry-run=client -o yaml | kubectl apply -f -
kubectl label namespace apps-prod otel-client=true --overwrite
kubectl apply -f ./prod/networkpolicy.prod.yaml

# 2) 安装或升级 OpenTelemetry Operator（release 名称：opentelemetry-operator）
helm upgrade --install opentelemetry-operator open-telemetry/opentelemetry-operator \
  --version 0.118.0 \
  -n opentelemetry-operator-system --create-namespace
kubectl get pods -n opentelemetry-operator-system

# 3) 创建密钥
kubectl create secret generic appinsights-conn \
  -n observability \
  --from-literal=connection_string="<APP_INSIGHTS_CONNECTION_STRING>" \
  --dry-run=client -o yaml | kubectl apply -f -

# 4) 通过 cert-manager 创建 TLS 证书与密钥
kubectl apply -f ./prod/collector-tls.prod.yaml

# 5) 部署 Gateway（release 名称：otel-gateway）
helm upgrade --install otel-gateway open-telemetry/opentelemetry-collector \
  --version 0.162.0 \
  -n observability --create-namespace \
  -f ./prod/gateway-values.prod.yaml

# 5.5) 应用 Gateway headless Service（agent 按 traceID 路由到 gateway Pod）
kubectl apply -f ./prod/otel-gateway-headless.prod.yaml

# 6) 部署 Agent（release 名称：otel-agent）
helm upgrade --install otel-agent open-telemetry/opentelemetry-collector \
  --version 0.162.0 \
  -n observability --create-namespace \
  -f ./prod/agent-values.prod.yaml

# 7) 应用 agent Service（稳定 OTLP 入口）
kubectl apply -f ./prod/otel-agent-service.prod.yaml

# 8) 应用 agent RBAC（k8sattributes 权限）
kubectl apply -f ./prod/otel-agent-rbac.prod.yaml

# 9) 应用 Instrumentation
kubectl apply -f ./prod/inst-crd-dotnet.prod.yaml
kubectl apply -f ./prod/inst-crd-python.prod.yaml

# 10) 部署 otelapidemo 示例应用（本地替换 ACR 镜像地址，不提交真实 ACR）
# 先设置 ACR_LOGIN_SERVER，或创建 prod/apps/.env.local（示例：ACR_LOGIN_SERVER=myacr.azurecr.io）
export ACR_LOGIN_SERVER="<ACR_LOGIN_SERVER>"
./prod/apps/deploy-apps.sh

# 10.5) 安装或更新 NGINX Ingress Controller（AKS 使用 TCP 健康探针）
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm repo update ingress-nginx
helm upgrade --install ingress-nginx ingress-nginx/ingress-nginx \
  --namespace ingress-nginx --create-namespace \
  -f ./prod/ingress-nginx-values.prod.yaml

# 11) 应用 API 路由 Ingress 与 UI 根路径 Ingress
kubectl apply -n apps-prod -f ./prod/apps/otelapidemo-ingress.prod.yaml
kubectl apply -n apps-prod -f ./prod/apps/otel-ui-ingress.prod.yaml

# 12) 验证
kubectl get pods -n observability
kubectl get deploy,ds -n observability
kubectl get svc -n observability otel-agent-opentelemetry-collector
kubectl get certificate -n observability
kubectl get pods -n apps-prod
kubectl get svc -n apps-prod otelapidemo otelapidemo-python otel-ui
kubectl get ingress -n apps-prod otelapidemo otel-ui
```

## 命令（PowerShell）

```powershell
# 1) 创建应用命名空间并放通 OTel 客户端流量
kubectl create namespace apps-prod --dry-run=client -o yaml | kubectl apply -f -
kubectl label namespace apps-prod otel-client=true --overwrite
kubectl apply -f ./prod/networkpolicy.prod.yaml

# 2) 安装或升级 OpenTelemetry Operator（release 名称：opentelemetry-operator）
helm upgrade --install opentelemetry-operator open-telemetry/opentelemetry-operator `
  --version 0.118.0 `
  -n opentelemetry-operator-system --create-namespace
kubectl get pods -n opentelemetry-operator-system

# 3) 创建密钥
kubectl create secret generic appinsights-conn `
  -n observability `
  --from-literal=connection_string="<APP_INSIGHTS_CONNECTION_STRING>" `
  --dry-run=client -o yaml | kubectl apply -f -

# 4) 通过 cert-manager 创建 TLS 证书与密钥
kubectl apply -f ./prod/collector-tls.prod.yaml

# 5) 部署 Gateway（release 名称：otel-gateway）
helm upgrade --install otel-gateway open-telemetry/opentelemetry-collector `
  --version 0.162.0 `
  -n observability --create-namespace `
  -f ./prod/gateway-values.prod.yaml

# 5.5) 应用 Gateway headless Service（agent 按 traceID 路由到 gateway Pod）
kubectl apply -f ./prod/otel-gateway-headless.prod.yaml

# 6) 部署 Agent（release 名称：otel-agent）
helm upgrade --install otel-agent open-telemetry/opentelemetry-collector `
  --version 0.162.0 `
  -n observability --create-namespace `
  -f ./prod/agent-values.prod.yaml

# 7) 应用 agent Service（稳定 OTLP 入口）
kubectl apply -f ./prod/otel-agent-service.prod.yaml

# 8) 应用 agent RBAC（k8sattributes 权限）
kubectl apply -f ./prod/otel-agent-rbac.prod.yaml

# 9) 应用 Instrumentation
kubectl apply -f ./prod/inst-crd-dotnet.prod.yaml
kubectl apply -f ./prod/inst-crd-python.prod.yaml

# 10) 部署 otelapidemo 示例应用（本地替换 ACR 镜像地址，不提交真实 ACR）
# 先设置 ACR_LOGIN_SERVER，或创建 prod/apps/.env.local（示例：ACR_LOGIN_SERVER=<ACR_LOGIN_SERVER>）
$env:ACR_LOGIN_SERVER = "<ACR_LOGIN_SERVER>"
./prod/apps/deploy-apps.ps1
# bash/zsh 可使用：./prod/apps/deploy-apps.sh

# 10.5) 安装或更新 NGINX Ingress Controller（AKS 使用 TCP 健康探针）
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm repo update ingress-nginx
helm upgrade --install ingress-nginx ingress-nginx/ingress-nginx `
  --namespace ingress-nginx --create-namespace `
  -f ./prod/ingress-nginx-values.prod.yaml

# 11) 应用 API 路由 Ingress 与 UI 根路径 Ingress
kubectl apply -n apps-prod -f ./prod/apps/otelapidemo-ingress.prod.yaml
kubectl apply -n apps-prod -f ./prod/apps/otel-ui-ingress.prod.yaml

# 12) 验证
kubectl get pods -n observability
kubectl get deploy,ds -n observability
kubectl get svc -n observability otel-agent-opentelemetry-collector
kubectl get instrumentation -n observability
kubectl get certificate -n observability
kubectl get pods -n apps-prod
kubectl get svc -n apps-prod otelapidemo otelapidemo-python otel-ui
kubectl get ingress -n apps-prod otelapidemo otel-ui

# 13) Collector 管道计数器（gateway）
$pod = kubectl get pods -n observability -l app.kubernetes.io/instance=otel-gateway -o jsonpath='{.items[0].metadata.name}'
kubectl get --raw "/api/v1/namespaces/observability/pods/${pod}:8888/proxy/metrics" |
  Select-String -Pattern "otelcol_receiver_accepted_spans|otelcol_exporter_sent_spans|otelcol_receiver_accepted_log_records|otelcol_exporter_sent_log_records|otelcol_receiver_accepted_metric_points|otelcol_exporter_sent_metric_points"

# 14) （可选）如你是从旧版 dev 清单迁移，才需要手工 patch 注解并重启
# 新的 ./prod/apps/otelapidemo-*.yaml 已内置生产注解，无需执行该步骤
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
- 生产 trace 使用 gateway tail sampling：应用侧 `always_on` 全量上报，gateway 保留错误 trace、超过 1000ms 的慢 trace，并对其余正常 trace 做 10% 概率采样。
- 当前 tail sampling 采用方案 B：gateway 保持多副本高可用，agent 的 traces pipeline 使用 `load_balancing/gateway` exporter 按 trace ID 路由到 headless Service 暴露的 gateway Pod，确保同一条 trace 的 span 进入同一个 tail sampler。
- Collector 会统一补充资源属性：`deployment.environment.name=prod`、`cloud.provider=azure`、`cloud.platform=azure_aks`、`k8s.cluster.name=<AKS_CLUSTER_NAME>`，并在未显式设置时从 `k8s.namespace.name` 补充 `service.namespace`。
- 示例应用会显式设置 `service.namespace=apps-prod`；当前示例镜像版本基线为 `.NET`=`1.0.4`、Python=`1.0.4`、UI=`1.0.1`。生产应用建议将 `service.version` 替换为真实发布版本或镜像版本。
- 应用通过服务 DNS `otel-agent-opentelemetry-collector.observability.svc.cluster.local:4317/4318` 上报到 agent。agent 的 traces pipeline 通过 `load_balancing/gateway` 和 gateway headless Service 按 trace ID 路由；metrics/logs pipeline 通过 `otlp_grpc/gateway` 发送到普通 gateway ClusterIP Service。
- 生产示例应用不直接暴露 `LoadBalancer` Service；`.NET`、Python 与 UI Service 均为 `ClusterIP`。
- 对外入口保持统一，但拆成两个 Ingress 资源：`apps/otel-ui-ingress.prod.yaml` 提供 `/`，`apps/otelapidemo-ingress.prod.yaml` 提供 `/dotnet/*` 与 `/python/*`。拆分的原因是 API Ingress 依赖 NGINX rewrite 注解，而 UI 根路径不应复用这组 rewrite 规则。
- 相同的 agent/gateway 架构同样适用于 Python 负载；差异仅在 Instrumentation CRD 与应用注解。
- `apps/otelapidemo-*.yaml` 中的镜像地址保留占位符 `<ACR_LOGIN_SERVER>`；部署脚本 `prod/apps/deploy-apps.ps1` 与 `prod/apps/deploy-apps.sh` 会从环境变量 `ACR_LOGIN_SERVER` 或本地忽略文件 `prod/apps/.env.local` 读取真实 ACR，并只在应用到集群前替换。
- 真实 ACR、订阅与发布配置不提交到仓库；如需本地发布配置，可从 `otelapidemo/otelapidemo/Properties/PublishProfiles/acr.pubxml.example` 复制为本地 `.pubxml` 文件后填写。
- 对 Python 来说，业务日志仍需应用主动输出；自动注入可开启 OTLP 日志导出，但不会自动产生日志内容。

## 方案 B 关键实现细节

- `load_balancing/gateway` 是 OpenTelemetry Collector Contrib 内置 exporter；`load_balancing` 是 exporter 类型，`gateway` 是本配置中的实例名。
- traces pipeline 通过 `routing_key: traceID` 指定按 OTLP span 原生 TraceID 路由。TraceID 不是 resource attribute，也不需要应用额外注入；exporter 会对 span 的 TraceID 做一致性哈希，并把同一条 trace 的 span 发到同一个 gateway Pod。
- `resolver.dns.hostname` 指向 `otel-gateway-opentelemetry-collector-headless.observability.svc.cluster.local`。该 Service 使用 `clusterIP: None`，DNS 返回 gateway Pod endpoint 列表，而不是普通 ClusterIP VIP。
- agent trace 路径会直接连接 gateway Pod IP。由于 gateway 服务端证书签发给普通 gateway Service DNS，而不是 Pod IP，TLS 配置必须保留 `server_name_override: otel-gateway-opentelemetry-collector.observability.svc.cluster.local`，否则可能出现证书名称不匹配。
- 当前 Collector `0.154.0` 的 `load_balancing` exporter 里，`protocol` 子字段仍使用 `otlp`。不要在这里改成 `otlp_grpc`；`otlp_grpc/gateway` 只用于 metrics/logs 普通转发 exporter。

## 排查步骤（访问应用后 AI 无数据）

1. 检查核心组件状态：`observability` 下 agent/gateway Pod 必须全部 `Running`。
2. 检查 OTLP 入口服务：`otel-agent-opentelemetry-collector` 必须存在且有 Endpoints。
3. 检查自动注入：.NET 应用 Pod 注解应为 `observability/dotnet-auto-prod`，并存在 `opentelemetry-auto-instrumentation-dotnet` initContainer；Python 应用 Pod 注解应为 `observability/python-auto-prod`，并存在 Python auto-instrumentation 注入相关的 initContainer/env。
4. 检查 Instrumentation：`dotnet-auto-prod` 与 `python-auto-prod` 的 endpoint/sampler 配置正确，且 sampler 为 `always_on`。Python 应使用 OTLP HTTP/protobuf 和 `4318` 端口。
5. 发送测试流量：对业务接口连续压测 50-200 次，并等待至少一个 tail sampling 决策窗口（当前 `decision_wait=10s`）后再查询。
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

#### Ingress 可访问，但 `/python/throw-custom-exception` 出现 302 跳转

现象：

- 集群内访问 Python Service（如 `http://otelapidemo-python.apps-prod.svc.cluster.local/throw-custom-exception`）返回 `500`，符合预期。
- 从公网访问 Ingress 地址（如 `http://<INGRESS_IP>/python/throw-custom-exception`）出现 `302`，且响应头 `Location` 指向其他外部地址。

结论：该跳转通常由公网链路侧注入（企业出口网关/运营商链路安全策略），不是应用代码、Pod、或 Ingress rewrite 规则主动返回。

说明：

- 这类公网重定向无法在应用代码中“关闭”。
- 处理方式是网络侧治理（企业网络白名单、运营商误报申诉）或入口治理（域名备案、WAF/CDN 合规配置等）。

生产架构下的建议排查顺序：

```powershell
# 1) 先确认 Ingress 路由在集群内是正常的（通过 ingress-nginx service）
kubectl run ingress-test --rm -i --restart=Never --image=curlimages/curl:8.11.1 -- curl -i --max-time 10 http://ingress-nginx-controller.ingress-nginx.svc.cluster.local/python/throw-custom-exception

# 2) 再确认后端 Service 直连是 500（排除应用侧问题）
kubectl run svc-test --rm -i --restart=Never --image=curlimages/curl:8.11.1 -n apps-prod -- curl -i --max-time 10 http://otelapidemo-python.apps-prod.svc.cluster.local/throw-custom-exception

# 3) 最后对公网地址做同一路径探测（若此处出现外部 302，基本可判定是公网链路侧重定向）
$ingressAddress = kubectl get ingress -n apps-prod otelapidemo -o jsonpath='{.status.loadBalancer.ingress[0].ip}'
curl.exe -i --max-time 10 ("http://{0}/python/throw-custom-exception" -f $ingressAddress)
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

### App Insights 异常查询 KQL（30 分钟）

- 建议先触发一轮异常接口（如 `/dotnet/throw-custom-exception`、`/python/throw-custom-exception`、`/dotnet/throw-and-catch-exception`、`/python/throw-and-catch-exception`），再等待 3-10 分钟查询。
- 先看 `exceptions` 总览，再与 `requests` 按 `operation_Id` 关联排查。

```kusto
// 1) 最近 30 分钟异常总览（prod）
exceptions
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
  or (
    tostring(customDimensions["service.namespace"]) =~ "apps-prod"
    and tostring(customDimensions["service.name"]) in~ ("otelapidemo", "otelapidemo-python")
  )
| project timestamp, cloud_RoleName, type, outerMessage, problemId, operation_Id
| order by timestamp desc
```

```kusto
// 2) 关联异常接口请求与异常记录（prod）
let Ex = exceptions
| where timestamp > ago(30m)
| project exTime=timestamp, operation_Id, exType=type, exMsg=outerMessage, exRole=cloud_RoleName;
requests
| where timestamp > ago(30m)
| where url has "throw-custom-exception" or url has "throw-and-catch-exception"
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
  or (
    tostring(customDimensions["service.namespace"]) =~ "apps-prod"
    and tostring(customDimensions["service.name"]) in~ ("otelapidemo", "otelapidemo-python")
  )
| project reqTime=timestamp, operation_Id, reqRole=cloud_RoleName, name, url, resultCode, success
| join kind=leftouter Ex on operation_Id
| order by reqTime desc
```

### App Insights Metrics 核验 KQL（30 分钟）

- OTel metrics 在 classic App Insights 中通常进入 `customMetrics` 表；workspace-based App Insights 中可能是 `AppMetrics` 表。下面的查询使用 `union isfuzzy=true` 同时兼容两种表。
- 如果查询结果为空，优先检查应用 Pod 的 `OTEL_METRICS_EXPORTER=otlp`、Collector `metrics` pipeline，以及 gateway 的 `azuremonitor` exporter。
- 指标名称会随语言、auto-instrumentation 版本和语义约定变化，建议先做发现查询，再按名称过滤。

发现最近入库的指标名称：

```kql
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(30m)
| summarize points=count(), lastSeen=max(timestamp) by name, cloud_RoleName
| order by points desc
```

按当前两个生产服务过滤指标：

```kql
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
  or (
    tostring(customDimensions["service.namespace"]) =~ "apps-prod"
    and tostring(customDimensions["service.name"]) in~ ("otelapidemo", "otelapidemo-python")
  )
| summarize points=count(), avgValue=avg(value), maxValue=max(value), lastSeen=max(timestamp) by name, cloud_RoleName
| order by points desc
```

常用应用指标筛选（HTTP、运行时、进程）：

```kql
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
| where name has_any ("http", "server", "request", "duration", "runtime", "process", "cpu", "memory", "gc", "thread")
| summarize points=count(), avgValue=avg(value), p95Value=percentile(value, 95), maxValue=max(value) by name, cloud_RoleName, bin(timestamp, 5m)
| order by timestamp desc, name asc
```

Collector metrics pipeline 自检：

```kql
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(30m)
| where name in~ (
  "otelcol_receiver_accepted_metric_points",
  "otelcol_exporter_sent_metric_points",
  "otelcol_receiver_refused_metric_points",
  "otelcol_exporter_send_failed_metric_points"
)
| summarize total=sum(value) by name, cloud_RoleName, bin(timestamp, 5m)
| order by timestamp desc, name asc
```

如果这些 Collector 指标在 App Insights 中为空，先在集群内确认 Collector 是否已经自抓取成功。当前 Collector 的 Prometheus 端点监听在 Pod IP 的 `8888` 端口，而不是 Pod 内 `127.0.0.1:8888`，因此 `kubectl port-forward` 可能报 `connect: connection refused`；应从集群内访问 `http://<collector-pod-ip>:8888/metrics` 验证。

如果 `customMetrics` 暂时没有应用指标，也可用 `requests` 表先观察请求量、失败数与延迟：

```kql
requests
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
| summarize requestCount=count(), failedCount=countif(success == false), avgDurationMs=avg(duration), p95DurationMs=percentile(duration, 95) by cloud_RoleName, bin(timestamp, 5m)
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
      AG1[Agent on Node 1\notlp receiver\nk8sattributes + resource standardize]
      AG2[Agent on Node 2\notlp receiver\nk8sattributes + resource standardize]
      AGN[Agent on Node N\notlp receiver\nk8sattributes + resource standardize]
    end

    subgraph Gateway[OTel Gateway Layer - Deployment]
      HSVC[(gateway headless Service\nDNS endpoints for Pods)]
      SVC[(gateway ClusterIP Service\nmetrics/logs path)]
      GW1[Gateway Pod 1\ntail_sampling + batch\nazuremonitor exporter]
      GW2[Gateway Pod 2\ntail_sampling + batch\nazuremonitor exporter]
      GW3[Gateway Pod 3\ntail_sampling + batch\nazuremonitor exporter]
    end

    HPA[HPA\nscale gateway replicas]
    PDB[PDB\nprotect availability]
  end

  AI[(Azure Monitor / Application Insights)]

  A1 --> AG1
  A2 --> AG2
  A3 --> AGN

  AG1 -- traces: load_balancing by traceID --> HSVC
  AG2 -- traces: load_balancing by traceID --> HSVC
  AGN -- traces: load_balancing by traceID --> HSVC

  AG1 -- metrics/logs: otlp_grpc --> SVC
  AG2 -- metrics/logs: otlp_grpc --> SVC
  AGN -- metrics/logs: otlp_grpc --> SVC

  HSVC -. resolves Pod endpoints .-> GW1
  HSVC -. resolves Pod endpoints .-> GW2
  HSVC -. resolves Pod endpoints .-> GW3
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

