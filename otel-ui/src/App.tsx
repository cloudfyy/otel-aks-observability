import { useEffect, useState } from 'react'
import './App.css'

type SourceKey = 'dotnet' | 'python' | 'cpp'
type Accent = 'dotnet' | 'python' | 'cpp'

type ForecastRecord = {
  date: string
  temperatureC: number
  temperatureF: number
  summary: string
}

type SourceConfig = {
  label: string
  endpoint: string
  exceptionEndpoint: string
  accent: Accent
}

type TraceDebug = {
  requestTraceparent: string
  requestTraceId: string
  requestParentSpanId: string
  requestFlags: string
  serverTraceId: string
}

type PanelState = {
  data: ForecastRecord[]
  error: string
  loading: boolean
  traceDebug: TraceDebug | null
}

type ForecastState = {
  dotnet: PanelState
  python: PanelState
  cpp: PanelState
  updatedAt: string
}

const refreshOptions = [5, 10, 30, 60]
const exceptionRequestRate = 0.2
const traceDebugEnabled = import.meta.env.VITE_TRACE_DEBUG_ENABLED === 'true'

function resolveApiBaseUrl(
  envValue: string | undefined,
  developmentDefault: string,
  productionDefault: string,
): string {
  if (envValue) {
    return envValue
  }

  return import.meta.env.DEV ? developmentDefault : productionDefault
}

const sources: Record<SourceKey, SourceConfig> = {
  dotnet: {
    label: '.NET API',
    endpoint: `${resolveApiBaseUrl(import.meta.env.VITE_DOTNET_API_BASE_URL, 'http://localhost:5041', '/dotnet')}/weatherforecast`,
    exceptionEndpoint: `${resolveApiBaseUrl(import.meta.env.VITE_DOTNET_API_BASE_URL, 'http://localhost:5041', '/dotnet')}/weatherforecast/throw-custom-exception`,
    accent: 'dotnet',
  },
  python: {
    label: 'Python API',
    endpoint: `${resolveApiBaseUrl(import.meta.env.VITE_PYTHON_API_BASE_URL, 'http://localhost:8000', '/python')}/weatherforecast`,
    exceptionEndpoint: `${resolveApiBaseUrl(import.meta.env.VITE_PYTHON_API_BASE_URL, 'http://localhost:8000', '/python')}/throw-custom-exception`,
    accent: 'python',
  },
  cpp: {
    label: 'C++ API',
    endpoint: `${resolveApiBaseUrl(import.meta.env.VITE_CPP_API_BASE_URL, 'http://localhost:8082', '/cpp')}/weatherforecast`,
    exceptionEndpoint: `${resolveApiBaseUrl(import.meta.env.VITE_CPP_API_BASE_URL, 'http://localhost:8082', '/cpp')}/weatherforecast/throw-custom-exception`,
    accent: 'cpp',
  },
}

function formatTemperature(record: ForecastRecord): string {
  return `${record.temperatureC}°C / ${record.temperatureF}°F`
}

function formatDate(value: string): string {
  return new Date(value).toLocaleDateString('en-CA', {
    month: 'short',
    day: 'numeric',
    weekday: 'short',
  })
}

function normalizeErrorMessage(value: string | null | undefined): string {
  if (!value) {
    return ''
  }

  return value.replace(/\s+/g, ' ').trim().slice(0, 300)
}

async function buildRequestError(response: Response, sourceLabel: string): Promise<Error> {
  const contentType = response.headers.get('content-type') ?? ''
  let detail = ''

  if (contentType.includes('application/json')) {
    const payload: unknown = await response.json().catch(() => null)

    if (payload && typeof payload === 'object') {
      const payloadRecord = payload as Record<string, unknown>
      detail = normalizeErrorMessage(
        String(
          payloadRecord.detail ??
            payloadRecord.title ??
            payloadRecord.message ??
            JSON.stringify(payload),
        ),
      )
    }
  } else {
    detail = normalizeErrorMessage(await response.text())
  }

  return new Error(
    detail
      ? `${sourceLabel} returned ${response.status}: ${detail}`
      : `${sourceLabel} returned ${response.status}`,
  )
}

function resolveRequestEndpoint(source: SourceConfig): string {
  if (Math.random() < exceptionRequestRate) {
    return source.exceptionEndpoint
  }

  return source.endpoint
}

function readTraceDebugHeaders(response: Response): TraceDebug | null {
  if (!traceDebugEnabled) {
    return null
  }

  const traceparent = response.headers.get('x-otel-traceparent-in') || ''
  const traceId = response.headers.get('x-otel-trace-id-in') || ''
  const parentSpanId = response.headers.get('x-otel-parent-span-id-in') || ''
  const flags = response.headers.get('x-otel-trace-flags-in') || ''
  const serverTraceId = response.headers.get('x-otel-server-trace-id') || ''

  if (!traceparent && !traceId && !parentSpanId && !flags && !serverTraceId) {
    return null
  }

  return {
    requestTraceparent: traceparent,
    requestTraceId: traceId,
    requestParentSpanId: parentSpanId,
    requestFlags: flags,
    serverTraceId,
  }
}

type ForecastPanelProps = {
  label: string
  endpoint: string
  accent: Accent
  state: PanelState
}

function ForecastPanel({ label, endpoint, accent, state }: ForecastPanelProps) {
  return (
    <article className={`panel panel-${accent}`}>
      <header className="panel-header">
        <div>
          <p className="eyebrow">Live source</p>
          <h2>{label}</h2>
        </div>
        <code>{endpoint}</code>
      </header>

      {state.loading ? <p className="status">Loading forecast data...</p> : null}

      {state.error ? <p className="status error">{state.error}</p> : null}

      {!state.loading && !state.error ? (
        <div className="forecast-list">
          {traceDebugEnabled && state.traceDebug ? (
            <section className="trace-debug">
              <p className="trace-debug-title">Trace compare</p>
              <p>
                traceparent in: <code>{state.traceDebug.requestTraceparent || '<empty>'}</code>
              </p>
              <p>
                trace id in: <code>{state.traceDebug.requestTraceId || '<empty>'}</code>
              </p>
              <p>
                parent span in: <code>{state.traceDebug.requestParentSpanId || '<empty>'}</code>
              </p>
              <p>
                flags in: <code>{state.traceDebug.requestFlags || '<empty>'}</code>
              </p>
              <p>
                server trace id: <code>{state.traceDebug.serverTraceId || '<empty>'}</code>
              </p>
            </section>
          ) : null}
          {state.data.map((record) => (
            <section className="forecast-card" key={`${label}-${record.date}-${record.summary}`}>
              <p className="forecast-date">{formatDate(record.date)}</p>
              <p className="forecast-summary">{record.summary}</p>
              <p className="forecast-temp">{formatTemperature(record)}</p>
            </section>
          ))}
        </div>
      ) : null}
    </article>
  )
}

function App() {
  const [forecastState, setForecastState] = useState<ForecastState>({
    dotnet: { data: [], error: '', loading: true, traceDebug: null },
    python: { data: [], error: '', loading: true, traceDebug: null },
    cpp: { data: [], error: '', loading: true, traceDebug: null },
    updatedAt: '',
  })
  const [autoRefreshEnabled, setAutoRefreshEnabled] = useState(true)
  const [refreshIntervalSeconds, setRefreshIntervalSeconds] = useState(10)

  useEffect(() => {
    const controller = new AbortController()
    let isRefreshing = false
    let timerId: number | undefined

    async function loadForecasts() {
      if (isRefreshing) {
        return
      }

      isRefreshing = true

      setForecastState((current) => ({
        ...current,
        dotnet: { ...current.dotnet, loading: current.dotnet.data.length === 0, error: '' },
        python: { ...current.python, loading: current.python.data.length === 0, error: '' },
        cpp: { ...current.cpp, loading: current.cpp.data.length === 0, error: '' },
      }))

      try {
        const entries = Object.entries(sources) as Array<[SourceKey, SourceConfig]>
        const results = await Promise.allSettled(
          entries.map(async ([key, source]): Promise<[SourceKey, ForecastRecord[], TraceDebug | null]> => {
            const endpoint = resolveRequestEndpoint(source)
            const response = await fetch(endpoint, { signal: controller.signal })

            if (!response.ok) {
              throw await buildRequestError(response, source.label)
            }

            return [
              key,
              (await response.json()) as ForecastRecord[],
              readTraceDebugHeaders(response),
            ]
          }),
        )

        if (controller.signal.aborted) {
          return
        }

        setForecastState({
          updatedAt: new Date().toLocaleTimeString('en-CA', {
            hour: '2-digit',
            minute: '2-digit',
            second: '2-digit',
          }),
          dotnet: derivePanelState('dotnet', results),
          python: derivePanelState('python', results),
          cpp: derivePanelState('cpp', results),
        })
      } catch (error) {
        if (error instanceof DOMException && error.name === 'AbortError') {
          return
        }

        const message = error instanceof Error ? error.message : 'Unexpected error while loading data.'
        setForecastState({
          updatedAt: '',
          dotnet: { data: [], error: message, loading: false, traceDebug: null },
          python: { data: [], error: message, loading: false, traceDebug: null },
          cpp: { data: [], error: message, loading: false, traceDebug: null },
        })
      } finally {
        isRefreshing = false
      }
    }

    loadForecasts().catch(() => undefined)

    if (autoRefreshEnabled) {
      timerId = window.setInterval(loadForecasts, refreshIntervalSeconds * 1000)
    }

    return () => {
      controller.abort()
      if (timerId) {
        window.clearInterval(timerId)
      }
    }
  }, [autoRefreshEnabled, refreshIntervalSeconds])

  return (
    <main className="app-shell">
      <section className="hero">
        <div>
          <p className="eyebrow">Cross-runtime weather board</p>
          <h1>One React front end, three backend runtimes.</h1>
          <p className="hero-copy">
            This dashboard calls the .NET, Python, and C++ weather forecast APIs directly and
            shows their live responses side by side.
          </p>
        </div>
        <div className="hero-meta">
          <p>Sources: {Object.keys(sources).length}</p>
          <p>Last refresh: {forecastState.updatedAt || 'Pending'}</p>
          <div className="refresh-controls">
            <label className="refresh-toggle">
              <input
                type="checkbox"
                checked={autoRefreshEnabled}
                onChange={(event) => setAutoRefreshEnabled(event.target.checked)}
              />
              <span>Auto refresh</span>
            </label>

            <label className="refresh-interval">
              <span>Interval</span>
              <select
                value={refreshIntervalSeconds}
                onChange={(event) => setRefreshIntervalSeconds(Number(event.target.value))}
                disabled={!autoRefreshEnabled}
              >
                {refreshOptions.map((seconds) => (
                  <option key={seconds} value={seconds}>
                    Every {seconds}s
                  </option>
                ))}
              </select>
            </label>
          </div>
        </div>
      </section>

      <section className="panel-grid">
        <ForecastPanel {...sources.dotnet} state={forecastState.dotnet} />
        <ForecastPanel {...sources.python} state={forecastState.python} />
        <ForecastPanel {...sources.cpp} state={forecastState.cpp} />
      </section>
    </main>
  )
}

function derivePanelState(
  key: SourceKey,
  results: PromiseSettledResult<[SourceKey, ForecastRecord[], TraceDebug | null]>[],
): PanelState {
  const sourceKeys = Object.keys(sources) as SourceKey[]
  const index = sourceKeys.indexOf(key)
  const result = results[index]

  if (!result || result.status === 'rejected') {
    const reasonMessage =
      result?.status === 'rejected' && result.reason instanceof Error
        ? result.reason.message
        : undefined

    return {
      data: [],
      error: reasonMessage ?? `Unable to load ${sources[key].label}`,
      loading: false,
      traceDebug: null,
    }
  }

  return {
    data: result.value[1],
    error: '',
    loading: false,
    traceDebug: result.value[2],
  }
}

export default App
