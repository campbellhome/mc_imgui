// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

// AUTOGENERATED FILE - DO NOT EDIT

// clang-format off

#include "mc_imgui_json_generated.h"
#include "bbclient/bb_array.h"
#include "va.h"

#include "fonts.h"
#include "sb.h"
#include "sdict.h"
#include "uuid_rfc4122\sysdep.h"

//////////////////////////////////////////////////////////////////////////

int json_object_get_boolean_safe(const JSON_Object *object, const char *name)
{
	return json_value_get_boolean(json_object_get_value(object, name)) == 1;
}

//////////////////////////////////////////////////////////////////////////

fontConfig_t json_deserialize_fontConfig_t(JSON_Value *src)
{
	fontConfig_t dst;
	memset(&dst, 0, sizeof(dst));
	if(src) {
		JSON_Object *obj = json_value_get_object(src);
		if(obj) {
			dst.enabled = json_object_get_boolean_safe(obj, "enabled");
			dst.size = (u32)json_object_get_number(obj, "size");
			dst.path = json_deserialize_sb_t(json_object_get_value(obj, "path"));
		}
	}
	return dst;
}

fontConfigs_t json_deserialize_fontConfigs_t(JSON_Value *src)
{
	fontConfigs_t dst;
	memset(&dst, 0, sizeof(dst));
	if(src) {
		JSON_Array *arr = json_value_get_array(src);
		if(arr) {
			for(u32 i = 0; i < json_array_get_count(arr); ++i) {
				bba_push(dst, json_deserialize_fontConfig_t(json_array_get_value(arr, i)));
			}
		}
	}
	return dst;
}

uuid_node_t json_deserialize_uuid_node_t(JSON_Value *src)
{
	uuid_node_t dst;
	memset(&dst, 0, sizeof(dst));
	if(src) {
		JSON_Object *obj = json_value_get_object(src);
		if(obj) {
			for(u32 i = 0; i < BB_ARRAYSIZE(dst.nodeID); ++i) {
				dst.nodeID[i] = (char)json_object_get_number(obj, va("nodeID.%u", i));
			}
		}
	}
	return dst;
}

//////////////////////////////////////////////////////////////////////////

JSON_Value *json_serialize_fontConfig_t(const fontConfig_t *src)
{
	JSON_Value *val = json_value_init_object();
	JSON_Object *obj = json_value_get_object(val);
	if(obj) {
		json_object_set_boolean(obj, "enabled", src->enabled);
		json_object_set_number(obj, "size", src->size);
		json_object_set_value(obj, "path", json_serialize_sb_t(&src->path));
	}
	return val;
}

JSON_Value *json_serialize_fontConfigs_t(const fontConfigs_t *src)
{
	JSON_Value *val = json_value_init_array();
	JSON_Array *arr = json_value_get_array(val);
	if(arr) {
		for(u32 i = 0; i < src->count; ++i) {
			JSON_Value *child = json_serialize_fontConfig_t(src->data + i);
			if(child) {
				json_array_append_value(arr, child);
			}
		}
	}
	return val;
}

JSON_Value *json_serialize_uuid_node_t(const uuid_node_t *src)
{
	JSON_Value *val = json_value_init_object();
	JSON_Object *obj = json_value_get_object(val);
	if(obj) {
		for(u32 i = 0; i < BB_ARRAYSIZE(src->nodeID); ++i) {
			json_object_set_number(obj, va("nodeID.%u", i), src->nodeID[i]);
		}
	}
	return val;
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
