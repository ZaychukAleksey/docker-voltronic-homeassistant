#include "logging.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>

#include "main.h"


void lprintf(const char* format, ...) {
  static std::mutex log_mutex;

  // Only print if debug flag is set, else do nothing
  if (debugFlag) {
    va_list ap;
    char fmt[2048];

    //actual time
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char buf[256];
    strcpy(buf, asctime(timeinfo));
    buf[strlen(buf) - 1] = 0;

    //connect with args
    snprintf(fmt, sizeof(fmt), "%s %s\n", buf, format);

    //put on screen:
    va_start(ap, format);
    vprintf(fmt, ap);
    va_end(ap);

    //to the logfile:
    static FILE* log;
    std::lock_guard lock(log_mutex);
    log = fopen(kLogFile, "a");
    va_start(ap, format);
    vfprintf(log, fmt, ap);
    va_end(ap);
    fclose(log);
  }
}
