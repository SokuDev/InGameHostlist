#pragma once
#include <windows.h>
#include <d3d9.h>
#include <cstdint>

#include "PatchMan.hpp"

struct VC9String {
	enum { _BUF_SIZE = 16 };

	void* alloc;
	union {
		char buf[16];
		char* ptr;
	} body;
	size_t size;
	size_t bufsize;

	VC9String() {
		memset(this, 0, sizeof(*this));
		this->bufsize = _BUF_SIZE - 1;
	}

	operator char* () {
		return bufsize >= _BUF_SIZE ? body.ptr : body.buf;
	}
	operator const char* () const {
		return bufsize >= _BUF_SIZE ? body.ptr : body.buf;
	}
};

struct CInputManagerCluster {
	uint8_t UNKNOWN[56];
	struct {
		int32_t Xaxis;
		int32_t Yaxis;
		int32_t A;
		int32_t B;
		int32_t C;
		int32_t D;
		int32_t E;
		int32_t F;
	} P1;
};

struct CDesignSprite {
	void *vftable; // =008576ac
	float UNKNOWN_1[2];
	float x;
	float y;
	uint8_t active;
	uint8_t UNKNOWN_2[3];
	int32_t UNKNOWN_3;
};

struct CMenuConnect {
	void *vftable;
	void *CNetworkBasePtr;
	uint8_t Choice;
	uint8_t Subchoice;
	uint8_t UNKNOWN_0[2];
	void *CDesignBasePtr;
	uint8_t UNKNOWN_1[48];
	CDesignSprite *MenuItemSprites[7];
	uint8_t UNKNOWN_2[860];
	bool NotSpectateFlag;
	bool Spectate;
	uint8_t UNKNOWN_3[2];
	int32_t MenuItem_Count;
	uint8_t UNKNOWN_4[8];
	int32_t CursorPos;
	int32_t CursorPos2;
	uint8_t UNKNOWN_5[48];
	int32_t NumberInput_ArrowPos;
	uint8_t UNKNOWN_6[36];
	int32_t Port;
	uint8_t UNKNOWN_7[4];
	char IPString[20]; // Final/Used val
	wchar_t IPWString[20]; 
	uint8_t UNKNOWN_8[2012];
	int32_t NotInSubMenuFlag;
	uint8_t UNKNOWN_9[332];
	bool InSubMenuFlag;
	uint8_t UNKNOWN_10[171];
	uint8_t UnknownJoinFlag;
	uint8_t UNKNOWN_12[835];
};

struct _MUnknownGlobal {
	struct {
		uint32_t UNKNOWN_1;
		struct {
			uint32_t UNKNOWN_2[2];
			CMenuConnect *CMenuConnectObj;
		} *UnknownContainer2;
	} *UnknownContainer1;
	bool InMenuFlag;
};

enum SokuSFX {
	Move = 0x27,
	Select = 0x28,
	Back = 0x29
};

namespace SokuMan {
	// Addresses
	const auto UnknownGlobalObj = (_MUnknownGlobal*)0x89a888;
	const auto CMenuConnectVTable = (void*)0x859300;
	const auto MessageBoxPtr = (CDesignSprite**)0x89a390;
	const auto CInputManagerClusterObj = (CInputManagerCluster*)0x89a248;
	const auto SceneID = (uint8_t*)0x8A0044;
	const auto Profile1Path = (VC9String*)0x899840;
	const auto Profile2Path = (VC9String*)0x89985C;
	const auto Hwnd = (HWND*)0x89FF90;
	const auto D3D9DevicePtr = (IDirect3DDevice9**)0x8a0e30;
	const auto D3DPresentParams = (D3DPRESENT_PARAMETERS*)0x8a0f68;
	const auto SfxPlay = (char (*)(int))0x43e1e0;
	const auto csvLoad = (bool(__thiscall*)(void*, const char* csvFile))(0x0040f370);
	const auto csvParseString = (void(__thiscall*)(void*, VC9String* output))(0x0040f7a0);
	const auto csvNextLine = (bool(__thiscall*)(void*))(0x0040f8a0);
	const auto csvDeconstructor = (void(__thiscall*)(void*))(0x0040ffd0);
	// Private addresses with wrapper functions around them
	namespace {
		const auto HostFun = (void(__thiscall*) (CMenuConnect*))0x0446a40;
		const auto JoinFun = (void(__thiscall*) (CMenuConnect*))0x0446b20;
	}
	// Constants
	// These two are based on what the Win32 api returns
	const float WindowHeight = 509.0;
	const float WindowWidth = 646.0;
	// Patches
	PatchMan::MultiPatch InputBlock, HideProfiles, InputWorkaround;
	// Variables
	auto OldCMenuConnectUpdate = (int (__thiscall*) (CMenuConnect*))nullptr;

	void Init() {
		InputBlock.AddPatch(0x0448e4a, "\x30\xC0\x90\x90\x90").AddPatch(0x0448e5d, "\x3C\x01\x90\x90").AddPatch(0x0449120, "\x85\xC9\x90\x90");
		HideProfiles.AddNOPs(0x0445e36, 7);
		// This one is kinda hackish, avoids the check if address.txt and history.txt exist
		// Since soku doesn't change choices otherwise.
		InputWorkaround.AddNOPs(0x0448f52, 8).AddNOPs(0x0448f2e, 8);
	}

	CMenuConnect *GetCMenuConnect() {
		if(*SceneID == 2) {
			if(UnknownGlobalObj->InMenuFlag) {
				CMenuConnect *menu = UnknownGlobalObj->UnknownContainer1->UnknownContainer2->CMenuConnectObj;
				if(menu->vftable == CMenuConnectVTable) {
					return menu;
				}
			}
		}
		return NULL;
	}

	CDesignSprite *GetMsgBox() {
		return *MessageBoxPtr;
	}

	CInputManagerCluster *GetInputManager() {
		return CInputManagerClusterObj;
	}

	string GetProfileName(int id) {
		if (id < 1 || id > 2) {
			return "";
		}
		string deckname = string(id == 1 ? *Profile1Path : *Profile2Path);
		int dotpos = deckname.find_last_of('.');
		return deckname.substr(0, dotpos);
	}

	// NOTE: This leaves the exit button
	void HideNativeMenu() {
		for (int i = 0; i < 6; i++) {
			GetCMenuConnect()->MenuItemSprites[i]->active = false;
		}
	}

	void SetupHost(int port, bool spectate) {
		CMenuConnect *menu = GetCMenuConnect();
		menu->Port = port;
		menu->Spectate = spectate;
		menu->Choice = 1;
		menu->Subchoice = 2;

		HostFun(menu);
		GetMsgBox()->active = false;
	}

	void JoinHost(const char *ip, int port, bool spectate = false) {
		CMenuConnect *menu = GetCMenuConnect();
		if (ip != NULL) {
			strcpy_s(menu->IPString, ip);
			menu->Port = port;
		}
		menu->Choice = 2;
		menu->Subchoice = (spectate ? 6 : 3);
		menu->UnknownJoinFlag = 1;
		menu->NotSpectateFlag = (char)!spectate;

		JoinFun(menu);
		GetMsgBox()->active = false;
	}

	// Resets choice/subchoice and clears any messagebox
	void ClearMenu() {
		GetMsgBox()->active = false;
		GetCMenuConnect()->Choice = 0;
		GetCMenuConnect()->Subchoice = 0;
	}

	void OnCMenuConnectUpdate(int (__fastcall *hook) (CMenuConnect*)) {
		OldCMenuConnectUpdate = (int(__thiscall *) (CMenuConnect*))PatchMan::HookVTable((DWORD*)(CMenuConnectVTable) + 3, (DWORD)hook);
	}
}; 

struct CsvParser {
	VC9String buffer;
	byte buf[200]; // Size is probably ~20, but not certain

	CsvParser() = default;

	bool loadFile(const char* path) {
		memset(this, 0, sizeof(this));
		return SokuMan::csvLoad(this, path);
	}

	~CsvParser() {
		SokuMan::csvDeconstructor(this);
	}

	std::string getNextCell() {
		VC9String temp;
		SokuMan::csvParseString(this, &temp);
		return string(temp);
	}

	bool goToNextLine() {
		return SokuMan::csvNextLine(this);
	}
};