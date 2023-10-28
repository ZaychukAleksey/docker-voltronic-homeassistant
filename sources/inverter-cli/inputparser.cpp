#include "inputparser.h"

#include <algorithm>
#include <string>

InputParser::InputParser(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    tokens_.emplace_back(argv[i]);
  }
}

const std::string& InputParser::GetCmdOption(const std::string& option) const {
  auto itr = std::find(tokens_.begin(), tokens_.end(), option);
  if (itr != tokens_.end() && ++itr != tokens_.end()) {
    return *itr;
  }
  static const std::string empty_string;
  return empty_string;
}

bool InputParser::CmdOptionExists(const std::string& option) const {
  return std::find(tokens_.begin(), tokens_.end(), option) != tokens_.end();
}
