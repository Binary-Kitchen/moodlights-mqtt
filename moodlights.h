#pragma once

#include <array>

#include "hausbus.h"
#include "data.h"

#define MOODLIGHTS_LAMPS 10

class Moodlights {
public:
    // Format: R:G:B
    using Color = std::array<unsigned char, 3>;

    Moodlights(const unsigned char src, const unsigned char dst);
    ~Moodlights();

    static Color rand_color();

    void set(unsigned char no, const Color &c);
    void set_all(const Color &c);
    const Color &get(unsigned char no) const;

    void rand(const unsigned char no);
    void rand_all();

    friend Hausbus& operator <<(Hausbus &in, const Moodlights &m);

private:

    const unsigned char _src;
    const unsigned char _dst;

    std::array<Color, MOODLIGHTS_LAMPS> _lamps;
};
