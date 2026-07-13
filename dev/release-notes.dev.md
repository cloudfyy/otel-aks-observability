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
