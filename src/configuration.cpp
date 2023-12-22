#include "configuration.h"

#include <algorithm>
#include <fstream>
#include <string>

#include "spdlog/fmt/fmt.h"

CommandLineArguments::CommandLineArguments(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    tokens_.emplace_back(argv[i]);
  }
}

const std::string& CommandLineArguments::Get(std::string_view option) const {
  auto itr = std::find(tokens_.begin(), tokens_.end(), option);
  if (itr != tokens_.end() && ++itr != tokens_.end()) {
    return *itr;
  }
  throw std::runtime_error(fmt::format("ERROR. Option '{}' hasn't been specified.", option));
}

bool CommandLineArguments::IsSet(std::string_view option, std::string_view option_alias) const {
  if (std::find(tokens_.begin(), tokens_.end(), option) != tokens_.end()) {
    return true;
  }
  if (option_alias.empty()) {
    return false;
  }
  return std::find(tokens_.begin(), tokens_.end(), option_alias) != tokens_.end();
}

static float ToFloat(const std::string& option_name, const std::string& option_value) {
  try {
    return stof(option_value);
  } catch (...) {
    throw std::runtime_error(fmt::format(
        "ERROR. Incorrect value '{}' for option '{}'. Floating point value is expected.",
        option_value, option_name));
  }
}

Settings LoadSettingsFromFile(const std::string& filename) {
  std::ifstream file(filename);
  if (!file) {
    throw std::runtime_error("ERROR. Failed to open configuration file: " + filename);
  }

  Settings result;
  std::string line;
  while (getline(file, line)) {
    // Skip empty or commented lines (lines starting with '#').
    const auto comment_position = line.find('#');
    if (comment_position != std::string::npos || line.empty()) continue;

    const auto delimiter = line.find('=');
    if (delimiter == std::string::npos || delimiter == 0 || (delimiter == line.length() - 1)) {
      throw std::runtime_error("ERROR. Incorrect line in configuration file: \"" + line + '"');
    }

    auto parameter_name = line.substr(0, delimiter);
    auto parameter_value = line.substr(delimiter + 1, std::string::npos - delimiter);
    if (parameter_name == "device") {
      result.device_name = parameter_value;
    } else if (parameter_name == "protocol") {
      result.protocol = ProtocolFromString(parameter_value);
    } else if (parameter_name == "amperage_factor") {
      result.amperage_factor = ToFloat(parameter_name, parameter_value);
    } else if (parameter_name == "watt_factor") {
      result.watt_factor = ToFloat(parameter_name, parameter_value);
    } else {
      throw std::runtime_error("ERROR. Unknown configuration parameter: " + parameter_name);
    }
  }
  return result;
}
