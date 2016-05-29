#include <unistd.h>

#include <iostream>
#include <mosquittopp.h>

#include "hausbus.h"
#include "moodlights.h"

using namespace std;

#define MOODLIGHT_DEVICE_IDENTIFIER 0x10
#define MY_DEVICE_IDENTIFIER 0xfe

int main(int argc, char **argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " device_name" << endl;
        return -1;
    }

    if (mosqpp::lib_init() != MOSQ_ERR_SUCCESS)
        throw std::runtime_error("Mosquitto initialisation failed");

    Hausbus hausbus(argv[1]);

    // source id: 0xFE
    // destination id: 0x10, moodlights device identifier
    Moodlights m(MY_DEVICE_IDENTIFIER, MOODLIGHT_DEVICE_IDENTIFIER);

    if (mosqpp::lib_cleanup() != MOSQ_ERR_SUCCESS)
        throw std::runtime_error("Mosquitto cleanup failed");

    return 0;
}
