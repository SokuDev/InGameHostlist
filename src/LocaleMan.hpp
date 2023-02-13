#pragma once
#include <string>
using namespace std;

// Stores strings, handles localization.
namespace LocaleMan {

	unordered_map<string, string> Text = { 
		{ "IntroTitle","Introduction" },
		{ "IntroBody","Welcome to InGame-Hostlist, \n"
			"A mod that lets you use a hostlist from inside the game.\n\n"
			"Usage instructions: \n"
			"Before hosting / joining please go into the options menu,\n"
			"to set your port and spectate options, as the game will\n"
			"no longer propmpt you for them before each host.\n"
			"You will also find settings for your host messageand\n"
			"whether you want your games to appear on the hostlist or not.\n"
			"After that you can enter the hostlist via the 'Join' button,\n"
			"or just host by pressing 'Host'.\n"
			"Note: For ease of viewing you can switch between the Playing\n"
			"and Waiting tabs from anywhere in the menu.\n\n"
			"Also please remember to change your profile name before playing\n"
			"online, have fun!\n\n"
			"Press A / B to continue.\n"},
		{ "HostMessageTitle","Please enter your host message" },
		{ "HostMessageBody","Host Message: " },
		{ "Confirm","Confirm" },
		{ "Host","Host" },
		{ "HostAborted","Host aborted." },
		{ "EditMeWarning","Consider changing your profile name before hosting, or press host again." },
		{ "Hosting","Hosting..." },
		{ "Join","Join" },
		{ "Refresh","Refresh" },
		{ "Options","Options" },
		{ "JoinClipboard","Join from clipboard" },
		{ "ClipboardTitle","Clipboard" },
		{ "ClipboardBody","Do you want to join or spectate this host?" },
		{ "Spectate","Spectate" },
		{ "ProfileSelect","Profile select" },
		{ "MatchInProgress","Match in progress, spectating..." },
		{ "ConnectionFailed","Failed to connect." },
		{ "ConnectionError","Connection error: " },
		{ "RateLimitError","Rate limit reached." },
		{ "UnknownError","Error code: " },
		{ "HostlistWaiting","Waiting(%d)" },
		{ "HostlistPlaying","Playing(%d)" },
		{ "HostlistName","Name" },
		{ "HostlistMessage","Message" },
		{ "HostlistPing","Ping" },
		{ "HostlistPlayer1","Player 1" },
		{ "HostlistPlayer2","Player 2" },
		{ "HostlistNoHosts","There are currently no hosts." },
		{ "HostlistJoining","Joining..." },
		{ "HostlistJoiningAborted","Joining aborted." },
		{ "OptionsPort","Port" },
		{ "OptionsSpectatable","Spectatable?    " },
		{ "OptionsPostToBot","Post to bot?    " },
		{ "OptionsSFX","SFX on new host?" },
		{ "OptionsMsgPrompt","Prompt for msg? " },
		{ "OptionsMsg","Host Messsage:" } ,
		{ "OptionsLanguage","Language:" },
	};

	void Init() {
		CsvParser csv;

		bool fileExists = csv.loadFile("modules-InGameHostlist.csv");
		if (fileExists) {
			string key, value;
			do {
				key = csv.getNextCell();
				value = csv.getNextCell();
				Text[key] = value;
			} while (csv.goToNextLine());
		}
	}
}