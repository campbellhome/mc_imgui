// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "fonts.h"
#include "bbclient/bb_array.h"
#include "common.h"
#include "imgui_core.h"
#include "imgui_utils.h"

// warning C4820 : 'StructName' : '4' bytes padding added after data member 'MemberName'
// warning C4365: '=': conversion from 'ImGuiTabItemFlags' to 'ImGuiID', signed/unsigned mismatch
BB_WARNING_PUSH(4820, 4365)
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
BB_WARNING_POP

extern "C" fontConfig_t fontConfig_clone(fontConfig_t *src);
extern "C" void fontConfigs_reset(fontConfigs_t *val);

static fontConfigs_t s_fontConfigs;

void QueueUpdateDpiDependentResources();

#if BB_USING(FEATURE_FREETYPE)

// warning C4548: expression before comma has no effect; expected expression with side-effect
// warning C4820 : 'StructName' : '4' bytes padding added after data member 'MemberName'
// warning C4255: 'FuncName': no function prototype given: converting '()' to '(void)'
// warning C4668: '_WIN32_WINNT_WINTHRESHOLD' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
// warning C4574: 'INCL_WINSOCK_API_TYPEDEFS' is defined to be '0': did you mean to use '#if INCL_WINSOCK_API_TYPEDEFS'?
// warning C4365: 'return': conversion from 'bool' to 'BOOLEAN', signed/unsigned mismatch
BB_WARNING_PUSH(4820 4255 4668 4574 4365)
#include "misc/freetype/imgui_freetype.cpp"
BB_WARNING_POP

#include "forkawesome-webfont.inc"

#pragma comment(lib, "freetype.lib")

#endif // #if BB_USING(FEATURE_FREETYPE)

struct fontBuilder {
#if BB_USING(FEATURE_FREETYPE)
	bool useFreeType = true;
#else  // #if BB_USING(FEATURE_FREETYPE)
	bool useFreeType = false;
#endif // #else // #if BB_USING(FEATURE_FREETYPE)
	bool rebuild = true;

	// Call _BEFORE_ NewFrame()
	bool UpdateRebuild()
	{
		if(!rebuild)
			return false;
		ImGuiIO &io = ImGui::GetIO();
#if BB_USING(FEATURE_FREETYPE)
		if(useFreeType) {
			ImGuiFreeType::BuildFontAtlas(io.Fonts, 0);
		} else
#endif // #if BB_USING(FEATURE_FREETYPE)
		{
			io.Fonts->Build();
		}
		rebuild = false;
		return true;
	}
};

static fontBuilder s_fonts;
void Fonts_MarkAtlasForRebuild(void)
{
	s_fonts.rebuild = true;
}

bool Fonts_UpdateAtlas(void)
{
	return s_fonts.UpdateRebuild();
}

void Fonts_Menu(void)
{
//ImGui::Checkbox("DEBUG Text Shadows", &g_config.textShadows);
#if BB_USING(FEATURE_FREETYPE)
	if(ImGui::Checkbox("DEBUG Use FreeType", &s_fonts.useFreeType)) {
		Fonts_MarkAtlasForRebuild();
	}
#endif // #if BB_USING(FEATURE_FREETYPE)
}

static ImFontAtlas::GlyphRangesBuilder s_glyphs;

static bool Glyphs_CacheText(ImFontAtlas::GlyphRangesBuilder *glyphs, const char *text, const char *text_end = nullptr)
{
	bool result = false;
	while(text_end ? (text < text_end) : *text) {
		unsigned int c = 0;
		int c_len = ImTextCharFromUtf8(&c, text, text_end);
		text += c_len;
		if(c_len == 0)
			break;
		if(c < 0x10000) {
			if(!glyphs->GetBit((ImWchar)c)) {
				glyphs->SetBit((ImWchar)c);
				result = true;
			}
		}
	}
	return result;
}

extern "C" void Fonts_CacheGlyphs(const char *text)
{
	if(Glyphs_CacheText(&s_glyphs, text)) {
		QueueUpdateDpiDependentResources();
	}
}

void Fonts_GetGlyphRanges(ImVector< ImWchar > *glyphRanges)
{
	ImFontAtlas tmp;
	s_glyphs.AddRanges(tmp.GetGlyphRangesDefault());
	s_glyphs.BuildRanges(glyphRanges);
}

static void Fonts_MergeIconFont(float fontSize)
{
	// merge in icons from Fork Awesome
	ImGuiIO &io = ImGui::GetIO();
	static const ImWchar ranges[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
	ImFontConfig config;
	config.MergeMode = true;
	config.PixelSnapH = true;
	io.Fonts->AddFontFromMemoryCompressedTTF(ForkAwesome_compressed_data, ForkAwesome_compressed_size,
	                                         fontSize, &config, ranges);
	//io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FA, 16.0f, &icons_config, icons_ranges);
}

extern "C" void Fonts_ClearFonts(void)
{
	fontConfigs_reset(&s_fontConfigs);
}

extern "C" void Fonts_AddFont(fontConfig_t font)
{
	bba_push(s_fontConfigs, fontConfig_clone(&font));
}

void Fonts_InitFonts(void)
{
	static ImVector< ImWchar > glyphRanges;
	glyphRanges.clear();
	Fonts_GetGlyphRanges(&glyphRanges);

	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->Clear();

	if(s_fontConfigs.count < 1) {
		io.Fonts->AddFontDefault();
		Fonts_MergeIconFont(12.0f);
	} else {
		float dpiScale = Imgui_Core_GetDpiScale();
		for(u32 i = 0; i < s_fontConfigs.count; ++i) {
			fontConfig_t *fontConfig = s_fontConfigs.data + i;
			if(fontConfig->enabled && fontConfig->size > 0 && *sb_get(&fontConfig->path)) {
				io.Fonts->AddFontFromFileTTF(sb_get(&fontConfig->path), fontConfig->size * dpiScale, nullptr, glyphRanges.Data);
				Fonts_MergeIconFont(fontConfig->size * dpiScale);
			} else {
				io.Fonts->AddFontDefault();
				Fonts_MergeIconFont(12.0f);
			}
		}
	}

	Fonts_MarkAtlasForRebuild();
}
