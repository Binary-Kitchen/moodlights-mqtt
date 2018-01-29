/*
 * libhausbus: A RS485 Hausbus library
 *
 * Copyright (c) Ralf Ramsauer, 2016
 *
 * Authors:
 *   Ralf Ramsauer <ralf@binary-kitchen.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 3.  See
 * the LICENSE file in the top-level directory.
 */

#pragma once

#include <array>
#include <experimental/optional>
#include <regex>

#include "hausbus.h"
#include "data.h"

#define MOODLIGHTS_LAMPS 10
#define MOODLIGHT_DEFAULT_IDENTIFIER 0x10

class Moodlights {
public:
    // Format: R:G:B
    using Color = std::array<Byte, 3>;

    Moodlights(const Byte src, const Byte dst = MOODLIGHT_DEFAULT_IDENTIFIER);
    ~Moodlights();

    static Color rand_color();
    static std::experimental::optional<Color> parse_color(const std::string &str);
    static std::string color_to_string(const Color &color);

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

    const static Byte _gamma_correction[256];
    const static std::regex _color_regex;
};
