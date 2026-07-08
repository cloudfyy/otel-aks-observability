import { useEffect, useState } from 'react'
import './App.css'

const refreshOptions = [5, 10, 30, 60]

const sources = {
  dotnet: {
    label: '.NET API',
    endpoint: `${import.meta.env.VITE_DOTNET_API_BASE_URL ?? 'http://localhost:5041'}/weatherforecast`,
    accent: 'dotnet',
  },
  python: {
    label: 'Python API',
    endpoint: `${import.meta.env.VITE_PYTHON_API_BASE_URL ?? 'http://localhost:8000'}/weatherforecast`,
    accent: 'python',
  },
}

function formatTemperature(record) {
  return `${record.temperatureC}C / ${record.temperatureF}F`
}

function formatDate(value) {
  return new Date(value).toLocaleDateString('en-CA', {
    month: 'short',
    day: 'numeric',
    weekday: 'short',
  })
}

function ForecastPanel({ label, endpoint, accent, state }) {
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
  const [forecastState, setForecastState] = useState({
    dotnet: { data: [], error: '', loading: true },
    python: { data: [], error: '', loading: true },
    updatedAt: '',
  })
  const [autoRefreshEnabled, setAutoRefreshEnabled] = useState(true)
  const [refreshIntervalSeconds, setRefreshIntervalSeconds] = useState(10)

  useEffect(() => {
    const controller = new AbortController()
    let isRefreshing = false
    let timerId

    async function loadForecasts() {
      if (isRefreshing) {
        return
      }

      isRefreshing = true

      setForecastState((current) => ({
        ...current,
        dotnet: { ...current.dotnet, loading: current.dotnet.data.length === 0, error: '' },
        python: { ...current.python, loading: current.python.data.length === 0, error: '' },
      }))

      try {
        const entries = Object.entries(sources)
        const results = await Promise.allSettled(
          entries.map(async ([key, source]) => {
            const response = await fetch(source.endpoint, { signal: controller.signal })

            if (!response.ok) {
              throw new Error(`${source.label} returned ${response.status}`)
            }

            return [key, await response.json()]
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
        })
      } catch (error) {
        if (error.name === 'AbortError') {
          return
        }

        setForecastState({
          updatedAt: '',
          dotnet: { data: [], error: error.message, loading: false },
          python: { data: [], error: error.message, loading: false },
        })
      } finally {
        isRefreshing = false
      }
    }

    loadForecasts()

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
          <h1>One React front end, two backend runtimes.</h1>
          <p className="hero-copy">
            This dashboard calls the .NET and Python weather forecast APIs directly and
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
      </section>
    </main>
  )
}

function derivePanelState(key, results) {
  const index = Object.keys(sources).indexOf(key)
  const result = results[index]

  if (!result || result.status === 'rejected') {
    return {
      data: [],
      error: result?.reason?.message ?? `Unable to load ${sources[key].label}`,
      loading: false,
    }
  }

  return {
    data: result.value[1],
    error: '',
    loading: false,
  }
}

export default App
