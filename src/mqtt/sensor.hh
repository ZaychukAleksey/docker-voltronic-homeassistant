#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

#include "protocols/types.hh"
#include "utils.h"

namespace mqtt {


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
  /// Set and update sensor's value in HomeAssistant.
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
  constexpr TypedSensor(std::string_view name, Kind device_class = Kind::kNone)
      : Sensor(name, device_class) {}

  std::optional<ValueType> GetValue() const {
    std::lock_guard lock(mutex_);
    return value_;
  }

  /// Set sensor's value without updating it in HomeAssistant.
  void SetValue(ValueType new_value) {
    std::lock_guard lock(mutex_);
    if (!value_.has_value()) {
      Register();
    }
    value_ = new_value;
  }

  std::string ValueToString() const override {
    if constexpr (std::is_arithmetic_v<ValueType>) {
      return std::format("{}", *value_);
    } else if constexpr (std::is_enum_v<ValueType>) {
      return std::string(ToString(*value_));
    } else if constexpr (std::is_same_v<ValueType, std::string>) {
      return *value_;
    } else {
      throw std::runtime_error("Unknown sensor type");
    }
  }

  static ValueType ValueFromString(const std::string& str) {
    if constexpr (std::is_integral_v<ValueType>) {
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

/// https://www.home-assistant.io/integrations/select.mqtt/
template<typename Enum> requires std::is_enum_v<Enum>
class Selector : public TypedSensor<Enum> {
 public:
  using Items = std::initializer_list<Enum>;
  using OnSelectedCallback = std::function<void(Enum)>;
 protected:
  constexpr Selector(std::string_view name,
                     std::vector<Enum>&& selectable_options,
                     OnSelectedCallback&& value_selected_callback)
      : TypedSensor<Enum>(name),
        selectable_options_(std::move(selectable_options)),
        on_value_selected_(std::move(value_selected_callback)) {}

  constexpr std::string_view Type() const final { return "select"; }
  std::string AdditionalRegistrationOptions() const final {
    return std::format(R"("command_topic":"{}", "options":[{}])", this->StateTopic(),
                       Concatenate(selectable_options_));
  }

  void OnRegisterSuccessful() override {
    auto OnMessageArrived = [this](const std::string& new_value) {
      Enum selected_value = this->ValueFromString(new_value);
      if (this->GetValue() != selected_value) {
        auto previous_value = this->GetValue();
        if (previous_value.has_value() && *previous_value != selected_value) {
          on_value_selected_(selected_value);
        }
        this->SetValue(selected_value);
      }
    };
    implementation_details::SubscribeToTopic(this->StateTopic(), std::move(OnMessageArrived));
  }

  const std::vector<Enum> selectable_options_;
  std::function<void(Enum)> on_value_selected_;
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

struct PvTotalGeneratedEnergy : public TypedSensor<int> {
  constexpr PvTotalGeneratedEnergy() : TypedSensor("PV_total_generated_energy", Kind::kEnergy) {}
};

///=================================================================================================
/// Mode & status & priorities.
///=================================================================================================

struct InverterMode : public TypedSensor<DeviceMode> {
  constexpr InverterMode() : TypedSensor("Mode") {}
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

}  // namespace mqtt
