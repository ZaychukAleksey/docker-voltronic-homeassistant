#pragma once

#include <format>
#include <string>
#include <string_view>
#include <span>
#include <vector>

namespace utils {

std::string PrintBytesAsHex(std::string_view str);
std::string EscapeString(std::string_view src);
unsigned AsDigit(char);

template<typename ValueType>
std::string ToString(const ValueType& value, bool for_json = false) {
  if constexpr (std::is_same_v<ValueType, bool>) {
    return value ? "1" : "0";
  } else if constexpr (std::is_arithmetic_v<ValueType>) {
    return std::format("{}", value);
  } else if constexpr (std::is_enum_v<ValueType>) {
    return for_json ? std::format("\"{}\"", ToString(value)) : std::string(ToString(value));
  } else if constexpr (std::is_same_v<ValueType, std::string>) {
    return for_json ? std::format("\"{}\"", value) : value;
  } else {
    throw std::runtime_error("Unknown type");
  }
}

std::string Concatenate(const auto& range) {
  std::string result;
  for (const auto& value: range) {
    if (!result.empty()) {
      result += ',';
    }
    result.append(ToString(value, true));
  }
  return result;
}

}  // namespace utils
