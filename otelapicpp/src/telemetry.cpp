#include "otelapicpp/telemetry.h"

#include <format>
#include <sstream>
#include <string>

#include <opentelemetry/exporters/otlp/otlp_http_exporter.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_log_record_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_log_record_exporter_options.h>
#include <opentelemetry/logs/logger_provider.h>
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/logs/provider.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor_factory.h>
#include <opentelemetry/sdk/trace/batch_span_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/trace/provider.h>

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace logs_sdk = opentelemetry::sdk::logs;
namespace resource_sdk = opentelemetry::sdk::resource;
namespace otlp = opentelemetry::exporter::otlp;

namespace otelapicpp
{
namespace
{
std::string BuildOtlpTracesUrl(const std::string &endpoint)
{
  auto url = endpoint;
  if (!url.empty() && url.back() == '/')
  {
    url.pop_back();
  }

  if (url.size() >= 10 && url.substr(url.size() - 10) == "/v1/traces")
  {
    return url;
  }

  return std::format("{}/v1/traces", url);
}

std::string BuildOtlpLogsUrl(const std::string &endpoint)
{
  auto url = endpoint;
  if (!url.empty() && url.back() == '/')
  {
    url.pop_back();
  }

  if (url.size() >= 8 && url.substr(url.size() - 8) == "/v1/logs")
  {
    return url;
  }

  return std::format("{}/v1/logs", url);
}
} // namespace

void InitTracerProvider(const AppConfig &config)
{
  auto resource_attrs = resource_sdk::ResourceAttributes{
      {"service.name", config.otel_service_name},
  {"service.namespace", kDefaultServiceNamespace},
  {"service.version", kTelemetryVersion},
  {"deployment.environment.name", kDefaultDeploymentEnvironment}};

  if (!config.otel_resource_attributes.empty())
  {
    std::stringstream attributes_stream(config.otel_resource_attributes);
    std::string pair;
    while (std::getline(attributes_stream, pair, ','))
    {
      auto split = pair.find('=');
      if (split == std::string::npos || split == 0 || split == pair.size() - 1)
      {
        continue;
      }

      auto key = pair.substr(0, split);
      auto value = pair.substr(split + 1);
      resource_attrs[key] = value;
    }
  }

  auto exporter_options = otlp::OtlpHttpExporterOptions();
  exporter_options.url = BuildOtlpTracesUrl(config.otel_exporter_otlp_endpoint);
  exporter_options.content_type = otlp::HttpRequestContentType::kBinary;

  auto exporter = otlp::OtlpHttpExporterFactory::Create(exporter_options);
  auto processor_options = trace_sdk::BatchSpanProcessorOptions();
  auto processor = std::unique_ptr<trace_sdk::SpanProcessor>(
      new trace_sdk::BatchSpanProcessor(std::move(exporter), processor_options));

  auto resource = resource_sdk::Resource::Create(resource_attrs);
  auto provider = opentelemetry::nostd::shared_ptr<trace_api::TracerProvider>(
      new trace_sdk::TracerProvider(std::move(processor), resource));

  trace_api::Provider::SetTracerProvider(provider);

  auto log_exporter_options = otlp::OtlpHttpLogRecordExporterOptions();
  log_exporter_options.url = BuildOtlpLogsUrl(config.otel_exporter_otlp_endpoint);
  log_exporter_options.content_type = otlp::HttpRequestContentType::kBinary;

  auto log_exporter = otlp::OtlpHttpLogRecordExporterFactory::Create(log_exporter_options);
  auto log_processor = logs_sdk::SimpleLogRecordProcessorFactory::Create(std::move(log_exporter));
  auto log_provider = logs_sdk::LoggerProviderFactory::Create(std::move(log_processor), resource);
  opentelemetry::nostd::shared_ptr<opentelemetry::logs::LoggerProvider> api_log_provider(
      log_provider.release());
  logs_sdk::Provider::SetLoggerProvider(api_log_provider);
}

opentelemetry::nostd::shared_ptr<trace_api::Tracer> GetTracer()
{
  auto provider = trace_api::Provider::GetTracerProvider();
  return provider->GetTracer(kTelemetryInstrumentationName, kTelemetryVersion);
}

opentelemetry::nostd::shared_ptr<opentelemetry::logs::Logger> GetLogger()
{
  auto provider = opentelemetry::logs::Provider::GetLoggerProvider();
  return provider->GetLogger(
      kTelemetryInstrumentationName,
      kTelemetryInstrumentationName,
      kTelemetryVersion);
}
} // namespace otelapicpp
