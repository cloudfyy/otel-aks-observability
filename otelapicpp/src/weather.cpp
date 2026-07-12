#include "otelapicpp/weather.h"

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "otelapicpp/exceptions.h"

namespace otelapicpp
{
namespace
{
int ToTemperatureF(int c)
{
  return 32 + static_cast<int>(c / 0.5556);
}

std::string DatePlusDays(int offset)
{
  const auto now = std::chrono::system_clock::now();
  const auto shifted = now + std::chrono::hours(24 * offset);
  const std::time_t tt = std::chrono::system_clock::to_time_t(shifted);
  std::tm tm{};
#ifdef _WIN32
  gmtime_s(&tm, &tt);
#else
  gmtime_r(&tt, &tm);
#endif

  std::ostringstream out;
  out << std::put_time(&tm, "%Y-%m-%d");
  return out.str();
}

void ThrowLevel3()
{
  throw CustomTestException("This exception is thrown and handled in C++ test endpoint.");
}

void ThrowLevel2()
{
  ThrowLevel3();
}

void ThrowLevel1()
{
  ThrowLevel2();
}
} // namespace

nlohmann::json BuildForecastPayload()
{
  static const std::vector<std::string> summaries = {
      "Freezing", "Bracing", "Chilly", "Cool", "Mild",
      "Warm", "Balmy", "Hot", "Sweltering", "Scorching"};

  thread_local std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<int> temp_dist(-20, 55);
  std::uniform_int_distribution<std::size_t> summary_dist(0, summaries.size() - 1);

  nlohmann::json result = nlohmann::json::array();
  for (int index = 1; index <= 5; ++index)
  {
    const int temperature_c = temp_dist(rng);
    result.push_back({
        {"date", DatePlusDays(index)},
        {"temperatureC", temperature_c},
        {"temperatureF", ToTemperatureF(temperature_c)},
        {"summary", summaries[summary_dist(rng)]},
    });
  }

  return result;
}

void ThrowCustomTestException()
{
  throw CustomTestException("This is a C++ custom exception for testing.");
}

void ThrowNestedTestException()
{
  ThrowLevel1();
}
} // namespace otelapicpp
