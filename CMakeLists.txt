cmake_minimum_required(VERSION 3.25)
project(uitoolkit C)

set(CMAKE_C_STANDARD 11)

add_executable(uitoolkit main.c ui.h dynarray.h hashmap.h json.h curl_wrapper.h manga.h)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -Wall -Wpedantic -Wextra")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-stack-protector")

target_link_libraries(uitoolkit SDL2 SDL2_ttf curl)
