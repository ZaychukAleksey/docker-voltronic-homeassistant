#include "mqtt.hh"

#include <atomic>
#include <mutex>
#include <unordered_map>

#include "configuration.h"
#include "spdlog/spdlog.h"

namespace {

std::unique_ptr<MqttClient> mqtt_;
std::unique_ptr<std::thread> subscription_handler_;

std::mutex subscriptions_mutex;
static std::unordered_map<std::string, MqttClient::SubscriptionCalllback> subscriptions;

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
  options.automatic_reconnect(true);
  const auto& mqtt_settings = Settings::Instance().mqtt;
  if (!mqtt_settings.user.empty()) {
    options.user_name(mqtt_settings.user);
  }
  if (!mqtt_settings.password.empty()) {
    options.password(mqtt_settings.password);
  }
  spdlog::debug("Connecting to mqtt broker on {}", GetBrokerAddress(mqtt_settings));
  client_.connect(options.finalize())->wait();
  client_.start_consuming();
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

void MqttClient::Publish(const std::string& topic, std::string_view payload, int qos, bool retain) {
  spdlog::debug("Publish to {}, payload: {}", topic, payload);
  client_.publish(topic, payload.data(), payload.size(), qos, retain);
}

void MqttClient::Subscribe(std::string topic, SubscriptionCalllback&& callback) {
  spdlog::debug("Subscribing to {}...", topic);
  if (!subscription_handler_) {
    subscription_handler_ = std::make_unique<std::thread>([this]() { SubscriptionHandler(); });
  }

  {
    std::lock_guard lock(subscriptions_mutex);
    subscriptions[topic] = std::move(callback);
  }
  client_.subscribe(topic, 0);
}

void MqttClient::SubscriptionHandler() {
  while (true) {
    auto message = client_.consume_message();
    if (!message) {
      // Wait for Reconnect?
      break;
    }

    // Call handler function  Dispatch to a handler function based on the Subscription ID
    spdlog::debug("Message on {}: {}", message->get_topic(), message->get_payload_str());

    std::lock_guard lock(subscriptions_mutex);
    auto& subscription_callback = subscriptions[message->get_topic()];
    if (subscription_callback) {
      subscription_callback(message->get_payload_str());
    }
  }
}
