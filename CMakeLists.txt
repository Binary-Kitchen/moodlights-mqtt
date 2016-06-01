cmake_minimum_required(VERSION 2.8)
project(moodlights_mqtt)

add_definitions(-std=c++1z)

add_subdirectory(libhausbus)

set(SRCS main.cpp)

add_executable(moodlights_mqtt ${SRCS})

target_link_libraries(moodlights_mqtt mosquittopp mosquitto hausbus)
