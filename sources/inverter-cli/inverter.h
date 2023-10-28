#pragma once

#include <atomic>
#include <thread>
#include <mutex>

class Inverter {
  unsigned char buf[1024]; //internal work buffer
  char warnings[1024];
  char status1[1024];
  char status2[1024];
  char mode;
  std::string device;
  std::mutex m;
  std::thread t1;
  std::atomic_bool quit_thread{false};

  void SetMode(char newmode);
  bool CheckCRC(unsigned char* buff, int len);
  bool query(const char* cmd);
  uint16_t cal_crc_half(uint8_t* pin, uint8_t len);

 public:
  Inverter(std::string devicename);
  void poll();

  void runMultiThread() {
    t1 = std::thread(&Inverter::poll, this);
  }

  void terminateThread() {
    quit_thread = true;
    t1.join();
  }

  std::string* GetQpiriStatus();
  std::string* GetQpigsStatus();
  std::string* GetWarnings();

  int GetMode();
  void ExecuteCmd(const std::string cmd);
};
