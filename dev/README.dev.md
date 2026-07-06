# OTel 开发部署

[中文首页](../README.md) | [English Home](../README.en.md) | [English Doc](README.dev.en.md)

## 文件清单

- otle-gateway-myvalues.yaml：当前开发主用的 Collector values（单 Collector）。
- current-values.yaml：开发阶段历史 values 版本（可用于回滚/比对）。
- myvalues.yaml：开发阶段自定义 values 样例（可用于试验参数）。
- inst-crd-dotnet.yaml：.NET 自动注入 Instrumentation CRD。
- inst-crd-python.yaml：Python 自动注入 Instrumentation CRD。
- otelapidemo-dotnet.yaml：.NET 示例应用部署清单。
- otelapidemo-python.yaml：Python 示例应用清单模板（当前仅示例用途，尚需进一步测试）。
- certmgr-test.yaml：cert-manager 功能测试清单（开发验证用途）。
- README.dev.md：当前中文开发部署说明。
- README.dev.en.md：英文开发部署说明。

## 前置条件

1. 已具备 AKS 集群访问权限，并已正确配置 kubectl 与 helm。
2. `observability` 命名空间已存在。
3. 应用命名空间已存在（示例：`apps-dev`）。
4. 若自动注入镜像使用私有仓库，请在应用命名空间配置 imagePullSecrets。

## 部署顺序

1. 创建并标记应用命名空间。
2. 安装或升级 OpenTelemetry Operator，并确认状态正常。
3. 检查 `otle-gateway-myvalues.yaml` 中的 `connection_string` 是否仍为占位符，并先替换为真实值。
4. 部署或升级单 Collector（开发模式）。
5. 应用 Instrumentation CRD。
6. 检查 demo app yaml 中的 `<ACR_LOGIN_SERVER>` 是否仍为占位符，并先替换为真实值。
7. 部署示例应用。
8. 验证 Collector 管道计数器与遥测上报。

## 命令（bash）

```bash
# 1) 创建并标记应用命名空间
kubectl create namespace apps-dev --dry-run=client -o yaml | kubectl apply -f -
kubectl label namespace apps-dev otel-client=true --overwrite

# 2) 安装或升级 OpenTelemetry Operator（release 名称：opentelemetry-operator）
helm upgrade --install opentelemetry-operator open-telemetry/opentelemetry-operator \
  -n opentelemetry-operator-system --create-namespace
kubectl get pods -n opentelemetry-operator-system

# 3) 检查 connection_string 是否仍为占位符；如果输出提示，请先替换后再部署
grep -q 'connection_string: "<APP_INSIGHTS_CONNECTION_STRING>"' ./dev/otle-gateway-myvalues.yaml && echo "请先将 ./dev/otle-gateway-myvalues.yaml 中的 <APP_INSIGHTS_CONNECTION_STRING> 替换为真实值" || echo "connection_string 已设置"

# 4) 部署单 Collector（release 名称：otel-collector）
helm upgrade --install otel-collector open-telemetry/opentelemetry-collector \
  -n observability --create-namespace \
  -f ./dev/otle-gateway-myvalues.yaml

# 5) 应用 Instrumentation CRD
kubectl apply -f ./dev/inst-crd-dotnet.yaml
kubectl apply -f ./dev/inst-crd-python.yaml

# 6) 检查 demo app yaml 中的 <ACR_LOGIN_SERVER> 是否仍为占位符
grep -Eq 'image:[[:space:]]*<ACR_LOGIN_SERVER>/otelapidemo:latest' ./dev/otelapidemo-dotnet.yaml && echo "请先将 ./dev/otelapidemo-dotnet.yaml 中的 <ACR_LOGIN_SERVER> 替换为真实值" || echo "dotnet demo 镜像地址看起来已设置"

# 7) 部署示例应用
kubectl apply -n apps-dev -f ./dev/otelapidemo-dotnet.yaml
# 可选：Python 清单当前仅示例用途，建议在测试通过后再启用
# kubectl apply -n apps-dev -f ./dev/otelapidemo-python.yaml

# 8) 验证基础状态
kubectl get pods -n observability
kubectl get deploy -n observability
kubectl get instrumentation -n observability
kubectl get pods -n apps-dev
```

## 命令（PowerShell）

```powershell
# 1) 创建并标记应用命名空间
kubectl create namespace apps-dev --dry-run=client -o yaml | kubectl apply -f -
kubectl label namespace apps-dev otel-client=true --overwrite

# 2) 安装或升级 OpenTelemetry Operator（release 名称：opentelemetry-operator）
helm upgrade --install opentelemetry-operator open-telemetry/opentelemetry-operator `
  -n opentelemetry-operator-system --create-namespace
kubectl get pods -n opentelemetry-operator-system

# 3) 检查 connection_string 是否仍为占位符；如果输出提示，请先替换后再部署
if (Select-String -Path ./dev/otle-gateway-myvalues.yaml -Pattern 'connection_string:\s*"<APP_INSIGHTS_CONNECTION_STRING>"' -Quiet) { Write-Host "请先将 ./dev/otle-gateway-myvalues.yaml 中的 <APP_INSIGHTS_CONNECTION_STRING> 替换为真实值" } else { Write-Host "connection_string 已设置" }

# 4) 部署单 Collector（release 名称：otel-collector）
helm upgrade --install otel-collector open-telemetry/opentelemetry-collector `
  -n observability --create-namespace `
  -f ./dev/otle-gateway-myvalues.yaml

# 5) 应用 Instrumentation CRD
kubectl apply -f ./dev/inst-crd-dotnet.yaml
kubectl apply -f ./dev/inst-crd-python.yaml

# 6) 检查 demo app yaml 中的 <ACR_LOGIN_SERVER> 是否仍为占位符
if (Select-String -Path ./dev/otelapidemo-dotnet.yaml -Pattern 'image:\s*<ACR_LOGIN_SERVER>/otelapidemo:latest' -Quiet) { Write-Host "请先将 ./dev/otelapidemo-dotnet.yaml 中的 <ACR_LOGIN_SERVER> 替换为真实值" } else { Write-Host "dotnet demo 镜像地址看起来已设置" }

# 7) 部署示例应用
kubectl apply -n apps-dev -f ./dev/otelapidemo-dotnet.yaml
# 可选：Python 清单当前仅示例用途，建议在测试通过后再启用
# kubectl apply -n apps-dev -f ./dev/otelapidemo-python.yaml

# 8) 验证基础状态
kubectl get pods -n observability
kubectl get deploy -n observability
kubectl get instrumentation -n observability
kubectl get pods -n apps-dev

# 9) Collector 管道计数器（单 Collector）
$pod = kubectl get pods -n observability -l app.kubernetes.io/instance=otel-collector -o jsonpath='{.items[0].metadata.name}'
kubectl get --raw "/api/v1/namespaces/observability/pods/${pod}:8888/proxy/metrics" |
  Select-String -Pattern "otelcol_receiver_accepted_spans|otelcol_exporter_sent_spans|otelcol_receiver_accepted_log_records|otelcol_exporter_sent_log_records|otelcol_receiver_accepted_metric_points|otelcol_exporter_sent_metric_points"
```

## 注解示例

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

## 说明

- 当前开发基线采用单 Collector 部署。
- 现有 dev values 同时包含 debug 与 azuremonitor exporter，便于联调与排障。
- 如果在 Azure Monitor 中看不到日志，先检查应用侧是否实际产生日志，以及 collector 的 sent/failed 计数器。
- `current-values.yaml` 与 `myvalues.yaml` 作为历史/备用 values，不在默认命令中直接引用。
- `otelapidemo-*.yaml` 中的镜像地址使用占位符 `<ACR_LOGIN_SERVER>`；部署时请通过本地临时替换或环境变量注入真实 ACR，不要将真实 ACR 写回并提交到清单。
- `otelapidemo-python.yaml` 目前仅作为示例模板，尚未完成完整验证，建议先在独立环境回归测试后再启用。
- 为提升 CRD 复用性，建议将服务级 OTEL_SERVICE_NAME 放在应用 Deployment 中，而非共享 Instrumentation CRD。
