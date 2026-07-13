#pragma once

#include <string>

namespace otelapicpp
{
struct AppConfig
{
  int port;
  std::string allowed_origins;
  std::string otel_service_name;
  std::string otel_exporter_otlp_endpoint;
  std::string otel_resource_attributes;
  bool trace_debug_enabled;
};

AppConfig LoadAppConfig();
} // namespace otelapicpp
