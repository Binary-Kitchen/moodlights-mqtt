/*
 * libhausbus: A RS485 Hausbus library
 *
 * Copyright (c) Ralf Ramsauer, 2016-2018
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

#define MOODLIGHTS_LAMPS 10
#define MOODLIGHT_DEFAULT_IDENTIFIER 0x10

class Moodlights
{
public:
	// Format: R:G:B
	using Color = std::array<unsigned char, 3>;

	Moodlights(const int fd);
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

	void update() const;

private:
	std::array<Color, MOODLIGHTS_LAMPS> _lamps;

	const int _fd;
	const static unsigned char _gamma_correction[256];
	const static std::regex _color_regex;
};
