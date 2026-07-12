#include "otelapicpp/api.h"

#include <exception>
#include <iostream>
#include <string>
#include <typeinfo>

#include <nlohmann/json.hpp>

#include <opentelemetry/context/context.h>
#include <opentelemetry/context/propagation/text_map_propagator.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/trace/scope.h>

#include "otelapicpp/exceptions.h"
#include "otelapicpp/weather.h"

namespace trace_api = opentelemetry::trace;

namespace otelapicpp
{
namespace
{
class CrowRequestCarrier : public opentelemetry::context::propagation::TextMapCarrier
{
public:
  explicit CrowRequestCarrier(const crow::request &request) : request_(request) {}

  opentelemetry::nostd::string_view Get(opentelemetry::nostd::string_view key) const noexcept override
  {
    cached_value_ = request_.get_header_value(std::string(key));
    if (cached_value_.empty())
    {
      return "";
    }

    return cached_value_;
  }

  void Set(opentelemetry::nostd::string_view, opentelemetry::nostd::string_view) noexcept override
  {
    // Request carrier is read-only for extraction.
  }

private:
  const crow::request &request_;
  mutable std::string cached_value_;
};

void AddCorsHeaders(crow::response &response, const std::string &allowed_origins)
{
  response.add_header("Access-Control-Allow-Origin", allowed_origins);
  response.add_header("Access-Control-Allow-Headers", "*");
  response.add_header("Access-Control-Allow-Methods", "GET,OPTIONS");
}

void RecordException(
    const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> &span,
    const std::exception &ex)
{
  const auto *custom_exception = dynamic_cast<const CustomTestException *>(&ex);
  const auto exception_type = std::string(
      custom_exception != nullptr ? "CustomTestException" : typeid(ex).name());
  const auto message = std::string(ex.what());
  span->SetAttribute("exception.type", exception_type);
  span->SetAttribute("exception.message", message);
  span->AddEvent(
      "exception",
      {{"exception.type", exception_type},
       {"exception.message", message},
       {"exception.escaped", false}});
  span->SetStatus(opentelemetry::trace::StatusCode::kError, message);
}

opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> StartServerSpan(
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

  return span;
}
} // namespace

void RegisterRoutes(
    crow::SimpleApp &app,
    const std::string &allowed_origins,
    const opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> &tracer)
{
  CROW_ROUTE(app, "/health")([] {
    return crow::response(200, "ok");
  });

  CROW_ROUTE(app, "/weatherforecast")([tracer, allowed_origins](const crow::request &request) {
    auto span = StartServerSpan(tracer, request, "GET /weatherforecast", "/weatherforecast");
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
      RecordException(span, ex);
      response.code = 500;
      response.body = R"({"message":"Unexpected error while building weather forecast."})";
    }

    span->End();
    return response;
  });

  CROW_ROUTE(app, "/weatherforecast/throw-custom-exception")([tracer, allowed_origins](const crow::request &request) {
    auto span = StartServerSpan(
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
      RecordException(span, ex);
      response.code = 500;
      response.body = nlohmann::json({{"message", ex.what()}}).dump();
    }

    span->End();
    return response;
  });

  CROW_ROUTE(app, "/weatherforecast/throw-and-catch-exception")([tracer, allowed_origins](const crow::request &request) {
    auto span = StartServerSpan(
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
      RecordException(span, ex);
    }

    crow::response response(200);
    response.set_header("Content-Type", "application/json");
    AddCorsHeaders(response, allowed_origins);
    response.body = R"({"message":"Exception was thrown, caught, and printed to console.","handled":true})";

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
