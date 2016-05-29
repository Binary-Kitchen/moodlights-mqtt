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
                 const int speed,
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
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (_fd_serial, &tty) != 0)
        throw_errno(_device_filename);

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

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

    // Configure RS485 direction selector
    try {
        _write_sys("/sys/class/gpio/export", "18\n");
    }
    catch (const std::system_error &err) {
        // EBUSY will be received if gpio is already exported
        if (err.code().value() != EBUSY)
            throw err;
    }
    _write_sys("/sys/class/gpio/gpio18/direction", "out\n");
    _fd_rs485_direction = open("/sys/class/gpio/gpio18/value", O_WRONLY);
    _rs485_rx();
}

Hausbus::~Hausbus()
{
    _write_sys("/sys/class/gpio/unexport", "18\n");

    if (_fd_rs485_direction == 0)
        goto release_serial;

    if (close(_fd_rs485_direction) != 0)
        throw_errno("RS485 Direction GPIO");

release_serial:
    // Only close device if it is opened correctly
    if (_fd_serial == 0)
        return;

    // Check return value of close()
    if (close(_fd_serial) != 0)
        throw_errno(_device_filename);
}

void Hausbus::_rs485_rx() {
    if (write(_fd_rs485_direction, "0\n", 2) != 2)
        throw_errno(_device_filename);
}

void Hausbus::_rs485_tx() {
    if (write(_fd_rs485_direction, "1\n", 2) != 2)
        throw_errno(_device_filename);
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
