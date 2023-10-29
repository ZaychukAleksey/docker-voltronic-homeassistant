#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include "inverter.h"
#include "logging.h"
#include "main.h"

namespace {

uint16_t CalCrcHalf(uint8_t* pin, uint8_t len) {
  uint16_t crc;

  uint8_t da;
  uint8_t* ptr;
  uint8_t bCRCHign;
  uint8_t bCRCLow;

  uint16_t crc_ta[16] = {
      0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
      0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef
  };
  ptr = pin;
  crc = 0;

  while (len-- != 0) {
    da = ((uint8_t)(crc >> 8)) >> 4;
    crc <<= 4;
    crc ^= crc_ta[da ^ (*ptr >> 4)];
    da = ((uint8_t)(crc >> 8)) >> 4;
    crc <<= 4;
    crc ^= crc_ta[da ^ (*ptr & 0x0f)];
    ptr++;
  }
  bCRCLow = crc;
  bCRCHign = (uint8_t)(crc >> 8);
  if (bCRCLow == 0x28 || bCRCLow == 0x0d || bCRCLow == 0x0a)
    bCRCLow++;
  if (bCRCHign == 0x28 || bCRCHign == 0x0d || bCRCHign == 0x0a)
    bCRCHign++;
  crc = ((uint16_t) bCRCHign) << 8;
  crc += bCRCLow;
  return (crc);
}

bool CheckCRC(unsigned char* data, int len) {
  uint16_t crc = CalCrcHalf(data, len - 3);
  return data[len - 3] == (crc >> 8) && data[len - 2] == (crc & 0xff);
}

}  // namespace


Inverter::Inverter(const std::string& device)
    : device_(device) {
  status1_[0] = 0;
  status2_[0] = 0;
  warnings_[0] = 0;
}

void Inverter::StartBackgroundPolling(std::size_t polling_interval_in_seconds) {
  polling_thread_ = std::thread([this, polling_interval = polling_interval_in_seconds]() {
    while (!polling_thread_termination_requested_) {
      Poll();
      sleep(polling_interval);
    }
  });
}

void Inverter::StopBackgroundPolling() {
  polling_thread_termination_requested_ = true;
  polling_thread_.join();
}

std::string Inverter::GetQpigsStatus() const {
  std::lock_guard lock(mutex_);
  return {status1_};
}

std::string Inverter::GetQpiriStatus() const {
  std::lock_guard lock(mutex_);
  return {status2_};
}

std::string Inverter::GetWarnings() const {
  std::lock_guard lock(mutex_);
  return {warnings_};
}

void Inverter::SetMode(char newmode) {
  std::lock_guard lock(mutex_);
  if (mode_ && newmode != mode_) {
    ups_status_changed = true;
  }
  mode_ = newmode;
}

int Inverter::GetMode() const {
  std::lock_guard lock(mutex_);
  switch (mode_) {
    case 'P': return 1;  // Power_On
    case 'S': return 2;  // Standby
    case 'L': return 3;  // Line
    case 'B': return 4;  // Battery
    case 'F': return 5;  // Fault
    case 'H': return 6;  // Power_Saving
    default: break;
  }
  return 0;  // Unknown
}

bool Inverter::Query(std::string_view cmd) {
  time_t started;
  int i = 0;

  auto fd = open(device_.data(), O_RDWR | O_NONBLOCK);
  if (fd == -1) {
    auto err_message("ERROR: Unable to open device " + device_ + ". " + strerror(errno) + '.');
    dlog(err_message.c_str());
    throw std::runtime_error(err_message);
  }

  // Once connected, set the baud rate and other serial config (Don't rely on this being correct on the system by default...)
  speed_t baud = B2400;

  // Speed settings (in this case, 2400 8N1)
  struct termios settings;
  tcgetattr(fd, &settings);

  cfsetospeed(&settings, baud);      // baud rate
  settings.c_cflag &= ~PARENB;       // no parity
  settings.c_cflag &= ~CSTOPB;       // 1 stop bit
  settings.c_cflag &= ~CSIZE;
  settings.c_cflag |= CS8 | CLOCAL;  // 8 bits
  // settings.c_lflag = ICANON;         // canonical mode_
  settings.c_oflag &= ~OPOST;        // raw output

  tcsetattr(fd, TCSANOW, &settings); // apply the settings
  tcflush(fd, TCOFLUSH);

  // ---------------------------------------------------------------

  // Generating CRC for a command
  uint16_t crc = CalCrcHalf((uint8_t*) cmd.data(), cmd.length());
  auto n = cmd.length();
  memcpy(&buf_, cmd.data(), n);
  dlog("DEBUG:  Current CRC: %X %X", crc >> 8, crc & 0xff);
  buf_[n++] = crc >> 8;
  buf_[n++] = crc & 0xff;
  buf_[n++] = 0x0d;

  // Send buffer in hex
  char messagestart[128];
  char* messageptr = messagestart;
  sprintf(messagestart, "DEBUG:  Send buffer hex bytes:  ( ");
  messageptr += strlen(messagestart);

  for (int j = 0; j < n; j++) {
    int size = sprintf(messageptr, "%02x ", buf_[j]);
    messageptr += 3;
  }
  dlog("%s)", messagestart);

  /* The below command doesn't take more than an 8-byte payload 5 chars (+ 3
     bytes of <CRC><CRC><CR>).  It has to do with low speed USB specifications.
     So we must chunk up the data and send it in a loop 8 bytes at a time.  */

  // Send the command (or part of the command if longer than chunk_size)
  int chunk_size = 8;
  if (n < chunk_size) // Send in chunks of 8 bytes, if less than 8 bytes to send... just send that
    chunk_size = n;
  int bytes_sent = 0;
  int remaining = n;

  while (remaining > 0) {
    ssize_t written = write(fd, &buf_[bytes_sent], chunk_size);
    bytes_sent += written;
    if (remaining - written >= 0)
      remaining -= written;
    else
      remaining = 0;

    if (written < 0)
      dlog("DEBUG:  Write command failed, error number %d was returned", errno);
    else
      dlog("DEBUG:  %d bytes written, %d bytes sent, %d bytes remaining", written, bytes_sent,
           remaining);

    chunk_size = remaining;
    usleep(50000);   // Sleep 50ms before sending another 8 bytes of info
  }

  time(&started);

  // Instead of using a fixed size for expected response length, lets find it
  // by searching for the first returned <cr> char instead.
  char* startbuf = 0;
  char* endbuf = 0;
  do {
    // According to protocol manual, it appears no query should ever exceed 120 byte size in response
    n = read(fd, (void*) buf_ + i, 120 - i);
    if (n < 0) {
      // Wait 5 secs before timeout
      if (time(NULL) - started > 5) {
        dlog("DEBUG:  %s read timeout", cmd);
        break;
      } else {
        usleep(50000);  // sleep 50ms
        continue;
      }
    }
    i += n;
    buf_[i] = '\0';  // terminate what we have so far with a null string
    dlog("DEBUG:  %d bytes read, %d total bytes:  %02x %02x %02x %02x %02x %02x %02x %02x",
        n, i, buf_[i - 8], buf_[i - 7], buf_[i - 6], buf_[i - 5], buf_[i - 4], buf_[i - 3],
        buf_[i - 2], buf_[i - 1]);

    startbuf = (char*) &buf_[0];
    endbuf = strchr(startbuf, '\r');

    //log("DEBUG:  %s Current buffer: %s", cmd, startbuf);
  } while (endbuf == nullptr);     // Still haven't found end <cr> char as long as pointer is null
  close(fd);

  int replysize = endbuf - startbuf + 1;
  dlog("DEBUG:  Found reply <cr> at byte: %d", replysize);

  if (buf_[0] != '(' || buf_[replysize - 1] != 0x0d) {
    dlog("DEBUG:  %s: incorrect buffer start/stop bytes.  Buffer: %s", cmd, buf_);
    return false;
  }
  if (!(CheckCRC(buf_, replysize))) {
    dlog("DEBUG:  %s: CRC Failed!  Reply size: %d  Buffer: %s", cmd, replysize, buf_);
    return false;
  }
  buf_[replysize - 3] = '\0';      // Null-terminating on first CRC byte
  dlog("DEBUG:  %s: %d bytes read: %s", cmd, i, buf_);

  dlog("DEBUG:  %s query finished", cmd);
  return true;
}

void Inverter::Poll() {
  // Reading mode_
  if (!ups_qmod_changed) {
    if (Query("QMOD") && strcmp((char*) &buf_[1], "NAK") != 0) {
      SetMode(buf_[1]);
      ups_qmod_changed = true;
    }
  }

  // Reading QPIGS status
  if (!ups_qpigs_changed) {
    if (Query("QPIGS") && strcmp((char*) &buf_[1], "NAK") != 0) {
      std::lock_guard lock(mutex_);
      strcpy(status1_, (const char*) buf_ + 1);
      ups_qpigs_changed = true;
    }
  }

  // Reading QPIRI status
  if (!ups_qpiri_changed) {
    if (Query("QPIRI") && strcmp((char*) &buf_[1], "NAK") != 0) {
      std::lock_guard lock(mutex_);
      strcpy(status2_, (const char*) buf_ + 1);
      ups_qpiri_changed = true;
    }
  }

  // Get any device warnings_...
  if (!ups_qpiws_changed) {
    if (Query("QPIWS") && strcmp((char*) &buf_[1], "NAK") != 0) {
      std::lock_guard lock(mutex_);
      strcpy(warnings_, (const char*) buf_ + 1);
      ups_qpiws_changed = true;
    }
  }
}

void Inverter::ExecuteCmd(std::string_view cmd) {
  // Sending any command raw
  if (Query(cmd.data())) {
    std::lock_guard lock(mutex_);
    strcpy(status2_, (const char*) buf_ + 1);
  }
}
