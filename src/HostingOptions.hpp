#pragma once
#include <fstream>
#include <iostream>
#include <sstream>
#include "DialogMan.hpp"
using namespace std;

extern std::wstring module_path;

namespace HostingOptions {
	int port;
	bool spectate = true;
	bool publicHost = true;
	bool showMessagePrompt = true;
	bool playSfxOnNewHost = true;
	char message[256] = {0};

	Dialog *dialog;

	void SaveConfig() {
		std::wstring config_path = module_path;
		config_path.append(L"\\InGameHostlist.ini");

		WritePrivateProfileStringW(L"InGameHostlist", L"Port", &std::to_wstring(port)[0], &config_path[0]);
		WritePrivateProfileStringW(L"InGameHostlist", L"Spectatable", spectate ? L"1" : L"0", &config_path[0]);
		WritePrivateProfileStringW(L"InGameHostlist", L"Hostlist", publicHost ? L"1" : L"0", &config_path[0]);
		WritePrivateProfileStringW(L"InGameHostlist", L"PlaySfxOnNewHost", playSfxOnNewHost ? L"1" : L"0", &config_path[0]);
		WritePrivateProfileStringW(L"InGameHostlist", L"ShowMessagePrompt", showMessagePrompt ? L"1" : L"0", &config_path[0]);
		WritePrivateProfileStringA("InGameHostlist", "Message", message, &std::string(config_path.begin(), config_path.end())[0]);
	}

	void LoadConfig() {
		std::wstring config_path = module_path;
		config_path.append(L"\\InGameHostlist.ini");

		port = GetPrivateProfileIntW(L"InGameHostlist", L"Port", 10800, &config_path[0]);
		spectate = GetPrivateProfileIntW(L"InGameHostlist", L"Spectatable", 1, &config_path[0]) != 0;
		publicHost = GetPrivateProfileIntW(L"InGameHostlist", L"Hostlist", 1, &config_path[0]) != 0;
		playSfxOnNewHost = GetPrivateProfileIntW(L"InGameHostlist", L"PlaySfxOnNewHost", 1, &config_path[0]) != 0;
		showMessagePrompt = GetPrivateProfileIntW(L"InGameHostlist", L"ShowMessagePrompt", 1, &config_path[0]) != 0;
		GetPrivateProfileStringA("InGameHostlist", "Message", "", message, sizeof(message), &std::string(config_path.begin(), config_path.end())[0]);
	}

	void Init() {
		dialog = DialogMan::AddDialog("Hosting Options", ImGuiWindowFlags_NoResize, []() {
			ImGui::SetCursorPosY(30);

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Port");
			ImGui::SameLine();
			ImGui::PushItemWidth(50);
			ImGui::InputInt("##port", &port, 0, 0);
			ImGui::PopItemWidth();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Spectatable?    ");
			ImGui::SameLine();
			ImGui::Checkbox("##spec", &spectate);

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Post to bot?    ");
			ImGui::SameLine();
			ImGui::Checkbox("##public", &publicHost);

			ImGui::AlignTextToFramePadding();
			ImGui::Text("SFX on new host?");
			ImGui::SameLine();
			ImGui::Checkbox("##sfx", &playSfxOnNewHost);

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Prompt for msg? ");
			ImGui::SameLine();
			ImGui::Checkbox("##static", &showMessagePrompt);

			ImGui::Text("Host message:");
			ImGui::PushItemWidth(130);
			ImGui::InputText("##msg", message, 255);
			ImGui::PopItemWidth();
			
			if (ImGui::Button("Save")) {
				SaveConfig();
				return false;
			}
			return true;
		});
	}
};
