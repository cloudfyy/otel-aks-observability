#pragma once

#include <nlohmann/json.hpp>

namespace otelapicpp
{
nlohmann::json BuildForecastPayload();
void ThrowCustomTestException();
void ThrowNestedTestException();
} // namespace otelapicpp
