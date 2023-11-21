#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "protocols/protocol.hh"

/// This class simply finds cmd line args and parses them for use in a program.
/// It is not posix compliant and wont work with args like:   ./program -xf filename
/// You must place each arg after its own separate dash like: ./program -x -f filename
class CommandLineArguments {
 public:
  CommandLineArguments(int argc, char** argv);

  /// @returns true if the @option (or its alias @option_alias) was set via command line arguments.
  bool IsSet(std::string_view option, std::string_view option_alias = "") const;

  /// @returns option value if the option was provided.
  /// @throws runtime_error if the option wasn't provided.
  const std::string& Get(std::string_view option) const;

 private:
  std::vector<std::string> tokens_;
};

struct Settings {
  /// The device in OS, e.g. "/dev/hidraw0".
  std::string device_name;
  Protocol protocol;

  /// This allows you to modify the amperage in case the inverter is giving an incorrect
  /// reading compared to measurement tools.  Normally this will remain '1'
  float amperage_factor = 1.0f;

  /// This allows you to modify the wattage in case the inverter is giving an incorrect
  /// reading compared to measurement tools.  Normally this will remain '1'
  float watt_factor = 1.0f;
};

[[nodiscard]] Settings LoadSettingsFromFile(const std::string& filename);
