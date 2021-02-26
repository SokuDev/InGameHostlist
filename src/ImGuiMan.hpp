#pragma once
#include <Windows.h>
#include <iostream>
#include <WinUser.h>
#include <string>

#include <d3d9.h>
#include <d3dx9tex.h>

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

#include "PatchMan.hpp"

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
	typedef HRESULT(__stdcall* f_EndScene)(IDirect3DDevice9* pDevice); // Our function prototype 
	f_EndScene oldEndScene; // Original Endscene


	typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
	WNDPROC oldWndProc;

	typedef void (*PassedFun)(void);
	//For loading images/fonts.
	PassedFun LoadFunction = NULL;
	//For rendering with imgui/dx.
	PassedFun RenderFunction = NULL;

	HWND window;

	ImFont* fontDefault = NULL;
	ImFont* fontMenu = NULL;

	Image* backgroundImg = NULL;

	Image *LoadImageFromFile(wstring filename) {
		// Load texture from disk
		PDIRECT3DTEXTURE9 texture;
		HRESULT hr = D3DXCreateTextureFromFileW(*SOKU_D3D_DEVICE, filename.c_str(), &texture);
		if (hr != S_OK)
			return NULL;

		// Retrieve description of the texture surface so we can access its size
		D3DSURFACE_DESC image_desc;
		texture->GetLevelDesc(0, &image_desc);
		return new Image(texture, image_desc.Width, image_desc.Height);
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

		style.ItemSpacing = ImVec2(5, 5);
		style.ItemInnerSpacing = ImVec2(4, 4);
		style.FramePadding = ImVec2(5, 5);

		style.WindowPadding = ImVec2(8, 0);
		style.ScrollbarSize = 0;

		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.3, 0.3, 0.3, 0.9);
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

	HRESULT __stdcall Hooked_EndScene(IDirect3DDevice9* pDevice) {
		static bool init = true;
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

			ImGui_ImplWin32_Init(window);
			ImGui_ImplDX9_Init(pDevice);

			//Loading fonts/images has to be done here I think
			//Todo make a InitFunction you can pass to this.
			//Font loading
			fontDefault = io.Fonts->AddFontDefault();

			if (LoadFunction != NULL)
				LoadFunction();
			
			wstring font_path = module_path;
			font_path.append(L"\\romanan.ttf");
			char font_path_ansi[MAX_PATH];
			wcstombs(font_path_ansi, &font_path[0], MAX_PATH);
			fontMenu = io.Fonts->AddFontFromFileTTF(font_path_ansi, 20.0f);

			std::wstring image_path = module_path;
			image_path.append(L"\\hostlistBG.png");
			backgroundImg = ImGuiMan::LoadImageFromFile(image_path);

			SetupStyle();

			if(DEBUG) printf("Imgui init done.");
		}

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if(RenderFunction != nullptr)
			RenderFunction();

		/*
		ImGui::Begin("##test", (bool*)0, ImVec2(200, 200));
		ImGui::Text("Text");
		int a = 0;
		ImGui::InputInt("a", &a);
		ImGui::InputInt("b", &a);
		ImGui::End();
		*/

		ImGui::EndFrame();
		ImGui::Render();

		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

		return oldEndScene(pDevice);
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

		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
			return true;
		}
			
		return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
	}


	DWORD WINAPI HookThread(PassedFun load, PassedFun render) {
		while (*SOKU_D3D_DEVICE == 0 || *SOKU_HWND == 0)
			this_thread::sleep_for(10ms);
		
		window = *SOKU_HWND;
		std::cout << std::to_string((int)window) << std::endl;

		//std::cout << std::to_string((int)window) << std::endl;

		oldWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);

		IDirect3DDevice9* Device = *SOKU_D3D_DEVICE;
		void** pVTable = *((void***)Device);

		oldEndScene = (f_EndScene)PatchMan::HookVTable((DWORD*)&(pVTable[42]), (DWORD)Hooked_EndScene);

		LoadFunction = load;
		RenderFunction = render;

		return false;
	}
}