/*
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#include <stdio.h>

#include <Elementary.h>

#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "common.h"
#include "quickpanel-ui.h"
#include "accessibility.h"

HAPI Evas_Object *quickpanel_accessibility_screen_reader_object_get(void *obj, screen_reader_object_type_e type, const char *part, Evas_Object *parent)
{
	Evas_Object *to = NULL;
	Evas_Object *ao = NULL;

	retif(obj == NULL, NULL, "invalid parameter");
	retif(type == SCREEN_READER_OBJ_TYPE_EDJ_OBJECT && !part, NULL, "invalid parameter");

	switch (type) {
	case SCREEN_READER_OBJ_TYPE_ELM_OBJECT:
		if (part != NULL) {
			to = (Evas_Object *)elm_object_part_content_get(obj, part);
			ao = (Evas_Object *)to;
		} else {
			ao = (Evas_Object *)obj;
		}
		break;

	case SCREEN_READER_OBJ_TYPE_EDJ_OBJECT:
		to = (Evas_Object *)edje_object_part_object_get(elm_layout_edje_get((Evas_Object *)obj), part);
		break;

	default:// evas, icon
		to = (Evas_Object *)obj;
	}

	if (!ao && to && parent) {		// edj, evas, icon, elm_object_item
		ao = elm_access_object_get(to);
		if (ao == NULL) {
			ao = elm_access_object_register(to, parent);
		}
	}

	return ao;
}

HAPI Evas_Object *quickpanel_accessibility_ui_get_focus_object(Evas_Object *parent)
{
	Evas_Object *focus = elm_button_add(parent);
	retif(focus == NULL, NULL, "failed to create focus object");

	elm_object_style_set(focus, "focus");

	elm_access_info_set(focus, ELM_ACCESS_INFO, "");
	elm_access_info_set(focus, ELM_ACCESS_TYPE, "");

	return focus;
}

HAPI char *quickpanel_accessibility_info_cb(void *data, Evas_Object *obj)
{
	char *str = NULL;
	retif(data == NULL, NULL, "invalid parameter");

	str = _((const char *)data);
	if (str != NULL) {
		return strdup(str);
	}

	return NULL;
}

HAPI char *quickpanel_accessibility_info_cb_s(void *data, Evas_Object *obj)
{
	char *str = NULL;
	retif(data == NULL, NULL, "invalid parameter");

	/**
	 * @note
	 * system string is not supported.
	 * data should be DID from application po files.
	 */
	str = _(data);
	if (str != NULL) {
		return strdup(str);
	}

	return NULL;
}

HAPI void quickpanel_accessibility_screen_reader_data_set(Evas_Object *view, const char *part, char *type, char *info)
{
	Evas_Object *ao = NULL;
	retif(view == NULL, , "invalid parameter");
	retif(part == NULL, , "invalid parameter");

	ao = quickpanel_accessibility_screen_reader_object_get(view,
			SCREEN_READER_OBJ_TYPE_ELM_OBJECT, part, view);
	if (ao != NULL) {
		elm_access_info_set(ao, ELM_ACCESS_TYPE, type);
		elm_access_info_set(ao, ELM_ACCESS_INFO, info);
	}
}
