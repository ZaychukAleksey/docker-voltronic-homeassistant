# MQTT C++ client library
# https://github.com/eclipse/paho.mqtt.cpp
option(PAHO_BUILD_STATIC "" ON)
option(PAHO_BUILD_SHARED "" OFF)
option(PAHO_WITH_MQTT_C "" ON)
add_subdirectory(paho.mqtt.cpp)

# The logging library.
# https://github.com/gabime/spdlog
add_subdirectory(spdlog)
