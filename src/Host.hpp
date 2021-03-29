#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <ctime>
using namespace std;
using json = nlohmann::json;

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

	Host(json &jHost):
		name(jHost["host_name"].get<string>()), message(jHost["message"].get<string>()), spectateable(jHost["spectatable"].get<bool>()), playing(jHost["started"].get<bool>()),
		opponentName(jHost["client_name"].get<string>()), startTime(jHost["start"].get<int>()) {
		string _ipstr = jHost["ip"].get<string>();
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

	bool Compare(json &jHost) {
		return this->startTime == jHost["start"].get<int>() &&
			this->name == jHost["host_name"].get<string>();
	}
};
