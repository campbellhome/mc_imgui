// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "imgui_core.h"
#include "bbclient/bb_time.h"
#include "cmdline.h"
#include "common.h"
#include "fonts.h"
#include "imgui_themes.h"
#include "keys.h"
#include "message_box.h"
#include "sb.h"
#include "time_utils.h"
#include "ui_message_box.h"
#include "wrap_imgui.h"
#include "wrap_shellscalingapi.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "xinput9_1_0.lib")

static HRESULT SetProcessDpiAwarenessShim(_In_ PROCESS_DPI_AWARENESS value)
{
	HMODULE hModule = GetModuleHandleA("shcore.dll");
	if(hModule) {
		typedef HRESULT(WINAPI * Proc)(_In_ PROCESS_DPI_AWARENESS value);
		Proc proc = (Proc)(void *)(GetProcAddress(hModule, "SetProcessDpiAwareness"));
		if(proc) {
			return proc(value);
		}
	}
	return STG_E_UNIMPLEMENTEDFUNCTION;
}

typedef struct tag_Imgui_Core_Window {
	HWND hwnd;
	LPDIRECT3DDEVICE9 pd3dDevice;
} Imgui_Core_Window;

static LPDIRECT3D9 s_pD3D;
static D3DPRESENT_PARAMETERS g_d3dpp;
static WNDCLASSEX s_wc;
static Imgui_Core_Window s_wnd;
static bool g_hasFocus;
static bool g_trackingMouse;
static int g_dpi = USER_DEFAULT_SCREEN_DPI;
static float g_dpiScale;
static b32 g_bTextShadows;
static bool g_needUpdateDpiDependentResources;
static sb_t g_colorscheme;
static int g_appRequestRenderCount;
static bool g_shuttingDown;
static bool g_bDirtyWindowPlacement;
static bool g_setCmdline;
static bool g_bCloseHidesWindow;

extern "C" b32 Imgui_Core_Init(const char *cmdline)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	Style_Init();

	SetProcessDpiAwarenessShim(PROCESS_PER_MONITOR_DPI_AWARE);

	s_pD3D = Direct3DCreate9(D3D_SDK_VERSION);

	if(s_pD3D) {
		g_d3dpp.Windowed = TRUE;
		g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
		g_d3dpp.EnableAutoDepthStencil = TRUE;
		g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
		g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // Present with vsync
	}

	if(!cmdline_argc()) {
		g_setCmdline = true;
		cmdline_init_composite(cmdline);
	}

	return s_pD3D != nullptr;
}

void Imgui_Core_ResetD3D()
{
	if(!s_wnd.pd3dDevice)
		return;
	ImGui_ImplDX9_InvalidateDeviceObjects();
#ifdef NDEBUG
	s_wnd.pd3dDevice->Reset(&g_d3dpp);
#else
	HRESULT hr = s_wnd.pd3dDevice->Reset(&g_d3dpp);
	IM_ASSERT(hr != D3DERR_INVALIDCALL);
#endif
	ImGui_ImplDX9_CreateDeviceObjects();
}

extern "C" void Imgui_Core_Shutdown(void)
{
	Fonts_Shutdown();
	mb_shutdown();

	if(g_setCmdline) {
		cmdline_shutdown();
	}

	sb_reset(&g_colorscheme);

	ImGui::DestroyContext();

	if(s_pD3D) {
		s_pD3D->Release();
		s_pD3D = nullptr;
	}
}

extern "C" b32 Imgui_Core_HasFocus(void)
{
	return g_hasFocus;
}

extern "C" float Imgui_Core_GetDpiScale(void)
{
	return g_dpiScale;
}

extern "C" void Imgui_Core_SetCloseHidesWindow(b32 bCloseHidesWindow)
{
	g_bCloseHidesWindow = bCloseHidesWindow;
}

extern "C" void Imgui_Core_HideUnhideWindow(void)
{
	if(s_wnd.hwnd) {
		WINDOWPLACEMENT wp = { BB_EMPTY_INITIALIZER };
		wp.length = sizeof(wp);
		GetWindowPlacement(s_wnd.hwnd, &wp);
		if(wp.showCmd == SW_SHOWMINIMIZED) {
			ShowWindow(s_wnd.hwnd, SW_RESTORE);
			Imgui_Core_BringWindowToFront();
		} else if(!IsWindowVisible(s_wnd.hwnd)) {
			ShowWindow(s_wnd.hwnd, SW_SHOW);
			Imgui_Core_BringWindowToFront();
		} else {
			ShowWindow(s_wnd.hwnd, SW_HIDE);
		}
	}
}

extern "C" void Imgui_Core_HideWindow(void)
{
	if(s_wnd.hwnd) {
		ShowWindow(s_wnd.hwnd, SW_HIDE);
	}
}

extern "C" void Imgui_Core_UnhideWindow(void)
{
	if(s_wnd.hwnd) {
		if(IsIconic(s_wnd.hwnd)) {
			ShowWindow(s_wnd.hwnd, SW_RESTORE);
		} else {
			ShowWindow(s_wnd.hwnd, SW_SHOW);
		}
		Imgui_Core_RequestRender();
	}
}

extern "C" void Imgui_Core_BringWindowToFront(void)
{
	if(s_wnd.hwnd) {
		if(!BringWindowToTop(s_wnd.hwnd)) {
			char *errorMessage = nullptr;
			DWORD lastError = GetLastError();
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMessage, 0, NULL);
			BB_ERROR("Window", "Failed to bring window to front:\n  Error %u (0x%8.8X): %s",
			         lastError, lastError, errorMessage ? errorMessage : "unable to format error message");
			if(errorMessage) {
				LocalFree(errorMessage);
			}
		}
		SetForegroundWindow(s_wnd.hwnd);
		Imgui_Core_RequestRender();
	}
}

extern "C" void Imgui_Core_SetDpiScale(float dpiScale)
{
	if(g_dpiScale != dpiScale) {
		g_dpiScale = dpiScale;
		Imgui_Core_QueueUpdateDpiDependentResources();
	}
}

extern "C" void Imgui_Core_SetTextShadows(b32 bTextShadows)
{
	g_bTextShadows = bTextShadows;
}

extern "C" b32 Imgui_Core_GetTextShadows(void)
{
	return g_bTextShadows;
}

extern "C" void Imgui_Core_RequestRender(void)
{
	g_appRequestRenderCount = 3;
}

extern "C" b32 Imgui_Core_GetAndClearRequestRender(void)
{
	b32 ret = g_appRequestRenderCount > 0;
	g_appRequestRenderCount = BB_MAX(0, g_appRequestRenderCount - 1);
	return ret;
}

extern "C" void Imgui_Core_DirtyWindowPlacement(void)
{
	g_bDirtyWindowPlacement = true;
}

extern "C" b32 Imgui_Core_GetAndClearDirtyWindowPlacement(void)
{
	b32 ret = g_bDirtyWindowPlacement;
	g_bDirtyWindowPlacement = false;
	return ret;
}

extern "C" void Imgui_Core_RequestShutDown(void)
{
	g_shuttingDown = true;
}

extern "C" b32 Imgui_Core_IsShuttingDown(void)
{
	return g_shuttingDown;
}

extern "C" void Imgui_Core_SetColorScheme(const char *colorscheme)
{
	if(strcmp(sb_get(&g_colorscheme), colorscheme)) {
		sb_reset(&g_colorscheme);
		sb_append(&g_colorscheme, colorscheme);
		Imgui_Core_QueueUpdateDpiDependentResources();
	}
}

extern "C" const char *Imgui_Core_GetColorScheme(void)
{
	return sb_get(&g_colorscheme);
}

void UpdateDpiDependentResources()
{
	Fonts_InitFonts();
	Imgui_Core_ResetD3D();
	Style_Apply(sb_get(&g_colorscheme));
}

extern "C" void Imgui_Core_QueueUpdateDpiDependentResources(void)
{
	g_needUpdateDpiDependentResources = true;
}

BOOL EnableNonClientDpiScalingShim(_In_ HWND hwnd)
{
	HMODULE hModule = GetModuleHandleA("User32.dll");
	if(hModule) {
		typedef BOOL(WINAPI * Proc)(_In_ HWND hwnd);
		Proc proc = (Proc)(void *)(GetProcAddress(hModule, "EnableNonClientDpiScaling"));
		if(proc) {
			return proc(hwnd);
		}
	}
	return FALSE;
}

UINT GetDpiForWindowShim(_In_ HWND hwnd)
{
	HMODULE hModule = GetModuleHandleA("User32.dll");
	if(hModule) {
		typedef UINT(WINAPI * Proc)(_In_ HWND hwnd);
		Proc proc = (Proc)(void *)(GetProcAddress(hModule, "GetDpiForWindow"));
		if(proc) {
			return proc(hwnd);
		}
	}
	return USER_DEFAULT_SCREEN_DPI;
}

static Imgui_Core_UserWndProc *g_userWndProc;
void Imgui_Core_SetUserWndProc(Imgui_Core_UserWndProc *wndProc)
{
	g_userWndProc = wndProc;
}

LRESULT WINAPI Imgui_Core_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(!s_pD3D) {
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	switch(msg) {
	case WM_NCCREATE:
		EnableNonClientDpiScalingShim(hWnd);
		g_dpi = (int)GetDpiForWindowShim(hWnd);
		g_dpiScale = g_dpi / (float)USER_DEFAULT_SCREEN_DPI;
		Style_Apply(sb_get(&g_colorscheme));
		break;
	case WM_DPICHANGED: {
		g_dpi = HIWORD(wParam);
		g_dpiScale = g_dpi / (float)USER_DEFAULT_SCREEN_DPI;
		UpdateDpiDependentResources();

		RECT *const prcNewWindow = (RECT *)lParam;
		SetWindowPos(hWnd,
		             NULL,
		             prcNewWindow->left,
		             prcNewWindow->top,
		             prcNewWindow->right - prcNewWindow->left,
		             prcNewWindow->bottom - prcNewWindow->top,
		             SWP_NOZORDER | SWP_NOACTIVATE);
		break;
	}
	case WM_SETFOCUS:
		g_hasFocus = true;
		//ImGui::GetIO().MouseDrawCursor = true;
		break;
	case WM_KILLFOCUS: {
		g_hasFocus = false;
		//ImGui::GetIO().MouseDrawCursor = false;
		key_clear_all();
		auto &keysDown = ImGui::GetIO().KeysDown;
		memset(&keysDown, 0, sizeof(keysDown));
	} break;
	case WM_MOUSEMOVE:
		Imgui_Core_RequestRender();
		if(!g_trackingMouse) {
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(tme);
			tme.hwndTrack = hWnd;
			tme.dwFlags = TME_LEAVE;
			tme.dwHoverTime = HOVER_DEFAULT;
			TrackMouseEvent(&tme);
			g_trackingMouse = true;
		}
		break;
	case WM_MOUSELEAVE:
		Imgui_Core_RequestRender();
		g_trackingMouse = false;
		ImGui::GetIO().MousePos = ImVec2(-1, -1);
		for(int i = 0; i < BB_ARRAYSIZE(ImGui::GetIO().MouseDown); ++i) {
			ImGui::GetIO().MouseDown[i] = false;
		}
		break;
	case WM_KEYDOWN:
		if(wParam >= VK_F1 && wParam <= VK_F12) {
			key_on_pressed((key_e)(Key_F1 + wParam - VK_F1));
		} else if(wParam == VK_OEM_3) {
			key_on_pressed(Key_Tilde);
		}
		break;
	case WM_KEYUP:
		if(wParam >= VK_F1 && wParam <= VK_F12) {
			key_on_released((key_e)(Key_F1 + wParam - VK_F1));
		} else if(wParam == VK_OEM_3) {
			key_on_released(Key_Tilde);
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_CHAR:
		Imgui_Core_RequestRender();
		break;
	case WM_CLOSE:
		if(g_bCloseHidesWindow) {
			ShowWindow(hWnd, SW_HIDE);
			return 0;
		}
	default:
		break;
	}

	IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	if(g_userWndProc) {
		LRESULT userResult = (*g_userWndProc)(hWnd, msg, wParam, lParam);
		if(userResult) {
			return userResult;
		}
	}

	switch(msg) {
	case WM_MOVE:
		Imgui_Core_RequestRender();
		Imgui_Core_DirtyWindowPlacement();
		break;
	case WM_SIZE:
		Imgui_Core_RequestRender();
		Imgui_Core_DirtyWindowPlacement();
		if(wParam != SIZE_MINIMIZED) {
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			Imgui_Core_ResetD3D();
		}
		return 0;
	case WM_SYSCOMMAND:
		if((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

extern "C" HWND Imgui_Core_InitWindow(const char *classname, const char *title, HICON icon, WINDOWPLACEMENT wp)
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, Imgui_Core_WndProc, 0L, 0L, GetModuleHandle(NULL), icon, LoadCursor(NULL, IDC_ARROW), NULL, NULL, classname, NULL };
	s_wc = wc;
	RegisterClassEx(&s_wc);

	s_wnd.hwnd = CreateWindow(classname, title, WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, s_wc.hInstance, NULL);
	if(wp.rcNormalPosition.right > wp.rcNormalPosition.left) {
		if(wp.showCmd == SW_SHOWMINIMIZED) {
			wp.showCmd = SW_SHOWNORMAL;
		}
		SetWindowPlacement(s_wnd.hwnd, &wp);
	}
	BB_LOG("Startup", "hwnd: %p", s_wnd.hwnd);

	if(s_wnd.hwnd) {
		if(s_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, s_wnd.hwnd, D3DCREATE_MIXED_VERTEXPROCESSING, &g_d3dpp, &s_wnd.pd3dDevice) < 0) {
			s_wnd.pd3dDevice = nullptr;
		} else {
			ImGui_ImplWin32_Init(s_wnd.hwnd);
			ImGui_ImplDX9_Init(s_wnd.pd3dDevice);
			Fonts_InitFonts();
			if(wp.showCmd == SW_HIDE && !g_bCloseHidesWindow) {
				ShowWindow(s_wnd.hwnd, SW_SHOWDEFAULT);
			} else {
				ShowWindow(s_wnd.hwnd, (int)wp.showCmd);
			}
			UpdateWindow(s_wnd.hwnd);
			Time_StartNewFrame();
		}
	}

	return s_wnd.hwnd;
}

extern "C" void Imgui_Core_ShutdownWindow(void)
{
	if(s_wnd.pd3dDevice) {
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		s_wnd.pd3dDevice->Release();
	}
}

b32 Imgui_Core_BeginFrame(void)
{
	MSG msg = { BB_EMPTY_INITIALIZER };
	if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if(msg.message == WM_QUIT) {
			Imgui_Core_RequestShutDown();
		}
		return false;
	}
	if(g_needUpdateDpiDependentResources) {
		g_needUpdateDpiDependentResources = false;
		UpdateDpiDependentResources();
	}
	if(Fonts_UpdateAtlas()) {
		Imgui_Core_ResetD3D();
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	BB_TICK();

	return true;
}

void Imgui_Core_EndFrame(ImVec4 clear_col)
{
	UIMessageBox_Update();
	ImGui::EndFrame();

	bool requestRender = Imgui_Core_GetAndClearRequestRender() || key_is_any_down_or_released_this_frame();
	if(g_hasFocus) {
		ImGuiIO &io = ImGui::GetIO();
		for(bool mouseDown : io.MouseDown) {
			if(mouseDown) {
				requestRender = true;
				break;
			}
		}
		if(!requestRender) {
			for(bool keyDown : io.KeysDown) {
				if(keyDown) {
					requestRender = true;
					break;
				}
			}
		}
	}
	if(ImGui::GetPlatformIO().Viewports.size() > 1) {
		requestRender = true;
	}

	// ImGui Rendering
	if(requestRender) {
		s_wnd.pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
		s_wnd.pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
		s_wnd.pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_col.x * 255.0f), (int)(clear_col.y * 255.0f), (int)(clear_col.z * 255.0f), (int)(clear_col.w * 255.0f));
		s_wnd.pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
		if(s_wnd.pd3dDevice->BeginScene() >= 0) {
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			s_wnd.pd3dDevice->EndScene();
		} else {
			ImGui::EndFrame();
			Imgui_Core_RequestRender();
		}
		if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
		HRESULT hr = s_wnd.pd3dDevice->Present(NULL, NULL, NULL, NULL);
		if(FAILED(hr)) {
			bb_sleep_ms(100);
			Imgui_Core_ResetD3D();
			Imgui_Core_RequestRender();
		}
	} else {
		if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
		}
		ImGui::EndFrame();
		bb_sleep_ms(15);
	}
	Time_StartNewFrame();
}
