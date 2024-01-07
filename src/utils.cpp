#include "utils.h"

#include <cstdio>
#include <format>
#include <stdexcept>


namespace utils {

std::string PrintBytesAsHex(std::string_view str) {
  // Each input symbol will be turned into 2 hex symbols + whitespace
  std::string result;
  result.reserve(str.length() * 3 - 1);
  char buffer[20];
  for (std::size_t i = 0; i < str.length(); ++i) {
    sprintf(buffer, "%02x", str[i]);
    if (i != 0) {
      result += ' ';
    }
    result += buffer[0];
    result += buffer[1];
  }
  return result;
}

std::string EscapeString(std::string_view src) {
  static constexpr char kHexChar[] = "0123456789abcdef";

  std::string dest;
  bool last_hex_escape = false;  // true if last output char was \xNN.

  for (char c : src) {
    bool is_hex_escape = false;
    switch (c) {
      case '\n': dest.append("\\" "n"); break;
      case '\r': dest.append("\\" "r"); break;
      case '\t': dest.append("\\" "t"); break;
      case '\"': dest.append("\\" "\""); break;
      case '\'': dest.append("\\" "'"); break;
      case '\\': dest.append("\\" "\\"); break;
      default:
        // Note that if we emit \xNN and the src character after that is a hex
        // digit then that digit must be escaped too to prevent it being
        // interpreted as part of the character code by C.
        if ((c < 0x80) && (!isprint(c) || (last_hex_escape && isxdigit(c)))) {
          dest.append("\\" "x");
          dest.push_back(kHexChar[c / 16]);
          dest.push_back(kHexChar[c % 16]);
          is_hex_escape = true;
        } else {
          dest.push_back(c);
        }
    }
    last_hex_escape = is_hex_escape;
  }

  return dest;
}

unsigned AsDigit(char c) {
  if (!std::isdigit(c)) {
    throw std::runtime_error(std::format("Digit is expected, but got {}", c));
  }
  return static_cast<unsigned>(c - '0');
}

}  // namespace utils
