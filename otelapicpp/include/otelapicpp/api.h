#pragma once

#include <string>

#include <crow.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/trace/tracer.h>

#include "otelapicpp/config.h"

namespace otelapicpp
{
void RegisterRoutes(
    crow::SimpleApp &app,
    const AppConfig &config,
    const std::string &allowed_origins,
    const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> &tracer);
} // namespace otelapicpp
