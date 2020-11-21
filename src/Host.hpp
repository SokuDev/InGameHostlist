#pragma once
#include <string>
#include <json.hpp>
using namespace std;
using namespace json;
using namespace PingMan;

class Host {
    string _ipstr;
    size_t _sep;

public:
    string name;
    string message;
    string ip;
    short port;
    bool spectateable;

    bool playing;
    string opponentName;

    PingAsync ping;

    Host(const string &name, const string &msg, const string &ip, short port) : 
        name(name), message(msg), ip(ip), port(port), spectateable(false), playing(false), ping(ip.c_str(), port) {
    }

    Host(JSON &jHost) : 
        name(jHost["host_name"].ToString()),
        message(jHost["message"].ToString()),
        spectateable(jHost["spectateable"].ToBool()),
        playing(jHost["started"].ToBool()), 
        opponentName(jHost["client_name"].ToString()),
        _ipstr(jHost["ip"].ToString()),
        _sep(_ipstr.find(':')),
        ip(_ipstr.substr(0, _sep)),
        port(stoi(_ipstr.substr(_sep + 1, _ipstr.length()), nullptr)),
        ping(PingAsync(ip.c_str(), port)) {
    }
};
