#pragma once

#include "protocol_adapter.hh"

#include "mqtt/sensor.hh"

class Pi18ProtocolAdapter : public ProtocolAdapter {
 public:
  explicit Pi18ProtocolAdapter(const SerialPort&);

  void QueryProtocolId() override { GetProtocolIdRaw(); };
  void GetRatedInfo() override;

  void GetMode() override;
  void GetStatusInfo() override;
  void GetWarnings() override;

 protected:
  bool UseCrcInQueries() override { return true; }
  void GetTotalGeneratedEnergy();

  void SetChargerPriority(ChargerPriority);

  std::string GetProtocolIdRaw() { return Query("^P005PI", "^D00518"); }
  std::string GetCurrentTimeRaw() { return Query("^P005PI", "^D017"); }
  std::string GetTotalGeneratedEnergyRaw() { return Query("^P005ET", "^D011"); }
  std::string GetGeneratedEnergyOfYearRaw(std::string_view year);
  std::string GetGeneratedEnergyOfMonthRaw(std::string_view year, std::string_view month);
  std::string GetGeneratedEnergyOfDayRaw(
      std::string_view year, std::string_view month, std::string_view day);
  std::string GetSeriesNumberRaw() { return Query("^P005ID", "^D025"); }
  std::string GetCpuVersionRaw() { return Query("^P006VFW", "^D020"); }
  std::string GetRatedInformationRaw() { return Query("^P007PIRI", "^D0"); }
  std::string GetGeneralStatusRaw() { return Query("^P005GS", "^D106"); }
  std::string GetWorkingModeRaw() { return Query("^P006MOD", "^D005"); }
  std::string GetFaultAndWarningStatusRaw() { return Query("^P005FWS", "^D0"); }
  std::string GetEnableDisableFlagStatusRaw() { return Query("^P007FLAG", "^D020"); }
  std::string GetDefaultValueOfChangeableParameterRaw() { return Query("^P005DI", "^D068"); }
  std::string GetMaxChargingCurrentSelectableValueRaw() { return Query("^P009MCHGCR", "^D030"); }
  std::string GetMaxAcChargingCurrentSelectableValueRaw() { return Query("^P010MUCHGCR", "^D030"); }
  // ...
  // Here could go routines to query data for parallel system, but I haven't implemented them.
  // ...
  std::string GetAcChargeTimeBucketRaw() { return Query("^P005ACCT", "^D012"); }
  std::string GetAcSupplyLoadTimeBucketRaw() { return Query("^P005ACLT", "^D012"); }

 private:
  mqtt::ModeSelector mode_;

  mqtt::BatteryNominalVoltage battery_nominal_voltage_;
  mqtt::BatteryStopDischargingVoltageWithGrid battery_stop_discharging_voltage_with_grid_;
  mqtt::BatteryStopChargingVoltageWithGrid battery_stop_charging_voltage_with_grid_;
  mqtt::BatteryUnderVoltage battery_under_voltage_;
  mqtt::BatteryBulkVoltage battery_bulk_voltage_;
  mqtt::BatteryFloatVoltage battery_float_voltage_;
  mqtt::BatteryType battery_type_;

  mqtt::OutputSourcePrioritySelector output_source_priority_;
  mqtt::ChargerSourcePrioritySelector charger_source_priority_{
      {ChargerPriority::kSolarFirst,
       ChargerPriority::kSolarAndUtility,
       ChargerPriority::kOnlySolar},
       [this](ChargerPriority p) { SetChargerPriority(p); }
  };

  // Instant metrics.
  mqtt::GridVoltage grid_voltage_;
  mqtt::GridFrequency grid_frequency_;
  mqtt::OutputVoltage ac_output_voltage_;
  mqtt::OutputFrequency ac_output_frequency_;
  mqtt::OutputApparentPower ac_output_apparent_power_;
  mqtt::OutputActivePower ac_output_active_power_;
  mqtt::OutputLoadPercent output_load_percent_;

  mqtt::BatteryVoltage battery_voltage_;
  mqtt::BatteryVoltageFromScc battery_voltage_from_scc_;
  mqtt::BatteryVoltageFromScc2 battery_voltage_from_scc2_;
  mqtt::BatteryDischargeCurrent battery_discharge_current_;
  mqtt::BatteryChargeCurrent battery_charging_current_;
  mqtt::BatteryCapacity battery_capacity_;

  mqtt::HeatsinkTemperature inverter_heat_sink_temperature_;
  mqtt::Mptt1ChargerTemperature mptt1_charger_temperature_;
  mqtt::Mptt2ChargerTemperature mptt2_charger_temperature_;
  mqtt::PvWatts pv_input_power_;
  mqtt::PvWatts2 pv2_input_power_;
  mqtt::PvVoltage pv_input_voltage_;
  mqtt::Pv2Voltage pv2_input_voltage_;
  mqtt::PvTotalGeneratedEnergy total_energy_;
};
