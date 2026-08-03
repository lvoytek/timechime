cmake_minimum_required(VERSION 3.20.0)

set(SHIELD waveshare_epaper_gdew042t2)

find_package(Zephyr)
project(timechime_remote)


FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})