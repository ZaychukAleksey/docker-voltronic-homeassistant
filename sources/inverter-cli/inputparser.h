#pragma once

#include <string>
#include <vector>

/// This class simply finds cmd line args and parses them for use in a program.
/// It is not posix compliant and wont work with args like:   ./program -xf filename
/// You must place each arg after its own separate dash like: ./program -x -f filename
class InputParser {
 public:
  InputParser(int& argc, char** argv);
  const std::string& getCmdOption(const std::string& option) const;
  bool cmdOptionExists(const std::string& option) const;

 private:
  std::vector<std::string> tokens_;
};
