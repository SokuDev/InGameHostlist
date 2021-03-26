#pragma once
#include <json.hpp>
#include <string>
#include <ctime>
using namespace std;
using namespace json;

struct Host {
	string name;
	string message;
	string ip;
	short port;
	bool spectateable;
	bool playing;
	string opponentName;
	time_t startTime;

	long netIp;
	short netPort;

	Host(const string &name, const string &msg, const string &ip, short port): name(name), message(msg), ip(ip), port(port), spectateable(false), playing(false) {
		netIp = inet_addr(ip.c_str());
		netPort = htons(port);
	}

	Host(JSON &jHost):
		name(jHost["host_name"].ToString()), message(jHost["message"].ToString()), spectateable(jHost["spectateable"].ToBool()), playing(jHost["started"].ToBool()),
		opponentName(jHost["client_name"].ToString()), startTime(jHost["start"].ToInt()) {
		string _ipstr = jHost["ip"].ToString();
		size_t _sep = _ipstr.find(':');
		ip = _ipstr.substr(0, _sep);
		port = stoi(_ipstr.substr(_sep + 1));
		netIp = inet_addr(ip.c_str());
		netPort = htons(port);
	}

	bool Compare(Host *other_host) {
		return this->startTime == other_host->startTime && 
			this->name == other_host->name;
	}

	bool Compare(JSON &jHost) {
		return this->startTime == jHost["start"].ToInt() &&
			this->name == jHost["host_name"].ToString();
	}
};
