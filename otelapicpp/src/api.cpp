#include "otelapicpp/api.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <typeinfo>

#include <nlohmann/json.hpp>

#include <opentelemetry/context/context.h>
#include <opentelemetry/context/propagation/text_map_propagator.h>
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/logs/severity.h>
#include <opentelemetry/nostd/span.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/trace/scope.h>

#include "otelapicpp/exceptions.h"
#include "otelapicpp/telemetry.h"
#include "otelapicpp/weather.h"

namespace trace_api = opentelemetry::trace;

namespace otelapicpp
{
namespace
{
std::string GetHeaderValueCaseInsensitive(const crow::request &request, const std::string &key)
{
  auto value = request.get_header_value(key);
  if (!value.empty())
  {
    return value;
  }

  if (!key.empty())
  {
    auto title_key = key;
    title_key[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(title_key[0])));
    value = request.get_header_value(title_key);
    if (!value.empty())
    {
      return value;
    }
  }

  auto upper_key = key;
  std::transform(
      upper_key.begin(),
      upper_key.end(),
      upper_key.begin(),
      [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return request.get_header_value(upper_key);
}

class CrowRequestCarrier : public opentelemetry::context::propagation::TextMapCarrier
{
public:
  explicit CrowRequestCarrier(const crow::request &request) : request_(request) {}

  opentelemetry::nostd::string_view Get(opentelemetry::nostd::string_view key) const noexcept override
  {
    const auto key_string = std::string(key);
    auto cached = cached_values_.find(key_string);
    if (cached == cached_values_.end())
    {
      cached = cached_values_.emplace(key_string, GetHeaderValueCaseInsensitive(request_, key_string)).first;
    }

    if (cached->second.empty())
    {
      return "";
    }

    return cached->second;
  }

  void Set(opentelemetry::nostd::string_view, opentelemetry::nostd::string_view) noexcept override
  {
    // Request carrier is read-only for extraction.
  }

private:
  const crow::request &request_;
  mutable std::map<std::string, std::string> cached_values_;
};

void AddCorsHeaders(crow::response &response, const std::string &allowed_origins)
{
  response.add_header("Access-Control-Allow-Origin", allowed_origins);
  response.add_header("Access-Control-Allow-Headers", "*");
  response.add_header("Access-Control-Allow-Methods", "GET,OPTIONS");
}

void AddTraceDebugExposeHeaders(crow::response &response)
{
  response.add_header(
      "Access-Control-Expose-Headers",
      "x-otel-traceparent-in,x-otel-trace-id-in,x-otel-parent-span-id-in,x-otel-trace-flags-in,x-otel-server-trace-id");
}

struct ParsedTraceparent
{
  std::string version;
  std::string trace_id;
  std::string parent_span_id;
  std::string trace_flags;
};

std::string ToTraceIdHex(const trace_api::TraceId &trace_id)
{
  std::array<char, trace_api::TraceId::kSize * 2> buffer{};
  trace_id.ToLowerBase16(
      opentelemetry::nostd::span<char, trace_api::TraceId::kSize * 2>(buffer.data(), buffer.size()));
  return std::string(buffer.data(), buffer.size());
}

ParsedTraceparent ParseTraceparent(const std::string &traceparent)
{
  ParsedTraceparent parsed;
  if (traceparent.empty())
  {
    return parsed;
  }

  std::istringstream stream(traceparent);
  std::getline(stream, parsed.version, '-');
  std::getline(stream, parsed.trace_id, '-');
  std::getline(stream, parsed.parent_span_id, '-');
  std::getline(stream, parsed.trace_flags, '-');
  return parsed;
}

void AttachTraceDebugHeaders(
    crow::response &response,
    const crow::request &request,
    const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> &span)
{
  const auto traceparent_in = GetHeaderValueCaseInsensitive(request, "traceparent");
  const auto parsed = ParseTraceparent(traceparent_in);
  const auto server_trace_id = ToTraceIdHex(span->GetContext().trace_id());

  response.add_header("x-otel-traceparent-in", traceparent_in);
  response.add_header("x-otel-trace-id-in", parsed.trace_id);
  response.add_header("x-otel-parent-span-id-in", parsed.parent_span_id);
  response.add_header("x-otel-trace-flags-in", parsed.trace_flags);
  response.add_header("x-otel-server-trace-id", server_trace_id);

  std::cout << "[Trace Compare] traceparent_in=" << (traceparent_in.empty() ? "<empty>" : traceparent_in)
            << " trace_id_in=" << (parsed.trace_id.empty() ? "<empty>" : parsed.trace_id)
            << " parent_span_id_in=" << (parsed.parent_span_id.empty() ? "<empty>" : parsed.parent_span_id)
            << " flags_in=" << (parsed.trace_flags.empty() ? "<empty>" : parsed.trace_flags)
            << " server_trace_id=" << server_trace_id
            << std::endl;
}

void MaybeAttachTraceDebugHeaders(
    crow::response &response,
    const AppConfig &config,
    const crow::request &request,
    const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> &span)
{
  if (!config.trace_debug_enabled)
  {
    return;
  }

  AddTraceDebugExposeHeaders(response);
  AttachTraceDebugHeaders(response, request, span);
}

std::string ToSpanIdHex(const trace_api::SpanId &span_id)
{
  std::array<char, trace_api::SpanId::kSize * 2> buffer{};
  span_id.ToLowerBase16(
      opentelemetry::nostd::span<char, trace_api::SpanId::kSize * 2>(buffer.data(), buffer.size()));
  return std::string(buffer.data(), buffer.size());
}

void EmitApiStartLogAndEvent(
  const AppConfig &config,
    const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> &span,
    const crow::request &request,
    const std::string &operation,
    const std::string &route)
{
  const auto span_context = span->GetContext();
  const auto trace_id = ToTraceIdHex(span_context.trace_id());
  const auto span_id = ToSpanIdHex(span_context.span_id());
  const auto traceparent = GetHeaderValueCaseInsensitive(request, "traceparent");

  if (!config.trace_debug_enabled)
  {
    return;
  }

  std::cout << "[API Start] operation=" << operation
            << " route=" << route
            << " trace_id=" << trace_id
            << " span_id=" << span_id
            << std::endl;

  span->AddEvent(
      "api.request.start",
      {{"api.operation", operation},
       {"http.route", route},
       {"trace.id", trace_id},
       {"span.id", span_id},
       {"traceparent", traceparent.empty() ? "" : traceparent}});

    auto logger = GetLogger();
    std::map<std::string, std::string> attributes = {
      {"api.operation", operation},
      {"http.route", route},
      {"trace.id", trace_id},
      {"span.id", span_id},
      {"traceparent", traceparent}};
    logger->EmitLogRecord(
      opentelemetry::logs::Severity::kInfo,
      "api.request.start",
      attributes,
      span_context);
}

void RecordException(
    const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> &span,
    const std::exception &ex,
    const std::string &method_name)
{
  const auto *custom_exception = dynamic_cast<const CustomTestException *>(&ex);
  const auto exception_type = std::string(
      custom_exception != nullptr ? "CustomTestException" : typeid(ex).name());
  const auto message = std::string(ex.what());
    const auto stacktrace =
      (custom_exception != nullptr && !custom_exception->throw_stacktrace().empty())
        ? custom_exception->throw_stacktrace()
        : CaptureStackTrace();

  span->SetAttribute("exception.type", exception_type);
  span->SetAttribute("exception.message", message);
  span->SetAttribute("exception.assembly", "otelapicpp");
  span->SetAttribute("exception.method", method_name);
  if (!stacktrace.empty())
  {
    span->SetAttribute("exception.stacktrace", stacktrace);
  }

  span->AddEvent(
      "exception",
      {{"exception.type", exception_type},
       {"exception.message", message},
      {"exception.escaped", false},
      {"exception.assembly", "otelapicpp"},
      {"exception.method", method_name},
      {"exception.stack.origin", custom_exception != nullptr ? "throw" : "catch"},
      {"exception.stacktrace", stacktrace}});
  span->SetStatus(opentelemetry::trace::StatusCode::kError, message);
}

opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> StartServerSpan(
  const AppConfig &config,
    const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> &tracer,
    const crow::request &request,
    const std::string &span_name,
    const std::string &route)
{
  opentelemetry::trace::propagation::HttpTraceContext propagator;
  CrowRequestCarrier carrier(request);
  opentelemetry::context::Context root_context;
  auto parent_context = propagator.Extract(carrier, root_context);

  opentelemetry::trace::StartSpanOptions options;
  options.parent = parent_context;
  options.kind = opentelemetry::trace::SpanKind::kServer;

  auto span = tracer->StartSpan(span_name, options);
  span->SetAttribute("http.method", crow::method_name(request.method));
  span->SetAttribute("http.route", route);
  EmitApiStartLogAndEvent(config, span, request, span_name, route);

  return span;
}
} // namespace

void RegisterRoutes(
    crow::SimpleApp &app,
    const AppConfig &config,
    const std::string &allowed_origins,
    const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> &tracer)
{
  CROW_ROUTE(app, "/health")([tracer, config](const crow::request &request) {
    auto span = StartServerSpan(config, tracer, request, "GET /health", "/health");
    trace_api::Scope scope(span);

    crow::response response(200, "ok");
    MaybeAttachTraceDebugHeaders(response, config, request, span);
    span->SetStatus(opentelemetry::trace::StatusCode::kOk);
    span->End();
    return response;
  });

  CROW_ROUTE(app, "/weatherforecast")([tracer, allowed_origins, config](const crow::request &request) {
    auto span = StartServerSpan(config, tracer, request, "GET /weatherforecast", "/weatherforecast");
    trace_api::Scope scope(span);

    crow::response response(200);
    response.set_header("Content-Type", "application/json");
    AddCorsHeaders(response, allowed_origins);

    try
    {
      std::cout << "C++ weather forecast requested" << std::endl;
      response.body = BuildForecastPayload().dump();
      span->SetStatus(opentelemetry::trace::StatusCode::kOk);
    }
    catch (const std::exception &ex)
    {
      RecordException(span, ex, "GET /weatherforecast");
      response.code = 500;
      response.body = R"({"message":"Unexpected error while building weather forecast."})";
    }

    MaybeAttachTraceDebugHeaders(response, config, request, span);
    span->End();
    return response;
  });

  CROW_ROUTE(app, "/weatherforecast/throw-custom-exception")([tracer, allowed_origins, config](const crow::request &request) {
    auto span = StartServerSpan(
        config,
        tracer,
        request,
        "GET /weatherforecast/throw-custom-exception",
        "/weatherforecast/throw-custom-exception");
    trace_api::Scope scope(span);

    crow::response response;
    response.set_header("Content-Type", "application/json");
    AddCorsHeaders(response, allowed_origins);

    try
    {
      std::cout << "C++ custom exception test endpoint called" << std::endl;
      ThrowCustomTestException();
    }
    catch (const std::exception &ex)
    {
      RecordException(span, ex, "GET /weatherforecast/throw-custom-exception");
      response.code = 500;
      response.body = nlohmann::json({{"message", ex.what()}}).dump();
    }

    MaybeAttachTraceDebugHeaders(response, config, request, span);
    span->End();
    return response;
  });

  CROW_ROUTE(app, "/weatherforecast/throw-and-catch-exception")([tracer, allowed_origins, config](const crow::request &request) {
    auto span = StartServerSpan(
        config,
        tracer,
        request,
        "GET /weatherforecast/throw-and-catch-exception",
        "/weatherforecast/throw-and-catch-exception");
    trace_api::Scope scope(span);

    try
    {
      ThrowNestedTestException();
    }
    catch (const std::exception &ex)
    {
      std::cerr << "[Handled Exception] " << ex.what() << std::endl;
      RecordException(span, ex, "GET /weatherforecast/throw-and-catch-exception");
    }

    crow::response response(200);
    response.set_header("Content-Type", "application/json");
    AddCorsHeaders(response, allowed_origins);
    response.body = R"({"message":"Exception was thrown, caught, and printed to console.","handled":true})";

    MaybeAttachTraceDebugHeaders(response, config, request, span);
    span->End();
    return response;
  });

  CROW_ROUTE(app, "/<path>")([allowed_origins](const std::string &) {
    crow::response response(404);
    AddCorsHeaders(response, allowed_origins);
    response.set_header("Content-Type", "application/json");
    response.body = R"({"message":"Not found"})";
    return response;
  });
}
} // namespace otelapicpp
