#include "mqtt.hh"

#include <atomic>
#include <mutex>
#include <unordered_map>

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


MqttClient::MqttClient(const MqttSettings& settings, const std::string& client_id)
    : client_(GetBrokerAddress(settings), client_id) {

  mqtt::connect_options_builder options;
  options.keep_alive_interval(std::chrono::seconds(10));
  options.clean_session(true);
  options.automatic_reconnect(true);
  if (!settings.user.empty()) {
    options.user_name(settings.user);
  }
  if (!settings.password.empty()) {
    options.password(settings.password);
  }
  spdlog::debug("Connecting to mqtt broker on {}", GetBrokerAddress(settings));
  client_.connect(options.finalize())->wait();
  client_.start_consuming();
}

MqttClient::~MqttClient() {
  client_.disconnect();
}

MqttClient& MqttClient::Instance() {
  return *mqtt_;
}

void MqttClient::Init(const MqttSettings& settings, const std::string& client_id) {
  mqtt_ = std::unique_ptr<MqttClient>(new MqttClient(settings, client_id));
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
