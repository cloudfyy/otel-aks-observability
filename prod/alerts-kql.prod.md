# OTel Production Alert Queries

## 1) Collector Export Failures (Spans/Logs/Metrics)

```kusto
customMetrics
| where timestamp > ago(15m)
| where name in (
  "otelcol_exporter_send_failed_spans",
  "otelcol_exporter_send_failed_log_records",
  "otelcol_exporter_send_failed_metric_points"
)
| summarize Failed=sum(value) by name, bin(timestamp, 5m)
| order by timestamp desc
```

## 2) Collector Receiver Refused Data

```kusto
customMetrics
| where timestamp > ago(15m)
| where name in (
  "otelcol_receiver_refused_spans",
  "otelcol_receiver_refused_log_records",
  "otelcol_receiver_refused_metric_points"
)
| summarize Refused=sum(value) by name, bin(timestamp, 5m)
| order by timestamp desc
```

## 3) Throughput Baseline (Accepted vs Sent)

```kusto
customMetrics
| where timestamp > ago(30m)
| where name in (
  "otelcol_receiver_accepted_spans",
  "otelcol_exporter_sent_spans",
  "otelcol_receiver_accepted_log_records",
  "otelcol_exporter_sent_log_records",
  "otelcol_receiver_accepted_metric_points",
  "otelcol_exporter_sent_metric_points"
)
| summarize Total=sum(value) by name, bin(timestamp, 5m)
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
