cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr)
project(timechime_remote)

set(SHIELD waveshare_epaper_gdew042t2)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})