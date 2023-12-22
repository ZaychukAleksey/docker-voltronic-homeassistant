#include "protocol.hh"

#include <stdexcept>

#include "../exceptions.h"

Protocol ProtocolFromString(std::string_view s) {
  if (s == "PI17") return Protocol::PI17;
  if (s == "PI18") return Protocol::PI18;
  if (s == "PI17") return Protocol::PI17;
  throw UnsupportedProtocolException(s);
}

std::string_view ProtocolToString(Protocol protocol) {
  switch (protocol) {
    case Protocol::PI17: return "PI17";
    case Protocol::PI18: return "PI18";
    case Protocol::PI30: return "PI30";
  }
  throw std::runtime_error("");
}
