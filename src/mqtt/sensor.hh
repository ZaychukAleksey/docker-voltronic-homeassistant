#pragma once

#include <functional>
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

#include "protocols/types.hh"
#include "spdlog/spdlog.h"
#include "utils.h"

namespace mqtt {


/// https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
/// https://www.home-assistant.io/integrations/sensor.mqtt/
class Sensor {
 public:
  enum class Kind {
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

  virtual ~Sensor() = default;

  constexpr std::string_view GetName() const { return name_; }

 protected:
  /// @param device_class Optional. One of https://www.home-assistant.io/integrations/sensor/#device-class
  /// @param icon Optional. https://www.home-assistant.io/docs/configuration/customizing-devices/#icon
  constexpr Sensor(std::string_view name, Kind device_class)
      : name_(name), device_class_(device_class) {}

  std::string TopicRoot() const;
  std::string StateTopic() const;

  /// Register the sensor in MQTT so that Home Assistant is able to see it.
  /// @see https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
  void Register();
  void Publish() const;

  constexpr virtual std::string_view Type() const { return "sensor"; }
  constexpr virtual std::string_view Icon() const { return ""; }
  constexpr virtual std::string AdditionalRegistrationOptions() const { return ""; }
  constexpr virtual void OnRegisterSuccessful() {}

  inline static std::mutex mutex_;

 private:
  virtual std::string ValueToString() const = 0;

  const std::string_view name_;
  const Kind device_class_;
};


template<typename ValueType>
class TypedSensor : public Sensor {
 public:
  constexpr TypedSensor(std::string_view name, Kind device_class = Kind::kNone)
      : Sensor(name, device_class) {}

  std::optional<ValueType> GetValue() const {
    std::lock_guard lock(mutex_);
    return value_;
  }

  /// Set and update sensor's value in HomeAssistant.
  /// Does nothing if the new value is the same as the previous one.
  void Update(ValueType new_value) {
    std::lock_guard lock(mutex_);

    if (!value_.has_value()) {
      Register();
    } else if (new_value == value_) {
      return;
    }

    value_ = new_value;
    Publish();
  }

 protected:
  std::string ValueToString() const final { return ValueToString(*value_, false); }

  virtual std::string ValueToString(const ValueType& value, bool for_json) const {
    if constexpr (std::is_same_v<ValueType, bool>) {
      return value ? "1" : "0";
    } else if constexpr (std::is_arithmetic_v<ValueType>) {
      return std::format("{}", value);
    } else if constexpr (std::is_enum_v<ValueType>) {
      return for_json ? std::format("\"{}\"", ToString(value)) : ToString(value);
    } else if constexpr (std::is_same_v<ValueType, std::string>) {
      return for_json ? std::format("\"{}\"", value) : value;
    } else {
      throw std::runtime_error("Unknown type");
    }
  }

  virtual ValueType ValueFromString(const std::string& str) const {
    if constexpr (std::is_same_v<ValueType, bool>) {
      return str == "1";
    } else if constexpr (std::is_integral_v<ValueType>) {
      return std::stoi(str);
    } else if constexpr (std::is_floating_point_v<ValueType>) {
      return std::stof(str);
    } else if constexpr (std::is_enum_v<ValueType>) {
      ValueType result;
      FromString(str, result);
      return result;
    } else if constexpr (std::is_same_v<ValueType, std::string>) {
      return str;
    } else {
      throw std::runtime_error("Unknown sensor type");
    }
  }

 private:
  std::optional<ValueType> value_;
};

namespace implementation_details {

void SubscribeToTopic(const std::string&, std::function<void(const std::string)>&&);

}  // namespace implementation_details


/// Base class for sensors that allow to change it's state from Home Assistant interface.
template<typename ValueType>
class InteractiveTypedSensor : public TypedSensor<ValueType> {
 public:
  /// @return true, if successful, false otherwise
  using OnChangedCallback = std::function<bool(ValueType)>;
 protected:
  constexpr InteractiveTypedSensor(std::string_view name, OnChangedCallback&& on_value_changed)
      : TypedSensor<ValueType>(name),
        on_value_changed_(std::move(on_value_changed)) {}

  virtual std::string_view Type() const = 0;
  virtual std::string AdditionalRegistrationOptions() const = 0;
  virtual std::string CommandTopic() const { return std::format("{}/command", this->TopicRoot()); }

  void OnRegisterSuccessful() final {
    auto OnMessageArrived = [this](const std::string& new_value) {
      auto selected_value = this->ValueFromString(new_value);
      auto previous_value = this->GetValue();
      if (!previous_value.has_value() || previous_value == selected_value) return;

      spdlog::warn("Set {} to {}", this->GetName(), new_value);
      if (on_value_changed_(selected_value)) {
        // Value has been successfully changed. Update it.
        this->Update(selected_value);
      } else {
        // Failed to change the value. Publish the previous one.
        spdlog::error("Failed to set {} to {}.", this->GetName(), new_value);
        std::lock_guard lock(Sensor::mutex_);
        this->Publish();
      }
    };
    implementation_details::SubscribeToTopic(this->CommandTopic(), std::move(OnMessageArrived));
  }

  OnChangedCallback on_value_changed_;
};


/// https://www.home-assistant.io/integrations/switch.mqtt/
class Switch : public InteractiveTypedSensor<bool> {
 public:
  Switch(std::string_view name, OnChangedCallback&& value_selected_callback)
      : InteractiveTypedSensor<bool>(name, std::move(value_selected_callback)) {}

 protected:
  constexpr std::string_view Type() const final { return "switch"; }
  std::string AdditionalRegistrationOptions() const final {
    return std::format(R"("command_topic":"{}","payload_on":1,"payload_off":0)", CommandTopic());
  }
};


/// https://www.home-assistant.io/integrations/select.mqtt/
template<typename ValueType>
class Selector : public InteractiveTypedSensor<ValueType> {
 public:
  using Items = std::initializer_list<ValueType>;
  using OnSelectedCallback = InteractiveTypedSensor<ValueType>::OnChangedCallback;
 protected:
  constexpr Selector(std::string_view name,
                     std::vector<ValueType>&& selectable_options,
                     OnSelectedCallback&& value_selected_callback)
      : InteractiveTypedSensor<ValueType>(name, std::move(value_selected_callback)),
        selectable_options_(std::move(selectable_options)) {}

  constexpr std::string_view Type() const final { return "select"; }
  std::string CommandTopic() const final { return this->StateTopic(); }

  std::string AdditionalRegistrationOptions() const final {
    // Join options to a string with comma separator.
    std::string options;
    for (const auto& value : selectable_options_) {
      if (!options.empty()) {
        options += ',';
      }
      options.append(this->ValueToString(value, true));
    }

    return std::format(R"("command_topic":"{}", "options":[{}])", this->CommandTopic(), options);
  }

 private:
  const std::vector<ValueType> selectable_options_;
};

class AcVoltageSensor : public TypedSensor<float> {
 protected:
  constexpr AcVoltageSensor(std::string_view name) : TypedSensor(name, Kind::kVoltage) {}
};

class DcVoltageSensor : public TypedSensor<float> {
 protected:
  constexpr DcVoltageSensor(std::string_view name) : TypedSensor(name, Kind::kVoltage) {}
  constexpr std::string_view Icon() const override { return "current-dc"; }
};

class DcCurrentSensor : public TypedSensor<int> {
 protected:
  constexpr DcCurrentSensor(std::string_view name) : TypedSensor(name, Kind::kCurrent) {}
  constexpr std::string_view Icon() const override { return "current-dc"; }
};

class FrequencySensor : public TypedSensor<float> {
 protected:
  constexpr FrequencySensor(std::string_view name) : TypedSensor(name, Kind::kFrequency) {}
};

class PowerSensor : public TypedSensor<int> {
 protected:
  constexpr PowerSensor(std::string_view name) : TypedSensor(name, Kind::kPower) {}
};

class ApparentPowerSensor : public TypedSensor<int> {
 protected:
  constexpr ApparentPowerSensor(std::string_view name) : TypedSensor(name, Kind::kApparentPower) {}
};

class TemperatureSensor : public TypedSensor<int> {
 protected:
  constexpr TemperatureSensor(std::string_view name) : TypedSensor(name, Kind::kTemperature) {}
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

struct OutputLoadPercent : public TypedSensor<int> {
  constexpr OutputLoadPercent() : TypedSensor("Output_load_percent", Kind::kPercent) {}

  constexpr std::string_view Icon() const override { return "percent"; }
};

///=================================================================================================
/// INFO ABOUT BATTERIES.
///=================================================================================================

struct BatteryType : public Selector<::BatteryType> {
  constexpr BatteryType(Items types, OnSelectedCallback&& callback)
      : Selector("Battery_type", types, std::move(callback)) {}

  constexpr std::string_view Icon() const override { return "car-battery"; }
};

struct BatteryCapacity : public TypedSensor<int> {
  constexpr BatteryCapacity() : TypedSensor("Battery_capacity", Kind::kBattery) {}
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
//12V unit: 11V/11.3V/11.5V/11.8V/12V/12.3V/12.5V/12.8V
//24V unit: 22V/22.5V/23V/23.5V/24V/24.5V/25V/25.5V
//48V unit: 44V/45V/46V/47V/48V/49V/50V/51V
//00.0V means battery is full(charging in float mode).
struct BatteryStopDischargingVoltageWithGrid : public DcVoltageSensor {
  constexpr BatteryStopDischargingVoltageWithGrid()
      : DcVoltageSensor("Battery_stop_discharging_voltage_with_grid") {}
};

/// Battery stop charging voltage when grid is available.
/// Also called "battery re-discharge voltage".
/// @note value is stored internally and is expected to be set as int in 0.1V to avoid
///       float-point-related effects.
/// 00.0V means battery is full(charging in float mode).
struct BatteryStopChargingVoltageWithGrid : public Selector<int> {
 public:
  static std::unique_ptr<BatteryStopChargingVoltageWithGrid> Create(int inverter_voltage,
                                                                    OnSelectedCallback&& callback);
 protected:
  BatteryStopChargingVoltageWithGrid(std::vector<int>&& voltages, OnSelectedCallback&& callback)
      : Selector<int>("Battery_stop_charging_voltage_with_grid",
                      std::move(voltages), std::move(callback)) {}


  std::string ValueToString(const int&, bool) const override;
  int ValueFromString(const std::string&) const override;
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

struct PvTotalGeneratedEnergy : public TypedSensor<int> {
  constexpr PvTotalGeneratedEnergy() : TypedSensor("PV_total_generated_energy", Kind::kEnergy) {}
};

///=================================================================================================
/// Mode & status & priorities.
///=================================================================================================

struct InverterMode : public TypedSensor<DeviceMode> {
  constexpr InverterMode() : TypedSensor("Mode") {}
};

// TODO That will be a selector
struct MachineTypeSelector : public TypedSensor<MachineType> {
  constexpr MachineTypeSelector() : TypedSensor("Machine_type") {}
};

/// The AC-Input terminal of the off-grid inverters accepts a wide range of sinusoidal voltages.
/// The APL and UPS modes will allow a wider or narrower selection of voltages.
struct AcInputVoltageRangeSelector : public Selector<InputVoltageRange> {
  constexpr AcInputVoltageRangeSelector(Items ranges, OnSelectedCallback&& callback)
      : Selector("AC_input_voltage_range", ranges, std::move(callback)) {}

  constexpr std::string_view Icon() const override { return "sine-wave"; }
};

struct OutputSourcePrioritySelector : public Selector<OutputSourcePriority> {
  constexpr OutputSourcePrioritySelector(Items priorities, OnSelectedCallback&& callback)
      : Selector("Output_source_priority", priorities, std::move(callback)) {}
};

struct ChargerSourcePrioritySelector : public Selector<ChargerPriority> {
  ChargerSourcePrioritySelector(Items priorities, OnSelectedCallback&& callback)
      : Selector("Charger_source_priority", priorities, std::move(callback)) {}
};

struct SolarPowerPrioritySelector : public Selector<SolarPowerPriority> {
  SolarPowerPrioritySelector(Items priorities, OnSelectedCallback&& callback)
      : Selector("Solar_power_priority", priorities, std::move(callback)) {}
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

struct WarningsSensor : public TypedSensor<std::string> {
  constexpr WarningsSensor() : TypedSensor("Warnings") {}
  constexpr std::string_view Icon() const override { return "alert"; }
};

///=================================================================================================
/// Various settings.
///=================================================================================================

struct BacklightSwitch : public Switch {
  BacklightSwitch(OnChangedCallback&& value_selected_callback)
      : Switch("Backlight", std::move(value_selected_callback)) {}
  constexpr std::string_view Icon() const override { return "television-ambient-light"; }
};

}  // namespace mqtt
