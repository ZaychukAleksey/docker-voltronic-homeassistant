#pragma once

#include <memory>
#include <vector>

#include "../serial_port.hh"
#include "device_mode.hh"
#include "inverter_data.hh"
#include "protocol.hh"


class SerialPort;


class ProtocolAdapter {
 public:
  static std::unique_ptr<ProtocolAdapter> Get(Protocol, const SerialPort&);

  [[nodiscard]] virtual DeviceMode GetMode() = 0;
  [[nodiscard]] virtual std::vector<std::string> GetWarnings() = 0;

  /// @see descriptions for StatusInfo and RatedInformation.
  [[nodiscard]] virtual StatusInfo GetStatusInfo() = 0;
  [[nodiscard]] virtual RatedInformation GetRatedInfo() = 0;

 protected:
  ProtocolAdapter(SerialPort&&) = delete;
  explicit ProtocolAdapter(const SerialPort& port) : port_(port) {}

  virtual bool UseCrcInQueries() = 0;
  std::string Query(std::string_view query, std::string_view expected_response_prefix = "");


  const SerialPort& port_;
};
