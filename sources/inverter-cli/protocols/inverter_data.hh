#pragma once

/// Device general status parameters.
/// They reflect instant grid/PV/battery characteristics.
struct StatusInfo {
  ///{ Info about grid.
  float grid_voltage;                               // volts (V).
  float grid_frequency;                             // Hz.
  ///}

  ///{ Info about the output.
  float ac_output_voltage;                          // volts (V)
  float ac_output_frequency;                        // Hz.
  int ac_output_active_power;                       // watts (W)
  int ac_output_apparent_power;                     // VA
  int output_load_percent;                          // Maximum of W% or VA%.
  ///}

  ///{ Info about batteries
  int battery_capacity;                             // %
  float battery_voltage;                            // volts (V).
  // SCC - Solar Charge Controller.
  float battery_voltage_from_scc;                   // volts (V)
  float battery_voltage_from_scc2 = 0;              // Optional. volts (V)
  int battery_discharge_current;                    // amps (A).
  int battery_charging_current;                     // amps (A).
  ///}

  ///{ PV (Photovoltaics) data.
  int pv_input_power;                               // watts (W)
  int pv2_input_power = 0;                          // Optional. watts (W)
  float pv_input_voltage;                           // volts (V)
  float pv2_input_voltage = 0;                      // Optional. volts (V)
  // "BUS" means the connection (conductor) between solar panels and the inverter.
  int pv_bus_voltage = 0;                              // Optional. volts (V).
  ///}

  ///{ Various status info.
  int inverter_heat_sink_temperature;               // °C
  int mptt1_charger_temperature = 0;                // Optional. °C
  int mptt2_charger_temperature = 0;                // Optional. °C
  bool load_connection_on;
  ///}
};

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


/// Device Rating Information.
/// Rating information reflects inverter's nominal parameters. E.g. GeneralInfo::grid_voltage shows
/// the current grid voltage, it can fluctuate, whereas RatedInformation::grid_rating_voltage is the
/// nominal nominal voltage level that the inverter is designed to operate at.
struct RatedInformation {
  float grid_rating_voltage;                        // volts (V).
  float grid_rating_current;                        // amps (A).
  float ac_output_rating_voltage;                   // volts (V).
  float ac_output_rating_frequency;                 // Hz.
  float ac_output_rating_current;                   // amps (A).
  int ac_output_rating_apparent_power;              // VA.
  int ac_output_rating_active_power;                // watts (W).

  BatteryType battery_type;
  /// Nominal voltage of the battery, which is the voltage level at which it is designed to operate.
  /// For example, a 12-volt lead-acid battery has a rating voltage of 12 volts.
  float battery_nominal_voltage;                     // volts (V).
  /// Battery stop discharging voltage when grid is available.
  /// Also called "battery recharge voltage". Apparently used, when the inverter is instructed to
  /// drain the batteries even when the grid is available (Solar -> Battery -> Utility).
  float battery_stop_discharging_voltage_with_grid;    // volts (V).
  /// Battery stop charging voltage when grid is available.
  /// Also called "battery re-discharge voltage".
  float battery_stop_charging_voltage_with_grid;       // volts (V).
  /// Voltage level at which the inverter maintains a constant voltage to keep the battery fully
  /// charged.
  float battery_float_voltage;                      // volts (V).
  /// Voltage level at which the inverter delivers maximum charging current to the battery during
  /// the bulk charging phase.
  /// @note that's not batteries' voltage. It's the voltage that the inverter uses to charge them.
  /// The bulk charging phase is the initial stage of battery charging where the inverter delivers
  /// maximum charging current to the battery to quickly charge it.
  float battery_bulk_voltage;                       // volts (V).
  /// Cut off voltage, i.e. the voltage level at which the inverter will shut off to protect the
  /// battery from over-discharging.
  /// It is typically set slightly above the battery re-discharge voltage to provide a buffer.
  float battery_under_voltage;                      // volts (V).

  int max_ac_charging_current;              // amps (A).
  int max_charging_current;                 // amps (A).
  InputVoltageRange input_voltage_range;
  OutputSourcePriority output_source_priority;
  ChargerPriority charger_source_priority;
  int parallel_max_num;
  MachineType machine_type;
  Topology topology;
  OutputMode output_mode;
};
