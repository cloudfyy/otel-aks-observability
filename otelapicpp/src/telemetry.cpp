#include "otelapicpp/telemetry.h"

#include <format>
#include <sstream>
#include <string>

#include <opentelemetry/exporters/otlp/otlp_http_exporter.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_factory.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/trace/batch_span_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/trace/provider.h>

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
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
} // namespace

void InitTracerProvider(const AppConfig &config)
{
  auto resource_attrs = resource_sdk::ResourceAttributes{
      {"service.name", config.otel_service_name},
      {"service.namespace", "apps-dev"},
      {"service.version", "1.0.5"},
      {"deployment.environment.name", "dev"}};

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
}

opentelemetry::nostd::shared_ptr<trace_api::Tracer> GetTracer()
{
  auto provider = trace_api::Provider::GetTracerProvider();
  return provider->GetTracer("otelapicpp", "1.0.5");
}
} // namespace otelapicpp
