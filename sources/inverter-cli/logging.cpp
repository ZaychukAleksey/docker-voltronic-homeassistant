#include "logging.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>

#include "main.h"

namespace {

const char* kLogFile = "/dev/null";
bool debug_mode = false;

}  // namespace


void SetDebugMode() {
  debug_mode = true;
}

void log(const char* format, ...) {
  if (!debug_mode) {
    return;
  }

  va_list ap;

  //actual time
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  char buf[256];
  strcpy(buf, asctime(timeinfo));
  buf[strlen(buf) - 1] = 0;

  // Connect with args
  char fmt[2048];
  snprintf(fmt, sizeof(fmt), "%s %s\n", buf, format);

  // Put on screen:
  va_start(ap, format);
  vprintf(fmt, ap);
  va_end(ap);

  // To the logfile:
  static std::mutex log_mutex;
  std::lock_guard lock(log_mutex);
  auto log = fopen(kLogFile, "a");
  va_start(ap, format);
  vfprintf(log, fmt, ap);
  va_end(ap);
  fclose(log);
}
