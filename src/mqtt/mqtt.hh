#pragma once

#include <format>
#include <string>
#include <string_view>

#include "configuration.h"
#include "mqtt/async_client.h"

class MqttClient {
 public:
  static MqttClient& Instance();
  static void Init(const MqttSettings&, const std::string& client_id);

  ~MqttClient();

  /// @param retain Whether the message should be retained by the broker.
  /// @param qos https://www.hivemq.com/blog/mqtt-essentials-part-6-mqtt-quality-of-service-levels/
  void Publish(const std::string& sub_topic, std::string_view payload,
               int qos = 1, bool retain = false);
  static std::string_view GetPrefix();

  using SubscriptionCalllback = std::function<void(std::string)>;
  void Subscribe(std::string topic, SubscriptionCalllback&&);

 private:
  MqttClient(const MqttSettings&, const std::string& client_id);
  void SubscriptionHandler();

  mqtt::async_client client_;
};
