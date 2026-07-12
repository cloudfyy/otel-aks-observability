#pragma once

#include <nlohmann/json.hpp>

namespace otelapicpp
{
nlohmann::json BuildForecastPayload();
void ThrowNestedTestException();
} // namespace otelapicpp
