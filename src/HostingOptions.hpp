#pragma once
#include <Sokulib.h>
#include <json.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;
using namespace json;

namespace HostingOptions
{
	bool enabled = false;
	int port = 10800;
	bool spectate = true;
	bool publicHost = true;
	char message[256] = { 0 };

	void SaveConfig() {
		ofstream ofs("Modules/HostBotInt/config.json");
		JSON config = {
			"HostingOptions", {
				"port", port,
				"spectate", spectate,
				"public", publicHost,
				"message", message
			}
		};
		ofs << config;
	}

	void LoadConfig() {
		ifstream ifs("Modules/Soku-Hostlist/config.json");
		if (ifs.good()) {
			std::ostringstream ss;
			ss << ifs.rdbuf();
			JSON config = JSON::Load(ss.str());
			port = config["HostingOptions"]["port"].ToInt();
			spectate = config["HostingOptions"]["spectate"].ToBool();
			publicHost = config["HostingOptions"]["public"].ToBool();
			strcpy_s(message, 256, config["HostingOptions"]["message"].ToString().c_str());
		}
		else SaveConfig();
	}

	void Render() {
		if (enabled) {
			ImGui::SetNextWindowSize(ImVec2(150, 190));
			ImGui::SetNextWindowPos(ImVec2(231, 158));
			ImGui::Begin("Hosting Options##HostingOptions", &enabled, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
			ImGui::SetCursorPosY(30);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Port"); ImGui::SameLine(); ImGui::InputInt("##port", &port, 0, 0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Spectate"); ImGui::SameLine(); ImGui::Checkbox("##spec", &spectate);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Public"); ImGui::SameLine(); ImGui::Checkbox("##public", &publicHost);
			ImGui::Text("Host message:");
			ImGui::PushItemWidth(-1);
			ImGui::InputText("##msg", message, 255);
			if (ImGui::Button("Save")) {
				SaveConfig();
				enabled = false;
			}
			ImGui::PopItemWidth();
			ImGui::End();
		}
	}
};
