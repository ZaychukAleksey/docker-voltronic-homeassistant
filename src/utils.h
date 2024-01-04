#pragma once

#include <string>
#include <string_view>

std::string PrintBytesAsHex(std::string_view str);
std::string EscapeString(std::string_view src);
unsigned AsDigit(char);
