#include <iostream>

#include <crow.h>
#include "otelapicpp/api.h"
#include "otelapicpp/config.h"
#include "otelapicpp/telemetry.h"

int main()
{
  const auto config = otelapicpp::LoadAppConfig();
  otelapicpp::InitTracerProvider(config);
  auto tracer = otelapicpp::GetTracer();

  crow::SimpleApp app;
  otelapicpp::RegisterRoutes(app, config, config.allowed_origins, tracer);

  std::cout << "Starting otelapicpp on port " << config.port << std::endl;
  app.port(static_cast<uint16_t>(config.port)).multithreaded().run();
  return 0;
}
