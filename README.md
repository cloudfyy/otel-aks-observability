# OTel Project Overview

[English](README.en.md)

## 项目功能

本目录用于在 AKS 上落地 OpenTelemetry 采集方案，覆盖从开发验证到生产部署的完整路径。

主要能力如下：

- 采集链路：接收应用上报的 traces、metrics、logs（OTLP gRPC/HTTP）
- 自动注入：通过 OpenTelemetry Operator + Instrumentation CRD 为应用注入 .NET / Python 探针
- 导出目标：对接 Azure Monitor / Application Insights
- 生产加固：提供 agent + gateway 分层架构、mTLS 证书、网络策略、容量与告警建议
- 运维治理：提供版本台账、升级前检查、验证命令

## 目录结构

- [dev/](dev/)
  - 开发与联调环境示例（单 Collector 为主）
  - 包含 .NET / Python Instrumentation、示例应用清单、collector values
  - 参考文档：[dev/README.dev.md](dev/README.dev.md)
- [prod/](prod/)
  - 生产基线配置（agent + gateway）
  - 包含 NetworkPolicy、cert-manager 证书、生产 values、告警与版本台账
  - 参考文档：[prod/README.prod.md](prod/README.prod.md)
- [otelapidemo/](otelapidemo/)
  - 示例应用工程（用于验证自动注入与端到端链路）

## 推荐阅读顺序

1. 先看 [dev/README.dev.md](dev/README.dev.md)，完成单 Collector 的功能验证
2. 再看 [prod/README.prod.md](prod/README.prod.md)，按生产基线部署 agent + gateway
3. 升级或变更前，先更新 [prod/version-baseline.current.md](prod/version-baseline.current.md) 并执行 Upgrade Pre-Checks

## 关键文件索引

- 开发 collector values：[dev/otle-gateway-myvalues.yaml](dev/otle-gateway-myvalues.yaml)
- 开发 collector values（历史/备用）：[dev/current-values.yaml](dev/current-values.yaml)、[dev/myvalues.yaml](dev/myvalues.yaml)
- 开发 .NET 注入 CRD：[dev/inst-crd-dotnet.yaml](dev/inst-crd-dotnet.yaml)
- 开发 Python 注入 CRD：[dev/inst-crd-python.yaml](dev/inst-crd-python.yaml)
- 生产 gateway values：[prod/gateway-values.prod.yaml](prod/gateway-values.prod.yaml)
- 生产 agent values：[prod/agent-values.prod.yaml](prod/agent-values.prod.yaml)
- 生产 agent OTLP 入口 Service：[prod/otel-agent-service.prod.yaml](prod/otel-agent-service.prod.yaml)
- 生产 agent RBAC（k8sattributes 权限）：[prod/otel-agent-rbac.prod.yaml](prod/otel-agent-rbac.prod.yaml)
- 生产 .NET 示例应用清单：[prod/otelapidemo-dotnet.yaml](prod/otelapidemo-dotnet.yaml)
- 生产 Python 示例清单模板（仅示例、尚需测试）：[prod/otelapidemo-python.yaml](prod/otelapidemo-python.yaml)
- 生产网络策略：[prod/networkpolicy.prod.yaml](prod/networkpolicy.prod.yaml)
- 生产证书配置：[prod/collector-tls.prod.yaml](prod/collector-tls.prod.yaml)
- 生产告警查询建议：[prod/alerts-kql.prod.md](prod/alerts-kql.prod.md)
- 生产版本台账：[prod/version-baseline.current.md](prod/version-baseline.current.md)

## 说明

- dev 配置偏向可观测与排障，通常会保留更多调试项
- prod 配置偏向稳定性与安全性，强调最小权限、重试队列、证书与网络隔离
- prod 文档内已包含“排查步骤 + PowerShell/bash 脚本 + App Insights 最终核验 KQL（建议等待 3-10 分钟）”
- 建议固定镜像与 chart 版本，避免使用 latest
