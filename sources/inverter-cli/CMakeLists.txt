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
  inverter.cpp
  logging.cpp
)
