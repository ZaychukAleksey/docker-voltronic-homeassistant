#pragma once

#include <string_view>

enum class Protocol : char {
  PI17,
  PI18,
  PI30,
};

Protocol ProtocolFromString(std::string_view);
std::string_view ProtocolToString(Protocol);
