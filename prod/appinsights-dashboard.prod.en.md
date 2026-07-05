# App Insights Dashboard (Application Runtime + Common OTel Metrics)

This document provides a production-ready Azure Monitor Workbook design for:

- Application runtime health (request volume, failure rate, latency, exceptions)
- Dependency health (failure rate, latency)
- OTel Collector pipeline health (accepted/sent/refused/failed)
- Application OTel metrics (HTTP/Runtime/Process)

> Recommended context: create this workbook in the Log Analytics context connected to your Application Insights resource, with default time range set to last 30 minutes.

## 1. Recommended Dashboard Layout

Use 3 sections (or tabs):

1. Application Runtime
2. Collector Pipeline
3. App OTel Metrics

Recommended top KPI cards:

- Total Requests (30m)
- Failure Rate (30m)
- P95 Latency (30m)

## 2. Common Filtering Scope

Recommended production service filters:

- cloud_RoleName: apps-prod.otelapidemo, apps-prod.otelapidemo-python
- Fallback: customDimensions["service.namespace"] = apps-prod and service.name in (otelapidemo, otelapidemo-python)

All queries below already include these filters.

## 3. Application Runtime

### 1) KPI - Total Requests / Failure Rate / P95

```kql
let AppReq = requests
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
   or (
      tostring(customDimensions["service.namespace"]) =~ "apps-prod"
      and tostring(customDimensions["service.name"]) in~ ("otelapidemo", "otelapidemo-python")
   );
AppReq
| summarize
    Total=count(),
    Failed=countif(success == false),
    FailRatePct=100.0 * todouble(countif(success == false)) / iif(count() == 0, 1.0, todouble(count())),
    P95DurationMs=percentile(duration, 95)
```

### 2) Request and Failure Trend (5m)

```kql
requests
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
   or (
      tostring(customDimensions["service.namespace"]) =~ "apps-prod"
      and tostring(customDimensions["service.name"]) in~ ("otelapidemo", "otelapidemo-python")
   )
| summarize
    RequestCount=count(),
    FailedCount=countif(success == false)
  by cloud_RoleName, bin(timestamp, 5m)
| order by timestamp asc
```

### 3) Request Latency Trend (P50/P95)

```kql
requests
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
   or (
      tostring(customDimensions["service.namespace"]) =~ "apps-prod"
      and tostring(customDimensions["service.name"]) in~ ("otelapidemo", "otelapidemo-python")
   )
| summarize
    P50=percentile(duration, 50),
    P95=percentile(duration, 95)
  by cloud_RoleName, bin(timestamp, 5m)
| order by timestamp asc
```

### 4) Top Exceptions (type + message)

```kql
exceptions
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
| summarize Count=count(), LastSeen=max(timestamp) by cloud_RoleName, type, outerMessage
| top 20 by Count desc
```

### 5) Dependency Failures and Latency (optional, recommended)

```kql
dependencies
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
| summarize
    Total=count(),
    Failed=countif(success == false),
    FailRatePct=100.0 * todouble(countif(success == false)) / iif(count() == 0, 1.0, todouble(count())),
    P95DurationMs=percentile(duration, 95)
  by cloud_RoleName, type, target
| order by Failed desc, P95DurationMs desc
| take 30
```

## 4. OTel Collector Pipeline

### 6) Throughput (accepted vs sent)

```kql
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(30m)
| where name in (
  "otelcol_receiver_accepted_spans",
  "otelcol_exporter_sent_spans",
  "otelcol_receiver_accepted_log_records",
  "otelcol_exporter_sent_log_records",
  "otelcol_receiver_accepted_metric_points",
  "otelcol_exporter_sent_metric_points"
)
| summarize Total=sum(value) by name, cloud_RoleName, bin(timestamp, 5m)
| order by timestamp asc
```

### 7) Export Failed (spans/logs/metrics)

```kql
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(30m)
| where name in (
  "otelcol_exporter_send_failed_spans",
  "otelcol_exporter_send_failed_log_records",
  "otelcol_exporter_send_failed_metric_points"
)
| summarize Failed=sum(value) by name, cloud_RoleName, bin(timestamp, 5m)
| order by timestamp asc
```

### 8) Receiver Refused (spans/logs/metrics)

```kql
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(30m)
| where name in (
  "otelcol_receiver_refused_spans",
  "otelcol_receiver_refused_log_records",
  "otelcol_receiver_refused_metric_points"
)
| summarize Refused=sum(value) by name, cloud_RoleName, bin(timestamp, 5m)
| order by timestamp asc
```

## 5. Application OTel Metrics

### 9) Metric Discovery (run this first)

```kql
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
   or (
      tostring(customDimensions["service.namespace"]) =~ "apps-prod"
      and tostring(customDimensions["service.name"]) in~ ("otelapidemo", "otelapidemo-python")
   )
| summarize Points=count(), LastSeen=max(timestamp) by name, cloud_RoleName
| order by Points desc
```

### 10) Common Runtime Metrics Trend (HTTP/Runtime/Process)

```kql
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
   or (
      tostring(customDimensions["service.namespace"]) =~ "apps-prod"
      and tostring(customDimensions["service.name"]) in~ ("otelapidemo", "otelapidemo-python")
   )
| where name has_any ("http", "server", "request", "duration", "runtime", "process", "cpu", "memory", "gc", "thread")
| summarize AvgValue=avg(value), P95Value=percentile(value, 95), MaxValue=max(value) by name, cloud_RoleName, bin(timestamp, 5m)
| order by timestamp asc
```

## 6. Create the Dashboard in Azure Portal

1. Open Application Insights -> Workbooks -> New.
2. Create 3 sections: Application Runtime / Collector Pipeline / App OTel Metrics.
3. Add each query and paste the KQL from this document.
4. Recommended visualizations:
   - KPI query: Tile or Grid
   - Trend query: Time chart
   - Top/details query: Grid
5. Save as a shared workbook, for example: OTel-Prod-AppInsights-Dashboard.
6. Pin it to Azure Dashboard from the top-right corner.

## 6.5 Import Workbook JSON Directly (recommended)

The repository already includes an importable workbook template:

- prod/appinsights-dashboard.workbook.json

Note: the current workbook template is tab-based with 3 tabs:

- Application Runtime
- Collector Pipeline
- App OTel Metrics

Import steps:

1. Open Azure Portal -> Application Insights -> Workbooks.
2. Click New, then open Advanced Editor (or Edit as JSON).
3. Paste the full content of prod/appinsights-dashboard.workbook.json and apply.
4. Click Done Editing and Save. Suggested name: OTel-Prod-AppInsights-Dashboard.
5. Select the correct Application Insights resource and verify each chart returns data.
6. Use the top tabs to switch sections.

If you want this workbook to be interactive (namespace/serviceName/timeRange), convert fixed values into Workbook Parameters in the next iteration.

## 7. Alerting Recommendations (paired with dashboard)

Create Scheduled Query Alerts based on these rules:

- otelcol_exporter_send_failed_* > 0 for 10-15 minutes
- otelcol_receiver_refused_* > 0 for 10-15 minutes
- Request failure rate > 2% (per service)
- Request P95 over business SLO threshold

You can reuse existing baseline queries in:

- prod/alerts-kql.prod.md
