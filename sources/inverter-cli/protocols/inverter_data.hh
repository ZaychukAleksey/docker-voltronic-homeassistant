#pragma once

#include <optional>

/// Device general status parameters.
struct GeneralInfo {
  float grid_voltage;   // volts (V).
  float grid_frequency;   // Hz.
  
  float ac_output_voltage;  // volts (V)
  float ac_output_frequency;  // Hz.
  int ac_output_apparent_power;  // VA
  int ac_output_active_power;    // watts (W)
  int output_load_percent;       // Maximum of W% or VA%.
  std::optional<int> bus_voltage;  // volts (V).
  float battery_voltage;  // volts (V).
  float battery_voltage_from_scc; // volts (V). SCC - Solar Charge Controller
  std::optional<float> battery_voltage_from_scc2;  // volts (V). SCC - Solar Charge Controller
  int battery_discharge_current;  // amps (A).
  int battery_charging_current;   // amps (A).
  int battery_capacity;   // %
  int inverter_heat_sink_temperature;   // °C
  std::optional<int> mptt1_charger_temperature;   // °C
  std::optional<int> mptt2_charger_temperature;   // °C
  int pv_input_power;   // watts (W)
  std::optional<int> pv2_input_power;   // watts (W)
  float pv_input_voltage;   // volts (V)
  std::optional<float> pv2_input_voltage;   // volts (V)

  char device_status[8];  // b7b6b5b4b3b2b1b0
                          //  b7: add SBU priority version, 1:yes, 0:no
                          //  b6: configuration status: 1:Change, 0:unchanged
                          //  b5: SCC firmware version: 1:Updated, 0:unchanged
                          //  b4: Load status: 0:Load off, 1:Load on
                          //  b3: battery voltage to steady while charging
                          //  b2: Charging status(Charging on/off)
                          //  b1: Charging status(SCC charging on/off)
                          //  b0: Charging status(AC charging on/off)
                          //  b2b1b0: 000: Do nothing
                          //          110: Charging on with SCC charge on
                          //          101: Charging on with AC charge on
                          //          111: Charging on with SCC and AC charge on
  /// @}

  /// Additional fields for 5048MG & 5048MGX Remote Panel Communication Protocol
  /// @{
  int battery_voltage_offset_for_fans_on;
  int eeprom_version;
  int pv_charging_power;
  char device_status_2[3];  // b10b9b8
                            //  b10: flag for charging to floating mode
                            //  b9: Switch On
                            //  b8: flag for dustproof installed
                            //      1-dustproof installed
                            //      0-no dustproof
  /// @}
};

enum class BatteryType : char {
  kAgm,             //  AGM
  kFlooded,         //  Flooded
  kUser,            //  User-defined
  kPYL,             //  PYL (5048MG & 5048MGX Remote Panel Communication Protocol)
  kSH,              //  SH (5048MG & 5048MGX Remote Panel Communication Protocol)
};

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
    case ChargerPriority::kSolarAndUtility: return "Solar + Utility";
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
    case OutputSourcePriority::kSolarUtilityBattery: return "Solar -> Utility -> Battery";
    case OutputSourcePriority::kSolarBatteryUtility: return "Solar -> Battery -> Utility";
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

/// Device Rating Information
/// This structure holds the data that the inverter returns in response to such types of queries.
struct RatedInformation {
  float grid_rating_voltage;  // volts (V).
  float grid_rating_current;  // amps (A).
  float ac_output_rating_voltage;  // volts (V).
  float ac_output_rating_frequency;  // Hz.
  float ac_output_rating_current;  // amps (A).
  int ac_output_rating_apparent_power;    // VA.
  int ac_output_rating_active_power;      // watts (W).
  float battery_rating_voltage;  // volts (V).
  float battery_recharge_voltage;  // volts (V).
  float battery_redischarge_voltage;  // volts (V).
  float battery_under_voltage;  // volts (V).
  float battery_bulk_voltage;  // volts (V).
  float battery_float_voltage;  // volts (V).
  BatteryType battery_type;
  int current_max_ac_charging_current;  // amps (A).
  int current_max_charging_current;  // amps (A).
  InputVoltageRange input_voltage_range;
  OutputSourcePriority output_source_priority;
  ChargerPriority charger_source_priority;
  int parallel_max_num;
  MachineType machine_type;
  Topology topology;
  OutputMode output_mode;
};
