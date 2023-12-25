#include "mqtt.hh"

#include "configuration.h"
#include "mqtt/client.h"
#include "spdlog/spdlog.h"

namespace {

std::unique_ptr<MqttClient> mqtt_;

auto GetBrokerAddress(const MqttSettings& mqtt_settings) {
  return std::format("mqtt://{}:{}", mqtt_settings.server, mqtt_settings.port);
}

}  // namespace


MqttClient::MqttClient()
    : client_(GetBrokerAddress(Settings::Instance().mqtt),
              Settings::Instance().device.serial_number) {

  mqtt::connect_options_builder options;
  // Polling interval plus some span time to actually perform the polling itself.
  options.keep_alive_interval(std::chrono::seconds(Settings::Instance().polling_interval + 5));
  options.clean_session(true);
  const auto& mqtt_settings = Settings::Instance().mqtt;
  if (!mqtt_settings.user.empty()) {
    options.user_name(mqtt_settings.user);
  }
  if (!mqtt_settings.password.empty()) {
    options.password(mqtt_settings.password);
  }
  spdlog::debug("Connecting to mqtt broker on {}", GetBrokerAddress(mqtt_settings));
  client_.connect(options.finalize());
}

MqttClient::~MqttClient() {
  client_.disconnect();
}

MqttClient& MqttClient::Instance() {
  return *mqtt_;
}

void MqttClient::Init() {
  mqtt_ = std::unique_ptr<MqttClient>(new MqttClient());
}

std::string_view MqttClient::GetPrefix() {
  return Settings::Instance().mqtt.discovery_prefix;
}

void MqttClient::Publish(const std::string& topic, std::string_view payload, bool retain) {
  spdlog::debug("Publish to {}, payload: {}", topic, payload);
//  client_.publish(topic, payload.data(), payload.size(), 1, retain);
}
