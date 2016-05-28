#pragma once

#include <cstddef>

#include "data.h"

namespace CRC8 {
    bool verify(const char* data, size_t len, unsigned char cksum);

    unsigned char create(const unsigned char* data, size_t len);
    unsigned char create(const Data &d);
    unsigned char create(const Data &d, size_t len);
}
