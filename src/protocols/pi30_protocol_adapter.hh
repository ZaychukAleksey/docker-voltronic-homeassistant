#pragma once

#include "protocol_adapter.hh"
#include "mqtt/sensor.hh"


class Pi30ProtocolAdapter : public ProtocolAdapter {
 public:
  explicit Pi30ProtocolAdapter(const SerialPort& port) : ProtocolAdapter(port) {}

  void GetRatedInfo() override;

  void GetMode() override;
  void GetStatusInfo() override;
  void GetWarnings() override;

 protected:
  bool UseCrcInQueries() override { return true; }

  std::string GetDeviceProtocolIdRaw() { return Query("QPI", "(PI"); }
  std::string GetSerialNumberRaw() { return Query("QID", "("); }
  std::string GetMainCpuFirmwareVersionRaw() { return Query("QVFW", "(VERFW:"); }
  std::string GetAnotherCpuFirmwareVersionRaw() { return Query("QVFW2", "(VERFW2:"); }
  std::string GetDeviceRatingInformationRaw() { return Query("QPIRI", "("); }
  std::string GetDeviceFlagStatusRaw() { return Query("QFLAG", "("); }
  std::string GetDeviceGeneralStatusRaw() { return Query("QPIGS", "("); }
  std::string GetDeviceModeRaw() { return Query("QMOD", "("); }
  std::string GetDeviceWarningStatusRaw() { return Query("QPIWS", "("); }
  std::string GetDefaultSettingValueInformationRaw() { return Query("QDI", "("); }
  std::string GetSelectableValueAboutMaxChargingCurrentRaw() { return Query("QMCHGCR", "("); }
  std::string GetSelectableValueAboutMaxUtilityChargingCurrentRaw() { return Query("QMUCHGCR", "("); }
  std::string GetDspHasBootstrapOrNotRaw() { return Query("QBOOT", "("); }
  std::string GetOutputModeRaw() { return Query("QOPM", "("); }
  // ...
  // Here could go routines to query data for parallel system, but I haven't implemented them.
  // ...

  mqtt::ModeSelector mode_;

  mqtt::BatteryNominalVoltage battery_nominal_voltage_;
  mqtt::BatteryStopDischargingVoltageWithGrid battery_stop_discharging_voltage_with_grid_;
  mqtt::BatteryStopChargingVoltageWithGrid battery_stop_charging_voltage_with_grid_;
  mqtt::BatteryUnderVoltage battery_under_voltage_;
  mqtt::BatteryBulkVoltage battery_bulk_voltage_;
  mqtt::BatteryFloatVoltage battery_float_voltage_;
  mqtt::BatteryType battery_type_;

  mqtt::OutputSourcePrioritySelector output_source_priority_;
  mqtt::ChargerSourcePrioritySelector charger_source_priority_;

  mqtt::GridVoltage grid_voltage_;
  mqtt::GridFrequency grid_frequency_;
  mqtt::OutputVoltage ac_output_voltage_;
  mqtt::OutputFrequency ac_output_frequency_;
  mqtt::OutputApparentPower ac_output_apparent_power_;
  mqtt::OutputActivePower ac_output_active_power_;
  mqtt::OutputLoadPercent output_load_percent_;

  mqtt::BatteryVoltage battery_voltage_;
  mqtt::BatteryChargeCurrent battery_charging_current_;
  mqtt::BatteryDischargeCurrent battery_discharge_current_;
  mqtt::BatteryCapacity battery_capacity_;
  mqtt::BatteryVoltageFromScc battery_voltage_from_scc_;

  mqtt::PvWatts pv_input_power_;
  mqtt::PvBusVoltage pv_bus_voltage_;

  mqtt::HeatsinkTemperature inverter_heat_sink_temperature_;
};
