#pragma once
#include <iostream>
#include <list>
#include <string>
#include <imgui.h>
#include "ImGuiMan.hpp"
using namespace std;

typedef bool(*DialogFunction)();

struct Dialog {
	string Title;
	bool Active;
	ImGuiWindowFlags Flags;
	DialogFunction Callback;

	Dialog(string title, DialogFunction callback) :
		Title(title), Flags(0), Active(false), Callback(callback) {}

	Dialog(string title, ImGuiWindowFlags flags, DialogFunction callback) :
		Title(title), Flags(flags), Active(false), Callback(callback) {}
};

namespace DialogMan {
	list<Dialog*> dialogs;

	Dialog *AddDialog(string title, ImGuiWindowFlags flags, DialogFunction callback) {
		dialogs.push_back(new Dialog(title, flags, callback));
		return dialogs.back();
	}

	Dialog* AddDialog(string title, DialogFunction callback) {
		return AddDialog(title, 0, callback);
	}

	void Render() {
		ImGuiIO& io = ImGui::GetIO();
		for (Dialog *d : dialogs) {
			if (d->Active) {
				ImGuiMan::SetNextWindowPosCenter();
				ImGui::Begin(d->Title.c_str(), NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | d->Flags);
				if (ImGui::IsWindowAppearing()) {
					ImGui::GetIO().NavVisible = true;
					ImGui::GetIO().NavActive = true;
				}

				d->Active = d->Callback();

				if (io.NavInputs[ImGuiNavInput_Cancel] && !io.WantTextInput) {
					d->Active = false;
				}

				ImGui::End();
			}
		}
	}

	bool AnyActive() {
		for (Dialog *d : dialogs)
			if (d->Active)
				return true;
		return false;
	}

	void DisableAll() {
		for (Dialog* d : dialogs)
			d->Active = false;
	}

	//Wrapper for requiring the button to be unpressed for at least a frame before accepting input
	//Doesn't work with multiple dialogs open though
	bool IsActivatePressed() {
		static bool wasInactive = false;

		if (!ImGui::GetIO().NavInputs[ImGuiNavInput_Activate]) {
			wasInactive = true;
		}
		else if (wasInactive) {
			wasInactive = false;
			return true;
		}
		return false;
	}
};
