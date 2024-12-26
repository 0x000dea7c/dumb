cmake_minimum_required(VERSION 3.10)
project(dumb CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(SUPPRESSION_FILE "${CMAKE_SOURCE_DIR}/lsan_suppressions.txt")

# common compiler flags
add_compile_options(-Wall -Wextra -pedantic -fno-exceptions -fno-rtti)

# release flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -O3 -ffast-math -march=native")

# debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -DDUMB_DEBUG -ggdb -fsanitize=address,leak,undefined")

# ----------------------------------------------------------------------------
# create a shared library "dumb_game_logic" from code/dumb_game_logic.cpp
# ----------------------------------------------------------------------------
add_library(dumb_game_logic SHARED code/dumb_game_logic.cpp)

set_target_properties(dumb_game_logic PROPERTIES POSITION_INDEPENDENT_CODE ON)

# link SDL libraries to this library
target_link_libraries(dumb_game_logic PRIVATE SDL2)

# ----------------------------------------------------------------------------
# create an executable "dumb" from code/dumb_gnulinux_sdl.cpp
# ----------------------------------------------------------------------------
add_executable(dumb code/dumb_gnulinux_sdl.cpp)

# link SDL libraries to this executable as well
target_link_libraries(dumb PRIVATE SDL2 dumb_game_logic)

# ----------------------------------------------------------------------------
# convenience targets for "run" and "gdb"
# ----------------------------------------------------------------------------
add_custom_target(run
  COMMAND ${CMAKE_COMMAND} -E env LSAN_OPTIONS="suppressions=${SUPPRESSION_FILE}" $<TARGET_FILE:dumb>
  DEPENDS dumb
  COMMENT "run 'dumb' executable"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(gdb
  COMMAND gdb -q $<TARGET_FILE:dumb>
  DEPENDS dumb
  COMMENT "debug 'dumb' executable with gdb"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
