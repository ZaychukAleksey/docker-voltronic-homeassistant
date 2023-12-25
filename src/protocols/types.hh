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

enum class BatteryType : char {
  kAgm,             //  AGM
  kFlooded,         //  Flooded
  kUser,            //  User-defined
  kPYL,             //  PYL (5048MG & 5048MGX Remote Panel Communication Protocol)
  kSH,              //  SH (5048MG & 5048MGX Remote Panel Communication Protocol)
};

inline std::string_view BatteryTypeToString(BatteryType type) {
  switch (type) {
    case BatteryType::kAgm: return "AGM";
    case BatteryType::kFlooded: return "Flooded";
    case BatteryType::kUser: return "User-defined";
    case BatteryType::kPYL: return "PYL";
    case BatteryType::kSH: return "SH";
  }
  throw std::runtime_error(fmt::format("Unknown BatteryType: {}", (int) type));
}

enum class ChargerPriority : char {
  kUtilityFirst,    //  Utility first
  kSolarFirst,      //  Solar first
  kSolarAndUtility, //  Solar + Utility
  kOnlySolar,       //  Only solar charging permitted
};

inline std::string_view ChargerPriorityToString(ChargerPriority priority) {
  switch (priority) {
    case ChargerPriority::kUtilityFirst: return "Utility";
    case ChargerPriority::kSolarFirst: return "Solar";
    case ChargerPriority::kSolarAndUtility: return "Solar+Utility";
    case ChargerPriority::kOnlySolar: return "Solar only";
  }
  throw std::runtime_error(fmt::format("Unknown ChargerPriority: {}", (int) priority));
}

enum class OutputSourcePriority : char {
  kUtility,                   //  Utility -> ??? -> ???
  kSolarUtilityBattery,       //  Solar -> Utility -> Battery
  kSolarBatteryUtility,       //  Solar -> Battery -> Utility
};

inline std::string_view OutputSourcePriorityToString(OutputSourcePriority priority) {
  switch (priority) {
    case OutputSourcePriority::kUtility: return "Utility";
    case OutputSourcePriority::kSolarUtilityBattery: return "Solar->Utility->Battery";
    case OutputSourcePriority::kSolarBatteryUtility: return "Solar->Battery->Utility";
  }
  throw std::runtime_error(fmt::format("Unknown OutputSourcePriority: {}", (int) priority));
}

enum class OutputMode : char {
  kSingle,          //  single machine output
  kParallel,        //  parallel output
  kPhase1Of3,       //  Phase 1 of 3 Phase output
  kPhase2Of3,       //  Phase 2 of 3 Phase output
  kPhase3Of3,       //  Phase 3 of 3 Phase output
};

enum class MachineType : char {
  kGridTie,         //  Grid tie
  kOffGrid,         //  Off Grid
  kHybrid,          //  Hybrid
};

enum class Topology : char {
  kTransformless,   //  transformerless
  kTransformer,     //  transformer
};

enum class InputVoltageRange : char {
  kAppliance,       // Appliance
  kUps,             // UPS
};
