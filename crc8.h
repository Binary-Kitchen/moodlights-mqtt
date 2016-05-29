#pragma once

#include <cstddef>

#include "data.h"

namespace CRC8 {
    bool verify(const char* data, size_t len, Byte cksum);

    Byte create(const Byte* data, size_t len);
    Byte create(const Data &d);
    Byte create(const Data &d, size_t len);
}
