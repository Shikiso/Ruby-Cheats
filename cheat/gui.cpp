#include "gui.h"
#include "globals.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

#include <iostream>
#include <fstream>
#include <string>

void save_config() {
	std::ofstream configFile;
	configFile.open(globals::configFile);
	configFile << globals::glow << "\n";
	configFile << globals::glowColor << "\n";
	configFile << globals::teamGlow << "\n";
	configFile << globals::teamGlowColor << "\n";

	configFile << globals::charms << "\n";

	configFile << globals::radar << "\n";

	configFile << globals::aimbot << "\n";
	configFile << globals::aimbot_smoothness << "\n";
	configFile << globals::inView << "\n";

	configFile << globals::noFlash << "\n";

	configFile << globals::bhop << "\n";

	configFile << globals::triggerbot << "\n";
	configFile << globals::constantTriggerBot << "\n";

	configFile.close();
}

bool string_to_bool(std::string stringVar) {
	if (stringVar == "1") {
		return true;
	}
	return false;
}

void load_config() {
	std::string text;
	bool BooleanVariable;
	float ColourVariable[] = { 0.f, 0.f, 0.f, 1.f };
	int line = 1;
	std::ifstream configFile(globals::configFile);

	while (getline(configFile, text)) {
		if (line == 1){
			globals::glow = string_to_bool(text);
		}
		else if (line == 2) {

		}

		line++;
	}

	configFile.close();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) 
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;

	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter); 
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}

	}return 0;

	}

	return DefWindowProc(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(const char* windowName) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = "class001";
	windowClass.hIconSm = 0;

	RegisterClassEx(&windowClass);

	window = CreateWindowEx(
		0,
		"class001",
		windowName,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.message == WM_QUIT)
		{
			isRunning = !isRunning;
			return;
		}
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	// Handle loss of D3D9 device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

void gui::Render() noexcept
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });

	ImGuiStyle& style = ImGui::GetStyle();

	style.Colors[ImGuiCol_TitleBgActive] = ImColor(255, 0, 0);

	style.Colors[ImGuiCol_Button] = ImColor(255, 0, 0);
	style.Colors[ImGuiCol_ButtonActive] = ImColor(255, 0, 0);
	style.Colors[ImGuiCol_ButtonHovered] = ImColor(100, 0, 0);

	style.Colors[ImGuiCol_CheckMark] = ImColor(255, 255, 255);

	style.Colors[ImGuiCol_FrameBg] = ImColor(255, 0, 0);
	style.Colors[ImGuiCol_FrameBgHovered] = ImColor(100, 0, 0);
	style.Colors[ImGuiCol_FrameBgActive] = ImColor(100, 0, 0);

	style.Colors[ImGuiCol_SliderGrab] = ImColor(0, 0, 0);
	style.Colors[ImGuiCol_SliderGrabActive] = ImColor(0, 0, 0);

	style.Colors[ImGuiCol_WindowBg] = ImColor(0, 0, 0);
	style.WindowRounding = 0.25;

	ImGui::Begin(
		"Ruby Cheats - External",
		&isRunning,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove
	);

	/*
	ImGui::Text("Config");
	if (ImGui::Button("Save")) {
		save_config();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load")) {

	}
	ImGui::SameLine();
	ImGui::Text("Config File: Ruby-Config");

	ImGui::NewLine();
	*/

	ImGui::Text("Aimbot Key - LEFT ALT");
	ImGui::Text("Aimbot Toggle Key - F1");
	ImGui::Checkbox("Aimbot", &globals::aimbot);
	ImGui::SameLine();
	ImGui::Checkbox("In View", &globals::inView);
	ImGui::Checkbox("Smoothness", &globals::aimbot_smoothness_on);
	ImGui::SliderFloat("", &globals::aimbot_smoothness, 1.0f, 5.0f);

	ImGui::NewLine();
	ImGui::Text("Triggerbot Key - LEFT SHIFT");
	ImGui::Text("Triggerbot Toggle Key - F2");
	ImGui::Checkbox("Triggerbot", &globals::triggerbot);
	ImGui::SameLine();
	ImGui::Checkbox("Constant Triggerbot", &globals::constantTriggerBot);

	ImGui::NewLine();

	ImGui::Checkbox("Glow", &globals::glow);
	ImGui::ColorEdit4("Glow Colour", globals::glowColor);

	ImGui::NewLine();

	ImGui::Checkbox("Team Glow", &globals::teamGlow);
	ImGui::ColorEdit4("Team Glow Colour", globals::teamGlowColor);

	ImGui::NewLine();

	ImGui::Checkbox("Charms", &globals::charms);

	ImGui::SameLine();

	ImGui::Checkbox("No Flash", &globals::noFlash);

	ImGui::NewLine();

	ImGui::Checkbox("Radar", &globals::radar);

	ImGui::SameLine();

	ImGui::Checkbox("Bhop", &globals::bhop);

	ImGui::NewLine();
	ImGui::NewLine();

	ImGui::Text("Shikiso\nhttps://github.com/Shikiso");

	ImGui::End();
}
