// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "mc_imgui_example.h"
#include "cmdline.h"
#include "common.h"
#include "crt_leak_check.h"
#include "fonts.h"
#include "imgui_core.h"
#include "message_box.h"

static bool s_showImguiDemo;
static bool s_showImguiAbout;
static bool s_showImguiMetrics;
static bool s_showImguiUserGuide;
static bool s_showImguiStyleEditor;

static const char *s_colorschemes[] = {
	"ImGui Dark",
	"Light",
	"Classic",
	"Visual Studio Dark",
	"Windows",
};

void MC_Imgui_Example_MainMenuBar(void)
{
	if(ImGui::BeginMainMenuBar()) {
		if(ImGui::BeginMenu("File")) {
			if(ImGui::MenuItem("Exit")) {
				Imgui_Core_RequestShutDown();
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Edit")) {
			if(ImGui::MenuItem("Config")) {
				BB_LOG("UI::Menu::Config", "UIConfig_Open");
				messageBox mb = { BB_EMPTY_INITIALIZER };
				sdict_add_raw(&mb.data, "title", "Config error");
				sdict_add_raw(&mb.data, "text", "Failed to open config UI.\nThis is an example that doesn't have config save/load.");
				sdict_add_raw(&mb.data, "button1", "Ok");
				mb_queue(mb);
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Debug")) {
			if(ImGui::BeginMenu("Color schemes")) {
				const char *colorscheme = Imgui_Core_GetColorScheme();
				for(int i = 0; i < BB_ARRAYSIZE(s_colorschemes); ++i) {
					bool bSelected = !strcmp(colorscheme, s_colorschemes[i]);
					if(ImGui::MenuItem(s_colorschemes[i], nullptr, &bSelected)) {
						Imgui_Core_SetColorScheme(s_colorschemes[i]);
					}
				}
				ImGui::EndMenu();
			}
			if(ImGui::BeginMenu("DEBUG Scale")) {
				float dpiScale = Imgui_Core_GetDpiScale();
				if(ImGui::MenuItem("1", nullptr, dpiScale == 1.0f)) {
					Imgui_Core_SetDpiScale(1.0f);
				}
				if(ImGui::MenuItem("1.25", nullptr, dpiScale == 1.25f)) {
					Imgui_Core_SetDpiScale(1.25f);
				}
				if(ImGui::MenuItem("1.5", nullptr, dpiScale == 1.5f)) {
					Imgui_Core_SetDpiScale(1.5f);
				}
				if(ImGui::MenuItem("1.75", nullptr, dpiScale == 1.75f)) {
					Imgui_Core_SetDpiScale(1.75f);
				}
				if(ImGui::MenuItem("2", nullptr, dpiScale == 2.0f)) {
					Imgui_Core_SetDpiScale(2.0f);
				}
				ImGui::EndMenu();
			}
			Fonts_Menu();
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Imgui Help")) {
			ImGui::MenuItem("Demo", nullptr, &s_showImguiDemo);
			ImGui::MenuItem("About", nullptr, &s_showImguiAbout);
			ImGui::MenuItem("Metrics", nullptr, &s_showImguiMetrics);
			ImGui::MenuItem("User Guide", nullptr, &s_showImguiUserGuide);
			ImGui::MenuItem("Style Editor", nullptr, &s_showImguiStyleEditor);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void MC_Imgui_Example_Update(void)
{
	MC_Imgui_Example_MainMenuBar();

	if(s_showImguiDemo) {
		ImGui::ShowDemoWindow(&s_showImguiDemo);
	}
	if(s_showImguiAbout) {
		ImGui::ShowAboutWindow(&s_showImguiAbout);
	}
	if(s_showImguiMetrics) {
		ImGui::ShowMetricsWindow(&s_showImguiMetrics);
	}
	if(s_showImguiUserGuide) {
		ImGui::ShowUserGuide();
	}
	if(s_showImguiStyleEditor) {
		ImGui::ShowStyleEditor();
	}
}

int CALLBACK WinMain(_In_ HINSTANCE /*Instance*/, _In_opt_ HINSTANCE /*PrevInstance*/, _In_ LPSTR CommandLine, _In_ int /*ShowCode*/)
{
	crt_leak_check_init();

	cmdline_init_composite(CommandLine);

	BB_INIT_WITH_FLAGS("mc_imgui_example", kBBInitFlag_None);
	BB_THREAD_SET_NAME("main");
	BB_LOG("Startup", "Arguments: %s", CommandLine);

	Imgui_Core_Init();

	WINDOWPLACEMENT wp = { BB_EMPTY_INITIALIZER };
	if(Imgui_Core_InitWindow("mc_imgui_example_wndclass", "mc_imgui_example", LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAINICON)), wp)) {

		while(!Imgui_Core_IsShuttingDown()) {
			if(Imgui_Core_GetAndClearDirtyWindowPlacement()) {
				// config_getwindowplacement();
			}
			if(Imgui_Core_BeginFrame()) {
				MC_Imgui_Example_Update();
				ImVec4 clear_col = ImColor(34, 35, 34);
				Imgui_Core_EndFrame(clear_col);
			}
		}
		Imgui_Core_ShutdownWindow();
	}

	Imgui_Core_Shutdown();

	cmdline_shutdown();

	return 0;
}
