// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "imgui_input_text.h"
#include "bb_array.h"
#include "imgui_core.h"
#include "sb.h"

// warning C4820 : 'StructName' : '4' bytes padding added after data member 'MemberName'
// warning C4365: '=': conversion from 'ImGuiTabItemFlags' to 'ImGuiID', signed/unsigned mismatch
BB_WARNING_PUSH(4820 4365)
#include "imgui_internal.h"
BB_WARNING_POP

// InputTextMultilineScrolling and supporting functions adapted from https://github.com/ocornut/imgui/issues/383

struct MultilineScrollState {
	const char *externalBuf = nullptr;

	float scrollRegionX = 0.0f;
	float scrollX = 0.0f;
	float scrollRegionY = 0.0f;
	float scrollY = 0.0f;

	int oldCursorPos = 0;

	b32 bHasScrollTargetX = false;
	float scrollTargetX = 0.0f;
	b32 bHasScrollTargetY = false;
	float scrollTargetY = 0.0f;

	u8 pad[4];
};

typedef struct MultilineScrollStates_s {
	u32 count;
	u32 allocated;
	MultilineScrollState **data;
} MultilineScrollStates_t;

static MultilineScrollStates_t s_multilineScrollStates;

void ImGui::InputTextShutdown(void)
{
	for(u32 i = 0; i < s_multilineScrollStates.count; ++i) {
		delete s_multilineScrollStates.data[i];
	}
	bba_free(s_multilineScrollStates);
}

static int Imgui_Core_FindLineStart(const char *buf, int cursorPos)
{
	while((cursorPos > 0) && (buf[cursorPos - 1] != '\n')) {
		--cursorPos;
	}
	return cursorPos;
}

static int Imgui_Core_CountLines(const char *buf, int cursorPos)
{
	int lines = 1;
	int pos = 0;
	while(buf[pos]) {
		if(buf[pos] == '\n') {
			++lines;
		}
		if(pos == cursorPos)
			break;
		++pos;
	}
	return lines;
}

static int Imgui_Core_InputTextMultilineScrollingCallback(ImGuiTextEditCallbackData *data)
{
	MultilineScrollState *scrollState = (MultilineScrollState *)data->UserData;
	if(scrollState->oldCursorPos != data->CursorPos) {
		const char *buf = data->Buf ? data->Buf : scrollState->externalBuf;
		int lineStartPos = Imgui_Core_FindLineStart(buf, data->CursorPos);
		int lines = Imgui_Core_CountLines(buf, data->CursorPos);

		ImVec2 lineSize = ImGui::CalcTextSize(buf + lineStartPos, buf + data->CursorPos);
		float cursorX = lineSize.x;
		float cursorY = lines * lineSize.y;
		float scrollAmountX = scrollState->scrollRegionX * 0.25f;
		float scrollAmountY = scrollState->scrollRegionY * 0.25f;

		if(cursorX < scrollState->scrollX) {
			scrollState->bHasScrollTargetX = true;
			scrollState->scrollTargetX = ImMax(0.0f, cursorX - scrollAmountX);
		} else if((cursorX - scrollState->scrollRegionX) >= scrollState->scrollX) {
			scrollState->bHasScrollTargetX = true;
			if((cursorX - scrollState->scrollRegionX) > scrollAmountX) {
				scrollState->scrollTargetX = cursorX - scrollAmountX;
			} else {
				scrollState->scrollTargetX = cursorX - scrollState->scrollRegionX + scrollAmountX;
			}
		}

		if(cursorY < scrollState->scrollY) {
			scrollState->bHasScrollTargetY = true;
			scrollState->scrollTargetY = ImMax(0.0f, cursorY - scrollAmountY);
		} else if((cursorY - scrollState->scrollRegionY) >= scrollState->scrollY) {
			scrollState->bHasScrollTargetY = true;
			if((cursorY - scrollState->scrollRegionY) > scrollAmountY) {
				scrollState->scrollTargetY = cursorY - scrollAmountY;
			} else {
				scrollState->scrollTargetY = cursorY - scrollState->scrollRegionY + scrollAmountY;
			}
		}

		scrollState->oldCursorPos = data->CursorPos;
	}

	return 0;
}

bool ImGui::InputTextMultilineScrolling(const char *label, char *buf, size_t buf_size, const ImVec2 &size, ImGuiInputTextFlags flags)
{
	const char *labelVisibleEnd = FindRenderedTextEnd(label);

	ImVec2 textSize = CalcTextSize(buf);
	float scrollbarSize = GetStyle().ScrollbarSize;
	float labelWidth = CalcTextSize(label, labelVisibleEnd).x;

	ImVec2 availSize = GetContentRegionAvail();
	if(size.x > 0.0f) {
		availSize.x = size.x;
	} else if(size.x < 0.0f) {
		availSize.x -= size.x;
	}
	if(size.y > 0.0f) {
		availSize.y = size.y;
	} else if(size.y < 0.0f) {
		availSize.y -= size.y;
	}
	float childWidth = size.x;
	if(size.x == 0.0f) {
		if(labelWidth > 0.0f) {
			childWidth = -labelWidth;
		} else {
			childWidth = 0.0f;
		}
	}
	float childHeight = size.y;
	float textBoxWidth = availSize.x - scrollbarSize;
	if(textSize.x > childWidth * 0.8f) {
		textBoxWidth = textSize.x + childWidth * 0.8f;
	}
	float textBoxHeight = 0.0f;
	if(childHeight < 0) {
		textBoxHeight = availSize.y + childHeight - scrollbarSize - GetStyle().FramePadding.y;
	} else if(childHeight > 0.0f) {
		textBoxHeight = childHeight - scrollbarSize - GetStyle().FramePadding.y;
	}
	float minTextBoxHeight = textSize.y + 2.0f * GetTextLineHeightWithSpacing();
	if(textBoxHeight < minTextBoxHeight) {
		textBoxHeight = minTextBoxHeight;
	}

	ImGuiID scrollStateKey = GetID("textInputScrollState");
	ImGuiStorage *storage = GetStateStorage();
	MultilineScrollState *scrollState = (MultilineScrollState *)storage->GetVoidPtr(scrollStateKey);
	if(!scrollState) {
		scrollState = new MultilineScrollState;
		if(scrollState) {
			bba_push(s_multilineScrollStates, scrollState);
			storage->SetVoidPtr(scrollStateKey, scrollState);
		}
	}
	if(!scrollState)
		return false;

	BeginChild(label, ImVec2(childWidth, childHeight), false, ImGuiWindowFlags_HorizontalScrollbar);
	scrollState->scrollRegionX = ImMax(0.0f, GetWindowWidth() - scrollbarSize);
	scrollState->scrollX = GetScrollX();
	scrollState->scrollRegionY = ImMax(0.0f, GetWindowHeight() - scrollbarSize);
	scrollState->scrollY = GetScrollY();
	scrollState->externalBuf = buf;
	int oldCursorPos = scrollState->oldCursorPos;

	PushItemWidth(textBoxWidth);
	bool changed = InputTextMultiline(label, buf, buf_size, ImVec2(textBoxWidth, textBoxHeight),
	                                  flags | ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_NoHorizontalScroll, Imgui_Core_InputTextMultilineScrollingCallback, scrollState);
	PopItemWidth();
	if(IsItemActive() && Imgui_Core_HasFocus()) {
		Imgui_Core_RequestRender();
	}

	if(scrollState->bHasScrollTargetX) {
		float realMaxScrollX = GetScrollMaxX();
		float maxScrollX = textBoxWidth;
		float maxUserScrollX = maxScrollX - childWidth * 0.5f;
		if((realMaxScrollX == 0.0f && scrollState->scrollTargetX > 0.0f) || scrollState->scrollTargetX > textSize.x || (scrollState->scrollTargetX > scrollState->scrollX && maxScrollX < textSize.x)) {
			scrollState->oldCursorPos = oldCursorPos;
			SetScrollX(maxUserScrollX);
		} else {
			scrollState->bHasScrollTargetX = false;
			if(scrollState->scrollTargetX <= maxScrollX) {
				SetScrollX(scrollState->scrollTargetX);
			} else {
				SetScrollX(maxUserScrollX);
			}
		}
	}

	if(scrollState->bHasScrollTargetY) {
		float realMaxScrollY = GetScrollMaxY();
		float maxScrollY = textBoxHeight;
		float maxUserScrollY = maxScrollY - textBoxHeight * 0.5f;
		if((realMaxScrollY == 0.0f && scrollState->scrollTargetY > 0.0f) || scrollState->scrollTargetY > textSize.y || (scrollState->scrollTargetY > scrollState->scrollY && maxScrollY < textSize.y)) {
			scrollState->oldCursorPos = oldCursorPos;
			SetScrollY(maxUserScrollY);
		} else {
			scrollState->bHasScrollTargetY = false;
			if(scrollState->scrollTargetY <= maxScrollY) {
				SetScrollY(scrollState->scrollTargetY);
			} else {
				SetScrollY(maxUserScrollY);
			}
		}
	}

	EndChild();
	SameLine();
	TextUnformatted(label, labelVisibleEnd);

	return changed;
}

bool ImGui::InputTextMultilineScrolling(const char *label, sb_t *sb, u32 buf_size, const ImVec2 &size, ImGuiInputTextFlags flags)
{
	if(sb->allocated < buf_size) {
		sb_reserve(sb, buf_size);
	}
	bool ret = InputTextMultilineScrolling(label, (char *)sb->data, sb->allocated, size, flags);
	sb->count = sb->data ? (u32)strlen(sb->data) + 1 : 0;
	return ret;
}
