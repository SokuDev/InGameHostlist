#pragma once
#include <string>
using namespace std;

extern LARGE_INTEGER timer_frequency;

#define CHARS_PER_LINE 48

// Responsible for handling Status message on the hostlist.
namespace Status {
	enum Type { Type_Normal, Type_Error, Type_Count };
	ImVec4 Colors[Type_Count];

	const unsigned long long defaultLife = 5000;
	// Not really forever, but close enough
	const unsigned long long forever = -1;

	Type type;
	string message;
	unsigned long long timePosted;
	unsigned long long lifetime;
	int lines;

	void Init() {
		Colors[Type_Normal] = (ImVec4)ImColor(255, 255, 255, 255);
		Colors[Type_Error] = (ImVec4)ImColor(255, 0, 0, 255);

		lines = 0;
	}

	void OnMenuOpen() {
		message = "";
		lifetime = 0;
	}

	void Set(Type t, const string &msg, unsigned long long life = defaultLife) {
		type = t;
		message = msg;
		lifetime = life;
		lines = (msg.length() / CHARS_PER_LINE) + 1;

		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);
		timePosted = counter.QuadPart * 1000 / timer_frequency.QuadPart;
	}

	// Maybe print is a better name?
	void Normal(const string msg, unsigned long long life = defaultLife) {
		Set(Type_Normal, msg, life);
	}

	void Error(const string msg, unsigned long long life = defaultLife) {
		Set(Type_Error, msg, life);
	}

	void Update(unsigned long long time) {
		if (time - timePosted > lifetime) {
			message = "";
			lines = 0;
			lifetime = forever;
		}
	}

	void Render() {
		for (int i = 0; i < lines; i++) {
			string line;
			if (i == lines - 1)
				line = message.substr(i * CHARS_PER_LINE) + '\n';
			else
				line = message.substr(i * CHARS_PER_LINE, CHARS_PER_LINE) + '\n';

			ImGui::TextColored(Colors[type], line.c_str());
		}
	}
} 