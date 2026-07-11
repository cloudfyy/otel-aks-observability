import { trace, type Tracer } from '@opentelemetry/api'
import { getWebAutoInstrumentations } from '@opentelemetry/auto-instrumentations-web'
import { OTLPTraceExporter } from '@opentelemetry/exporter-trace-otlp-http'
import { registerInstrumentations } from '@opentelemetry/instrumentation'
import { Resource } from '@opentelemetry/resources'
import { BatchSpanProcessor } from '@opentelemetry/sdk-trace-base'
import { WebTracerProvider } from '@opentelemetry/sdk-trace-web'
import { getTelemetryConfig } from './telemetry.config'

const telemetryStateKey = '__otelUiTelemetryState__' as const

type TelemetryState = {
  tracer: Tracer
}

type OTelGlobalState = typeof globalThis & {
  [telemetryStateKey]?: TelemetryState
}

function createTelemetryState(): TelemetryState {
  // Read all OTEL knobs from the centralized config module.
  const telemetryConfig = getTelemetryConfig()
  const exporter = new OTLPTraceExporter({
    url: telemetryConfig.collectorEndpoint,
  })
  // Some OTEL packages in the dependency graph carry duplicate Resource types.
  const resource = new Resource(telemetryConfig.resourceAttributes)
  const providerConfigResource = resource as unknown as NonNullable<
    ConstructorParameters<typeof WebTracerProvider>[0]
  >['resource']

  const provider = new WebTracerProvider({
    resource: providerConfigResource,
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

function initializeTelemetry(): TelemetryState {
  const globalState = globalThis as OTelGlobalState

  // HMR/re-import safe guard: initialize OTEL exactly once per browser runtime.
  if (globalState[telemetryStateKey]) {
    return globalState[telemetryStateKey]
  }

  try {
    const state = createTelemetryState()
    globalState[telemetryStateKey] = state
    return state
  } catch (error) {
    // Keep app functionality even if telemetry setup fails.
    console.warn('OpenTelemetry initialization skipped for otel-ui.', error)
    const fallbackState: TelemetryState = { tracer: trace.getTracer('otel-ui-fallback') }
    globalState[telemetryStateKey] = fallbackState
    return fallbackState
  }
}

export const telemetry = initializeTelemetry()
export const uiTracer = telemetry.tracer
