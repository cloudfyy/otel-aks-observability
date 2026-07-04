# OTel Production Alert Queries

## 1) Collector Export Failures (Spans/Logs/Metrics)

```kusto
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(15m)
| where name in (
  "otelcol_exporter_send_failed_spans",
  "otelcol_exporter_send_failed_log_records",
  "otelcol_exporter_send_failed_metric_points"
)
| summarize Failed=sum(value) by name, cloud_RoleName, bin(timestamp, 5m)
| order by timestamp desc
```

## 2) Collector Receiver Refused Data

```kusto
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(15m)
| where name in (
  "otelcol_receiver_refused_spans",
  "otelcol_receiver_refused_log_records",
  "otelcol_receiver_refused_metric_points"
)
| summarize Refused=sum(value) by name, cloud_RoleName, bin(timestamp, 5m)
| order by timestamp desc
```

## 3) Throughput Baseline (Accepted vs Sent)

```kusto
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
| order by timestamp desc
```

## 4) App Request Health

```kusto
requests
| where timestamp > ago(30m)
| summarize
    Total=count(),
    Failed=countif(success == false),
    P95=percentile(duration, 95)
  by bin(timestamp, 5m)
| order by timestamp desc
```

## 5) App Exceptions

```kusto
exceptions
| where timestamp > ago(30m)
| summarize Count=count() by type, outerMessage, bin(timestamp, 5m)
| order by timestamp desc
```

## 6) Discover App Metrics

Use this first because metric names can vary across language runtimes and OpenTelemetry semantic convention versions. Classic Application Insights usually uses `customMetrics`; workspace-based Application Insights may expose the same data through `AppMetrics`.

```kusto
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(30m)
| summarize Points=count(), LastSeen=max(timestamp) by name, cloud_RoleName
| order by Points desc
```

## 7) App Metrics for Demo Services

```kusto
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
| summarize Points=count(), AvgValue=avg(value), P95Value=percentile(value, 95), MaxValue=max(value) by name, cloud_RoleName, bin(timestamp, 5m)
| order by timestamp desc, name asc
```

## 8) Useful HTTP / Runtime / Process Metrics

```kusto
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
| where name has_any ("http", "server", "request", "duration", "runtime", "process", "cpu", "memory", "gc", "thread")
| summarize Points=count(), AvgValue=avg(value), P95Value=percentile(value, 95), MaxValue=max(value) by name, cloud_RoleName, bin(timestamp, 5m)
| order by timestamp desc, name asc
```

## 9) Metrics Pipeline Health

```kusto
let MetricRows = union isfuzzy=true
  (customMetrics | project timestamp, name, value=todouble(value), cloud_RoleName, customDimensions),
  (AppMetrics | project timestamp=TimeGenerated, name=Name, value=todouble(Sum), cloud_RoleName=AppRoleName, customDimensions=Properties);
MetricRows
| where timestamp > ago(30m)
| where name in~ (
  "otelcol_receiver_accepted_metric_points",
  "otelcol_exporter_sent_metric_points",
  "otelcol_receiver_refused_metric_points",
  "otelcol_exporter_send_failed_metric_points"
)
| summarize Total=sum(value) by name, cloud_RoleName, bin(timestamp, 5m)
| order by timestamp desc, name asc
```
