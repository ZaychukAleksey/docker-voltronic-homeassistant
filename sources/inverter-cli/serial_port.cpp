#include "serial_port.hh"

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "CRC.h"
#include "exceptions.h"
#include "utils.h"

#include "spdlog/spdlog.h"


namespace {

time_t CurrentTimeInSeconds() {
  return time(nullptr);
}

int AvailableBytes(int device) {
  int bytes;
  ioctl(device, FIONREAD, &bytes);
  return bytes;
}

bool CheckCRC(std::string_view data) {
  const uint16_t actual_crc = CRC::Calculate(data.data(), data.length() - 3, CRC::CRC_16_XMODEM());
  char crc[2] = {static_cast<char>(actual_crc >> 8), static_cast<char>(actual_crc & 0xff)};
  const bool matches = data[data.length() - 3] == crc[0] && data[data.length() - 2] == crc[1];
  if (matches) {
    spdlog::debug("CRC OK: {:02x} {:02x}", crc[0], crc[1]);
  } else {
    spdlog::warn("CRC mismatch.\n\tActual: {:02x} {:02x}.\n\tExpected: {:02x} {:02x}.", crc[0],
                 crc[1],
                 data[data.length() - 3], data[data.length() - 2]);

  }
  return matches;
}

std::string GetCRC(std::string_view query) {
  const uint16_t crc = CRC::Calculate(query.data(), query.length(), CRC::CRC_16_XMODEM());
  return {static_cast<char>(crc >> 8), static_cast<char>(crc & 0xff)};
}

}  // namespace


SerialPort::SerialPort(std::string_view device) {
  file_descriptor_ = open(device.data(), O_RDWR | O_NONBLOCK);
  if (file_descriptor_ == -1) {
    throw std::runtime_error(fmt::format("Unable to open device {}: {}.", device, strerror(errno)));
  }

  // Acquire exclusive lock (non-blocking - flock won't block if someone already locks the port).
  if (flock(file_descriptor_, LOCK_EX | LOCK_NB) == -1) {
    throw std::runtime_error("Serial port with file descriptor " +
                             std::to_string(file_descriptor_) +
                             " is already locked by another process.");
  }

  // Set the baud rate and other serial config. Settings are: 2400 8N1.
  struct termios settings;
  if (tcgetattr(file_descriptor_, &settings)) {
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
  if (tcsetattr(file_descriptor_, TCSANOW, &settings)) {
    throw std::runtime_error(fmt::format("Error {} from tcsetattr: {}", errno, strerror(errno)));
  }
  tcflush(file_descriptor_, TCOFLUSH);
}

SerialPort::~SerialPort() {
  close(file_descriptor_);
}

void SerialPort::Send(std::string_view query, bool with_crc) const {
  std::string data(query);
  if (with_crc) {
    data += GetCRC(data);
  }
  data += '\r';  // Each query must end with carriage return (<cr>).
  spdlog::debug("Send: '{}', hex: {}.", EscapeString(data), PrintBytesAsHex(data));

  // The code below sends data by 8-bytes chunks. It has to do with low speed USB specifications.
  int bytes_sent = 0;
  int remaining = data.length();

  while (remaining > 0) {
    const auto bytes_to_send = (remaining > 8) ? 8 : remaining;
    const auto written = write(file_descriptor_, data.data() + bytes_sent, bytes_to_send);
    if (written < 0) {
      throw std::runtime_error(fmt::format("Failed to write. {}", strerror(errno)));
    }

    bytes_sent += written;
    remaining -= written;
    usleep(50000);   // Sleep 50ms before sending another 8 bytes of info
  }
}

std::string SerialPort::Receive() const {
  // We can't read or wait for response data infinitely. Use a timeout.
  // TODO use VMIN = 0, VTIME > 0 in port settings.
  const time_t deadline_time = CurrentTimeInSeconds() + 5;

  char buffer[1024];
  int bytes_read = 0;

  // Each response from inverter ends with <cr> (carriage return). So we read data until we find it.
  while (true) {
    usleep(50000);  // sleep 50ms TODO: make it configurable
    const auto n_bytes = read(file_descriptor_, buffer + bytes_read, std::size(buffer) - bytes_read);
    if (n_bytes < 0) {
      if (CurrentTimeInSeconds() > deadline_time) {
        throw TimeoutException("Read timeout");
      }
      continue;
//      throw std::runtime_error(fmt::format("Failed to read. {}", strerror(errno)));
    }

    const std::string_view data{&buffer[bytes_read], static_cast<std::size_t>(n_bytes)};
    spdlog::debug("Read {} bytes: '{}', hex: {}.",
                  n_bytes, EscapeString(data), PrintBytesAsHex(data));
    bytes_read += n_bytes;
    // Replies end with a carriage return (<cr>)
    if (data.back() == '\r') {
      break;
    }
  }

  const auto available_bytes = AvailableBytes(file_descriptor_);
  if (available_bytes) {
    throw std::runtime_error(
        fmt::format("{} bytes still available after carriage return.", available_bytes));
  }

  std::string response(buffer, bytes_read);
  if (!CheckCRC(response)) {
    throw CrcMismatchException();
  }

  // Cut crc and carriage return bytes.
  response.resize(response.length() - 3);
  return response;
}

std::string SerialPort::Query(std::string_view query, bool with_crc, int n_retries) const {
  while (true) {
    try {
      Send(query, with_crc);
      return Receive();
    } catch (const CrcMismatchException&) {
      if (--n_retries <= 0) throw;
    }
  }
}
