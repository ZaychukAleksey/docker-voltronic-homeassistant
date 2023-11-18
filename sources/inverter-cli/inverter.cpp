#include "inverter.h"

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/file.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "CRC.h"
#include "exceptions.h"
#include "utils.h"

#include "spdlog/spdlog.h"

namespace {

/// "<cr>" from protocol description.
/// https://en.wikipedia.org/wiki/Carriage_return
constexpr char kCarriageReturn = '\r';

//Generate 8 bit CRC of supplied string + 1 as used in REVO PI30 protocol
//CHK=DATE0+....+1
//CHK=The accumulated value of sent data+1, single byte.
char CHK(std::string_view s) {
  int crc = 0;
  for (char c: s) {
    crc += c;
  }
  ++crc;
  crc &= 0xFF;
  return crc;
}

std::string GetCRC(std::string_view query) {
  const uint16_t crc = CRC::Calculate(query.data(), query.length(), CRC::CRC_16_XMODEM());
  return {static_cast<char>(crc >> 8), static_cast<char>(crc & 0xff)};
}

bool CheckCRC(std::string_view data) {
  const uint16_t actual_crc = CRC::Calculate(data.data(), data.length() - 3, CRC::CRC_16_XMODEM());
  char crc[2] = {static_cast<char>(actual_crc >> 8), static_cast<char>(actual_crc & 0xff)};
  spdlog::debug("Actual CRC: {:02x} {:02x}. Expected: {:02x} {:02x}", crc[0], crc[1],
                data[data.length() - 3], data[data.length() - 2]);
  return data[data.length() - 3] == crc[0] && data[data.length() - 2] == crc[1];
}

time_t CurrentTimeInSeconds() {
  return time(nullptr);
}

/// To see what we sent in HEX.
void LogQueryInHex(std::string_view query) {
  spdlog::debug("Send: '{}', hex: {}.", EscapeString(query), PrintBytesAsHex(query));
}

}  // namespace


Inverter::Inverter(const std::string& device)
    : device_(device) {
  status1_[0] = 0;
  status2_[0] = 0;
  warnings_[0] = 0;
}

QpigsData Inverter::GetQpigsStatus() const {
  QpigsData result;
  // TODO: check sscanf returned value;
  sscanf(status1_, "%f %f %f %f %d %d %d %d %f %d %d %d %f %f %f %d %s %d %d %d %s",
         &result.grid_voltage,
         &result.grid_frequency,
         &result.ac_output_voltage,
         &result.ac_output_frequency,
         &result.ac_output_apparent_power,
         &result.ac_output_active_power,
         &result.output_load_percent,
         &result.bus_voltage,
         &result.battery_voltage,
         &result.battery_charging_current,
         &result.battery_capacity,
         &result.inverter_heat_sink_temperature,
         &result.pv_input_current_for_battery,
         &result.pv_input_voltage,
         &result.battery_voltage_from_scc,
         &result.battery_discharge_current,
         &result.device_status,
         &result.battery_voltage_offset_for_fans_on,
         &result.eeprom_version,
         &result.pv_charging_power,
         &result.device_status_2);
  return result;
}

QpiriData Inverter::GetQpiriStatus() const {
  QpiriData result;

  // TODO: check sscanf returned value;
  sscanf(status2_,
         "%f %f %f %f %f %d %d %f %f %f %f %f %d %d %d %d %d %d %d %d %d %d %f %d %d %d",
         &result.grid_rating_voltage,
         &result.grid_rating_current,
         &result.ac_output_rating_voltage,
         &result.ac_output_rating_frequency,
         &result.ac_output_rating_current,
         &result.ac_output_rating_apparent_power,
         &result.ac_output_rating_active_power,
         &result.battery_rating_voltage,
         &result.battery_recharge_voltage,
         &result.battery_under_voltage,
         &result.battery_bulk_voltage,
         &result.battery_float_voltage,
         &result.battery_type,
         &result.current_max_ac_charging_current,
         &result.current_max_charging_current,
         &result.input_voltage_range,
         &result.output_source_priority,
         &result.charger_source_priority,
         &result.parallel_max_num,
         &result.machine_type,
         &result.topology,
         &result.out_mode,
         &result.battery_redischarge_voltage,
         &result.pv_ok_condition_for_parallel,
         &result.pv_power_balance,
         &result.max_charging_time_at_cv_stage);

  return result;
}

std::string Inverter::GetWarnings() const {
  return {warnings_};
}

void Inverter::SetMode(char newmode) {
  mode_ = newmode;
}

int Inverter::GetMode() const {
  switch (mode_) {
    case 'P': return 1;  // Power On
    case 'S': return 2;  // Standby
    case 'L': return 3;  // Line
    case 'B': return 4;  // Battery
    case 'F': return 5;  // Fault
    case 'H': return 6;  // Power_Saving
    default: break;
  }
  return 0;  // Unknown
}

/// @returns file descriptor of the device if opened successfully.
/// @throws runtime_error if for some reason connection failed.
int Inverter::Connect() {
  auto fd = open(device_.data(), O_RDWR | O_NONBLOCK);
  if (fd == -1) {
    auto err_message("ERROR: Unable to open device " + device_ + ". " + strerror(errno) + '.');
    spdlog::debug(err_message);
    throw std::runtime_error(err_message);
  }

  // Acquire exclusive lock (non-blocking - flock won't block if someone already locks the port).
  if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
    throw std::runtime_error("Serial port with file descriptor " +
                             std::to_string(fd) + " is already locked by another process.");
  }

  // Set the baud rate and other serial config. Settings are: 2400 8N1.
  struct termios settings;
  if (tcgetattr(fd, &settings)) {
    printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
    throw std::runtime_error("");
  }

  // https://man7.org/linux/man-pages/man3/termios.3.html
  // https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
  cfsetspeed(&settings, B2400);      // baud rate
  settings.c_cflag &= ~PARENB;       // Clear parity bit, thus no parity is used.
  settings.c_cflag &= ~CSTOPB;       // Clear stop bit, thus only 1 stop bit (default) will be used.
  settings.c_cflag &= ~CSIZE;        // Clear number of bits per byte, and...
  settings.c_cflag |= CS8;           // ... use 8 bits
  settings.c_cflag &= ~CRTSCTS;      // Disable RTS/CTS hardware flow control (usage of two extra wires between the end points).
  settings.c_cflag |= CLOCAL;        // Ignore ctrl lines.
  settings.c_oflag |= CREAD;         // Allow reading data.
  settings.c_lflag = 0;              // Reset all Local Modes.

  // Input settings
  settings.c_iflag &= ~ISIG;         // Disable interpretation of INTR, QUIT and SUSP
  settings.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off software flow ctrl
  settings.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

  // Output settings
  settings.c_oflag &= ~OPOST;        // Raw output.
  settings.c_oflag &= ~ONLCR;        // Prevent conversion of newline to carriage return/line feed

  // apply the settings
  if (tcsetattr(fd, TCSANOW, &settings)) {
    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    throw std::runtime_error("");
  }
  tcflush(fd, TCOFLUSH);
  return fd;
}

int AvailableBytes(int device) {
  int bytes;
  ioctl(device, FIONREAD, &bytes);
  return bytes;
}

void WriteToSerialPort(int device, std::string_view query) {
  // The below command doesn't take more than an 8-byte payload 5 chars (+ 3
  // bytes of <CRC><CRC><CR>).  It has to do with low speed USB specifications.
  // So we must chunk up the data and send it in a loop 8 bytes at a time.
  int bytes_sent = 0;
  int remaining = query.length();

  while (remaining > 0) {
    const auto bytes_to_send = (remaining > 8) ? 8 : remaining;
    const auto written = write(device, query.data() + bytes_sent, bytes_to_send);
    if (written < 0) {
      // TODO thrown an exception?
      spdlog::info("ERROR: Write command failed: {}.", strerror(errno));
    } else {
      bytes_sent += written;
      remaining -= written;
    }
    usleep(500000);   // Sleep 50ms before sending another 8 bytes of info
  }
}

/// @throws TimeoutException if the response can't be read in 5 seconds.
std::string Inverter::ReadResponse(int device) {
  // We can't read or wait for response data infinitely. Use a timeout.
  // TODO use VMIN = 0, VTIME > 0 in port settings.
  const time_t deadline_time = CurrentTimeInSeconds() + 5;

  char buffer[1024];
  int bytes_read = 0;

  // Each response from inverter ends with <cr> (carriage return). So we read data until we find it.
  while (true) {
    spdlog::debug("Available {} bytes.", AvailableBytes(device));
    const auto n_bytes = read(device, buffer + bytes_read, std::size(buffer) - bytes_read);
    if (n_bytes < 0) {
      if (CurrentTimeInSeconds() > deadline_time) {
        throw TimeoutException(std::string(current_query_) + " read timeout");
      } else {
        // TODO: make it configurable
        usleep(50000);  // sleep 50ms
        continue;
      }
    }

    const std::string_view data{&buffer[bytes_read], static_cast<std::size_t>(n_bytes)};
    spdlog::debug("Read {} bytes: '{}', hex: {}.",
                  n_bytes, EscapeString(data), PrintBytesAsHex(data));
    bytes_read += n_bytes;
    if (data.back() == kCarriageReturn) {
      break;
    }
  }

  spdlog::debug("Available {} bytes.", AvailableBytes(device));
  return std::string(buffer, bytes_read);
}

std::string Inverter::Query(std::string_view cmd, bool with_crc) {
  current_query_ = cmd;

  const auto device = Connect();
  std::string query(cmd);
  if (with_crc) {
    query += GetCRC(query);
  }

  query += kCarriageReturn;  // Each query must end with carriage return (<cr>).
  LogQueryInHex(query);

  WriteToSerialPort(device, query);

  auto response = ReadResponse(device);
  close(device);

  if (!CheckCRC(response)) {
    throw CrcMismatchException();
  }

  // Cut crc and carriage return bytes.
  response.resize(response.length() - 3);

  spdlog::debug("'{}' query finished", cmd);
  return response;
}

// TODO: fix "NAK" behavior
bool Inverter::Poll() {
  // Reading mode_
//  if (Query("QMOD") && strcmp((char*) &buf_[1], "NAK") != 0) {
//    SetMode(buf_[1]);
//  }
//
//  // Reading QPIGS status
//  if (Query("QPIGS") && strcmp((char*) &buf_[1], "NAK") != 0) {
//    strcpy(status1_, (const char*) buf_ + 1);
//  }
//
//  // Reading QPIRI status
//  if (Query("QPIRI") && strcmp((char*) &buf_[1], "NAK") != 0) {
//    strcpy(status2_, (const char*) buf_ + 1);
//  }
//
//  // Get any device warnings_...
//  if (Query("QPIWS") && strcmp((char*) &buf_[1], "NAK") != 0) {
//    strcpy(warnings_, (const char*) buf_ + 1);
//  }
  return true;
}
