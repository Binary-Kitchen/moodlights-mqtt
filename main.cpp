#include <unistd.h>

#include <iostream>

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

    Hausbus hausbus(argv[1]);

    // source id: 0xFE
    // destination id: 0x10, moodlights device identifier
    Moodlights m(MY_DEVICE_IDENTIFIER, MOODLIGHT_DEVICE_IDENTIFIER);

    // Set all lights and wait a second
    m.set_all(Moodlights::Color{255, 0, 123});
    hausbus << m;
    sleep(1);

    // Example:
    // set all lamps to random level and update every 500ms
    for (;;) {
        m.rand_all();
        hausbus << m;
        usleep(500000);
    }

    return 0;
}
