#pragma once

#include "protocol_adapter.hh"
#include "mqtt/sensor.hh"


class Pi30ProtocolAdapter : public ProtocolAdapter {
 public:
  explicit Pi30ProtocolAdapter(const SerialPort&);

  std::string GetSerialNumber() override { return GetSerialNumberRaw(); }
  void QueryProtocolId() override { GetDeviceProtocolIdRaw(); };

  void GetRatedInfo() override;
  void GetStatusInfo() override;

 protected:
  bool UseCrcInQueries() override { return true; }

  bool SetInputVoltageRange(InputVoltageRange);
  bool SetChargerPriority(ChargerPriority);
  bool SetOutputSourcePriority(OutputSourcePriority);
  bool SetBatteryType(BatteryType);

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

 private:
  bool SendCommand(std::string_view);


  mqtt::InverterMode mode_;

  mqtt::BatteryNominalVoltage battery_nominal_voltage_;
  mqtt::BatteryStopDischargingVoltageWithGrid battery_stop_discharging_voltage_with_grid_;

//  12V unit: 00.0V12V/12.3V/12.5V/12.8V/13V/13.3V/13.5V/13.8V/14V/14.3V/14.5
//      24V unit: 00.0V/24V/24.5V/25V/25.5V/26V/26.5V/27V/27.5V/28V/28.5V/29V
//      48V unit: 00.0V48V/49V/50V/51V/52V/53V/54V/55V/56V/57V/58V
//  mqtt::BatteryStopChargingVoltageWithGrid battery_stop_charging_voltage_with_grid_;
  mqtt::BatteryUnderVoltage battery_under_voltage_;
  mqtt::BatteryBulkVoltage battery_bulk_voltage_;
  mqtt::BatteryFloatVoltage battery_float_voltage_;
  mqtt::BatteryType battery_type_{
      {BatteryType::kAgm, BatteryType::kFlooded},
      [this](BatteryType b) { return SetBatteryType(b); }
  };

  mqtt::AcInputVoltageRangeSelector input_voltage_range_{
      {InputVoltageRange::kAppliance, InputVoltageRange::kUps},
      [this](InputVoltageRange r) { return SetInputVoltageRange(r); }
  };

  mqtt::OutputSourcePrioritySelector output_source_priority_{
      {OutputSourcePriority::kUtility,
       OutputSourcePriority::kSolarUtilityBattery,
       OutputSourcePriority::kSolarBatteryUtility},
      [this](OutputSourcePriority p) { return SetOutputSourcePriority(p); }
  };
  mqtt::ChargerSourcePrioritySelector charger_source_priority_{
      {ChargerPriority::kUtilityFirst,
       ChargerPriority::kSolarFirst,
       ChargerPriority::kSolarAndUtility,
       ChargerPriority::kOnlySolar},
      [this](ChargerPriority p) { return SetChargerPriority(p); }
  };

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

  // TODO: implement.
//  mqtt::BacklightSwitch backlight_{[this](bool state) { return TurnBacklight(state); }};
};
