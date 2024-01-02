#include "protocol_adapter.hh"

#include <format>

#include "../exceptions.h"
#include "pi18_protocol_adapter.hh"
#include "pi30_protocol_adapter.hh"

#include "spdlog/spdlog.h"

namespace {

void CheckStartsWith(std::string_view response, std::string_view expected_prefix) {
  if (!response.starts_with(expected_prefix)) {
    throw std::runtime_error(
        std::format("Response '{}' is expected to start with '{}'", response, expected_prefix));
  }
}

std::unique_ptr<ProtocolAdapter> TryProtocol(Protocol p, SerialPort& port) {
  auto adapter = ProtocolAdapter::Get(p, port);
  try {
    adapter->QueryProtocolId();
    spdlog::debug("Using protocol {}", ToString(p));
    return adapter;
  } catch (const std::exception& e) {
    spdlog::debug("Failed to try protocol {}: {}", ToString(p), e.what());
  }
  return nullptr;
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

std::unique_ptr<ProtocolAdapter> DetectProtocol(SerialPort& port) {
  for (auto protocol : {Protocol::PI30, Protocol::PI18}) {
    if (auto adapter = TryProtocol(protocol, port)) {
      return adapter;
    }
  }
  throw UnsupportedProtocolException();
}
