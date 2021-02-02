#pragma warning(disable:4996)

#include "PingMan.hpp" 
#include <SokuLib.h>
#include <curl/curl.h>
#include <iostream>
#include <json.hpp>
#include <thread>
#include "Host.hpp"
#include "HostingOptions.hpp"
#include "WebHandler.hpp"
#include "SokuAPI.hpp"
#include "Menu.hpp"
#include "Hostlist.hpp"

using namespace std;
using namespace Soku;

#define DEBUG false

bool firstTime = true;
bool hosting = false;
bool inMenu = false;

void Update() {
    while (true) {
        if (inMenu == true)
            Hostlist::Update();
    }
    this_thread::sleep_for(1ms);
}

thread *updateThread;

SOKU_MODULE EntryPoint()
{
    SokuAPI::Init();

    WebHandler::Init();
    atexit(WebHandler::Cleanup);

    PingMan::Init();
    atexit(PingMan::Cleanup);

    HostingOptions::LoadConfig();

    Menu::Init();
    Hostlist::Init();

    updateThread = new thread(Update);

    if (DEBUG) {
        AllocConsole();
        SetConsoleTitleA("Hostlist Debug");
        freopen("CONOUT$", "w", stdout);
    }

    Menu::AddItem("Host", []() {
        if (hosting == true) {
            Status::Normal("Host aborted.");
            SokuAPI::ClearMenu();
        }
        else {
            Status::Normal("Hosting...", Status::forever);
            SokuAPI::SetupHost(HostingOptions::port, HostingOptions::spectate);
            
            if (HostingOptions::publicHost) {
                JSON data = {
                    "profile_name", SokuAPI::GetProfileName(1),
                    "message", HostingOptions::message,
                    "port", HostingOptions::port,
                };
                //TODO: Add proper error checking.
                WebHandler::Put("http://delthas.fr:14762/games", data.dump());
            }
        }
        hosting = !hosting;
    });

    Menu::AddItem("Join", []() {
        Hostlist::active = true;
        //Prevents the hostlist from instantly registering the input
        //Very ugly, but eh...
        SokuAPI::GetInputManager()->P1.A = 10;
        SokuAPI::InputBlock.Toggle(true);
        SokuAPI::ClearMenu();
    });

    Menu::AddItem("Refresh", []() {
        Hostlist::oldTime = 0;
        SokuAPI::ClearMenu();
    });

    Menu::AddItem("Options", []() {
        HostingOptions::enabled = !HostingOptions::enabled;
        SokuAPI::InputBlock.Toggle(true);
        SokuAPI::ClearMenu();
    });

    Menu::AddItem("Join from clipboard", []() {
        Status::Normal("Joining...", Status::forever);
        SokuAPI::JoinHost(NULL, 0);
    });

    Menu::AddItem("Profile select");

    Menu::AddEventHandler(Menu::Event::AlreadyPlaying, [&]() {
        Status::Normal("Match in progress, spectating...");
        SokuAPI::JoinHost(NULL, 0, true);
    });

    Menu::AddEventHandler(Menu::Event::ConnectionFailed, [&]() {
        Hostlist::joining = false;
        Status::Error("Failed to connect.");
        SokuAPI::SfxPlay(SFX_BACK);
        SokuAPI::ClearMenu();
    });

    Soku::SubscribeEvent(SokuEvent::Render, [](SokuData::RenderData& data) {
        if (data.menu == MENU::VS_NETWORK)
        {
            if (firstTime) {
                Menu::OnMenuOpen();
                Hostlist::OnMenuOpen();

                HostingOptions::enabled = false;
                SokuAPI::InputBlock.Toggle(false);
                SokuAPI::InputWorkaround.Toggle(true);
                hosting = false;
                inMenu = true;
                
                firstTime = false;
            }

            CMenuConnect* menu = SokuAPI::GetCMenuConnect();
            CDesignSprite* msgbox = SokuAPI::GetMsgBox();
            CInputManagerCluster* input = SokuAPI::GetInputManager();

            if ((menu->Choice == 1 || menu->Choice == 2) && menu->Subchoice == 255) return;

            Menu::Render();
            Hostlist::Render();
            HostingOptions::Render(); 

            //Debug window
            if (DEBUG) {
                ImGui::Begin("DebugInfo", 0);
                ImGui::Text("DebugInfo:");
                ImGui::Text("   MsgBox addr: %08X", msgbox);
                ImGui::Text("   MsgBox active: %d", msgbox->active);
                ImGui::Text("   MsgBox X: %f", msgbox->x);
                ImGui::Text("   MsgBox Y: %f", msgbox->y);
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

            Menu::HandleInput();
            Hostlist::HandleInput();

            if (SokuAPI::InputBlock.Check() && (input->P1.B == 1 || (!HostingOptions::enabled && !Hostlist::active))) {
                Hostlist::active = false;
                HostingOptions::enabled = false;
                SokuAPI::InputBlock.Toggle(false);
                SokuAPI::SfxPlay(SFX_BACK);
                input->P1.B = 10; 
            }
        }
        else {
            firstTime = true;
            inMenu = false;
        }  
    }); 
}