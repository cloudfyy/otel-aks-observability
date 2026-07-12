#pragma once

#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/trace/tracer.h>

#include "otelapicpp/config.h"

namespace otelapicpp
{
void InitTracerProvider(const AppConfig &config);

opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> GetTracer();
} // namespace otelapicpp
