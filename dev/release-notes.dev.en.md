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

## 2026-07-16

### Release Scope

- Environment: `observability` (Collector)
- Component: `otel-collector` (Helm release)

### Key Changes

- Switched dev Collector deployment mode from `daemonset` to Gateway style (`deployment`).
- Removed `file_log` receiver and node log path mounts; log ingestion is now OTLP-only.
- Added `connection` as the primary source in `k8s_attributes.pod_association`.
- Updated dev documentation and architecture diagrams to align with Gateway Collector terminology.

### Azure Monitor Configuration

- `dev/otel-gateway-myvalues.yaml` keeps the `"<APP_INSIGHTS_CONNECTION_STRING>"` placeholder to avoid committing plaintext secrets.
- Actual deployment injects a valid `azuremonitor.connection_string` via Helm override and verifies it at runtime.

### Deployment

- Deploy command (equivalent): `helm upgrade --install otel-collector ... -f ./dev/otel-gateway-myvalues.yaml --set-string config.exporters.azuremonitor.connection_string=<REDACTED>`
- Upgrade result: `otel-collector` revision `8`, status `deployed`.
- Rollout verification: `deployment/otel-collector-opentelemetry-collector` successfully rolled out.
