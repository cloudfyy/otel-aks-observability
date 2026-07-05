# App Insights Dashboard（程序运行 + OTel 常用指标）

本文给出一个可直接落地的 Azure Monitor Workbook 方案，用于展示：

- 应用运行状态（请求量、失败率、延迟、异常）
- 依赖调用健康度（失败率、延迟）
- OTel Collector 管道健康（accepted/sent/refused/failed）
- OTel 应用指标（HTTP/Runtime/Process）

> 建议在 Application Insights 对应的 Log Analytics 上下文中创建 Workbook，并将时间范围默认设为最近 30 分钟。

## 一、Dashboard 布局建议

建议使用 3 个分组（Section）：

1. 应用运行（Application Runtime）
2. OTel 管道（Collector Pipeline）
3. 应用 OTel 指标（App OTel Metrics）

建议顶部放 3 个 KPI：

- 请求总量（30m）
- 失败率（30m）
- P95 延迟（30m）

## 二、通用过滤条件

当前生产服务推荐过滤：

- cloud_RoleName: apps-prod.otelapidemo, apps-prod.otelapidemo-python
- 兜底：customDimensions["service.namespace"] = apps-prod 且 service.name in (otelapidemo, otelapidemo-python)

以下查询已内置上述过滤条件。

## 三、应用运行（Application Runtime）

### 1) KPI - 请求总量 / 失败率 / P95

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

### 2) 请求量与失败趋势（5m）

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

### 3) 请求延迟趋势（P50/P95）

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

### 4) 异常 Top（类型 + 信息）

```kql
exceptions
| where timestamp > ago(30m)
| where cloud_RoleName in~ ("apps-prod.otelapidemo", "apps-prod.otelapidemo-python")
| summarize Count=count(), LastSeen=max(timestamp) by cloud_RoleName, type, outerMessage
| top 20 by Count desc
```

### 5) 依赖失败与延迟（可选但推荐）

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

## 四、OTel 管道（Collector Pipeline）

### 6) Throughput（accepted vs sent）

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

### 7) Export failed（spans/logs/metrics）

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

### 8) Receiver refused（spans/logs/metrics）

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

## 五、应用 OTel 指标（App OTel Metrics）

### 9) 指标发现（先跑一次）

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

### 10) 常用运行时指标趋势（HTTP/Runtime/Process）

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

## 六、在 Azure Portal 中创建 Dashboard 的步骤

1. 打开 Application Insights -> Workbooks -> New。
2. 新建 3 个 Section：Application Runtime / Collector Pipeline / App OTel Metrics。
3. 每个图表选择 Add query，将上面的 KQL 粘贴进去。
4. 图表类型建议：
   - KPI 查询：Tile 或 Grid
   - 趋势查询：Time chart
   - Top/明细：Grid
5. 保存为共享 Workbook，例如：`OTel-Prod-AppInsights-Dashboard`。
6. 在 Workbook 右上角 Pin 到 Azure Dashboard，形成团队共享看板。

## 六点五、直接导入 Workbook JSON（推荐）

仓库已提供可直接导入的 Workbook 模板：

- `prod/appinsights-dashboard.workbook.json`

说明：当前模板已改为 Tab 页形式，包含 3 个页签：

- `Application Runtime`
- `Collector Pipeline`
- `App OTel Metrics`

导入步骤：

1. 打开 Azure Portal -> Application Insights -> Workbooks。
2. 点击 New，然后进入 Advanced Editor（或 Edit as JSON）。
3. 将 `prod/appinsights-dashboard.workbook.json` 全量内容粘贴并应用。
4. 点击 Done Editing 并 Save，命名建议：`OTel-Prod-AppInsights-Dashboard`。
5. 选择对应的 Application Insights 资源后，检查各图表是否返回数据。
6. 通过顶部 Tab 在三个分区间切换查看（无需滚动整页）。

若你希望改成参数化（例如 namespace、serviceName、timeRange 可交互），可在后续版本中把固定过滤值抽成 Workbook Parameters。

## 七、告警建议（与 Dashboard 配套）

建议基于以下查询建立 Scheduled Query Alert：

- `otelcol_exporter_send_failed_* > 0` 持续 10-15 分钟
- `otelcol_receiver_refused_* > 0` 持续 10-15 分钟
- 请求失败率 > 2%（按服务）
- 请求 P95 超过业务 SLO 阈值

已有基础查询可直接复用：`prod/alerts-kql.prod.md`。
