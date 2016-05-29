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
            const Byte preamble = HAUSBUS_PREAMBLE);
    ~Hausbus();

    Data create_packet(const Byte src, const Byte dst, const Data &payload) const;
    void send_packet(const Data &packet);
    void send(const Byte src, const Byte dst, const Data &payload);

    // TBD: Receive hausbus packets

private:

    // header structure according to Hausbus protocol
    struct Header {
        Byte preamble;
        Byte src;
        Byte dst;
        Byte length;
    } __attribute__((packed));

    const Byte _preamble;
    const std::string _device_filename;

    int _fd;
};
