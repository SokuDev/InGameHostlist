#pragma warning(disable : 4996)

#define WIN32_LEAN_AND_MEAN
#ifdef DEBUG_MODE
#define DEBUG true
#else
#define DEBUG false
#endif 

#define VERSION "v1.3.2"

#include <Windows.h>
#include <winsock2.h>
#include <process.h>
#include <Shlwapi.h>
#include <iostream>
#include <stdio.h>
#include <deque>
#include <thread>

#include <imgui.h>
#include <nlohmann/json.hpp>

#include "PingMan.hpp"
#include "WebMan.hpp"
#include "SokuMan.hpp"
#include "ImGuiMan.hpp"
#include "DialogMan.hpp"

#include "HostingOptions.hpp"
#include "Menu.hpp"
#include "Hostlist.hpp"
#include "Host.hpp"
#include "Status.hpp"

using namespace std;
using json = nlohmann::json;

std::wstring module_path;
LARGE_INTEGER timer_frequency;

bool warnedForName = false;
bool firstRun = false;
bool pingmanInit = false;

bool firstTime = true;
bool hosting = false;
bool inMenu = false;

mutex host_mutex;
deque<string> host_payloads;

Dialog *IntroDialog;
Dialog *HostMessageDialog;
Dialog *ClipboardDialog;

// Independent update thread.
void Update() {
	while (true) {
		if (inMenu) {
			Hostlist::Update();
		} else {
			gotFirstHostlist = false;
		}
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
		string response = WebMan::Put("https://konni.delthas.fr/games", payload);
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
	CMenuConnect* menu = SokuMan::GetCMenuConnect();
	if (menu != NULL) {
		if (firstTime) {
			if(!pingmanInit) {
				pingmanInit = true;
				PingMan::Init();
				atexit(PingMan::Cleanup);
			}
			Menu::OnMenuOpen();
			Hostlist::OnMenuOpen();

			DialogMan::DisableAll();
			SokuMan::InputBlock.Toggle(false);
			SokuMan::InputWorkaround.Toggle(true);
			hosting = false;
			inMenu = true;

			firstTime = false;

			if (firstRun) {
				firstRun = false;
				IntroDialog->Active = true;
			}
		}

		// Debug window
		if (DEBUG) {
			ImGui::Begin("DebugInfo", 0);
			ImGui::Text("DebugInfo:");
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
		DialogMan::Render();

		ImGui::SetNextWindowPos(ImVec2(SokuMan::WindowWidth - 135, -3));
		if(ImGui::Begin("##version", 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::Text("IGHostlist %s", VERSION);
			ImGui::End();
		}
	}
	else {
		firstTime = true;
		inMenu = false;
	}
}

// Hooked Update thread.
int __fastcall CMenuConnect_Update(CMenuConnect* menu) {
	if (DialogMan::AnyActive()) {
		SokuMan::InputBlock.Toggle(true);
	}
	else {
		// Very hackish but I need to redo this whole thing at some point.
		bool wasJoining = Hostlist::joining;

		if(!Menu::HandleInput())
			Hostlist::HandleInput();
		CInputManagerCluster *input = SokuMan::GetInputManager();

		if (SokuMan::InputBlock.Check() && (input->P1.B == 1 || !Hostlist::active)) {
			if (!ImGui::GetIO().WantTextInput && !wasJoining) {
				Hostlist::active = false;
				SokuMan::InputBlock.Toggle(false);
				SokuMan::SfxPlay(SokuSFX::Back);
			}
		}
	}
	return SokuMan::OldCMenuConnectUpdate(menu);
}

void Exit() {
	SokuMan::Init();
	WebMan::Cleanup();
}

thread *updateThread;
thread *hostThread;
thread *hookThread;

void Init(void *unused) {
	QueryPerformanceFrequency(&timer_frequency);

	SokuMan::Init();
	WebMan::Init();

	HostingOptions::Init();
	Menu::Init();
	Hostlist::Init();

	atexit(Exit);
	
	updateThread = new thread(Update);
	hostThread = new thread(HostLoop);
	SokuMan::OnCMenuConnectUpdate(CMenuConnect_Update);
	
	if (DEBUG) {
		AllocConsole();
		SetConsoleTitleA("InGameHostlist Debug");
		freopen("CONOUT$", "w", stdout);
	}

	IntroDialog = DialogMan::AddDialog("Introduction", []() {
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
			"online, have fun!\n\n"
			"Press A/B to continue.\n");

		if (DialogMan::IsActivatePressed()) {
			SokuMan::InputBlock.Toggle(false);
			SokuMan::SfxPlay(SokuSFX::Back);
			return false;
		}
		return true;
	});

	HostMessageDialog = DialogMan::AddDialog("Please enter your host message", []() {
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Host Message:");
		ImGui::SameLine();
		bool guiConfirm = ImGui::InputText("", HostingOptions::message, 256, ImGuiInputTextFlags_EnterReturnsTrue);
		guiConfirm = guiConfirm || ImGui::Button("Confirm");
		if (guiConfirm || (DialogMan::IsActivatePressed() && !ImGui::IsAnyItemFocused() && !ImGui::GetIO().WantTextInput)) {
			Status::Normal("Hosting...", Status::forever);
			SokuMan::SetupHost(HostingOptions::port, HostingOptions::spectate);
			HostingOptions::SaveConfig();

			if (HostingOptions::publicHost) {
				json data = {
					{"profile_name", SokuMan::GetProfileName(1)},
					{"message", HostingOptions::message},
					{"port", HostingOptions::port}
				};
				host_mutex.lock();
				host_payloads.emplace_back(data.dump());
				host_mutex.unlock();
			}

			SokuMan::InputBlock.Toggle(false);
			SokuMan::SfxPlay(SokuSFX::Select);

			hosting = true;
			return false;
		}
		return true;
	});
	
	Menu::AddItem("Host", []() {
		if (hosting == true) {
			Status::Normal("Host aborted.");
			SokuMan::ClearMenu();
		}
		else {
			if (!warnedForName && SokuMan::GetProfileName(1) == "edit me") {
				Status::Error("Consider changing your profile name before hosting, or press host again.");
				SokuMan::ClearMenu();

				warnedForName = true;
				return;
			}

			if (HostingOptions::showMessagePrompt) {
				HostMessageDialog->Active = true;
				SokuMan::ClearMenu();
				return;
			}
			else {
				Status::Normal("Hosting...", Status::forever);
				SokuMan::SetupHost(HostingOptions::port, HostingOptions::spectate);

				if (HostingOptions::publicHost) {
					json data = {
						{"profile_name", SokuMan::GetProfileName(1)},
						{"message", HostingOptions::message},
						{"port", HostingOptions::port}
					};
					host_mutex.lock();
					host_payloads.emplace_back(data.dump());
					host_mutex.unlock();
				}
			}
		}
		hosting = !hosting;
	});
	
	Menu::AddItem("Join", []() {
		Hostlist::active = true;
		SokuMan::InputBlock.Toggle(true);
		SokuMan::ClearMenu();
	});
	
	Menu::AddItem("Refresh", []() {
		Hostlist::oldTime = 0;
		SokuMan::ClearMenu();
	});
	
	Menu::AddItem("Options", []() {
		HostingOptions::dialog->Active = true;
		SokuMan::ClearMenu();
	});

	ClipboardDialog = DialogMan::AddDialog("Clipboard", []() {
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Do you want to join or spectate this host?");

		ImGui::SetCursorPosX(100);
		if (ImGui::Button("Join")) {
			Status::Normal("Joining...", Status::forever);

			ImGui::CloseCurrentPopup(); 
			SokuMan::JoinHost(NULL, 0);
			SokuMan::InputBlock.Toggle(false);
			SokuMan::SfxPlay(SokuSFX::Select);
			return false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Spectate")) {
			Status::Normal("Joining...", Status::forever);

			SokuMan::JoinHost(NULL, 0, true);
			SokuMan::InputBlock.Toggle(false);
			SokuMan::SfxPlay(SokuSFX::Select);
			return false;
		}
		return true;
	});
	
	Menu::AddItem("Join from clipboard", []() {
		ClipboardDialog->Active = true;
		SokuMan::ClearMenu();
	});
	
	Menu::AddItem("Profile select");
	
	Menu::AddEventHandler(Menu::Event::AlreadyPlaying, []() {
		Status::Normal("Match in progress, spectating...");
		SokuMan::JoinHost(NULL, 0, true);
	});
	
	Menu::AddEventHandler(Menu::Event::ConnectionFailed, []() {
		Hostlist::joining = false;
		Status::Error("Failed to connect.");
		SokuMan::SfxPlay(SokuSFX::Back);
		SokuMan::ClearMenu();
	});
	
	printf("Init done.\n");
}

extern "C" __declspec(dllexport) bool CheckVersion(const BYTE hash[16]) {
       return true;
}

extern "C" __declspec(dllexport) bool Initialize(HMODULE hMyModule, HMODULE hParentModule) {
	//Init path variables
	wchar_t wd[MAX_PATH];
	// The game sets the working directory to the executable file folder on start
	// -> use that as our wd instead of GetCurrentDirectory
	GetModuleFileNameW(0, wd, MAX_PATH);
	PathRemoveFileSpecW(wd);
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(hMyModule, path, MAX_PATH);
	PathRemoveFileSpecW(path);
	wchar_t relative[MAX_PATH];
	if (PathRelativePathToW(relative, wd, FILE_ATTRIBUTE_DIRECTORY, path, FILE_ATTRIBUTE_DIRECTORY)) {
		module_path = wstring(relative);
	} else {
		module_path = wstring(path);
	}

	HKEY key;
	if (!RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Touhou\\Soku", 0, NULL, 0, KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, NULL, &key, NULL)) {
		DWORD value;
		DWORD size = sizeof(value);
		if (!RegQueryValueExW(key, L"HostlistShownPrompt", NULL, NULL, (byte*)&value, &size)) {
			firstRun = !value;
		} else {
			value = 1;
			RegSetValueExW(key, L"HostlistShownPrompt", 0, REG_DWORD, (BYTE*)&value, sizeof(value));
			firstRun = true;
		}
		RegCloseKey(key);
	}
	
	_beginthread(Init, 0, NULL);

	ImGuiMan::SetupHookSync(Load, Render);

	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved) {
	return TRUE;
}
