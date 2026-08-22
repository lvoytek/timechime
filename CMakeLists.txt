cmake_minimum_required(VERSION 3.20.0)

# Allow local board extensions
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Add vs1053 driver
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/drivers/vs1053)

set(SHIELD waveshare_epaper_gdew042t2)

find_package(Zephyr)
project(timechime)

# CMake's language init caches the host objcopy before the cross-toolchain can
# override it; force it to the SDK's ARM binary.
set(CMAKE_OBJCOPY "${CROSS_COMPILE}objcopy" CACHE FILEPATH "" FORCE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})