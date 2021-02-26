#pragma once
#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <imgui.h>
using namespace std;

enum DialogOption {
	DIALOG_OPTION1 = 0,
	DIALOG_OPTION2 = 1,
};

namespace DialogMan {
	struct Dialog {
		string Title;
		string Message;
		string Option1;
		string Option2;
		function<void(DialogOption)> Callback;

		Dialog(string title, string message, string option1, string option2, function<void(DialogOption)> callback) :
			Title(title), Message(message), Option1(option1), Option2(option2), Callback(callback) {}

		void Render() {
			ImGui::SetNextWindowPosCenter();
			ImGui::SetNextWindowSize(ImVec2(300, 60));
			if (ImGui::BeginPopup(Title.c_str(), ImGuiWindowFlags_NoCollapse)) {
				if (ImGui::IsWindowAppearing()) {
					ImGui::GetIO().NavVisible = true;
					ImGui::GetIO().NavActive = true;
				}

				ImGui::AlignTextToFramePadding();
				ImGui::Text(Message.c_str());

				ImGui::SetCursorPosX(100);
				if (ImGui::Button(Option1.c_str())) {
					ImGui::CloseCurrentPopup();
					Callback(DIALOG_OPTION1);
				}
				ImGui::SameLine();
				if (ImGui::Button(Option2.c_str())) {
					ImGui::CloseCurrentPopup();
					Callback(DIALOG_OPTION2);
				}
				ImGui::EndPopup();
			}
		}
	};

	vector<Dialog> dialogs;

	void OpenDialog(string id) {
		ImGui::OpenPopup(id.c_str());
	}

	void AddDialog(string title, string message, string option1, string option2, function<void(DialogOption)> callback) {
		dialogs.emplace_back(title, message, option1, option2, callback);
	}

	void Render() {
		for (Dialog& d : dialogs)
			d.Render();
	}

	bool AnyActive() {
		for (Dialog& d : dialogs)
			if (ImGui::IsPopupOpen(d.Title.c_str()))
				return true;
		return false;
	}
};
