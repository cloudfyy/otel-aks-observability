# Dev Release Notes

## 2026-07-13

### Release Scope

- Environment: `apps-dev`
- Components: `otel-ui`, `otelapidemo-cpp`

### Version Changes

- `otel-ui`: `1.0.8` -> `1.0.9`
- `otelapicpp`: `1.0.11` -> `1.0.12`

### Key Changes

- Added frontend/backend trace debug toggles, now disabled by default:
  - UI: `VITE_TRACE_DEBUG_ENABLED` (default: `false`)
  - C++: `OTEL_TRACE_DEBUG_ENABLED` (default: `false`)
- Fixed trace context extraction stability on the C++ side and restored parent-child trace correlation from UI to C++.

### Build and Push

- UI image: `qiqiacr.azurecr.io/otel-ui:1.0.9`
  - ACR run: `ck1y`
  - digest: `sha256:38f67045b7cb0e272ae67c78510e9b1f227ffefe75ac38139d9cd332c2b9f3da`
- C++ image: `qiqiacr.azurecr.io/otelapicpp:1.0.12`
  - ACR run: `ck20`
  - digest: `sha256:20cc75ae8f5ebc1339f2ab249f3e0e0f93194ec4ed25b7add7426deafdf19b09`

### Deployment

- Deploy command: `./dev/deploy-apps.ps1 -AcrLoginServer qiqiacr.azurecr.io`
- Rollout verification:
  - `deployment/otelapidemo-cpp` successfully rolled out
  - `deployment/otel-ui` successfully rolled out
- Cluster image verification:
  - `otelapidemo-cpp => qiqiacr.azurecr.io/otelapicpp:1.0.12`
  - `otel-ui => qiqiacr.azurecr.io/otel-ui:1.0.9`
