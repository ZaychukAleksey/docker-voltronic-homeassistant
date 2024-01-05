cmake_minimum_required(VERSION 3.5)

# Retrieves the number of commits in the project. It will be used as a patch-level version.
execute_process(
  COMMAND git rev-list --count HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE VERSION_PATCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

project("inverter_poller"
  VERSION 0.9.${VERSION_PATCH}
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)
add_subdirectory(submodules)
