import { trace } from '@opentelemetry/api'
import { getWebAutoInstrumentations } from '@opentelemetry/auto-instrumentations-web'
import { OTLPTraceExporter } from '@opentelemetry/exporter-trace-otlp-http'
import { registerInstrumentations } from '@opentelemetry/instrumentation'
import { Resource } from '@opentelemetry/resources'
import { BatchSpanProcessor } from '@opentelemetry/sdk-trace-base'
import { WebTracerProvider } from '@opentelemetry/sdk-trace-web'

const telemetryStateKey = '__otelUiTelemetryState__'

function resolveCollectorEndpoint() {
  return import.meta.env.VITE_OTEL_EXPORTER_OTLP_ENDPOINT || '/otlp/v1/traces'
}

function resolveEnvironmentName() {
  return import.meta.env.MODE || 'development'
}

function createTelemetryState() {
  const exporter = new OTLPTraceExporter({
    url: resolveCollectorEndpoint(),
  })

  const provider = new WebTracerProvider({
    resource: new Resource({
      'service.name': 'otel-ui',
      'service.version': '1.0.4',
      'deployment.environment.name': resolveEnvironmentName(),
    }),
    spanProcessors: [new BatchSpanProcessor(exporter)],
  })

  provider.register()

  registerInstrumentations({
    instrumentations: [
      getWebAutoInstrumentations({
        '@opentelemetry/instrumentation-fetch': {
          ignoreUrls: [/^\/otlp\/v1\/traces/],
          propagateTraceHeaderCorsUrls: [
            /^http:\/\/localhost:5041\/weatherforecast/,
            /^http:\/\/localhost:8000\/weatherforecast/,
            /^\/dotnet\/weatherforecast/,
            /^\/python\/weatherforecast/,
          ],
        },
        '@opentelemetry/instrumentation-xml-http-request': {
          ignoreUrls: [/^\/otlp\/v1\/traces/],
          propagateTraceHeaderCorsUrls: [
            /^http:\/\/localhost:5041\/weatherforecast/,
            /^http:\/\/localhost:8000\/weatherforecast/,
            /^\/dotnet\/weatherforecast/,
            /^\/python\/weatherforecast/,
          ],
        },
      }),
    ],
  })

  return {
    tracer: trace.getTracer('otel-ui'),
  }
}

function initializeTelemetry() {
  if (globalThis[telemetryStateKey]) {
    return globalThis[telemetryStateKey]
  }

  try {
    const state = createTelemetryState()
    globalThis[telemetryStateKey] = state
    return state
  } catch (error) {
    console.warn('OpenTelemetry initialization skipped for otel-ui.', error)
    const fallbackState = { tracer: trace.getTracer('otel-ui-fallback') }
    globalThis[telemetryStateKey] = fallbackState
    return fallbackState
  }
}

export const telemetry = initializeTelemetry()
export const uiTracer = telemetry.tracer