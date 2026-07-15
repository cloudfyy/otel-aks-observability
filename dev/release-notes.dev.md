# Dev Release Notes

## 2026-07-13

### Release Scope

- 发布环境：`apps-dev`
- 发布组件：`otel-ui`、`otelapidemo-cpp`

### Version Changes

- `otel-ui`: `1.0.8` -> `1.0.9`
- `otelapicpp`: `1.0.11` -> `1.0.12`

### Key Changes

- 新增前后端 trace debug 开关，默认关闭：
  - UI: `VITE_TRACE_DEBUG_ENABLED`（默认 `false`）
  - C++: `OTEL_TRACE_DEBUG_ENABLED`（默认 `false`）
- 修复 C++ 侧 trace context 提取稳定性问题，恢复前端到 C++ 的父子链路关联。

### Build and Push

- UI image: `qiqiacr.azurecr.io/otel-ui:1.0.9`
  - ACR run: `ck1y`
  - digest: `sha256:38f67045b7cb0e272ae67c78510e9b1f227ffefe75ac38139d9cd332c2b9f3da`
- C++ image: `qiqiacr.azurecr.io/otelapicpp:1.0.12`
  - ACR run: `ck20`
  - digest: `sha256:20cc75ae8f5ebc1339f2ab249f3e0e0f93194ec4ed25b7add7426deafdf19b09`

### Deployment

- 部署命令：`./dev/deploy-apps.ps1 -AcrLoginServer qiqiacr.azurecr.io`
- rollout 校验结果：
  - `deployment/otelapidemo-cpp` successfully rolled out
  - `deployment/otel-ui` successfully rolled out
- 集群镜像确认：
  - `otelapidemo-cpp => qiqiacr.azurecr.io/otelapicpp:1.0.12`
  - `otel-ui => qiqiacr.azurecr.io/otel-ui:1.0.9`

## 2026-07-16

### Release Scope

- 发布环境：`observability`（Collector）
- 发布组件：`otel-collector`（Helm release）

### Key Changes

- Dev Collector 部署模式由 `daemonset` 调整为 Gateway 方式（`deployment`）。
- 移除 `file_log` 接收器及节点日志路径挂载，日志入口统一为 `otlp`。
- `k8s_attributes.pod_association` 增加 `connection` 作为优先关联来源。
- 同步更新开发文档与架构图，统一为 Gateway Collector 口径。

### Azure Monitor Configuration

- `dev/otel-gateway-myvalues.yaml` 继续保留占位符 `"<APP_INSIGHTS_CONNECTION_STRING>"`，避免明文凭据入库。
- 实际部署通过 Helm 参数注入已有有效 `azuremonitor.connection_string`，并完成生效校验。

### Deployment

- 部署命令（等效）：`helm upgrade --install otel-collector ... -f ./dev/otel-gateway-myvalues.yaml --set-string config.exporters.azuremonitor.connection_string=<REDACTED>`
- 升级结果：`otel-collector` revision `8`，状态 `deployed`。
- rollout 校验结果：`deployment/otel-collector-opentelemetry-collector` successfully rolled out。
