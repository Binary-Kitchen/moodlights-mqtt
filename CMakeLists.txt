project(moodlights_mqtt)

add_definitions(-std=c++11)

set (SRCS main.cpp crc8.cpp hausbus.cpp moodlights.cpp)

add_executable(moodlights_mqtt ${SRCS})
