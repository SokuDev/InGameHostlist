#pragma once
#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <imgui.h>
#include "ImGuiMan.hpp"
using namespace std;

enum DialogOption {
	DIALOG_OPTION1 = 0,
	DIALOG_OPTION2 = 1,
};

namespace DialogMan {
	typedef void(*DialogFunction)();

	struct Dialog {
		string Title;
		ImGuiWindowFlags Flags;
		DialogFunction Callback;

		Dialog(string title, DialogFunction callback) :
			Title(title), Flags(0), Callback(callback) {}

		Dialog(string title, ImGuiWindowFlags flags, DialogFunction callback) :
			Title(title), Flags(flags), Callback(callback) {}
	};

	vector<Dialog> dialogs;

	void OpenDialog(string id) {
		ImGui::OpenPopup(id.c_str());
	}

	void AddDialog(string title, DialogFunction callback) {
		dialogs.emplace_back(title, callback);
	}

	void AddDialog(string title, ImGuiWindowFlags flags, DialogFunction callback) {
		dialogs.emplace_back(title, flags, callback);
	}

	void Render() {
		for (Dialog& d : dialogs) {
			ImGuiMan::SetNextWindowPosCenter();
			if (ImGui::BeginPopup(d.Title.c_str(), ImGuiWindowFlags_NoCollapse | d.Flags)) {
				if (ImGui::IsWindowAppearing()) {
					ImGui::GetIO().NavVisible = true;
					ImGui::GetIO().NavActive = true;
				}

				d.Callback();
				
				ImGui::EndPopup();
			}
		}
	}

	bool AnyActive() {
		for (Dialog& d : dialogs)
			if (ImGui::IsPopupOpen(d.Title.c_str()))
				return true;
		return false;
	}

	bool ModalActive() {
		for (Dialog& d : dialogs)
			if (ImGui::IsPopupOpen(d.Title.c_str()) && d.Flags & ImGuiWindowFlags_Modal)
				return true;
		return false;
	}
};
