#pragma once

#ifndef _IMGUIMAN
#define _IMGUIMAN

#include <Windows.h>
#include <iostream>
#include <WinUser.h>
#include <string>
#include <thread>

#include <d3d9.h>
#include <d3dx9tex.h>

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

#include "PatchMan.hpp"
#include "SokuAPI.hpp"

#ifndef DEBUG
#define DEBUG false
#endif

using namespace std;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define WINDOW_HEIGHT 509.0
#define WINDOW_WIDTH 646.0
#define SOKU_HWND ((HWND*)0x0089FF90)
#define SOKU_D3D_DEVICE ((IDirect3DDevice9**)0x08a0e30)

struct Image {
	PDIRECT3DTEXTURE9 Texture;
	ImVec2 Size;

	Image(PDIRECT3DTEXTURE9 tex, float w, float h) : Texture(tex), Size(w, h) {}
};

namespace ImGuiMan {
	typedef HRESULT(__stdcall* EndSceneFn)(IDirect3DDevice9* pDevice); 
	typedef HRESULT(__stdcall* ResetFn)(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* params);

	typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
	WNDPROC oldWndProc;

	void** oldVTable = NULL;
	void** newVTable = NULL;

	typedef void (*PassedFn)(void);
	//For loading images/fonts.
	PassedFn LoadFunction = NULL;
	//For rendering with imgui/dx.
	PassedFn RenderFunction = NULL;

	HWND window;

	ImFont* fontDefault = NULL;

	bool RemapNavFlag = true;

	bool active = true;
	bool windowed = true;

	Image* LoadImageFromTexture(PDIRECT3DTEXTURE9 texture) {
		// Retrieve description of the texture surface so we can access its size
		D3DSURFACE_DESC image_desc;
		texture->GetLevelDesc(0, &image_desc);
		return new Image(texture, image_desc.Width, image_desc.Height);
	}

	Image *LoadImageFromFile(wstring filename) {
		// Load texture from disk
		PDIRECT3DTEXTURE9 texture;
		HRESULT hr = D3DXCreateTextureFromFileW(*SOKU_D3D_DEVICE, filename.c_str(), &texture);
		if (hr != S_OK)
			return NULL;

		return LoadImageFromTexture(texture);
	}

	// Use instead of the ImGui one, for compatability with the resize mod.
	inline void SetNextWindowPosCenter(ImGuiCond c = 0) {
		ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f), c, ImVec2(0.5f, 0.5f));
	}

	void SetupStyle() {
		ImGuiStyle& style = ImGui::GetStyle();
		style.Alpha = 1;
		
		style.WindowRounding = 0;
		style.ChildRounding = 0;
		style.FrameRounding = 0;
		style.GrabRounding = 0;
		style.PopupRounding = 0;
		style.ScrollbarRounding = 0;
		style.TabRounding = 0;

		style.ItemSpacing = ImVec2(4, 4);
		style.ItemInnerSpacing = ImVec2(4, 4);
		style.FramePadding = ImVec2(4, 4);

		style.WindowPadding = ImVec2(4, 4);
		style.ScrollbarSize = 10;

		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.3, 0.3, 0.3, 0.9);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.3, 0.3, 0.3, 0.9);
		style.Colors[ImGuiCol_Border] = ImVec4(1, 1, 1, 0.9);

		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.3, 0.3, 0.3, 0.9);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.3, 0.3, 0.3, 0.9);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.4, 0.4, 0.4, 0.9);

		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.8, 0.8, 0.8, 0.9);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.9, 0.9, 0.9, 0.9);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1, 1, 1, 1);

		style.Colors[ImGuiCol_Button] = ImVec4(0.4, 0.4, 0.4, 1);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.5, 0.5, 0.5, 1);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.6, 0.6, 0.6, 1);

		style.Colors[ImGuiCol_CheckMark] = (ImVec4)ImColor(0, 200, 0, 200);

		style.Colors[ImGuiCol_Header] = (ImVec4)ImColor(0, 200, 0, 200);
		style.Colors[ImGuiCol_HeaderHovered] = (ImVec4)ImColor(0, 200, 0, 100);
		style.Colors[ImGuiCol_HeaderActive] = (ImVec4)ImColor(0, 100, 0, 100);

		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.4, 0.4, 0.4, 1);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.5, 0.5, 0.5, 1);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.6, 0.6, 0.6, 1);

		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.5, 0.5, 0.5, 1);

		style.Colors[ImGuiCol_Separator] = (ImVec4)ImColor(255, 255, 255, 150);

		style.Colors[ImGuiCol_NavHighlight] = (ImVec4)ImColor(0, 200, 0, 200);
	}

	void RemapNav() {
		ImGuiIO& io = ImGui::GetIO();
		CInputManagerCluster* SokuInput = SokuAPI::GetInputManager();
		if (SokuInput->P1.Yaxis > 0) io.NavInputs[ImGuiNavInput_DpadDown] = 1.0f;
		if (SokuInput->P1.Yaxis < 0) io.NavInputs[ImGuiNavInput_DpadUp] = 1.0f;
		if (SokuInput->P1.Xaxis > 0) io.NavInputs[ImGuiNavInput_DpadRight] = 1.0f;
		if (SokuInput->P1.Xaxis < 0) io.NavInputs[ImGuiNavInput_DpadLeft] = 1.0f;
		if (SokuInput->P1.A) io.NavInputs[ImGuiNavInput_Activate] = 1.0f;
		if (SokuInput->P1.B) io.NavInputs[ImGuiNavInput_Cancel] = 1.0f;
		if (SokuInput->P1.C) io.NavInputs[ImGuiNavInput_Menu] = 1.0f;
		if (io.KeysDown[io.KeyMap[ImGuiKey_Enter]]) io.NavInputs[ImGuiNavInput_Input] = 1.0f;
		if (io.KeysDown[io.KeyMap[ImGuiKey_Tab]]) io.NavInputs[ImGuiNavInput_KeyTab_] = 1.0f;
	}

	LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		if (uMsg == WM_MOUSEMOVE) {
			RECT rect;
			GetWindowRect(window, &rect);
			float resized_width = rect.right - rect.left;
			float resized_height = rect.bottom - rect.top;
			long resized_x = (short)lParam;
			long resized_y = (short)(lParam >> 16);
			float correct_x = (resized_x * (WINDOW_WIDTH / resized_width));
			float correct_y = (resized_y * (WINDOW_HEIGHT / resized_height));

			ImGuiIO& io = ImGui::GetIO();
			io.MousePos.x = correct_x;
			io.MousePos.y = correct_y;
		}
		else if (uMsg == WM_SIZE) {
			void*** Device = *(void****)SOKU_D3D_DEVICE;
			if (wParam == SIZE_MINIMIZED)
				active = false;
			else if (wParam == SIZE_RESTORED)
				active = true;
		}
		else if (uMsg == WM_ACTIVATEAPP && !windowed) {
			void*** Device = *(void****)SOKU_D3D_DEVICE;
			if(!wParam)
				active = false;
			else
				active = true;
		}
	

		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
			return true;
		}

		return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
	}

	HRESULT __stdcall Hooked_EndScene(IDirect3DDevice9* pDevice) {
		static bool init = true;

		//if (GetForegroundWindow() != *SOKU_HWND) return ((EndSceneFn)oldVTable[42])(pDevice);
		if(!active) return ((EndSceneFn)oldVTable[42])(pDevice);

		if (init)
		{
			init = false;
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();

			io.WantCaptureKeyboard = true;
			io.WantCaptureMouse = true;
			io.WantTextInput = true;
			
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

			io.RemapNavFlag = RemapNavFlag;
			io.RemapNavFun = RemapNav;

			ImGui_ImplWin32_Init(window);
			ImGui_ImplDX9_Init(pDevice);

			oldWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);

			fontDefault = io.Fonts->AddFontDefault();

			if (LoadFunction != NULL)
				LoadFunction();

			SetupStyle();

			if(DEBUG) printf("Imgui init done.");
		}

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if(RenderFunction != nullptr)
			RenderFunction();

		ImGui::EndFrame();
		ImGui::Render();

		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

		return ((EndSceneFn)oldVTable[42])(pDevice);
	}

	HRESULT __stdcall Hooked_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* params) {
		ImGui_ImplDX9_InvalidateDeviceObjects();

		HRESULT hr = ((ResetFn)oldVTable[16])(pDevice, params);
		if (hr != S_OK)
			return NULL;

		ImGui_ImplDX9_CreateDeviceObjects();

		windowed = params->Windowed;

		return S_OK;
	}

	//Necessary since Dx9 has functions that refresh the vtable...
	void **CreateDummyVTable(void** oldVTable) {
		void** newVTable = (void**)malloc(175 * sizeof(void*));
		memcpy(newVTable, oldVTable, 175 * sizeof(void*));
		
		newVTable[42] = (void*)Hooked_EndScene;
		newVTable[16] = (void*)Hooked_Reset;
		return newVTable;
	}

	DWORD WINAPI HookThread(PassedFn load, PassedFn render) {
		while (*SOKU_D3D_DEVICE == 0 || *SOKU_HWND == 0)
			this_thread::sleep_for(10ms);

		LoadFunction = load;
		RenderFunction = render;

		window = *SOKU_HWND;
		void*** Device = *(void****)SOKU_D3D_DEVICE;
		oldVTable = *(void***)Device;
		newVTable = CreateDummyVTable(oldVTable);
		(((void**)Device)[0]) = newVTable;

		return 0;
	}
}

#endif