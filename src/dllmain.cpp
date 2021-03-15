#pragma warning(disable : 4996)

#define WIN32_LEAN_AND_MEAN
#ifdef DEBUG_MODE
#define DEBUG true
#else
#define DEBUG false
#endif 

#include <Windows.h>
#include <winsock2.h>
#include <process.h>
#include <Shlwapi.h>
#include <curl/curl.h>
#include <iostream>
#include <json.hpp>
#include <thread>
#include <imgui.h>

#include "PingMan.hpp"

#include "Host.hpp"
#include "HostingOptions.hpp"

#include "WebHandler.hpp"

#include "SokuAPI.hpp"

#include "Menu.hpp"

#include "Hostlist.hpp"

#include "ImGuiMan.hpp"

#include "DialogMan.hpp"

using namespace std;

std::wstring module_path;
LARGE_INTEGER timer_frequency;

bool warnedForName = false;
bool firstRun = false;

bool firstTime = true;
bool hosting = false;
bool inMenu = false;

mutex host_mutex;
deque<string> host_payloads;

void Update() {
	while (true) {
		if (inMenu == true)
			Hostlist::Update();
		this_thread::sleep_for(1ms);
	}
}

void HostLoop() {
	while (true) {
		host_mutex.lock();
		if (host_payloads.empty()) {
			host_mutex.unlock();
			this_thread::sleep_for(1ms);
			continue;
		}
		string payload = host_payloads.front();
		host_payloads.pop_front();
		host_mutex.unlock();
		string response = WebHandler::Put("http://delthas.fr:14762/games", payload);
		if (!response.empty()) {
			Status::Error(response);
		}
	}
}

//For loading resources
void Load() {
	Hostlist::Load();
	Menu::Load();
}

void Render() {
	CMenuConnect* menu = SokuAPI::GetCMenuConnect();
	if (menu != NULL) {
		if (firstTime) {
			Menu::OnMenuOpen();
			Hostlist::OnMenuOpen();

			HostingOptions::enabled = false;
			SokuAPI::InputBlock.Toggle(false);
			SokuAPI::InputWorkaround.Toggle(true);
			hosting = false;
			inMenu = true;

			firstTime = false;

			if (firstRun) {
				DialogMan::OpenDialog("Introduction");
			}
		}
		CDesignSprite* msgbox = SokuAPI::GetMsgBox();
		CInputManagerCluster* input = SokuAPI::GetInputManager();

		// Debug window
		if (DEBUG) {
			ImGui::Begin("DebugInfo", 0);
			ImGui::Text("DebugInfo:");
			ImGui::Text("   MsgBox addr: %08X", msgbox);
			ImGui::Text("   MsgBox active: %d", msgbox->active);
			ImGui::Text("   MsgBox X: %f", msgbox->x);
			ImGui::Text("   MsgBox Y: %f", msgbox->y);
			ImGui::Text("   Menu addr: %08X", menu);
			ImGui::Text("   Menu choice: %d", menu->Choice);
			ImGui::Text("   Menu subchoice: %d", menu->Subchoice);
			ImGui::Text("   Spectate: %d", menu->Spectate);
			ImGui::Text("   IPStr: %s", menu->IPString);
			ImGui::Text("   Port: %d", menu->Port);
			ImGui::Text("   Ip Id: %d", Hostlist::ip_id);
			ImGui::Text("   Refresh Count: %d", Hostlist::refreshCount);
			ImGui::End();
		}

		if ((menu->Choice == 1 || menu->Choice == 2) && menu->Subchoice == 255)
			return;

		Menu::Render();
		Hostlist::Render();
		HostingOptions::Render();
		DialogMan::Render();

		Menu::HandleInput();
		Hostlist::HandleInput();

		//This got surprisingly messy, basic logic is that if inputs are disabled 
		//and you press B or both of the submenus are inactive it exits out, this is
		//mostly meant so you can't end up in situations where the inputs are fully blocked
		//With the extra hosting options check it's looking extra messy tho.
		bool dialogsActive = DialogMan::AnyActive();
		if(dialogsActive)
			SokuAPI::InputBlock.Toggle(true);

		if (SokuAPI::InputBlock.Check() && (input->P1.B == 1 || (!HostingOptions::enabled && !Hostlist::active && !dialogsActive))) {
			if (!HostingOptions::enabled && !DialogMan::ModalActive()) {
				Hostlist::active = false;
				HostingOptions::enabled = false;
				SokuAPI::InputBlock.Toggle(false);
				SokuAPI::SfxPlay(SFX_BACK);
				input->P1.B = 10;
			}
		}
	}
	else {
		firstTime = true;
		inMenu = false;
	}
}

thread *updateThread;
thread *hostThread;
thread *hookThread;

void Init(void *unused) {
	QueryPerformanceFrequency(&timer_frequency);
	
	SokuAPI::Init();
	
	WebHandler::Init();
	atexit(WebHandler::Cleanup);
	
	PingMan::Init();
	atexit(PingMan::Cleanup);
	
	HostingOptions::LoadConfig();
	
	Menu::Init();
	Hostlist::Init();
	
	updateThread = new thread(Update);
	hostThread = new thread(HostLoop);
	
	if (DEBUG) {
		AllocConsole();
		SetConsoleTitleA("InGameHostlist Debug");
		freopen("CONOUT$", "w", stdout);
	}

	DialogMan::AddDialog("Introduction", []() {
		ImGui::Text(
			"Welcome to InGame-Hostlist,\n"
			"A mod that lets you use a hostlist from inside the game.\n\n"
			"Usage instructions:\n"
			"Before hosting/joining please go into the options menu,\n"
			"to set your port and spectate options, as the game will\n"
			"no longer propmpt you for them before each host.\n"
			"You will also find settings for your host message and\n"
			"whether you want your games to appear on the hostlist or not.\n\n"
			"After that you can enter the hostlist via the 'Join' button,\n"
			"or just host by pressing 'Host'.\n"
			"Note: For ease of viewing you can switch between the Playing\n"
			"and Waiting tabs from anywhere in the menu.\n\n"
			"Also please remember to change your profile name before playing\n"
			"online, have fun!");
	});
	
	Menu::AddItem("Host", []() {
		if (hosting == true) {
			Status::Normal("Host aborted.");
			SokuAPI::ClearMenu();
		}
		else {
			if (!warnedForName && SokuAPI::GetProfileName(1) == "edit me") {
				Status::Error("Consider changing your profile name before hosting, or press host again.");
				SokuAPI::ClearMenu();

				warnedForName = true;
				return;
			}

			Status::Normal("Hosting...", Status::forever);
			SokuAPI::SetupHost(HostingOptions::port, HostingOptions::spectate);
			
			if (HostingOptions::publicHost) {
				JSON data = {
					"profile_name",
					SokuAPI::GetProfileName(1),
					"message",
					HostingOptions::message,
					"port",
					HostingOptions::port,
				};
				host_mutex.lock();
				host_payloads.emplace_back(data.dump());
				host_mutex.unlock();
			}
		}
		hosting = !hosting;
	});
	
	Menu::AddItem("Join", []() {
		Hostlist::active = true;
		// Prevents the hostlist from instantly registering the input
		// Very ugly, but eh...
		SokuAPI::GetInputManager()->P1.A = 10;
		SokuAPI::InputBlock.Toggle(true);
		SokuAPI::ClearMenu();
	});
	
	Menu::AddItem("Refresh", []() {
		Hostlist::oldTime = 0;
		SokuAPI::ClearMenu();
	});
	
	Menu::AddItem("Options", []() {
		HostingOptions::enabled = !HostingOptions::enabled;
		SokuAPI::InputBlock.Toggle(true);
		SokuAPI::ClearMenu();
	});

	DialogMan::AddDialog("Clipboard", []() {
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Do you want to join or spectate this host?");

		ImGui::SetCursorPosX(100);
		if (ImGui::Button("Join")) {
			ImGui::CloseCurrentPopup(); 
			SokuAPI::JoinHost(NULL, 0);
			SokuAPI::InputBlock.Toggle(false);
			SokuAPI::SfxPlay(SFX_SELECT);
		}
		ImGui::SameLine();
		if (ImGui::Button("Spectate")) {
			ImGui::CloseCurrentPopup();
			SokuAPI::JoinHost(NULL, 0, true);
			SokuAPI::InputBlock.Toggle(false);
			SokuAPI::SfxPlay(SFX_SELECT);
		}
	});
	
	Menu::AddItem("Join from clipboard", []() {
		Status::Normal("Joining...", Status::forever);
		DialogMan::OpenDialog("Clipboard");
		SokuAPI::ClearMenu();
	});
	
	Menu::AddItem("Profile select");
	
	Menu::AddEventHandler(Menu::Event::AlreadyPlaying, []() {
		Status::Normal("Match in progress, spectating...");
		SokuAPI::JoinHost(NULL, 0, true);
	});
	
	Menu::AddEventHandler(Menu::Event::ConnectionFailed, []() {
		Hostlist::joining = false;
		Status::Error("Failed to connect.");
		SokuAPI::SfxPlay(SFX_BACK);
		SokuAPI::ClearMenu();
	});
	
	hookThread = new thread(ImGuiMan::HookThread, Load, Render);
}

extern "C" __declspec(dllexport) bool CheckVersion(const BYTE hash[16]) {
       return true;
}

extern "C" __declspec(dllexport) bool Initialize(HMODULE hMyModule, HMODULE hParentModule) {
	//Init path variables
	wchar_t wd[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, wd);
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(hMyModule, path, MAX_PATH);
	PathRemoveFileSpecW(path);
	wchar_t relative[MAX_PATH];
	if (PathRelativePathToW(relative, wd, FILE_ATTRIBUTE_DIRECTORY, path, FILE_ATTRIBUTE_DIRECTORY)) {
		module_path = wstring(relative);
	} else {
		module_path = wstring(path);
	}

	DWORD dataSize = 1;
	DWORD data = 0;
	if (ERROR_SUCCESS == RegGetValueA(HKEY_CURRENT_USER, "Software", "SokuIGHostlistFirstRun", RRF_RT_REG_BINARY, NULL, &data, &dataSize)) {
		firstRun = data;
	}
	else {
		firstRun = true;
		HKEY hKey;
		BYTE value = 0;
		if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
			RegSetValueEx(hKey, L"SokuIGHostlistFirstRun", 0, REG_BINARY, &value, 1);
			RegCloseKey(hKey);
		}
	}
	
	_beginthread(Init, 0, NULL);
	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved) {
	return TRUE;
}
