#include <cstdlib>

#include "moodlights.h"

#define RAND_BYTE ((unsigned char)::rand())

Moodlights::Moodlights(const unsigned char src, const unsigned char dst) :
    _src(src),
    _dst(dst)
{
}

Moodlights::~Moodlights()
{
}

void Moodlights::set(unsigned char no, const Color &c)
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

const Moodlights::Color &Moodlights::get(unsigned char no) const
{
    if (no > MOODLIGHTS_LAMPS)
        throw std::runtime_error("Invalid lamp");

    return _lamps[no];
}

void Moodlights::rand(const unsigned char no)
{
    set(no, Moodlights::Color {RAND_BYTE, RAND_BYTE, RAND_BYTE});
}

void Moodlights::rand_all()
{
    for (int i = 0 ; i < MOODLIGHTS_LAMPS ; i++)
        rand(i);
}

Hausbus &operator <<(Hausbus &h, const Moodlights &m)
{
    Data payload(MOODLIGHTS_LAMPS * 3);

    // assemble packet
    for (int i = 0 ; i < MOODLIGHTS_LAMPS ; i++) {
        payload[i*3] = m._lamps[i][0];
        payload[i*3 + 1] = m._lamps[i][1];
        payload[i*3 + 2] = m._lamps[i][2];
    }

    // send packet
    h.send(m._src, m._dst, payload);
}
