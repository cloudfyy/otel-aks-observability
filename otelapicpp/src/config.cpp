#include "otelapicpp/config.h"

#include <cstdlib>
#include <string>

namespace otelapicpp
{
namespace
{
std::string GetEnvOrDefault(const char *name, const std::string &fallback)
{
  const char *value = std::getenv(name);
  if (value == nullptr || std::string(value).empty())
  {
    return fallback;
  }

  return value;
}

int GetPortOrDefault(const std::string &value, int fallback)
{
  try
  {
    return std::stoi(value);
  }
  catch (...)
  {
    return fallback;
  }
}
} // namespace

AppConfig LoadAppConfig()
{
  AppConfig config;
  config.port = GetPortOrDefault(GetEnvOrDefault("PORT", "8082"), 8082);
  config.allowed_origins = GetEnvOrDefault("CORS_ALLOWED_ORIGINS", "http://localhost:5173");
  config.otel_service_name = GetEnvOrDefault("OTEL_SERVICE_NAME", "otelapidemo-cpp");
  config.otel_exporter_otlp_endpoint = GetEnvOrDefault(
      "OTEL_EXPORTER_OTLP_ENDPOINT",
      "http://otel-collector-opentelemetry-collector.observability.svc.cluster.local:4318");
  config.otel_resource_attributes = GetEnvOrDefault("OTEL_RESOURCE_ATTRIBUTES", "");
  return config;
}
} // namespace otelapicpp
