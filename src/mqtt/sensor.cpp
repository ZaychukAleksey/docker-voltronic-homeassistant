#include "sensor.hh"
#include "mqtt.hh"
#include "configuration.h"

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
    return std::string(DeviceModeToString(std::get<DeviceMode>(value)));
  }
  if (std::holds_alternative<::BatteryType>(value)) {
    return std::string(BatteryTypeToString(std::get<::BatteryType>(value)));
  }
  if (std::holds_alternative<ChargerPriority>(value)) {
    return std::string(ChargerPriorityToString(std::get<::ChargerPriority>(value)));
  }
  if (std::holds_alternative<OutputSourcePriority>(value)) {
    return std::string(OutputSourcePriorityToString(std::get<::OutputSourcePriority>(value)));
  }
  throw std::runtime_error("New value type detected");
}

}  // namespace

std::string Sensor::StateTopic() const {
  const auto mqtt_prefix = MqttClient::GetPrefix();
  return std::format("{}/{}/{}/{}", mqtt_prefix, Type(), GetDeviceId(), name_);
}

void Sensor::Update(Value new_value) {
  if (new_value != value_) {
    if (!registered_) {
      Register();
      registered_ = true;
    }

    value_ = new_value;

    const std::string value_str = ValueToString(value_);
    spdlog::info("{}: {}", name_, value_str);
    MqttClient::Instance().Publish(StateTopic(), value_str);
  }
}

void Sensor::Register() {
  std::string payload = "{\n";
  payload.append(std::format("\t\"device\":{}", GetDeviceInfo()));
  if (device_class_ != DeviceClass::kNone) {
    payload.append(std::format(",\n\t\"device_class\":\"{}\"", DeviceClassToString(device_class_)));

    // If NOT "None", the sensor is assumed to be numerical and will be displayed as a line-chart in
    // the frontend instead of as discrete values.
    // https://developers.home-assistant.io/docs/core/entity/sensor/#available-state-classes
    payload.append(",\n\t\"state_class\":\"measurement\"");
  }

  if (auto icon = Icon(); !icon.empty()) {
    payload.append(std::format(",\n\t\"icon\":\"mdi:{}\"", icon));
  }
  std::string control_name(name_);
  std::ranges::replace(control_name, '_', ' ');  // replace underscores with whitespaces
  payload.append(std::format(",\n\t\"name\":\"{}\"", control_name));

  auto state_topic = StateTopic();
  payload.append(std::format(",\n\t\"state_topic\":\"{}\"", state_topic));

  auto unique_id = std::format("{}_{}", Settings::Instance().device.serial_number, name_);
  payload.append(std::format(",\n\t\"unique_id\":\"{}\"", unique_id));

  const auto unit_of_measurement = GetMeasurement(device_class_);
  if (!unit_of_measurement.empty()) {
    payload.append(std::format(",\n\t\"unit_of_measurement\":\"{}\"", unit_of_measurement));
  }
  payload += "\n}";

  MqttClient::Instance().Publish(std::format("{}/config", state_topic), payload, true);
}

}  // namespace mqtt