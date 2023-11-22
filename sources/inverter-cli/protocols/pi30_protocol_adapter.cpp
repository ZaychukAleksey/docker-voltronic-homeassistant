#include "pi30_protocol_adapter.hh"

#include "spdlog/spdlog.h"


namespace {

//Generate 8 bit CRC of supplied string + 1 as used in REVO PI30 protocol
//CHK=DATE0+....+1
//CHK=The accumulated value of sent data+1, single byte.
char CHK(std::string_view s) {
  int crc = 0;
  for (char c: s) {
    crc += c;
  }
  ++crc;
  crc &= 0xFF;
  return crc;
}

}  // namespace

DeviceMode Pi30ProtocolAdapter::GetMode() {
  // Expected response: "M"
  const auto mode = GetDeviceModeRaw();
  if (mode == "P") return DeviceMode::kPowerOn;
  if (mode == "S") return DeviceMode::kStandby;
  if (mode == "L") return DeviceMode::kLine;
  if (mode == "B") return DeviceMode::kBattery;
  if (mode == "F") return DeviceMode::kFault;
  if (mode == "H") return DeviceMode::kPowerSaving;
  if (mode == "D") return DeviceMode::kShutdown;
  if (mode == "C") return DeviceMode::kCharge;
  if (mode == "E") return DeviceMode::kEco;
  throw std::runtime_error(fmt::format("Unknown device mode: {}", mode));
}

RatedInformation Pi30ProtocolAdapter::GetRatedInfo() {
  throw std::runtime_error("Not implemented");
}

std::vector<std::string> Pi30ProtocolAdapter::GetWarnings() {
  throw std::runtime_error("Not implemented");
}

StatusInfo Pi30ProtocolAdapter::GetStatusInfo() {
  throw std::runtime_error("Not implemented");
}
