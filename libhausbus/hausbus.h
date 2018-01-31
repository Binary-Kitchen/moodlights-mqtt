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

#include <string>

#include <termios.h>

#include "data.h"

#define HAUSBUS_PREAMBLE 0x40
#define HAUSBUS_MAX_PACKET_LENGTH 64
#define HAUSBUS_BAUDRATE B115200

class Hausbus
{

public:

	Hausbus(const std::string &device_filename,
	        const speed_t speed = HAUSBUS_BAUDRATE,
	        const Byte preamble = HAUSBUS_PREAMBLE);
	~Hausbus();

	Data create_packet(const Byte src, const Byte dst, const Data &payload) const;
	void send_packet(const Data &packet);
	void send(const Byte src, const Byte dst, const Data &payload);

	// TBD: Receive hausbus packets

private:

	static void _write_sys(const char* file, const char* content);

	// header structure according to Hausbus protocol
	struct Header {
		Byte preamble;
		Byte src;
		Byte dst;
		Byte length;
	} __attribute__((packed));

	const Byte _preamble;
	const std::string _device_filename;

	int _fd_serial;
};
