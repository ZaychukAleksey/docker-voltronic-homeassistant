#pragma once

#include <atomic>
#include <mutex>
#include <string_view>
#include <thread>

class Inverter {
 public:
  /// @param device is the device in OS, e.g. "/dev/hidraw0".
  explicit Inverter(const std::string& device);
  void Poll();

  void RunMultiThread() {
    t1_ = std::thread(&Inverter::Poll, this);
  }

  void TerminateThread() {
    quit_thread_ = true;
    t1_.join();
  }

  std::string GetQpiriStatus();
  std::string GetQpigsStatus();
  std::string GetWarnings();

  int GetMode();
  void ExecuteCmd(std::string_view cmd);

 private:
  void SetMode(char newmode);
  bool CheckCRC(unsigned char* buff, int len);
  bool Query(std::string_view cmd);
  uint16_t CalCrcHalf(uint8_t* pin, uint8_t len);

  unsigned char buf_[1024]; //internal work buffer
  char warnings_[1024];
  char status1_[1024];
  char status2_[1024];
  char mode_ = 0;
  const std::string device_;
  std::mutex mutex_;
  std::thread t1_;
  std::atomic_bool quit_thread_{false};
};
