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

#ifndef HOOK_METHOD
#define HOOK_METHOD CallNearHookMethod
#endif

#define RESET_PATCH_ADDR 0x4151a6
#define RESET_CALL_ADDR 0x4151ac
#define ENDSCENE_CALL_ADDR 0x40104c

using namespace std;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct Image {
	PDIRECT3DTEXTURE9 Texture;
	ImVec2 Size;

	Image(PDIRECT3DTEXTURE9 tex, float w, float h) : Texture(tex), Size(w, h) {}
};

namespace ImGuiMan {
	typedef HRESULT(__stdcall* EndSceneFn)(IDirect3DDevice9*); 
	typedef HRESULT(__stdcall* ResetFn)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
	typedef bool(__thiscall* SokuSetupFn)(void**, HWND*);

	typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
	WNDPROC oldWndProc;

	typedef void (*PassedFn)(void);
	//For loading images/fonts.
	PassedFn LoadFunction = NULL;
	//For rendering with imgui/dx.
	PassedFn RenderFunction = NULL;

	HWND window;

	ImFont* fontDefault = NULL;

	bool RemapNavFlag = true;

	bool active = true;

	SokuSetupFn Original_SokuSetup = NULL;
	ResetFn Original_Reset = NULL;
	EndSceneFn Original_EndScene = NULL;

	Image* LoadImageFromTexture(PDIRECT3DTEXTURE9 texture) {
		// Retrieve description of the texture surface so we can access its size
		D3DSURFACE_DESC image_desc;
		texture->GetLevelDesc(0, &image_desc);
		return new Image(texture, image_desc.Width, image_desc.Height);
	}

	Image *LoadImageFromFile(wstring filename) {
		// Load texture from disk
		PDIRECT3DTEXTURE9 texture;
		HRESULT hr = D3DXCreateTextureFromFileW(*SOKU_D3D9_DEVICE, filename.c_str(), &texture);
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
		ImGuiIO& io = ImGui::GetIO();

		if (uMsg == WM_MOUSEMOVE) {
			RECT rect;
			GetWindowRect(window, &rect);
			float resized_width = rect.right - rect.left;
			float resized_height = rect.bottom - rect.top;
			long resized_x = (short)lParam;
			long resized_y = (short)(lParam >> 16);
			float correct_x = (resized_x * (WINDOW_WIDTH / resized_width));
			float correct_y = (resized_y * (WINDOW_HEIGHT / resized_height));

			io.MousePos.x = correct_x;
			io.MousePos.y = correct_y;
		}
		else if (uMsg == WM_SIZE) {
			if (wParam == SIZE_MINIMIZED)
				active = false;
			else if (wParam == SIZE_RESTORED)
				active = true;
		}
		else if (uMsg == WM_ACTIVATEAPP && !SOKU_D3DPRESENT_PARAMETERS->Windowed) {
			if(!wParam)
				active = false;
			else
				active = true;
		}

		LRESULT res = CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);

		if (!res)
			if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam) || ImGui::GetIO().WantCaptureMouse)
				return true;
		return res;
	}

	int __stdcall Hooked_EndScene(IDirect3DDevice9* pDevice) {
		static bool init = true;

		if (active) {
			if (init) {
				init = false;
				ImGui::CreateContext();
				ImGuiIO& io = ImGui::GetIO();

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

				if (DEBUG) printf("Imgui init done.");
			}

			ImGui_ImplDX9_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			if (RenderFunction != NULL)
				RenderFunction();

			ImGui::EndFrame();
			ImGui::Render();

			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		}

		//This is necessary so we can fit in the hook...
		//That said, the return value is never even checked in soku.
		Original_EndScene(pDevice);
		return 0x8a0e14;
	}

	HRESULT __stdcall Hooked_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* params) {
		ImGui_ImplDX9_InvalidateDeviceObjects();

		HRESULT hr = Original_Reset(pDevice, params);
		if (hr != S_OK)
			return hr;

		ImGui_ImplDX9_CreateDeviceObjects();

		return S_OK;
	}

	namespace VTableHookMethod {
		//Necessary since Dx9 has functions that refresh the vtable...
		void** CreateDummyVTable(void** oldVTable) {
			void** newVTable = (void**)malloc(175 * sizeof(void*));
			memcpy(newVTable, oldVTable, 175 * sizeof(void*));

			newVTable[42] = (void*)Hooked_EndScene;
			newVTable[16] = (void*)Hooked_Reset;
			return newVTable;
		}

		void Hook(HWND hwnd, IDirect3DDevice9* device) {
			window = hwnd;
			void** oldVTable = *(void***)device;
			void** newVTable = CreateDummyVTable(oldVTable);
			(((void**)device)[0]) = newVTable;

			Original_Reset = (ResetFn)oldVTable[16];
			Original_EndScene = (EndSceneFn)oldVTable[42];
		}
	}

	namespace CallNearHookMethod {
		void Hook(HWND hwnd, IDirect3DDevice9* device) {
			window = hwnd;
			// Check if something already hooked DxReset.
			if (*((unsigned char*)RESET_CALL_ADDR) != 0xe8) {
				PatchMan::Patch(RESET_PATCH_ADDR, "\x68\x68\x0f\x8a\x00\x50\xe8\x00\x00\x00\x00\x90\x90", 13).Toggle(true);
				PatchMan::HookNear(RESET_CALL_ADDR, (DWORD)Hooked_Reset);
				Original_Reset = (*(ResetFn**)device)[16];
			}
			else {
				Original_Reset = (ResetFn)PatchMan::HookNear(0x4151ac, (DWORD)Hooked_Reset);
			}
			// Check if something already hooked DxEndScene
			if (*((unsigned char*)ENDSCENE_CALL_ADDR) != 0xe8) {
				PatchMan::Patch(ENDSCENE_CALL_ADDR, "\xe8\x00\x00\x00\x00\x50\x90", 7).Toggle(true);
				PatchMan::HookNear(ENDSCENE_CALL_ADDR, (DWORD)Hooked_EndScene);
				Original_EndScene = (*(EndSceneFn**)device)[42];
			}
			else {
				Original_EndScene = (EndSceneFn)PatchMan::HookNear(0x40104c, (DWORD)Hooked_EndScene);
			}
		}
	}

	bool __fastcall Hooked_SokuSetup(void** DxWinHwnd, void* EDX, HWND* hwnd) {
		bool ret = Original_SokuSetup(DxWinHwnd, hwnd);
		HOOK_METHOD::Hook(*hwnd, *SOKU_D3D9_DEVICE);
		return ret;
	}

	void SetupHookSync(PassedFn load, PassedFn render) {
		LoadFunction = load;
		RenderFunction = render;

		Original_SokuSetup = (SokuSetupFn)PatchMan::HookNear(0x7fb871, (DWORD)Hooked_SokuSetup);
	}

	void SetupHookAsync(PassedFn load, PassedFn render) {
		while (*SOKU_D3D9_DEVICE == 0 || *SOKU_HWND == 0)
			this_thread::sleep_for(10ms);

		LoadFunction = load;
		RenderFunction = render;

		HOOK_METHOD::Hook(*SOKU_HWND, *SOKU_D3D9_DEVICE);
	}
}

#endif