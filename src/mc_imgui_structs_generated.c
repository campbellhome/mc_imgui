// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

// AUTOGENERATED FILE - DO NOT EDIT

// clang-format off

#include "mc_imgui_structs_generated.h"
#include "bb_array.h"
#include "str.h"
#include "va.h"

#include "fonts.h"
#include "sb.h"
#include "sdict.h"

#include <string.h>


void fontConfig_reset(fontConfig_t *val)
{
	if(val) {
		sb_reset(&val->path);
	}
}
fontConfig_t fontConfig_clone(const fontConfig_t *src)
{
	fontConfig_t dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.enabled = src->enabled;
		dst.size = src->size;
		dst.path = sb_clone(&src->path);
	}
	return dst;
}

void fontConfigs_reset(fontConfigs_t *val)
{
	if(val) {
		for(u32 i = 0; i < val->count; ++i) {
			fontConfig_reset(val->data + i);
		}
		bba_free(*val);
	}
}
fontConfigs_t fontConfigs_clone(const fontConfigs_t *src)
{
	fontConfigs_t dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		for(u32 i = 0; i < src->count; ++i) {
			if(bba_add_noclear(dst, 1)) {
				bba_last(dst) = fontConfig_clone(src->data + i);
			}
		}
	}
	return dst;
}
