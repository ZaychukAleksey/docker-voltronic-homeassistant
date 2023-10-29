#pragma once

#include <atomic>
#include <mutex>
#include <string_view>
#include <thread>

#include "inverter_data.h"

class Inverter {
 public:
  /// @param device is the device in OS, e.g. "/dev/hidraw0".
  explicit Inverter(const std::string& device);

  /// Fetch data from the inverter once.
  void Poll();
  /// Start a background thread that periodically polls the inverter.
  void StartBackgroundPolling(std::size_t polling_interval_in_seconds = 5);
  void StopBackgroundPolling();

  /// Device general status parameters inquiry.
  QpigsData GetQpigsStatus() const;
  /// Device Rated Information inquiry.
  QpiriData GetQpiriStatus() const;
  std::string GetQpiriStatusRaw() const;
  std::string GetWarnings() const;

  int GetMode() const;
  void ExecuteCmd(std::string_view cmd);

 private:
  void SetMode(char newmode);
  bool Query(std::string_view cmd);

  unsigned char buf_[1024]; //internal work buffer
  char warnings_[1024];
  char status1_[1024];
  char status2_[1024];
  char mode_ = 0;
  const std::string device_;

  mutable std::mutex mutex_;
  std::thread polling_thread_;
  std::atomic_bool polling_thread_termination_requested_{false};
};
