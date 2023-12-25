#pragma once

#include <memory>
#include <vector>

#include "serial_port.hh"
#include "protocol.hh"


class ProtocolAdapter {
 public:
  static std::unique_ptr<ProtocolAdapter> Get(Protocol, const SerialPort&);

  /// Information that doesn't change over time (various settings and presets), or changes only when
  /// some relevant setting is directly changed by the user (i.e. by this application).
  /// It can be retrieved once then updated quire rarely.
  /// Rating information reflects inverter's nominal parameters. E.g. @a instant grid_voltage shows
  /// the current grid voltage, it can fluctuate, whereas @a rated grid_rating_voltage is the
  /// nominal voltage level that the inverter is designed to operate at.
  virtual void GetRatedInfo() = 0;

  /// The current state of the inverter (volatile, instant metrics).
  virtual void GetMode() = 0;
  virtual void GetStatusInfo() = 0;
  virtual void GetWarnings() = 0;

 protected:
  ProtocolAdapter(SerialPort&&) = delete;
  explicit ProtocolAdapter(const SerialPort& port) : port_(port) {}

  virtual bool UseCrcInQueries() = 0;
  std::string Query(std::string_view query, std::string_view expected_response_prefix = "");


  const SerialPort& port_;
};
