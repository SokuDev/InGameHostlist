#pragma once
#include <string>
#include <vector>
#include <mmsystem.h>
#include <nlohmann/json.hpp>

#include "SokuMan.hpp"

#include "HostingOptions.hpp"
#include "Status.hpp"
#include "Host.hpp"

using namespace std;

#define SHORT_WAITTIME 3000
#define LONG_WAITTIME 30000

#define PING_DELAY 3000

#define HOSTS_PER_PAGE 15

extern std::wstring module_path;
extern LARGE_INTEGER timer_frequency;

bool gotFirstHostlist;

namespace Hostlist {
	enum { WAITING, PLAYING, PAGE_COUNT };
	enum { NORMAL_MODE, EQUAL_MODE, NAME_MODE, MESSAGE_MODE };
	vector<Host *> hosts[PAGE_COUNT];

	CInputManagerCluster *InputManager;

	bool joining = false;

	Image *imageBackground;
	bool active = false;

	int page_id = WAITING;
	unsigned int ip_id = 0;

	unsigned long long oldTime = 0;
	unsigned long long newTime = 0;
	unsigned long long delayTime = 0;

	bool offline = false;
	int refreshCount = 0;

	const ImColor colorNormal = ImColor(255, 255, 255, 255);
	const ImColor colorGrayedOut = ImColor(180, 180, 180, 255);
	const ImColor colorError = ImColor(255, 0, 0, 255);

	wstring sfxPath;

	mutex updatingHostlist;

	void Init() {
		Status::Init();

		sfxPath = module_path;
		sfxPath.append(L"\\NewHostSFX.wav");
		return;
	}

	void Load() {
		std::wstring imagePath = module_path;
		imagePath.append(L"\\hostlistBG.png");

		imageBackground = ImGuiMan::LoadImageFromFile(imagePath);
	}

	void OnMenuOpen() {
		active = false;
		joining = false;

		Status::OnMenuOpen();

		InputManager = SokuMan::GetInputManager();
	}

	bool CheckForNewHosts(json &jHosts) {
		for (auto &jHost : jHosts) {
			if (jHost["started"].get<bool>())
				continue;
			bool isNewHost = true;
			for (unsigned int j = 0; j < hosts[WAITING].size(); ++j) {
				if (hosts[WAITING][j]->Compare(jHost)) {
					isNewHost = false;
					break;
				}
			}
			if (isNewHost)
				return true;
		}
		return false;
	}

	//Play sfx when a new waiting host appears
	void OnNewHost() {
		if(HostingOptions::playSfxOnNewHost)
			PlaySoundW(sfxPath.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
	}

	void Update() {
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);
		newTime = counter.QuadPart * 1000 / timer_frequency.QuadPart;

		Status::Update(newTime);

		// Get new hosts
		if (!offline) {
			if (newTime - oldTime >= delayTime) {
				try {
					string s = WebMan::Request("https://konni.delthas.fr/games");
					json res = json::parse(s);
					lock_guard<mutex> lock(updatingHostlist);

					if (gotFirstHostlist && CheckForNewHosts(res))
						OnNewHost();
					gotFirstHostlist = true;

					for (unsigned int page = 0; page < PAGE_COUNT; ++page) {
						for (unsigned int i = 0; i < hosts[page].size(); ++i) {
							delete hosts[page][i];
						}
						hosts[page].clear();
					}

					for (auto &jHost : res) {
						Host *host = new Host(jHost);
						if (host->playing)
							hosts[PLAYING].push_back(host);
						else
							hosts[WAITING].push_back(host);
					}

					if (active) {
						if (ip_id >= hosts[page_id].size()) {
							ip_id = hosts[page_id].size() - 1;
						}
					}

					delayTime = SHORT_WAITTIME;
				} catch (const char *e) {
					Status::Error(e);
					delayTime = LONG_WAITTIME;
				} catch (int e) {
					if (e == 429)
						Status::Error(LocaleMan::Text["RateLimitError"].c_str());
					else
						Status::Error(LocaleMan::Text["UnknownError"].c_str() + to_string(e));
					delayTime = LONG_WAITTIME;
				}

				oldTime = newTime;
				refreshCount++;
			}
		}
	}

	void Render() {
		if (SokuMan::GetCMenuConnect()->Choice != 6) {
			float statusSize = (Status::lines - 1) * 16;
			if (Status::lines == 0) statusSize = 0;

			float nameColumnSize = 100;
			if (HostingOptions::columnMode == EQUAL_MODE) {
				nameColumnSize = 150;
			} 
			else if (HostingOptions::columnMode == NAME_MODE) {
				nameColumnSize = 303;
			}
			else if (HostingOptions::columnMode == MESSAGE_MODE) {
				nameColumnSize = 0;
			}

			ImGui::SetNextWindowPos(ImVec2(250, 85));
			ImGui::SetNextWindowSize(ImVec2(360, 336 + statusSize));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 1);
			ImGui::Begin("HostList##Hosts", 0,
				ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove
					| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNav
					| ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0, 0, 0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3.0f, 3.0f));

			ImGui::SetCursorPos(ImVec2(3, 5));
			ImGui::Image(imageBackground->Texture, ImVec2(350, 330 + statusSize));
			ImGui::SetCursorPos(ImVec2(6, 5));

			ImGui::PushItemWidth(-1);
			ImGui::ListBoxHeader("##hosts_listbox", ImVec2(0, -20));
			ImGui::SetCursorPosX(152); 
			ImGui::Text("Hostlist");
			ImGui::Separator();

			updatingHostlist.lock();

			ImGui::SetCursorPosX(95); 
			ImGui::TextColored((page_id == WAITING) ? colorNormal : colorGrayedOut, LocaleMan::Text["HostlistWaiting"].c_str(), hosts[WAITING].size());
			ImGui::SameLine();
			ImGui::Text(" / ");
			ImGui::SameLine();
			ImGui::TextColored((page_id == PLAYING) ? colorNormal : colorGrayedOut, LocaleMan::Text["HostlistPlaying"].c_str(), hosts[PLAYING].size());

			if (page_id == WAITING) {
				ImGui::Columns(3, "waiting");
				ImGui::SetColumnWidth(0, nameColumnSize);
				ImGui::SetColumnWidth(1, 303 - nameColumnSize);
				//If you don't add the 2 it cuts off the text despite it not being
				//Close to the border lol
				ImGui::SetColumnWidth(2, 32);
				ImGui::Separator();
				ImGui::Text(LocaleMan::Text["HostlistName"].c_str());
				ImGui::NextColumn();
				ImGui::Text(LocaleMan::Text["HostlistMessage"].c_str());
				ImGui::NextColumn();
				ImGui::Text(LocaleMan::Text["HostlistPing"].c_str());
				ImGui::NextColumn();

				if (hosts[WAITING].size() > 0) {
					ImGui::Separator(); // Necessary to avoid graphical glitches

					unsigned int start_pos = ip_id - (ip_id % HOSTS_PER_PAGE);
					for (unsigned int i = start_pos; i < start_pos + HOSTS_PER_PAGE; i++) {
						if (i < hosts[WAITING].size()) {
							ImGui::PushID(i);

							ImGui::Selectable(hosts[WAITING][i]->name.c_str(), i == ip_id && active, ImGuiSelectableFlags_SpanAllColumns);
							ImGui::NextColumn();

							ImGui::Text("%s", hosts[WAITING][i]->message.c_str());
							ImGui::NextColumn();

							long ping = PingMan::Ping(*hosts[WAITING][i]);
							if (ping != PingMan::PING_UNINITIALIZED)
								ImGui::TextColored(((ping >= 0) ? colorNormal : colorError), "%ld", ping);
							else
								ImGui::Text("...");
							ImGui::NextColumn();

							ImGui::PopID();
						}
					}
				}
			}
			// Pretty much a duplicate, I should find a way to streamline it
			else if (page_id == PLAYING) {
				ImGui::Columns(3, "playing");
				ImGui::SetColumnWidth(0, 150);
				ImGui::SetColumnWidth(1, 153);
				ImGui::SetColumnWidth(2, 32);
				ImGui::Separator();
				ImGui::Text(LocaleMan::Text["HostlistPlayer1"].c_str());
				ImGui::NextColumn();
				ImGui::Text(LocaleMan::Text["HostlistPlayer2"].c_str());
				ImGui::NextColumn();
				ImGui::Text(LocaleMan::Text["HostlistPing"].c_str());
				ImGui::NextColumn();

				if (hosts[PLAYING].size() > 0) {
					ImGui::Separator(); // Necessary to avoid graphical glitches

					unsigned int start_pos = ip_id - (ip_id % HOSTS_PER_PAGE);
					for (unsigned int i = start_pos; i < start_pos + HOSTS_PER_PAGE; i++) {
						if (i < hosts[PLAYING].size()) {
							ImGui::PushID(i);

							ImGui::Selectable(hosts[PLAYING][i]->name.c_str(), i == ip_id && active, ImGuiSelectableFlags_SpanAllColumns);
							ImGui::NextColumn();

							ImGui::Text("%s", hosts[PLAYING][i]->opponentName.c_str());
							ImGui::NextColumn();

							long ping = PingMan::Ping(*hosts[PLAYING][i]);
							if (ping != PingMan::PING_UNINITIALIZED)
								ImGui::TextColored(((ping >= 0) ? colorNormal : colorError), "%ld", ping);
							else
								ImGui::Text("...");
							ImGui::NextColumn();

							ImGui::PopID();
						}
					}
				}
			}

			if (hosts[page_id].size() == 0) {
				ImGui::Columns(1);
				ImGui::Separator();

				ImGui::Text(LocaleMan::Text["HostlistNoHosts"].c_str());
			}

			updatingHostlist.unlock();

			// Move the cursor to the end of the listbox
			// so that the column borders extend all the way down
			ImGui::SetCursorPosY(305);

			ImGui::Columns(1);
			ImGui::Separator();

			ImGui::ListBoxFooter();
			ImGui::PopItemWidth();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();

			ImGui::SetCursorPos(ImVec2(8, 315));
			Status::Render();

			ImGui::End();
			ImGui::PopStyleVar(2);
		}
	}

	void HandleInput() {
		if (!SokuMan::InputBlock.Check() || active) {
			if (InputManager->P1.Xaxis == +1 || (InputManager->P1.Xaxis >= +36 && InputManager->P1.Xaxis % 6 == 0) || 
				InputManager->P1.Xaxis == -1 || (InputManager->P1.Xaxis <= -36 && InputManager->P1.Xaxis % 6 == 0)) {
				page_id = !page_id;
				ip_id = 0;

				SokuMan::SfxPlay(SokuSFX::Move);
			}
			else if (InputManager->P1.D == 1) {
				HostingOptions::columnMode++;
				if (HostingOptions::columnMode > MESSAGE_MODE || HostingOptions::columnMode < NORMAL_MODE) {
					HostingOptions::columnMode = NORMAL_MODE;
				}

				SokuMan::SfxPlay(SokuSFX::Select);
			}
		}

		if (active) {
			// Normal menu inputs
			if (!joining) {
				//Down
				if (InputManager->P1.Yaxis == 1 || (InputManager->P1.Yaxis >= 36 && InputManager->P1.Yaxis % 6 == 0)) {
					ip_id += 1;
					if (ip_id >= hosts[page_id].size())
						ip_id = 0;

					SokuMan::SfxPlay(SokuSFX::Move);
				//Up
				} else if (InputManager->P1.Yaxis == -1 || (InputManager->P1.Yaxis <= -36 && InputManager->P1.Yaxis % 6 == 0)) {
					if (ip_id == 0)
						ip_id = hosts[page_id].size() - 1;
					else
						ip_id -= 1;

					SokuMan::SfxPlay(SokuSFX::Move);
				}

				if (InputManager->P1.A == 1 && hosts[page_id].size() != 0 && ip_id < hosts[page_id].size()) {
					SokuMan::JoinHost(hosts[page_id][ip_id]->ip.c_str(), hosts[page_id][ip_id]->port, (page_id == PLAYING ? true : false));
					joining = true;
					Status::Normal(LocaleMan::Text["HostlistJoining"], Status::forever);
					SokuMan::SfxPlay(SokuSFX::Select);
				}
			}

			// Block B input if currently joining.
			// This part is a bit iffy but it works I guess
			// It's necessary since otherwise if we press B while joining it'd
			// play SFX_BACK twice (first the game, then the mod), and there's
			// no other way to check if we were joining
			//(choice gets reset before we check)
			if (SokuMan::InputBlock.Check() && InputManager->P1.B == 1 && joining) {
				Status::Normal(LocaleMan::Text["HostlistJoiningAborted"]);
				joining = false;
			}
		} else {
			ip_id = 0;
		}
	}
} 