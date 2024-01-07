#pragma once

#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace utils {

std::string PrintBytesAsHex(std::string_view str);
std::string EscapeString(std::string_view src);
unsigned AsDigit(char);

template<typename Enum> requires std::is_enum_v<Enum>
std::string Concatenate(const std::vector<Enum>& enum_values) {
  std::string result;
  for (auto& value: enum_values) {
    if (!result.empty()) {
      result += ',';
    }
    result += '"';
    result.append(ToString(value));
    result += '"';
  }
  return result;
}

template<typename ValueType>
std::string ToString(const ValueType& value) {
  if constexpr (std::is_same_v<ValueType, bool>) {
    return value ? "1" : "0";
  } else if constexpr (std::is_arithmetic_v<ValueType>) {
    return std::format("{}", value);
  } else if constexpr (std::is_enum_v<ValueType>) {
    return std::string(ToString(value));
  } else if constexpr (std::is_same_v<ValueType, std::string>) {
    return value;
  } else {
    throw std::runtime_error("Unknown type");
  }
}

}  // namespace utils
