#pragma once

#include <cstdlib>
#ifdef __linux__
#include <execinfo.h>
#endif
#include <stdexcept>
#include <sstream>
#include <string_view>
#include <string>

namespace otelapicpp
{
inline std::string CaptureStackTrace()
{
#ifdef __linux__
  constexpr int kMaxFrames = 32;
  void *frames[kMaxFrames];
  const int frame_count = backtrace(frames, kMaxFrames);
  if (frame_count <= 0)
  {
    return {};
  }

  char **symbols = backtrace_symbols(frames, frame_count);
  if (symbols == nullptr)
  {
    return {};
  }

  std::ostringstream out;
  for (int i = 0; i < frame_count; ++i)
  {
    out << symbols[i] << '\n';
  }

  free(symbols);
  return out.str();
#else
  return {};
#endif
}

class CustomTestException : public std::runtime_error
{
public:
  explicit CustomTestException(const std::string &message)
      : std::runtime_error(message), throw_stacktrace_(CaptureStackTrace())
  {
  }

  const std::string &throw_stacktrace() const noexcept
  {
    return throw_stacktrace_;
  }

private:
  std::string throw_stacktrace_;
};
} // namespace otelapicpp
