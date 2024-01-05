#pragma once

#include <string>
#include <vector>
#include <string_view>

std::string PrintBytesAsHex(std::string_view str);
std::string EscapeString(std::string_view src);
unsigned AsDigit(char);

template<typename Enum> requires std::is_enum_v<Enum>
std::string Concatenate(const std::vector<Enum>& enum_values) {
  std::string result;
  for (auto& value : enum_values) {
    if (!result.empty()) {
      result += ',';
    }
    result += '"';
    result.append(ToString(value));
    result += '"';
  }
  return result;
}
