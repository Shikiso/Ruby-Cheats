#include "gui.h"
#include "hacks.h"
#include "globals.h"

#include <thread>

int __stdcall wWinMain(
	HINSTANCE instance,
	HINSTANCE previousInstance,
	PWSTR arguments,
	int commandShow)
{
	Memory mem("csgo.exe");

	globals::clientAddress = mem.GetModuleAddress("client.dll");
	globals::engineAddress = mem.GetModuleAddress("engine.dll");

	std::thread(hacks::AimbotThread, mem).detach();
	std::thread(hacks::PlayerThread, mem).detach();
	std::thread(hacks::TriggerbotThread, mem).detach();
	std::thread(hacks::VisualsThread, mem).detach();

	gui::CreateHWindow("Ruby Menu");
	gui::CreateDevice();
	gui::CreateImGui();

	while (gui::isRunning)
	{
		gui::BeginRender();
		gui::Render();
		gui::EndRender();

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	
	gui::DestroyImGui();
	gui::DestroyDevice();
	gui::DestroyHWindow();

	return EXIT_SUCCESS;
}
