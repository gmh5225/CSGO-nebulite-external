#include <iostream>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx11.h>
#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <thread>
#include "memory.h"
#include "offsets.h"
#include "cheats/BunnyHop.h"
#include "cheats/Glow.h"
#include "cheats/ESP.h"
#include "cheats/TriggerBot.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam)) {
		return 0L;
	}

	switch (message) {
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0L;
		default:
			return DefWindowProc(window, message, wParam, lParam);
	}
}

INT APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT showCmd) {
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = windowProcedure;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"External Overlay Class";

	RegisterClassExW(&wc);

	const HWND window = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
		wc.lpszClassName,
		L"External Overlay",
		WS_POPUP,
		0,
		0,
		1920,
		1080,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);

	SetLayeredWindowAttributes(window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

	{
		RECT clientArea{};
		GetClientRect(window, &clientArea);

		RECT windowArea{};
		GetWindowRect(window, &windowArea);

		POINT diff{};
		ClientToScreen(window, &diff);

		const MARGINS margins{
			windowArea.left + (diff.x - windowArea.left),
			windowArea.top + (diff.y - windowArea.top),
			clientArea.right,
			clientArea.bottom
		};

		DwmExtendFrameIntoClientArea(window, &margins);
	}

	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferDesc.RefreshRate.Numerator = 60U;
	sd.BufferDesc.RefreshRate.Denominator = 1U;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.SampleDesc.Count = 1U;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2U;
	sd.OutputWindow = window;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	constexpr D3D_FEATURE_LEVEL levels[2]{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0
	};

	ID3D11Device* device{ nullptr };
	ID3D11DeviceContext* deviceContext{ nullptr };
	IDXGISwapChain* swapChain{ nullptr };
	ID3D11RenderTargetView* renderTargetView{ nullptr };
	D3D_FEATURE_LEVEL level{};

	D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		levels,
		2U,
		D3D11_SDK_VERSION,
		&sd,
		&swapChain,
		&device,
		&level,
		&deviceContext
	);

	ID3D11Texture2D* backBuffer{ nullptr };
	swapChain->GetBuffer(0U, IID_PPV_ARGS(&backBuffer));
	
	if (backBuffer) {
		device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
		backBuffer->Release();
	}
	else {
		return 1;
	}

	ShowWindow(window, showCmd);
	UpdateWindow(window);

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(device, deviceContext);

	constexpr float color[4]{ 0.f, 0.f, 0.f, 0.f };

	bool running = true;
	while (running) {
		MSG msg;
		while (PeekMessage(&msg, nullptr, 8U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT) {
				running = false;
			}
		}

		if (!running) {
			break;
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		// the cheat

		const auto mem = Memory("csgo.exe");
		const auto client = mem.GetModuleAddress("client.dll");

		const auto localPlayer = mem.Read<std::uintptr_t>(client + offsets::dwLocalPlayer);

		if (localPlayer) {
			const auto localPlayerTeam = mem.Read<std::uintptr_t>(localPlayer + offsets::m_iTeamNum);

			Color glowColor{ 1.f, 1.f, 0.f, 1.f };

			for (int i = 1; i <= 32; i++) {
				const auto entity = mem.Read<std::uintptr_t>(client + offsets::dwEntityList + i * 0x10);

				if (!entity)
					continue;

				if (mem.Read<bool>(entity + offsets::m_bDormant))
					continue;

				if (mem.Read<std::uintptr_t>(entity + offsets::m_iTeamNum) == localPlayerTeam)
					continue;

				if (mem.Read<int>(entity + offsets::m_lifeState) != 0)
					continue;

				Glow(mem, client, entity, glowColor);
				ESP(mem, client, entity);
				TriggerBot(mem, client, localPlayer, localPlayerTeam);
				mem.Write<bool>(entity + offsets::m_bSpotted, true); // radar hack
			}

			BunnyHop(mem, client, localPlayer);
		}

		// rendering imgui

		ImGui::Render();
		deviceContext->OMSetRenderTargets(1U, &renderTargetView, nullptr);
		deviceContext->ClearRenderTargetView(renderTargetView, color);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		swapChain->Present(0U, 0U);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	if (swapChain)
		swapChain->Release();

	if (deviceContext)
		deviceContext->Release();

	if (device)
		device->Release();

	if (renderTargetView)
		renderTargetView->Release();

	DestroyWindow(window);
	UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}