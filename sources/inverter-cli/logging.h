#pragma once

/// Switch to debug mode so all debug logs are printed to the output.
void SetDebugMode();

/// Only print if debug flag is set (see SetDebugMode()), else do nothing.
void log(const char* format, ...);
