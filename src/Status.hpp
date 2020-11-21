#pragma once
#include <SokuLib.h>
#include <string>
using namespace std;

//Responsible for handling Status message on the hostlist.
namespace Status {
	enum Type { 
		Type_Normal, 
		Type_Error, 
		Type_Count 
	};
	ImVec4 Colors[Type_Count];

	const unsigned long defaultLife = 5000;
	//Not really forever, but close enough
	const unsigned long forever = -1;

	Type type;
	string message;
	unsigned long timePosted;
	unsigned long lifetime;

	void Init() {
		Colors[Type_Normal] = (ImVec4)ImColor(255, 255, 255, 255);
		Colors[Type_Error] = (ImVec4)ImColor(255, 0, 0, 255);
	}

	void OnMenuOpen() {
		message = "";
		lifetime = 0;
	}

	void Set(Type t, const string& msg, unsigned long life = defaultLife) {
		type = t;
		message = msg;
		lifetime = life;
		timePosted = GetTickCount();
	}

	//Maybe print is a better name?
	void Normal(const string msg, unsigned long life = defaultLife) {
		Set(Type_Normal, msg, life);
	}

	void Error(const string msg, unsigned long life = defaultLife) {
		Set(Type_Error, msg, life);
	}

	void Update(unsigned long time) {
		if (time - timePosted > lifetime) {
			message = "";
			lifetime = forever;
		}
	}

	void Render() {
		ImGui::TextColored(Colors[type], message.c_str());
	}
}