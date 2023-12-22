#pragma once

#include "spdlog/spdlog.h"

enum class DeviceMode : char {
  kPowerOn,             //  Power on mode
  kStandby,             //  Standby mode
  kBypass,              //  Bypass mode
  kBattery,             //  Battery mode
  kFault,               //  Fault mode
  kPowerSaving,         //  Power saving Mode
  kHybrid,              //  Hybrid mode(Line mode, Grid mode)
  kLine,                //  Line Mode
  kBatteryTest,         //  Battery test mode
  kShutdown,            //  Shutdown Mode
  kGrid,                //  Grid mode
  kCharge,              //  Charge mode
  kEco,                 //  ECO mode
};

inline std::string_view DeviceModeToString(DeviceMode mode) {
  switch (mode) {
    case DeviceMode::kPowerOn: return "Power on";
    case DeviceMode::kStandby: return "Standby";
    case DeviceMode::kBypass: return "Bypass";
    case DeviceMode::kBattery: return "Battery";
    case DeviceMode::kFault: return "Fault";
    case DeviceMode::kPowerSaving: return "Power saving";
    case DeviceMode::kHybrid: return "Hybrid";
    case DeviceMode::kLine: return "Line";
    case DeviceMode::kBatteryTest: return "Battery test";
    case DeviceMode::kShutdown: return "Shutdown";
    case DeviceMode::kGrid: return "Grid";
    case DeviceMode::kCharge: return "Charge";
    case DeviceMode::kEco: return "ECO";
  }
  throw std::runtime_error(fmt::format("Unknown DeviceMode: {}", (int) mode));
}