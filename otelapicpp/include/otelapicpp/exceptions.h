#pragma once

#include <stdexcept>
#include <string>

namespace otelapicpp
{
class CustomTestException : public std::runtime_error
{
public:
  explicit CustomTestException(const std::string &message) : std::runtime_error(message) {}
};
} // namespace otelapicpp
