#pragma once

#include <exception>
#include <string>
#include <string_view>


/// Base class for exceptions
class BaseException : public std::exception {
 public:
  const char* what() const noexcept override { return message_.c_str(); }
 protected:
  BaseException(std::string_view message) : message_(message) {}
 private:
  const std::string message_;
};


/// Thrown when some operation takes too long to execute so it's aborted due to a timeout.
class TimeoutException : public BaseException {
 public:
  TimeoutException(std::string_view message) : BaseException(message) {}
};


/// Thrown when device replies with an incorrect CRC.
class CrcMismatchException : public std::exception {
 public:
  const char* what() const noexcept override { return "CRC doesn't match"; }
};


/// Thrown when connecting to the device requires the usage of the protocol that is unsupported.
class UnsupportedProtocolException : public BaseException {
 public:
  UnsupportedProtocolException(std::string_view protocol) : BaseException(
      "Unsupported protocol: " + std::string(protocol)) {}
};
