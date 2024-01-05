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
  mqtt/mqtt.cpp
  mqtt/sensor.cpp
)

target_link_libraries(inverter_poller
 PRIVATE
  spdlog::spdlog
  paho-mqtt3as-static
  paho-mqttpp3-static
)

target_compile_definitions(inverter_poller
  PRIVATE APP_NAME="${PROJECT_NAME}" APP_VERSION="${PROJECT_VERSION}"
)

target_link_options(inverter_poller PRIVATE -static-libgcc -static-libstdc++)

target_include_directories(inverter_poller PRIVATE .)
