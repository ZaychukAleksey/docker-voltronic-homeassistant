#include "protocol_adapter.hh"

#include "../exceptions.h"
#include "pi18_protocol_adapter.hh"
#include "pi30_protocol_adapter.hh"

#include "spdlog/spdlog.h"

namespace {

void CheckStartsWith(std::string_view response, std::string_view expected_prefix) {
  if (!response.starts_with(expected_prefix)) {
    throw std::runtime_error(
        fmt::format("Response '{}' is expected to start with '{}'", response, expected_prefix));
  }
}

}  // namespace


std::unique_ptr<ProtocolAdapter> ProtocolAdapter::Get(Protocol protocol, const SerialPort& port) {
  switch (protocol) {
    case Protocol::PI17: throw UnsupportedProtocolException("PI17");
    case Protocol::PI18: return std::make_unique<Pi18ProtocolAdapter>(port);
    case Protocol::PI30: return std::make_unique<Pi30ProtocolAdapter>(port);;
  }
  throw std::runtime_error("Unreachable");
}

std::string ProtocolAdapter::Query(std::string_view query,
                                   std::string_view expected_response_prefix) {
  auto response = port_.Query(query, UseCrcInQueries());
  CheckStartsWith(response, expected_response_prefix);
  response.erase(0, expected_response_prefix.length());
  return response;
}
