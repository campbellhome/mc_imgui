// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "ui_message_box.h"

#include "imgui_core.h"
#include "bbclient/bb_string.h"
#include "imgui_utils.h"
#include "message_box.h"
#include "va.h"

static int s_activeFrames;

bool UIMessageBox_Draw(messageBox *mb)
{
	sdict_t *sd = &mb->data;
	const char *title = sdict_find(sd, "title");
	if(!title)
		return false;

	const char *text = sdict_find(sd, "text");
	if(text) {
		ImGui::TextUnformatted(text);
	}

	sdictEntry_t *inputNumber = sdict_find_entry(sd, "inputNumber");
	if(inputNumber) {
		if(s_activeFrames < 3) {
			ImGui::SetKeyboardFocusHere();
		}
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;
		bool entered = ImGui::InputText("##path", &inputNumber->value, 1024, flags);
		if(entered) {
			if(mb->callback) {
				mb->callback(mb, sb_get(&inputNumber->value));
			}
			return false;
		}
	}

	u32 buttonIndex = 0;
	const char *button;
	while((button = sdict_find(sd, va("button%u", ++buttonIndex))) != nullptr) {
		if(buttonIndex == 1) {
			ImGui::Separator();
		} else {
			ImGui::SameLine();
		}
		if(ImGui::Button(button, ImVec2(120 * Imgui_Core_GetDpiScale(), 0.0f))) {
			if(mb->callback) {
				mb->callback(mb, button);
			}
			return false;
		}
	};

	if(ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Escape])) {
		if(mb->callback) {
			mb->callback(mb, "escape");
		}
		return false;
	}

	return true;
}

void UIMessageBox_Update()
{
	messageBox *mb = mb_get_active();
	if(!mb)
		return;

	const char *title = sdict_find(&mb->data, "title");
	if(!title) {
		title = "Untitled";
	}

	int flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;
	if(!ImGui::BeginPopupModal(title, nullptr, flags)) {
		s_activeFrames = 0;
		ImGui::OpenPopup(title);
		if(!ImGui::BeginPopupModal(title, nullptr, flags)) {
			if(mb->callback) {
				mb->callback(mb, "");
			}
			mb_remove_active();
			return;
		}
	}

	++s_activeFrames;
	if(!UIMessageBox_Draw(mb)) {
		ImGui::CloseCurrentPopup();
		mb_remove_active();
	}

	ImGui::EndPopup();
}
