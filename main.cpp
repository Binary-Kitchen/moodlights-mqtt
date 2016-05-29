/*
 * moodlights-mqtt: A RS485 <-> mqtt bridge
 *
 * Copyright (c) Ralf Ramsauer, 2016
 *
 * Authors:
 *   Ralf Ramsauer <ralf@binary-kitchen.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 3.  See
 * the LICENSE file in the top-level directory.
 */

#include <unistd.h>

#include <iostream>
#include <memory>
#include <mutex>
#include <regex>

#include <mosquittopp.h>

#include "libhausbus/hausbus.h"
#include "libhausbus/moodlights.h"

using namespace std;

#define MOODLIGHT_DEVICE_IDENTIFIER 0x10
#define MY_DEVICE_IDENTIFIER 0xfe

// gobal access mutex to prevent race conditions
static std::mutex global_mutex;

// allow global access to hausbus and moodlights via unique_ptrs
static std::unique_ptr<Hausbus> hausbus = nullptr;
static std::unique_ptr<Moodlights> moodlights = nullptr;

class MQTT_Moodlights : public mosqpp::mosquittopp
{
public:
    MQTT_Moodlights(const std::string &id,
                    const std::string &host,
                    const std::string &moodlight_topic,
                    const std::string &shutdown_topic,
                    int port = 1883) :
        mosquittopp(id.c_str()),
        _id(id),
        _moodlight_topic(moodlight_topic),
        _shutdown_topic(shutdown_topic),
        _host(host),
        _port(port),
        _keepalive(60)
    {
        if (connect(_host.c_str(), _port, _keepalive) != MOSQ_ERR_SUCCESS)
            throw std::runtime_error("Mosquitto connection to " + _host + " failed");

        if (loop_start() != MOSQ_ERR_SUCCESS)
            throw std::runtime_error("Mosquitto loop_start failed");
    }

    virtual ~MQTT_Moodlights() {
        if (loop_stop() != MOSQ_ERR_SUCCESS)
            throw std::runtime_error("Mosquitto loop_stop failed");
    }

private:
    const std::string _id;
    const std::string _moodlight_topic;
    const std::string _shutdown_topic;
    const std::string _host;
    const int _port;
    const int _keepalive;

    const static std::regex _message_regex;
    const static std::regex _rgb_regex;

    const static Byte _fromHex(const std::string hex) {
        return (unsigned char)strtoul(hex.c_str(), nullptr, 16);
    }

    void on_connect(int rc) {
        if (rc == 0)
            cout << "Mosquitto connected: " << _host << endl;
        else
            cerr << "Unable to connect to server " << _host << " (" << rc << ")" << endl;

        if (subscribe(nullptr, _moodlight_topic.c_str()) != MOSQ_ERR_SUCCESS)
            cerr << "Subscription failed for topic " << _moodlight_topic << endl;

        if (subscribe(nullptr, _shutdown_topic.c_str()) != MOSQ_ERR_SUCCESS)
            cerr << "Subscription failed for topic " << _shutdown_topic << endl;
    }

    void on_disconnect(int rc) {
        cout << "Mosquitto disconnected with code " << rc << endl;
    }

    void on_message(const struct mosquitto_message* msg) {
        Moodlights::Color color;
        bool rand = false;
        std::string topic(msg->topic);
        std::string payload((const char*)msg->payload, msg->payloadlen);

        // check topic
        if (topic == _shutdown_topic) {
            cout << "Received shutdown message" << endl;

            // Lock critical section
            std::lock_guard<std::mutex> _m(global_mutex);
            moodlights->blank_all();
            *hausbus << *moodlights;
            return;
        }

        if (topic != _moodlight_topic)
            return;

        if (!std::regex_match(payload, _message_regex)) {
            cerr << "Got unknown message for topic " << topic << ": " << payload << endl;
            return;
        }

        std::smatch sm;
        std::regex_match(payload, sm, _message_regex);

        Byte lamp = _fromHex(sm[1]);

        if (sm[2] == "rand") {
            rand = true;
        } else {
            const std::string rgb_str = sm[2];
            std::smatch rgb_sm;

            if (!std::regex_match(rgb_str, rgb_sm, _rgb_regex)) {
                cerr << "Unknown RGB message" << endl;
                return;
            }

            color = {_fromHex(rgb_sm[1]),
                     _fromHex(rgb_sm[2]),
                     _fromHex(rgb_sm[3])};

        }

        std::lock_guard<std::mutex> _m(global_mutex);
        if (rand)
            if (lamp > 9)
                moodlights->rand_all();
            else
                moodlights->rand(lamp);
        else
            if (lamp > 9)
                moodlights->set_all(color);
            else
                moodlights->set(lamp, color);

        *hausbus << *moodlights;
    }
};

const std::regex MQTT_Moodlights::_message_regex("([0-9a-fA-F])#(.*)");
const std::regex MQTT_Moodlights::_rgb_regex("([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})");

int main(int argc, char **argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " device_name" << endl;
        return -1;
    }

    if (mosqpp::lib_init() != MOSQ_ERR_SUCCESS)
        throw std::runtime_error("Mosquitto initialisation failed");

    // Initialise Moodlights and Hausbus
    hausbus = std::unique_ptr<Hausbus>(new Hausbus(argv[1]));

    // source id: 0xFE
    // destination id: 0x10, moodlights device identifier
    moodlights = std::unique_ptr<Moodlights>(new Moodlights(MY_DEVICE_IDENTIFIER, MOODLIGHT_DEVICE_IDENTIFIER));

    // initialise lamps
    *hausbus << *moodlights;

    MQTT_Moodlights mq("MqttMoodlights",
                       "sushi.binary.kitchen",
                       "kitchen/moodlights",
                       "kitchen/shutdown");

    while (true) {
        auto res = mq.loop();
        if (res)
            mq.reconnect();

        // sleep for 10ms
        usleep(1e4);
    }

    if (mosqpp::lib_cleanup() != MOSQ_ERR_SUCCESS)
        throw std::runtime_error("Mosquitto cleanup failed");

    return 0;
}
