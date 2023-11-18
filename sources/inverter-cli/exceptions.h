#pragma once

#include <exception>
#include <string>
#include <string_view>


/// Thrown when some operation takes too long to execute so it's aborted due to a timeout.
class TimeoutException : public std::exception {
 public:
  TimeoutException(std::string_view message) : message_(message) {}

  const char* what() const noexcept override { return message_.c_str(); }

 private:
  std::string message_;
};
