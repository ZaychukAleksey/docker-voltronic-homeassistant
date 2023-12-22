cmake_minimum_required(VERSION 3.5)
project("inverter_poller" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
target_link_options(inverter_poller PRIVATE -static-libgcc -static-libstdc++)

target_include_directories(inverter_poller PRIVATE .)
