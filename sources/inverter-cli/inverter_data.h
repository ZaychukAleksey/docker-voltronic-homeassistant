#pragma once

/// QPIGS is a request to inverter to obtain "Device general status parameters".
/// This structure holds the data that the inverter returns in response.
struct QpigsData {
  /// The parameters below are in both HS_MS_MSX_RS232_Protocol and in
  /// 5048MG & 5048MGX Remote Panel Communication Protocol
  /// @{
  float grid_voltage;
  float grid_frequency;
  float ac_output_voltage;
  float ac_output_frequency;
  int ac_output_apparent_power;  // The units is VA
  int ac_output_active_power;    // The units is W
  int output_load_percent;       // Maximum of W% or VA%.
  int bus_voltage;
  float battery_voltage;
  int battery_charging_current;
  int battery_capacity;
  int inverter_heat_sink_temperature;
  float pv_input_current_for_battery;
  float pv_input_voltage;
  float battery_voltage_from_scc;
  int battery_discharge_current;
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


/// QPIRI is a request to inverter to obtain "Device Rating Information inquiry".
/// This structure holds the data that the inverter returns in response.
struct QpiriData {
  float grid_rating_voltage;
  float grid_rating_current;
  float ac_output_rating_voltage;
  float ac_output_rating_frequency;
  float ac_output_rating_current;
  int ac_output_rating_apparent_power;    // The unit is VA.
  int ac_output_rating_active_power;      // The unit is W.
  float battery_rating_voltage;
  float battery_recharge_voltage;
  float battery_under_voltage;
  float battery_bulk_voltage;
  float battery_float_voltage;
  int battery_type;                       // 0: AGM
                                          // 1: Flooded
                                          // 2: User
                                          // 3: PYL (5048MG & 5048MGX Remote Panel Communication Protocol)
                                          // 4: SH (5048MG & 5048MGX Remote Panel Communication Protocol)
  int current_max_ac_charging_current;
  int current_max_charging_current;
  int input_voltage_range;                // 0: Appliance
                                          // 1: UPS

  int output_source_priority;             // 0: Utility first
                                          // 1: Solar first
                                          // 2: SBU first

  int charger_source_priority;            // For HS Series:
                                          //      0: Utility first
                                          //      1: Solar first
                                          //      2: Solar + Utility
                                          //      3: Only solar charging permitted
                                          //  For MS/MSX Series 1K~3K:
                                          //      0: Utility first
                                          //      1: Solar first
                                          //      2: Solar + Utility
                                          //      3: Only solar charging permitted
  int parallel_max_num;
  int machine_type;                       // 00: Grid tie;
                                          // 01: Off Grid;
                                          // 10: Hybrid.
  int topology;                           // 0 transformerless
                                          // 1 transformer
  int out_mode;                           // 00: single machine output
                                          //  01: parallel output
                                          //  02: Phase 1 of 3 Phase output
                                          //  03: Phase 2 of 3 Phase output
                                          //  04: Phase 3 of 3 Phase output
  float battery_redischarge_voltage;
  int pv_ok_condition_for_parallel;       // 0: As long as one unit of inverters has connect PV,
                                          //    parallel system will consider PV OK;
                                          // 1: Only All of inverters have connect PV, parallel
                                          //    system will consider PV OK
  int pv_power_balance;                   // 0: PV input max current will be the max charged current
                                          // 1: PV input max power will be the sum of the max
                                          //    charged power and loads power.


  /// Additional fields for 5048MG & 5048MGX Remote Panel Communication Protocol
  /// @{
  int max_charging_time_at_cv_stage;
  /// @}
};
