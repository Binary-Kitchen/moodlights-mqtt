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

#include <fcntl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <system_error>

#include "crc8.h"
#include "hausbus.h"

#define throw_errno(x) throw std::system_error(errno, std::system_category(), x)

Hausbus::Hausbus(const std::string &device_filename,
                 const speed_t speed,
                 const Byte preamble) :
   _device_filename(device_filename),
   _preamble(preamble)
{
    struct termios tty;

    // Open device descriptor
    _fd_serial = open(_device_filename.c_str(), O_RDWR);
    if (_fd_serial == 0)
        throw_errno(_device_filename);

    // Set baudrate
    memset(&tty, 0, sizeof tty);
    if (tcgetattr (_fd_serial, &tty) != 0)
        throw_errno(_device_filename);

    if (cfsetospeed (&tty, speed))
        throw_errno(_device_filename);
    if (cfsetispeed(&tty, speed))
        throw_errno(_device_filename);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 5;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (_fd_serial, TCSANOW, &tty) != 0)
        throw_errno(_device_filename);

    _rs485_rx();
}

Hausbus::~Hausbus()
{
    if (_fd_serial == 0)
        return;

    if (close(_fd_serial) != 0)
        throw_errno(_device_filename);
}

void Hausbus::_rs485_rx() {
}

void Hausbus::_rs485_tx() {
}

Data Hausbus::create_packet(const Byte src, const Byte dst, const Data &payload) const
{
    if (payload.size() > HAUSBUS_MAX_PACKET_LENGTH)
        throw std::runtime_error("Exeeded maximum packet size");

    Data packet(sizeof(Header) + payload.size() + 1);

    Header header;
    header.preamble = _preamble;
    header.dst = dst;
    header.src = src;
    header.length = payload.size();

    // set header
    memcpy(packet.data(), &header, sizeof(Header));

    // copy payload
    std::copy(payload.begin(), payload.end(), packet.begin() + sizeof(Header));

    // calculate and place checksum
    packet.back() = CRC8::create(packet, packet.size() - 1);

    return packet;
}

void Hausbus::send_packet(const Data &packet)
{
    // set uart to output
    _rs485_tx();

    // write data
    const auto nbytes = write(_fd_serial, packet.data(), packet.size());
    if (nbytes != packet.size())
        throw_errno(_device_filename);

    // wait for data to be transmitted
    if (tcdrain(_fd_serial) != 0)
        throw_errno(_device_filename + " tcdrain");

    // set uart to input
    _rs485_rx();
}

void Hausbus::send(const Byte src, const Byte dst, const Data &payload)
{
    send_packet(create_packet(src, dst, payload));
}

void Hausbus::_write_sys(const char* file, const char* content)
{
    int fd = open(file, O_WRONLY);
    if (fd == 0)
        throw_errno(file);

    auto nbytes = write(fd, content, strlen(content));
    if (nbytes != strlen(content))
        throw_errno(file);

    if (close(fd) != 0)
        throw_errno(file);
}
