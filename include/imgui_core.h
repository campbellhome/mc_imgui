// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"

#if defined(__cplusplus)
#include "wrap_imgui.h"
b32 Imgui_Core_BeginFrame(void);
void Imgui_Core_EndFrame(ImVec4 clear_col);
extern "C" {
#endif

b32 Imgui_Core_Init(void);
void Imgui_Core_Shutdown(void);

b32 Imgui_Core_InitWindow(const char *classname, const char *title, HICON icon, WINDOWPLACEMENT wp);
void Imgui_Core_ShutdownWindow(void);

void Imgui_Core_SetDpiScale(float dpiScale);
float Imgui_Core_GetDpiScale(void);

void Imgui_Core_SetTextShadows(b32 bTextShadows);
b32 Imgui_Core_GetTextShadows(void);

b32 Imgui_Core_HasFocus(void);

void Imgui_Core_RequestRender(void);

void Imgui_Core_RequestShutDown(void);
b32 Imgui_Core_IsShuttingDown(void);

void Imgui_Core_SetColorScheme(const char *colorscheme);
const char *Imgui_Core_GetColorScheme(void);

b32 Imgui_Core_GetAndClearDirtyWindowPlacement(void);

void Imgui_Core_QueueUpdateDpiDependentResources(void);

//extern b32 App_Init(const char *commandLine);
//extern void App_Shutdown(void);
//extern void App_Update(void);
//extern b32 App_IsShuttingDown(void);
//extern b32 App_HasFocus(void);
//extern void App_RequestRender(void);
//extern b32 App_GetAndClearRequestRender(void);
//extern void App_RequestShutDown(void);

#if defined(__cplusplus)
}
#endif
