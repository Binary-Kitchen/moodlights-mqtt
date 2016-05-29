#include <cstdlib>

#include "moodlights.h"

#define rand_byte() ((Byte)::rand())

Moodlights::Moodlights(const Byte src, const Byte dst) :
    _src(src),
    _dst(dst)
{
    set_all(Color {0, 0, 0});
}

Moodlights::~Moodlights()
{
}

Moodlights::Color Moodlights::rand_color()
{
    return Color {rand_byte(), rand_byte(), rand_byte()};
}

void Moodlights::set(unsigned int no, const Color &c)
{
    if (no > MOODLIGHTS_LAMPS)
        throw std::runtime_error("Invalid lamp");

    _lamps[no] = c;
}

void Moodlights::set_all(const Color &c)
{
    for (int i = 0 ; i < MOODLIGHTS_LAMPS ; i++)
        set(i, c);
}

const Moodlights::Color &Moodlights::get(unsigned int no) const
{
    if (no > MOODLIGHTS_LAMPS)
        throw std::runtime_error("Invalid lamp");

    return _lamps[no];
}

void Moodlights::rand(const unsigned int no)
{
    set(no, rand_color());
}

void Moodlights::rand_all()
{
    for (auto &lamp : _lamps)
        lamp = rand_color();
}

Data Moodlights::get_payload() const
{
    Data payload(MOODLIGHTS_LAMPS * 3);

    // assemble packet
    for (int i = 0 ; i < MOODLIGHTS_LAMPS ; i++) {
        payload[i*3] = _lamps[i][0];
        payload[i*3 + 1] = _lamps[i][1];
        payload[i*3 + 2] = _lamps[i][2];
    }

    return payload;
}

void Moodlights::blank(unsigned int no)
{
    set(no, Color {0, 0, 0});
}

void Moodlights::blank_all()
{
    set_all(Color {0, 0, 0});
}

Hausbus &operator <<(Hausbus &h, const Moodlights &m)
{
    h.send(m._src, m._dst, m.get_payload());
}
