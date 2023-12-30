#include "sensor.hh"
#include "mqtt.hh"
#include "configuration.h"
#include "spdlog/spdlog.h"

#include <format>
#include <ranges>
#include <stdexcept>

namespace mqtt {
namespace {

constexpr std::string_view DeviceClassToString(DeviceClass d) {
  switch (d) {
    case DeviceClass::kVoltage: return "voltage";
    case DeviceClass::kCurrent: return "current";
    case DeviceClass::kFrequency: return "frequency";
    case DeviceClass::kPower: return "power";
    case DeviceClass::kApparentPower: return "apparent_power";
    case DeviceClass::kEnergy: return "energy";
    case DeviceClass::kPercent: return "None";
    case DeviceClass::kTemperature: return "temperature";
    case DeviceClass::kBattery: return "battery";
    case DeviceClass::kNone: return "";
  }
  throw std::runtime_error("unreachable");
}

constexpr std::string_view GetMeasurement(DeviceClass d) {
  switch (d) {
    case DeviceClass::kVoltage: return "V";
    case DeviceClass::kCurrent: return "A";
    case DeviceClass::kFrequency: return "Hz";
    case DeviceClass::kPower: return "W";
    case DeviceClass::kApparentPower: return "VA";
    case DeviceClass::kEnergy: return "kWh";
    case DeviceClass::kPercent: return "%";
    case DeviceClass::kTemperature: return "°C";
    case DeviceClass::kBattery: return "%";
    case DeviceClass::kNone: return "";
  }
  throw std::runtime_error("unreachable");
}

std::string_view GetDeviceInfo() {
  static std::string info;
  if (info.empty()) {
    const auto& device = Settings::Instance().device;
    info = std::format(
        R"({{"ids":"{}","mf":"{}","mdl":"{}","name":"{}"}})",
        device.serial_number, device.manufacturer, device.model, device.name);
  }
  return info;
}

std::string_view GetDeviceId() {
  static std::string id;
  if (id.empty()) {
    const auto& device = Settings::Instance().device;
    id = std::format("{}_{}", device.name, device.serial_number);
  }
  return id;
}

std::string ValueToString(const Sensor::Value& value) {
  if (std::holds_alternative<int>(value)) {
    return std::format("{}", std::get<int>(value));
  }
  if (std::holds_alternative<float>(value)) {
    return std::format("{}", std::get<float>(value));
  }
  if (std::holds_alternative<DeviceMode>(value)) {
    return std::string(ToString(std::get<DeviceMode>(value)));
  }
  if (std::holds_alternative<::BatteryType>(value)) {
    return std::string(ToString(std::get<::BatteryType>(value)));
  }
  if (std::holds_alternative<ChargerPriority>(value)) {
    return std::string(ToString(std::get<::ChargerPriority>(value)));
  }
  if (std::holds_alternative<OutputSourcePriority>(value)) {
    return std::string(ToString(std::get<::OutputSourcePriority>(value)));
  }
  throw std::runtime_error("New value type detected");
}

std::string OptionsToString(const std::vector<std::string_view>& options) {
  std::string result;
  for (auto& option : options) {
    if (!result.empty()) {
      result += ',';
    }
    result += '"';
    result.append(option);
    result += '"';
  }
  return result;
}

}  // namespace

std::string Sensor::SensorTopicRoot() const {
  const auto mqtt_prefix = MqttClient::GetPrefix();
  return std::format("{}/{}/{}/{}", mqtt_prefix, Type(), GetDeviceId(), name_);
}

void Sensor::Update(Value new_value) {
  if (new_value == value_) {
    if (UpdateWhenChangedOnly()) return;
  }

  bool retain = false;
  if (!registered_) {
    Register();
    registered_ = true;
    // The very first sensor update should be retained, otherwise, after Register() Home Assistant
    // often has no enough time to create sensor object and subscribe to its topic before we send
    // the first initial value (so it's often ignored).
    retain = true;
  }

  value_ = new_value;

  const std::string value_str = ValueToString(value_);
  spdlog::info("{}: {}", name_, value_str);
  MqttClient::Instance().Publish(StateTopic(), value_str, 0, retain);
}

void Sensor::Register() {
  std::string payload = "{\n";
  payload.append(std::format("\t\"device\":{}", GetDeviceInfo()));
  if (device_class_ != DeviceClass::kNone) {
    payload.append(std::format(",\n\t\"device_class\":\"{}\"", DeviceClassToString(device_class_)));

    // If NOT "None", the sensor is assumed to be numerical and will be displayed as a line-chart in
    // the frontend instead of as discrete values.
    // https://developers.home-assistant.io/docs/core/entity/sensor/#available-state-classes
    payload.append(",\n\t").append(R"("state_class":"measurement")");
  }

  if (auto icon = Icon(); !icon.empty()) {
    payload.append(",\n\t").append(std::format(R"("icon":"mdi:{}")", icon));
  }
  std::string control_name(name_);
  std::ranges::replace(control_name, '_', ' ');  // replace underscores with whitespaces
  payload.append(",\n\t").append(std::format(R"("name":"{}")", control_name));
  payload.append(",\n\t").append(std::format(R"("state_topic":"{}")", StateTopic()));

  auto unique_id = std::format("{}_{}", Settings::Instance().device.serial_number, name_);
  payload.append(",\n\t").append(std::format(R"("unique_id":"{}")", unique_id));

  const auto unit_of_measurement = GetMeasurement(device_class_);
  if (!unit_of_measurement.empty()) {
    payload.append(",\n\t").append(std::format(R"("unit_of_measurement":"{}")", unit_of_measurement));
  }
  const auto additional_info = AdditionalRegistrationOptions();
  if (!additional_info.empty()) {
    payload.append(std::format(",\n\t{}", additional_info));
  }
  payload += "\n}";

  MqttClient::Instance().Publish(std::format("{}/config", SensorTopicRoot()), payload, 1, true);
}

std::string Selector::AdditionalRegistrationOptions() const {
  return std::format(R"("command_topic":"{}", "options":[{}])", StateTopic(),
                     OptionsToString(selectable_options_));
}

template<typename Enum>
constexpr std::vector<std::string_view> ToStrings(std::initializer_list<Enum> list) {
  std::vector<std::string_view> result;
  for (auto e : list) {
    result.push_back(ToString(e));
  }
  return result;
}

ChargerSourcePrioritySelector::ChargerSourcePrioritySelector(
    std::initializer_list<ChargerPriority> priorities)
    : Selector("Charger_source_priority", ToStrings(priorities)) {}

}  // namespace mqtt
