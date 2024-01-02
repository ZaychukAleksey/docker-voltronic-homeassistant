#include "sensor.hh"
#include "mqtt.hh"
#include "configuration.h"
#include "spdlog/spdlog.h"

#include <format>
#include <ranges>
#include <stdexcept>

namespace mqtt {
namespace {

constexpr std::string_view ToString(Sensor::Kind d) {
  switch (d) {
    case Sensor::Kind::kVoltage: return "voltage";
    case Sensor::Kind::kCurrent: return "current";
    case Sensor::Kind::kFrequency: return "frequency";
    case Sensor::Kind::kPower: return "power";
    case Sensor::Kind::kApparentPower: return "apparent_power";
    case Sensor::Kind::kEnergy: return "energy";
    case Sensor::Kind::kPercent: return "None";
    case Sensor::Kind::kTemperature: return "temperature";
    case Sensor::Kind::kBattery: return "battery";
    case Sensor::Kind::kNone: return "";
  }
  throw std::runtime_error("unreachable");
}

constexpr std::string_view GetMeasurement(Sensor::Kind d) {
  switch (d) {
    case Sensor::Kind::kVoltage: return "V";
    case Sensor::Kind::kCurrent: return "A";
    case Sensor::Kind::kFrequency: return "Hz";
    case Sensor::Kind::kPower: return "W";
    case Sensor::Kind::kApparentPower: return "VA";
    case Sensor::Kind::kEnergy: return "kWh";
    case Sensor::Kind::kPercent: return "%";
    case Sensor::Kind::kTemperature: return "°C";
    case Sensor::Kind::kBattery: return "%";
    case Sensor::Kind::kNone: return "";
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

}  // namespace

std::string Sensor::TopicRoot() const {
  const auto mqtt_prefix = MqttClient::GetPrefix();
  return std::format("{}/{}/{}/{}", mqtt_prefix, Type(), GetDeviceId(), name_);
}

std::string Sensor::StateTopic() const {
  return std::format("{}/state", TopicRoot());
}

void Sensor::Register() {
  std::string payload = "{\n";
  payload.append(std::format("\t\"device\":{}", GetDeviceInfo()));
  if (device_class_ != Kind::kNone) {
    payload.append(std::format(",\n\t\"device_class\":\"{}\"", ToString(device_class_)));

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

  MqttClient::Instance().Publish(std::format("{}/config", TopicRoot()), payload, 1, true);
  OnRegisterSuccessful();
}

void Sensor::Publish() const {
  const auto value_str = ValueToString();
  spdlog::info("{}: {}", name_, value_str);

  // Using "retain" always is just easier. If not retain messages then Home Assistant often skips
  // the first message sensor update after the sensor is created (because HA needs some time to
  // create the sensor). If retain only the first sensor update, then some tricky situations are
  // possible with "select" sensors.
  MqttClient::Instance().Publish(StateTopic(), value_str, 0, true);
}

namespace implementation_details {

void SubscribeToTopic(const std::string& topic, std::function<void(const std::string)>&& callback) {
  MqttClient::Instance().Subscribe(topic, std::move(callback));
}

}  // namespace implementation_details
}  // namespace mqtt
