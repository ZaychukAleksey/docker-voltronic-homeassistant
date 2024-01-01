#pragma once

#include <string_view>

class SerialPort {
 public:
  SerialPort(std::string_view device);
  SerialPort(const SerialPort&) = delete;
  SerialPort(SerialPort&&) = delete;

  SerialPort& operator=(const SerialPort&) = delete;
  SerialPort& operator=(SerialPort&&) = delete;
  ~SerialPort();

  /// Send @a query to the device.
  /// @warning This function is NOT thread-safe.
  /// @param query - the query to be send. Should be without carriage return (<cr>) and crc.
  /// @param with_crc - if set, generate and append crc bytes to the query.
  void Send(std::string_view query, bool with_crc) const;

  /// Receive data from device and check its CRC.
  /// @warning This function is NOT thread-safe.
  /// @returns a reply from the device, excluding CRC and carriage return (<cr>).
  std::string Receive(int timeout_in_seconds = 5) const;

  /// Combination of Send() and Receive() with checking CRC and retrying on CRC mismatch.
  /// This function is thread-safe.
  /// @param query - see Send().
  /// @param with_crc - see Send().
  /// @param n_retries how many times to retry the query in case if CRC doesn't match.
  std::string Query(std::string_view query, bool with_crc, int n_retries = 5) const;

 private:
  int file_descriptor_;
};
