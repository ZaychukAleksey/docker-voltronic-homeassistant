#pragma once

/// QPIGS is a request to inverter to obtain "Device general status parameters".
/// This structure holds the data that the inverter returns in response
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
