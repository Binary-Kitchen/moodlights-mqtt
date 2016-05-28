#pragma once

#include <string>

#include "data.h"

#define HAUSBUS_PREAMBLE 0x40
#define HAUSBUS_MAX_PACKET_LENGTH 64
#define HAUSBUS_BAUDRATE 115200

class Hausbus {

public:

    Hausbus(const std::string &device_filename,
            const int speed = HAUSBUS_BAUDRATE,
            const unsigned char preamble = HAUSBUS_PREAMBLE);
    ~Hausbus();

    Data create_packet(const unsigned char src, const unsigned char dst, const Data &payload) const;
    void send_packet(const Data &packet);
    void send(const unsigned char src, const unsigned char dst, const Data &payload);

    // TBD: Receive hausbus packets

private:

    // header structure according to Hausbus protocol
    struct Header {
        unsigned char preamble;
        unsigned char src;
        unsigned char dst;
        unsigned char length;
    } __attribute__((packed));

    const unsigned char _preamble;
    const std::string _device_filename;

    int _fd;
};
