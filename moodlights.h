#pragma once

#include <array>

#include "hausbus.h"
#include "data.h"

#define MOODLIGHTS_LAMPS 10

class Moodlights {
public:
    // Format: R:G:B
    using Color = std::array<Byte, 3>;

    Moodlights(const Byte src, const Byte dst);
    ~Moodlights();

    static Color rand_color();

    void set(unsigned int no, const Color &c);
    void set_all(const Color &c);
    const Color &get(unsigned int no) const;

    void blank(unsigned int no);
    void blank_all();

    void rand(const unsigned int no);
    void rand_all();

    Data get_payload() const;

    friend Hausbus& operator <<(Hausbus &in, const Moodlights &m);

private:

    const Byte _src;
    const Byte _dst;

    std::array<Color, MOODLIGHTS_LAMPS> _lamps;
};
