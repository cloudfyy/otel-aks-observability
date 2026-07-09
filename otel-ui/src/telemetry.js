import { trace } from '@opentelemetry/api'
import { getWebAutoInstrumentations } from '@opentelemetry/auto-instrumentations-web'
import { OTLPTraceExporter } from '@opentelemetry/exporter-trace-otlp-http'
import { registerInstrumentations } from '@opentelemetry/instrumentation'
import { Resource } from '@opentelemetry/resources'
import { BatchSpanProcessor } from '@opentelemetry/sdk-trace-base'
import { WebTracerProvider } from '@opentelemetry/sdk-trace-web'
import { getTelemetryConfig } from './telemetry.config'

const telemetryStateKey = '__otelUiTelemetryState__'

function createTelemetryState() {
  // Read all OTEL knobs from the centralized config module.
  const telemetryConfig = getTelemetryConfig()
  const exporter = new OTLPTraceExporter({
    url: telemetryConfig.collectorEndpoint,
  })

  const provider = new WebTracerProvider({
    resource: new Resource(telemetryConfig.resourceAttributes),
    spanProcessors: [new BatchSpanProcessor(exporter)],
  })

  // Register provider before instrumentation so auto-instrumented spans use this provider.
  provider.register()

  registerInstrumentations({
    instrumentations: [
      getWebAutoInstrumentations({
        // Capture browser API calls while excluding OTLP self-export requests.
        '@opentelemetry/instrumentation-fetch': {
          ignoreUrls: telemetryConfig.instrumentation.ignoreUrls,
          propagateTraceHeaderCorsUrls:
            telemetryConfig.instrumentation.propagateTraceHeaderCorsUrls,
        },
        '@opentelemetry/instrumentation-xml-http-request': {
          ignoreUrls: telemetryConfig.instrumentation.ignoreUrls,
          propagateTraceHeaderCorsUrls:
            telemetryConfig.instrumentation.propagateTraceHeaderCorsUrls,
        },
      }),
    ],
  })

  return {
    tracer: trace.getTracer('otel-ui'),
  }
}

function initializeTelemetry() {
  // HMR/re-import safe guard: initialize OTEL exactly once per browser runtime.
  if (globalThis[telemetryStateKey]) {
    return globalThis[telemetryStateKey]
  }

  try {
    const state = createTelemetryState()
    globalThis[telemetryStateKey] = state
    return state
  } catch (error) {
    // Keep app functionality even if telemetry setup fails.
    console.warn('OpenTelemetry initialization skipped for otel-ui.', error)
    const fallbackState = { tracer: trace.getTracer('otel-ui-fallback') }
    globalThis[telemetryStateKey] = fallbackState
    return fallbackState
  }
}

export const telemetry = initializeTelemetry()
export const uiTracer = telemetry.tracer