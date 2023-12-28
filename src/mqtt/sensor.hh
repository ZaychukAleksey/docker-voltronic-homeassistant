#pragma once

#include <string_view>
#include <variant>
#include <vector>

#include "protocols/types.hh"

namespace mqtt {

enum class DeviceClass {
  kVoltage, // in volts (V)
  kCurrent, // in amps (A)
  kFrequency, // in hertz (Hz)
  kPower, // in watts (W)
  kApparentPower, // in volt-amperes (VA)
  kEnergy, // kilo watt hour, kWh
  kPercent, // no class, measurement is %
  kTemperature, // celsius, °C
  kBattery, // in %
  kNone, // no class
};


/// https://www.home-assistant.io/integrations/sensor.mqtt/
class Sensor {
 public:
  using Value = std::variant<int, float, DeviceMode, BatteryType, ChargerPriority,
      OutputSourcePriority>;

  /// @param device_class Optional. One of https://www.home-assistant.io/integrations/sensor/#device-class
  /// @param icon Optional. https://www.home-assistant.io/docs/configuration/customizing-devices/#icon
  constexpr Sensor(std::string_view name, DeviceClass device_class = DeviceClass::kNone)
      : name_(name), device_class_(device_class) {}

  void Update(Value new_value);

 protected:
  std::string SensorTopicRoot() const;
  std::string StateTopic() const { return std::format("{}/state", SensorTopicRoot()); }

  /// Sensor values will be sent to MQTT only when something changes.
  constexpr virtual bool UpdateWhenChangedOnly() const { return true; }

 private:
  // https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
  void Register();

  constexpr virtual std::string_view Type() const { return "sensor"; }
  constexpr virtual std::string_view Icon() const { return ""; }
  constexpr virtual std::string AdditionalRegistrationOptions() const { return ""; }


  std::string_view name_;
  DeviceClass device_class_;
  Value value_;
  bool registered_ = false;
};

class AcVoltageSensor : public Sensor {
 protected:
  constexpr AcVoltageSensor(std::string_view name) : Sensor(name, DeviceClass::kVoltage) {}
};

class DcVoltageSensor : public Sensor {
 protected:
  constexpr DcVoltageSensor(std::string_view name) : Sensor(name, DeviceClass::kVoltage) {}
  constexpr std::string_view Icon() const override { return "current-dc"; }
};

class DcCurrentSensor : public Sensor {
 protected:
  constexpr DcCurrentSensor(std::string_view name) : Sensor(name, DeviceClass::kVoltage) {}
  constexpr std::string_view Icon() const override { return "current-dc"; }
};

class FrequencySensor : public Sensor {
 protected:
  constexpr FrequencySensor(std::string_view name) : Sensor(name, DeviceClass::kFrequency) {}
};

class PowerSensor : public Sensor {
 protected:
  constexpr PowerSensor(std::string_view name) : Sensor(name, DeviceClass::kPower) {}
};

class ApparentPowerSensor : public Sensor {
 protected:
  constexpr ApparentPowerSensor(std::string_view name) : Sensor(name, DeviceClass::kApparentPower) {}
};

class TemperatureSensor : public Sensor {
 protected:
  constexpr TemperatureSensor(std::string_view name) : Sensor(name, DeviceClass::kTemperature) {}
};

///=================================================================================================
/// INFO ABOUT GRID.
///=================================================================================================

struct GridVoltage : public AcVoltageSensor {
  constexpr GridVoltage() : AcVoltageSensor("Grid_voltage") {}
};

struct GridFrequency : public FrequencySensor {
  constexpr GridFrequency() : FrequencySensor("Grid_frequency") {}
};

///=================================================================================================
/// INFO ABOUT THE OUTPUT.
///=================================================================================================
struct OutputVoltage : public AcVoltageSensor {
  constexpr OutputVoltage() : AcVoltageSensor("Output_voltage") {}
};

struct OutputFrequency : public FrequencySensor {
  constexpr OutputFrequency() : FrequencySensor("Output_frequency") {}
};

struct OutputApparentPower : public ApparentPowerSensor {
  constexpr OutputApparentPower() : ApparentPowerSensor("Output_apparent_power") {}
};

struct OutputActivePower : public PowerSensor {
  constexpr OutputActivePower() : PowerSensor("Output_active_power") {}
};

struct OutputLoadPercent : public Sensor {
  constexpr OutputLoadPercent() : Sensor("Output_load_percent", DeviceClass::kPercent) {}
};

///=================================================================================================
/// INFO ABOUT BATTERIES.
///=================================================================================================

struct BatteryType : public Sensor { constexpr BatteryType() : Sensor("Battery_type") {} };

struct BatteryCapacity : public Sensor {
  constexpr BatteryCapacity() : Sensor("Battery_capacity", DeviceClass::kBattery) {}
};

/// Nominal voltage of the battery, which is the voltage level at which it is designed to operate.
/// For example, a 12-volt lead-acid battery has a rating voltage of 12 volts.
struct BatteryVoltage : public DcVoltageSensor {
  constexpr BatteryVoltage() : DcVoltageSensor("Battery_voltage") {}
};

struct BatteryVoltageFromScc : public DcVoltageSensor {
  constexpr BatteryVoltageFromScc() : DcVoltageSensor("Battery_voltage_from_SCC") {}
};

struct BatteryVoltageFromScc2 : public DcVoltageSensor {
  constexpr BatteryVoltageFromScc2() : DcVoltageSensor("Battery_voltage_from_SCC2") {}
};

struct BatteryDischargeCurrent : public DcCurrentSensor {
  constexpr BatteryDischargeCurrent() : DcCurrentSensor("Battery_discharge_current") {}
};

struct BatteryChargeCurrent : public DcCurrentSensor {
  constexpr BatteryChargeCurrent() : DcCurrentSensor("Battery_charge_current") {}
};

/// Nominal voltage of the battery, which is the voltage level at which it is designed to operate.
/// For example, a 12-volt lead-acid battery has a rating voltage of 12 volts.
struct BatteryNominalVoltage : public DcVoltageSensor {
  constexpr BatteryNominalVoltage() : DcVoltageSensor("Battery_nominal_voltage") {}
};

/// Cut off voltage, i.e. the voltage level at which the inverter will shut off to protect the
/// battery from over-discharging.
/// It is typically set slightly above the battery re-discharge voltage to provide a buffer.
struct BatteryUnderVoltage : public DcVoltageSensor {
  constexpr BatteryUnderVoltage() : DcVoltageSensor("Battery_under_voltage") {}
};

/// Voltage level at which the inverter maintains a constant voltage to keep the battery fully
/// charged.
struct BatteryFloatVoltage : public DcVoltageSensor {
  constexpr BatteryFloatVoltage() : DcVoltageSensor("Battery_float_voltage") {}
};

/// Voltage level at which the inverter delivers maximum charging current to the battery during
/// the bulk charging phase.
/// @note that's not batteries' voltage. It's the voltage that the inverter uses to charge them.
/// The bulk charging phase is the initial stage of battery charging where the inverter delivers
/// maximum charging current to the battery to quickly charge it.
struct BatteryBulkVoltage : public DcVoltageSensor {
  constexpr BatteryBulkVoltage() : DcVoltageSensor("Battery_bulk_voltage") {}
};

/// Battery stop discharging voltage when grid is available.
/// Also called "battery recharge voltage". Apparently used, when the inverter is instructed to
/// drain the batteries even when the grid is available (Solar -> Battery -> Utility).
struct BatteryStopDischargingVoltageWithGrid : public DcVoltageSensor {
  constexpr BatteryStopDischargingVoltageWithGrid()
      : DcVoltageSensor("Battery_stop_discharging_voltage_with_grid") {}
};

/// Battery stop charging voltage when grid is available.
/// Also called "battery re-discharge voltage".
struct BatteryStopChargingVoltageWithGrid : public DcVoltageSensor {
  constexpr BatteryStopChargingVoltageWithGrid()
      : DcVoltageSensor("Battery_stop_charging_voltage_with_grid") {}
};

///=================================================================================================
/// PV (Photovoltaics, i.e. solar panels) data.
///=================================================================================================

struct PvWatts : public PowerSensor { constexpr PvWatts() : PowerSensor("PV_watts") {} };
struct PvWatts2 : public PowerSensor { constexpr PvWatts2() : PowerSensor("PV2_watts") {} };

struct PvVoltage : public DcVoltageSensor {
  constexpr PvVoltage() : DcVoltageSensor("PV_voltage") {}
};

struct Pv2Voltage : public DcVoltageSensor {
  constexpr Pv2Voltage() : DcVoltageSensor("PV2_voltage") {}
};

struct PvBusVoltage : public DcVoltageSensor {
  constexpr PvBusVoltage() : DcVoltageSensor("PV_bus_voltage") {}
};

struct PvTotalGeneratedEnergy : public Sensor {
  constexpr PvTotalGeneratedEnergy() : Sensor("PV_total_generated_energy", DeviceClass::kEnergy) {}
};

///=================================================================================================
/// Mode & status & priorities.
///=================================================================================================
// TODO: make these https://www.home-assistant.io/integrations/select.mqtt/


/// https://www.home-assistant.io/integrations/select.mqtt/
class Selector : public Sensor {
 protected:
  constexpr Selector(std::string_view name, std::vector<std::string_view> selectable_options)
      : Sensor(name), selectable_options_(std::move(selectable_options)) {}

  constexpr std::string_view Type() const override { return "select"; }
  std::string AdditionalRegistrationOptions() const override;
  constexpr bool UpdateWhenChangedOnly() const override { return false; }

  std::vector<std::string_view> selectable_options_;
};

struct ModeSelector : public Sensor { constexpr ModeSelector() : Sensor("Mode") {} };

struct OutputSourcePrioritySelector : public Sensor {
  constexpr OutputSourcePrioritySelector() : Sensor("Output_source_priority") {}
};

struct ChargerSourcePrioritySelector : public Selector {
  constexpr ChargerSourcePrioritySelector(std::vector<std::string_view> selectable_options)
      : Selector("Charger_source_priority", std::move(selectable_options)) {}
};

///=================================================================================================
/// Various info.
///=================================================================================================

struct HeatsinkTemperature : public TemperatureSensor {
  constexpr HeatsinkTemperature() : TemperatureSensor("Heatsink_temperature") {}
};

struct Mptt1ChargerTemperature : public TemperatureSensor {
  constexpr Mptt1ChargerTemperature() : TemperatureSensor("Mptt1_charger_temperature") {}
};

struct Mptt2ChargerTemperature : public TemperatureSensor {
  constexpr Mptt2ChargerTemperature() : TemperatureSensor("Mptt2_charger_temperature") {}
};

// Warnings sensor
// TODO: find out how these will be handled. Like whether the inverter accumulates warnings and
//  returns them once (and then "resets" them), or warnings aren't accumulated and shown only to
//  reflect the current situation.
// Warnings - const list of strings

// Add a separate topic so we can send raw commands from HomeAssistant back to the inverter via MQTT
struct RawCommands : public Sensor { constexpr RawCommands() : Sensor("COMMANDS") {} };

}  // namespace mqtt
