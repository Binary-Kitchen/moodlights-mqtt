/*
 * moodlights-mqtt: A RS485 <-> mqtt bridge
 *
 * Copyright (c) Ralf Ramsauer, 2016-2018
 *
 * Authors:
 *   Ralf Ramsauer <ralf@binary-kitchen.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 3.  See
 * the LICENSE file in the top-level directory.
 */

#define DEBUG 1

#include <termios.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <mutex>
#include <regex>

#include <boost/tokenizer.hpp>
#include <mosquittopp.h>

#include "moodlights.h"

using namespace std;

// gobal access mutex to prevent race conditions
static std::mutex global_mutex;

class MQTT_Moodlights : public mosqpp::mosquittopp
{
public:
	MQTT_Moodlights(const int fd,
			const std::string &id,
	                const std::string &host,
	                const std::string &moodlight_topic,
	                const std::string &shutdown_topic,
	                const std::string &status_topic,
	                int port = 1883) :
		mosquittopp(id.c_str()),
		_moodlights(fd),
		_id(id),
		_moodlight_topic(moodlight_topic + "/"),
		_shutdown_topic(shutdown_topic),
		_status_topic(status_topic),
		_host(host),
		_port(port),
		_keepalive(60),
		_topic_regex(_moodlight_topic + "(.*)")
	{
		if (connect(_host.c_str(), _port, _keepalive) != MOSQ_ERR_SUCCESS)
			throw std::runtime_error("Mosquitto connection to " + _host + " failed");

		if (loop_start() != MOSQ_ERR_SUCCESS)
			throw std::runtime_error("Mosquitto loop_start failed");
	}

	virtual ~MQTT_Moodlights()
	{
		_moodlights.blank_all();
		loop_stop();
	}

private:
	const std::string _id;
	const std::string _moodlight_topic;
	const std::string _shutdown_topic;
	const std::string _status_topic;
	const static std::string _get_subtopic;
	const static std::string _rand_identifier;
	const std::string _host;
	const int _port;
	const int _keepalive;
	Moodlights _moodlights;


	const std::regex _topic_regex;
	const static std::regex _lamp_regex;

	void on_connect(int rc)
	{
		if (rc == 0)
			cout << "Mosquitto connected: " << _host << endl;
		else
			cerr << "Unable to connect to server " << _host << " (" << rc << ")" << endl;

		if (subscribe(nullptr, (_moodlight_topic + "#").c_str()) != MOSQ_ERR_SUCCESS)
			cerr << "Subscription failed for topic " << _moodlight_topic << endl;

		if (subscribe(nullptr, _shutdown_topic.c_str()) != MOSQ_ERR_SUCCESS)
			cerr << "Subscription failed for topic " << _shutdown_topic << endl;

		publish_status();
	}

	void on_disconnect(int rc)
	{
		cout << "Mosquitto disconnected with code " << rc << endl;
	}

	void on_message(const struct mosquitto_message* msg)
	{
		// lock for critical sections
		std::unique_lock<std::mutex> lock(global_mutex, std::defer_lock);

		Moodlights::Color color;
		bool rand = false;
		std::string topic(msg->topic);
		std::string payload((const char*)msg->payload, msg->payloadlen);

		unsigned char lamp = 0;
		std::smatch sm;

#ifdef DEBUG
		cerr << "DEBUG: topic: '" << topic << "' payload: '" << payload << "'" << endl;
#endif

		// check if shutdown topic
		if (topic == _shutdown_topic) {
			cout << "Received shutdown message" << endl;
			lock.lock();
			_moodlights.blank_all();
			goto update_unlock;
		}

		// ignore status topic
		if (topic == _status_topic)
			return;

		// check if moodlight topic
		if (!std::regex_match(topic, sm, _topic_regex)) {
			cerr << "Unknown topic: " << topic << endl;
			return;
		}
		topic = sm[1];

		if (topic == "set") {
			const boost::char_separator<char> sep(" ");
			const boost::tokenizer<boost::char_separator<char>> tokens(payload, sep);
			int i = 0;
			lock.lock();
			for (const auto &token: tokens) {
				const std::experimental::optional<Moodlights::Color> tmp_color = Moodlights::parse_color(token);
				if (tmp_color) {
					_moodlights.set(i++, *tmp_color);
				} else if (token == _rand_identifier) {
					_moodlights.rand(i++);
				} else {
					cerr << "Unable to parse part of payload" << endl;
					goto update_unlock;
				}
				if (i == 10)
					break;
			}
			goto update_unlock;
		} else if (topic == _get_subtopic) {
			goto status_out;
		} else if (!std::regex_match(topic, sm, _lamp_regex)) {
			cerr << "Unknown subtopic: " << topic << endl;
			return;
		}

		lamp = (unsigned char)::strtoul(((std::string)sm[1]).c_str(), nullptr, 16);

		if (payload == _rand_identifier) {
			rand = true;
		} else {
			auto tmp_color = Moodlights::parse_color(payload);
			if (tmp_color)
				color = *tmp_color;
			else {
				cerr << "Unable to parse color" << endl;
				return;
			}
		}

		lock.lock();
		if (rand)
			if (lamp < MOODLIGHTS_LAMPS) {
				_moodlights.rand(lamp);
			} else {
				_moodlights.rand_all();
			}
		else if (lamp < MOODLIGHTS_LAMPS) {
			_moodlights.set(lamp, color);
		} else {
			_moodlights.set_all(color);
		}

update_unlock:
		lock.unlock();
		_moodlights.update();
status_out:
		publish_status();
	}

	void publish_status()
	{
		std::string status;
		int err;
		for (int i = 0 ; i < 10 ; i++) {
			status += Moodlights::color_to_string(_moodlights.get(i));
			if (i != 9) status += ' ';
		}
		err = publish(nullptr, _status_topic.c_str(), status.size()+1, status.c_str(), 0, false);
		if (err != MOSQ_ERR_SUCCESS) {
			cerr << "Mosquitto publish error: " << mosquitto_strerror(err) << endl;
		}
	}
};

const std::string MQTT_Moodlights::_rand_identifier("rand");
const std::string MQTT_Moodlights::_get_subtopic("get");
const std::regex MQTT_Moodlights::_lamp_regex("set/([0-9a-fA-F])");

int main(int argc, char **argv)
{
	speed_t speed = B115200;
        struct termios tty;
	int err, fd;

	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " device_name" << endl;
		return -1;
	}

	err = mosqpp::lib_init();
	if (err != MOSQ_ERR_SUCCESS) {
		cerr << "Mosquitto initialisation failed: " << mosquitto_strerror(err) << endl;
		return -1;
	}

        fd = open(argv[1], O_RDWR);
        if (fd == -1) {
		perror(("opening " + string(argv[1])).c_str());
		goto mosq_out;
	}

        memset(&tty, 0, sizeof tty);
        err = tcgetattr (fd, &tty);
	if (err) {
		perror("tcgetattr");
		goto close_out;
	}

        err = cfsetospeed (&tty, speed);
	if (err) {
		perror("cfsetospeed");
		goto close_out;
	}

        err = cfsetispeed(&tty, speed);
	if (err) {
		perror("cfsetispeed");
		goto close_out;
	}

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

        err = tcsetattr (fd, TCSANOW, &tty);
	if (err) {
		perror("tcsetattr: ");
		goto close_out;
	}

	srand(time(nullptr));

	for(;;) {
		try {
			MQTT_Moodlights mq(fd,
					   "MqttMoodlights",
			                   "172.23.4.6",
			                   "kitchen/moodlights",
			                   "kitchen/shutdown",
			                   "kitchen/moodlights/status");

			while (true) {
				err = mq.loop_forever();
				if (err != MOSQ_ERR_SUCCESS) {
					cerr << "Mosquitto runtime error: " << mosquitto_strerror(err) << endl;
					mq.reconnect();
				}
			}
		} catch (const std::exception &ex) {
			cerr << argv[0] << " failed: " << ex.what() << endl;
			sleep(10);
			cerr << "Retrying..." << endl;
		}
	}

close_out:
	close(fd);
mosq_out:
	mosqpp::lib_cleanup();
	return err;
}
