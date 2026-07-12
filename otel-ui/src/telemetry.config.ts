const DEFAULT_SERVICE_NAME = 'otel-ui'
const DEFAULT_SERVICE_VERSION = '1.0.6'
const DEFAULT_OTLP_ENDPOINT = '/otlp/v1/traces'
const DEFAULT_IGNORE_URLS = ['^/otlp/v1/traces']
const DEFAULT_PROPAGATE_TRACE_HEADER_URLS = [
  '^http://localhost:5041/weatherforecast',
  '^http://localhost:8000/weatherforecast',
  '^http://localhost:8082/weatherforecast',
  '^/dotnet/.*',
  '^/python/.*',
  '^/cpp/.*',
  '^https?://[^/]+/dotnet/.*',
  '^https?://[^/]+/python/.*',
  '^https?://[^/]+/cpp/.*',
]

interface TelemetryConfig {
  collectorEndpoint: string
  resourceAttributes: {
    'service.name': string
    'service.version': string
    'deployment.environment.name': string
  }
  instrumentation: {
    ignoreUrls: RegExp[]
    propagateTraceHeaderCorsUrls: RegExp[]
  }
}

function parseCsv(value: string | undefined, fallback: string[]): string[] {
  // Accept comma-separated env vars and fall back to built-ins when unset.
  if (!value) {
    return fallback
  }

  const parsed = value
    .split(',')
    .map((item) => item.trim())
    .filter(Boolean)

  return parsed.length > 0 ? parsed : fallback
}

function toRegexList(patterns: string[], configKey: string): RegExp[] {
  // Keep telemetry resilient by skipping invalid regex patterns instead of throwing.
  return patterns
    .map((pattern) => {
      try {
        return new RegExp(pattern)
      } catch (error) {
        console.warn(`Invalid regex in ${configKey}: ${pattern}`, error)
        return null
      }
    })
    .filter((item): item is RegExp => item !== null)
}

export function getTelemetryConfig(): TelemetryConfig {
  const env = import.meta.env

  const ignoreUrlPatterns = parseCsv(env.VITE_OTEL_IGNORE_URLS, DEFAULT_IGNORE_URLS)
  const propagateTraceHeaderUrlPatterns = parseCsv(
    env.VITE_OTEL_PROPAGATE_TRACE_HEADER_URLS,
    DEFAULT_PROPAGATE_TRACE_HEADER_URLS,
  )

  return {
    // Use same-origin OTLP endpoint by default to avoid CORS complexity in browsers.
    collectorEndpoint: env.VITE_OTEL_EXPORTER_OTLP_ENDPOINT || DEFAULT_OTLP_ENDPOINT,
    resourceAttributes: {
      'service.name': env.VITE_OTEL_SERVICE_NAME || DEFAULT_SERVICE_NAME,
      'service.version': env.VITE_OTEL_SERVICE_VERSION || DEFAULT_SERVICE_VERSION,
      // Vite production builds set MODE=production; override with VITE_OTEL_ENVIRONMENT_NAME when needed.
      'deployment.environment.name': env.VITE_OTEL_ENVIRONMENT_NAME || env.MODE || 'development',
    },
    instrumentation: {
      ignoreUrls: toRegexList(ignoreUrlPatterns, 'VITE_OTEL_IGNORE_URLS'),
      propagateTraceHeaderCorsUrls: toRegexList(
        propagateTraceHeaderUrlPatterns,
        'VITE_OTEL_PROPAGATE_TRACE_HEADER_URLS',
      ),
    },
  }
}
