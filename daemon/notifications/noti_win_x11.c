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

#include <Elementary.h>
#include <utilX.h>
#include <efl_util.h>

#define NOTI_HEIGHT 200
#define NOTI_BTN_HEIGHT 80

#ifndef __UNUSED__
#define __UNUSED__ __attribute__((unused))
#endif
/* Using this macro to emphasize that some portion like stacking and
rotation handling are implemented for X based platform
*/

#include <Ecore_X.h>
#include "common.h"
#include "noti_win.h"
#include "dbus_utility.h"

struct Internal_Data {
	Evas_Object *content;
	Ecore_Event_Handler *rotation_event_handler;
	Evas_Coord w;
	Evas_Coord h;
	int angle;
	enum Noti_Orient orient;
};

static const char *data_key = "_data";

static void _show(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj,
	void *event_info __UNUSED__)
{
	struct Internal_Data *wd = evas_object_data_get(obj, data_key);

	if (!wd) {
		return;
	}
	if (wd->content) {
		evas_object_show(wd->content);
	}
}

static void _content_changed_size_hints(void *data, Evas *e __UNUSED__,
	Evas_Object *obj, void *event_info __UNUSED__)
{
	Evas_Coord h = 0;
	struct Internal_Data *wd = evas_object_data_get(data, data_key);

	if (!wd) {
		return;
	}

	evas_object_size_hint_min_get(obj, NULL, &h);
	if ((h > 0)) {
		wd->h = h;
		evas_object_size_hint_min_set(obj, wd->w, wd->h);
		evas_object_size_hint_min_set(data, wd->w, wd->h);
		evas_object_resize(data, wd->w, wd->h);
	}
}

static void _sub_del(void *data __UNUSED__, Evas_Object *obj, void *event_info)
{
	struct Internal_Data *wd = evas_object_data_get(obj, data_key);
	Evas_Object *sub = event_info;

	if (!wd) {
		return;
	}
	if (sub == wd->content) {
		evas_object_event_callback_del(wd->content,
			EVAS_CALLBACK_CHANGED_SIZE_HINTS,
			_content_changed_size_hints);
		wd->content = NULL;
	}
}

static void _resized(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj,
	void *event_info __UNUSED__)
{
	evas_object_show(obj);
}

static void _del(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj,
	void *event_info __UNUSED__)
{

	struct Internal_Data *wd = evas_object_data_get(obj, data_key);

	if (wd) {
		if (wd->rotation_event_handler) {
			ecore_event_handler_del(wd->rotation_event_handler);
		}
		free(wd);
	}

	evas_object_data_set(data, data_key, NULL);
}

static void _update_geometry_on_rotation(Evas_Object *obj, int angle,
		int *x, int *y, int *w)
{
	Evas_Coord root_w, root_h;
	struct Internal_Data *wd = evas_object_data_get(obj, data_key);

	if (!wd) {
		return;
	}

	ecore_x_window_size_get(ecore_x_window_root_first_get(), &root_w, &root_h);

	switch (angle) {
	case 90:
		*w = root_h;
		if (wd->orient == NOTI_ORIENT_BOTTOM) {
			*x = root_w - wd->h;
		}
		break;
	case 270:
		*w = root_h;
		if (!(wd->orient == NOTI_ORIENT_BOTTOM)) {
			*x = root_w - wd->h;
		}
		break;
	case 180:
		*w = root_w;
		if (!wd->orient == NOTI_ORIENT_BOTTOM) {
			*y = root_h - wd->h;
		}
		break;
	case 0:
	default:
		*w = root_w;
		if (wd->orient == NOTI_ORIENT_BOTTOM) {
			*y = root_h - wd->h;
		}
		break;
	}
}
static void _win_rotated(Evas_Object *obj)
{
	int x = 0;
	int y = 0;
	int w = 0;
	int angle = 0;
	struct Internal_Data *wd =  evas_object_data_get(obj, data_key);

	if (!wd) {
		return;
	}
	angle = elm_win_rotation_get(obj);
	if (angle % 90) {
		return;
	}
	angle %= 360;
	if (angle < 0)
		angle += 360;
	wd->angle = angle;

	_update_geometry_on_rotation(obj, wd->angle, &x, &y, &w);

	evas_object_move(obj, x, y);
	wd->w = w;
	evas_object_resize(obj, wd->w, wd->h);
}

static void _ui_rotation_wm_cb(void *data, Evas_Object *obj, void *event)
{
	int angle = 0;
	angle = elm_win_rotation_get((Evas_Object *)obj);

	DBG("ACTIVENOTI ROTATE:%d", angle);

	_win_rotated(obj);
}

HAPI Evas_Object *quickpanel_noti_win_add(Evas_Object *parent)
{
	Evas_Object *win;
	Evas_Object *bg;
	struct Internal_Data *wd;
	Evas_Coord w = 0, h = 0;

	win = elm_win_add(parent, "noti_win", ELM_WIN_NOTIFICATION);
	if (!win) {
		return NULL;
	}

	elm_win_indicator_mode_set(win, ELM_WIN_INDICATOR_SHOW);
	elm_win_alpha_set(win, EINA_FALSE);
	elm_win_indicator_type_set(win,ELM_WIN_INDICATOR_TYPE_1);
	elm_win_title_set(win, "noti_win");
	elm_win_borderless_set(win, EINA_TRUE);
	elm_win_autodel_set(win, EINA_TRUE);
	evas_object_size_hint_weight_set(win, EVAS_HINT_EXPAND,	EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(win, EVAS_HINT_FILL, EVAS_HINT_FILL);

	efl_util_set_notification_window_level(win, EFL_UTIL_NOTIFICATION_LEVEL_DEFAULT); 
	elm_win_aux_hint_add(win, "wm.policy.win.user.geometry", "1");
	elm_win_prop_focus_skip_set(win, EINA_TRUE);

	bg = elm_bg_add(win);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, bg);

	if (elm_win_wm_rotation_supported_get(win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(win, rots, 4);
	}
	evas_object_smart_callback_add(win, "wm,rotation,changed", _ui_rotation_wm_cb, NULL);

	wd = (struct Internal_Data *) calloc(1, sizeof(struct Internal_Data));
	if (!wd) {
		if (win) {
			evas_object_del(win);
		}
		return NULL;
	}
	evas_object_data_set(win, data_key, wd);
	wd->angle = 0;
	wd->orient = NOTI_ORIENT_TOP;
	evas_object_move(win, 0, 0);
	elm_win_screen_size_get(win, NULL, NULL, &w, &h);

	wd->w = w;
	wd->h = NOTI_HEIGHT;

	evas_object_resize(win, w, wd->h);
	evas_object_event_callback_add(win, EVAS_CALLBACK_SHOW, _show, NULL);
	evas_object_event_callback_add(win, EVAS_CALLBACK_DEL, _del, NULL);
	evas_object_event_callback_add(win, EVAS_CALLBACK_RESIZE, _resized, NULL);

	return win;
}

HAPI void quickpanel_noti_win_content_set(Evas_Object *obj, Evas_Object *content, int btn_cnt)
{
	Evas_Coord h;
	struct Internal_Data *wd;

	if (!obj) {
		return;
	}
	wd = evas_object_data_get(obj, data_key);
	if (!wd) {
		return;
	}
	if (wd->content && content != NULL) {
		evas_object_del(content);
		content = NULL;
	}
	wd->content = content;

	if (btn_cnt > 0) {
		wd->h += NOTI_BTN_HEIGHT;
	}
	if (content) {
		evas_object_size_hint_weight_set(wd->content, EVAS_HINT_EXPAND,	EVAS_HINT_EXPAND);
		elm_win_resize_object_add(obj, wd->content);
		evas_object_size_hint_min_set(wd->content, wd->w, wd->h);
		evas_object_resize(obj, wd->w, wd->h);
		evas_object_event_callback_add(wd->content,	EVAS_CALLBACK_CHANGED_SIZE_HINTS, _content_changed_size_hints, obj);
	}
}

HAPI void quickpanel_noti_win_orient_set(Evas_Object *obj, enum Noti_Orient orient)
{
	Evas_Coord root_w, root_h;
	struct Internal_Data *wd = evas_object_data_get(obj, data_key);

	if (!wd) {
		return;
	}
	if (orient >= NOTI_ORIENT_LAST) {
		return;
	}
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &root_w, &root_h);
	switch (orient) {
	case NOTI_ORIENT_BOTTOM:
		evas_object_move(obj, 0, root_h - wd->h);
		wd->orient = NOTI_ORIENT_BOTTOM;
		break;
	case NOTI_ORIENT_TOP:
	default:
		evas_object_move(obj, 0, 0);
		wd->orient = NOTI_ORIENT_TOP;
		break;
	}
}
