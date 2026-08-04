cmake_minimum_required(VERSION 3.20.0)

# Allow local board extensions
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(SHIELD waveshare_epaper_gdew042t2)

find_package(Zephyr)
project(timechime_remote)


FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})