cmake_minimum_required(VERSION 2.8)
project(moodlights_mqtt)

add_definitions(-std=c++1z)

set(SRCS
	main.cpp
	libhausbus/crc8.cpp
	libhausbus/hausbus.cpp
	libhausbus/moodlights.cpp)

add_executable(moodlights_mqtt ${SRCS})

target_link_libraries(moodlights_mqtt mosquittopp mosquitto)

install(TARGETS moodlights_mqtt RUNTIME DESTINATION bin)
install(FILES scripts/moodlights.service DESTINATION /etc/systemd/system/)
