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

  /// Fetch data from the inverter.
  bool Poll();

  /// Device general status parameters inquiry.
  QpigsData GetQpigsStatus() const;
  /// Device Rated Information inquiry.
  QpiriData GetQpiriStatus() const;
  std::string GetWarnings() const;

  int GetMode() const;
  [[nodiscard]] std::string Query(std::string_view cmd, bool with_crc = true);

 private:
  int Connect();

  void SetMode(char newmode);
  [[nodiscard]] std::string ReadResponse(int);

  std::string_view current_query_;
  char warnings_[1024];
  char status1_[1024];
  char status2_[1024];
  char mode_ = 0;
  const std::string device_;
};
