#pragma once

#include <opentelemetry/logs/logger.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/trace/tracer.h>

#include "otelapicpp/config.h"

namespace otelapicpp
{
inline constexpr const char kTelemetryInstrumentationName[] = "otelapicpp";
inline constexpr const char kTelemetryVersion[] = "1.0.10";
inline constexpr const char kDefaultServiceNamespace[] = "apps-dev";
inline constexpr const char kDefaultDeploymentEnvironment[] = "dev";

void InitTracerProvider(const AppConfig &config);

opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> GetTracer();
opentelemetry::nostd::shared_ptr<opentelemetry::logs::Logger> GetLogger();
} // namespace otelapicpp
