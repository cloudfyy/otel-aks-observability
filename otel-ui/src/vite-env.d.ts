/// <reference types="vite/client" />

interface ImportMetaEnv {
  readonly VITE_DOTNET_API_BASE_URL?: string
  readonly VITE_PYTHON_API_BASE_URL?: string
  readonly VITE_CPP_API_BASE_URL?: string
  readonly VITE_OTEL_EXPORTER_OTLP_ENDPOINT?: string
  readonly VITE_OTEL_SERVICE_NAME?: string
  readonly VITE_OTEL_SERVICE_VERSION?: string
  readonly VITE_OTEL_ENVIRONMENT_NAME?: string
  readonly VITE_OTEL_IGNORE_URLS?: string
  readonly VITE_OTEL_PROPAGATE_TRACE_HEADER_URLS?: string
}

interface ImportMeta {
  readonly env: ImportMetaEnv
}
