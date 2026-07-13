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

bool GetBoolOrDefault(const char *name, bool fallback)
{
  const char *value = std::getenv(name);
  if (value == nullptr)
  {
    return fallback;
  }

  std::string normalized(value);
  for (auto &ch : normalized)
  {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }

  if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on")
  {
    return true;
  }

  if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off")
  {
    return false;
  }

  return fallback;
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
  config.trace_debug_enabled = GetBoolOrDefault("OTEL_TRACE_DEBUG_ENABLED", false);
  return config;
}
} // namespace otelapicpp
