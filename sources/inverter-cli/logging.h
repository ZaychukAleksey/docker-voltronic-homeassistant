#pragma once

#include <string>
#include <string_view>


/// Switch to debug mode so all debug logs are printed to the output.
void SetDebugMode();
bool IsInDebugMode();

void log(const char* format, ...);
/// Only print if debug flag is set (see SetDebugMode()), else do nothing.
void dlog(const char* format, ...);

std::string PrintBytesAsHex(std::string_view str);
std::string EscapeString(std::string_view src);
