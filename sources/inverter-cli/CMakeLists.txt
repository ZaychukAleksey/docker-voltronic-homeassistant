cmake_minimum_required(VERSION 3.5)
project("inverter_poller" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(spdlog QUIET)
if (NOT spdlog_FOUND)
  MESSAGE(FATAL_ERROR "spdlog is not installed, please, install it with \"apt install libspdlog-dev\".")
endif()

add_executable(inverter_poller)
target_sources(inverter_poller
 PRIVATE
  main.cpp

  configuration.cpp
  utils.cpp
  serial_port.cpp
  protocols/protocol.cpp
  protocols/protocol_adapter.cpp
  protocols/pi18_protocol_adapter.cpp
  protocols/pi30_protocol_adapter.cpp
)
target_link_libraries(inverter_poller PRIVATE spdlog::spdlog)
