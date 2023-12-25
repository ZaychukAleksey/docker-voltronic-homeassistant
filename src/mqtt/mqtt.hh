#pragma once

#include <format>
#include <string>
#include <string_view>

#include "mqtt/client.h"

class MqttClient {
 public:
  static MqttClient& Instance();
  static void Init();

  ~MqttClient();

  /// @param retain Whether the message should be retained by the broker.
  void Publish(const std::string& sub_topic, std::string_view payload, bool retain = false);
  static std::string_view GetPrefix();

 private:
  MqttClient();

  mqtt::client client_;
};
