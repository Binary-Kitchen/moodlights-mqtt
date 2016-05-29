#include <unistd.h>

#include <iostream>
#include <memory>
#include <mutex>
#include <regex>

#include <mosquittopp.h>

#include "hausbus.h"
#include "moodlights.h"

using namespace std;

#define MOODLIGHT_DEVICE_IDENTIFIER 0x10
#define MY_DEVICE_IDENTIFIER 0xfe

// gobal access mutex to prevent race conditions
static std::mutex global_mutex;

// allow global access to hausbus and moodlights via unique_ptrs
static std::unique_ptr<Hausbus> hausbus = nullptr;
static std::unique_ptr<Moodlights> moodlights = nullptr;

int main(int argc, char **argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " device_name" << endl;
        return -1;
    }

    if (mosqpp::lib_init() != MOSQ_ERR_SUCCESS)
        throw std::runtime_error("Mosquitto initialisation failed");

    // Initialise Moodlights and Hausbus
    hausbus = std::unique_ptr<Hausbus>(new Hausbus(argv[1]));

    // source id: 0xFE
    // destination id: 0x10, moodlights device identifier
    moodlights = std::unique_ptr<Moodlights>(new Moodlights(MY_DEVICE_IDENTIFIER, MOODLIGHT_DEVICE_IDENTIFIER));

    if (mosqpp::lib_cleanup() != MOSQ_ERR_SUCCESS)
        throw std::runtime_error("Mosquitto cleanup failed");

    return 0;
}
