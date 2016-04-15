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
#include <efl_util.h>

#define NOTI_HEIGHT ELM_SCALE_SIZE(260)
#define NOTI_START_Y ELM_SCALE_SIZE(36)


#ifndef __UNUSED__
#define __UNUSED__ __attribute__((unused))
#endif
/* Using this macro to emphasize that some portion like stacking and
   rotation handling are implemented for X based platform
 */

#include "common.h"
#include "noti_win.h"


struct Internal_Data {
	Evas_Object *content;
	Ecore_Event_Handler *rotation_event_handler;
	Evas_Coord scr_w;
	Evas_Coord scr_h;
	Evas_Coord w;
	Evas_Coord h;
	int angle;
	enum Noti_Orient orient;
};

#define E_DATA_KEY "E_DATA_KEY"
#define E_DATA_BASE_RECT "E_DATA_BASE_RECT"

static void _show(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
	struct Internal_Data *wd = evas_object_data_get(obj, E_DATA_KEY);

	if (!wd) {
		return;
	}

	if (wd->content) {
		evas_object_show(wd->content);
	}

}

static void _content_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
	Evas_Coord h = 0;
	struct Internal_Data *wd = evas_object_data_get(data, E_DATA_KEY);

	if (!wd) {
		return;
	}

	evas_object_size_hint_min_get(obj, NULL, &h);
	if ((h > 0)) {
		evas_object_size_hint_min_set(obj, wd->w, wd->h);
		evas_object_size_hint_min_set(data, wd->w, wd->h);
		evas_object_resize(data, wd->w, wd->h);
	}
}

static void _resized(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
	evas_object_show(obj);
}

static void _del(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
	struct Internal_Data *wd = evas_object_data_get(obj, E_DATA_KEY);

	if (wd) {
		if (wd->rotation_event_handler) {
			ecore_event_handler_del(wd->rotation_event_handler);
		}
		free(wd);
	}

	evas_object_data_set(data, E_DATA_KEY, NULL);
}

static void _win_rotated(Evas_Object *obj)
{
	int angle = 0;
	struct Internal_Data *wd =  evas_object_data_get(obj, E_DATA_KEY);

	if (!wd) {
		return;
	}

	angle = elm_win_rotation_get(obj);
	if (angle % 90) {
		return;
	}

	angle %= 360;
	if (angle < 0) {
		angle += 360;
	}
	wd->angle = angle;

	switch (angle) {
	case 0:
	case 180:
		evas_object_size_hint_min_set(obj, wd->scr_w, wd->h);
		evas_object_resize(obj, wd->scr_w, wd->h);
		evas_object_move(obj, 0, NOTI_START_Y);
		break;
	case 90:
		evas_object_size_hint_min_set(obj, wd->scr_h, wd->h);
		evas_object_resize(obj, wd->scr_h, wd->h);
		evas_object_move(obj, NOTI_START_Y, 0);
		break;
	case 270:
		evas_object_size_hint_min_set(obj, wd->scr_h, wd->h);
		evas_object_resize(obj, wd->scr_h, wd->h);
		evas_object_move(obj, wd->scr_w - wd->h - NOTI_START_Y, 0);
		break;
	default:
		ERR("cannot reach here");
	}
}

static void _ui_rotation_wm_cb(void *data, Evas_Object *obj, void *event)
{
	int angle = 0;
	angle = elm_win_rotation_get((Evas_Object *)obj);

	DBG("ACTIVENOTI ROTATE:%d", angle);

	_win_rotated(obj);
}

static void _resize_cb (void *data, Evas *e,  Evas_Object *eo, void *event_info)
{
	int x, y, w, h;
	evas_object_geometry_get(eo, &x, &y, &w, &h);
	DBG("%s: %d:%d:%d:%d\n", data, x, y, w, h);
}

HAPI Evas_Object *quickpanel_noti_win_add(Evas_Object *parent)
{
	Evas_Object *win;
	struct Internal_Data *wd;
	Evas_Coord w = 0, h = 0;
	Evas *e = NULL;
	Ecore_Evas *ee = NULL;

	win = elm_win_add(parent, "noti_win", ELM_WIN_NOTIFICATION);
	if (!win) {
		return NULL;
	}

	e = evas_object_evas_get(win);
	if (!e) {
		evas_object_del(win);
		return NULL;
	}

	ee = ecore_evas_ecore_evas_get(e);
	if (!ee) {
		evas_object_del(win);
		return NULL;
	}

	ecore_evas_name_class_set(ee, "APP_POPUP", "APP_POPUP");

	elm_win_alpha_set(win, EINA_FALSE);
	elm_win_indicator_mode_set(win, ELM_WIN_INDICATOR_HIDE);
	elm_win_title_set(win, "noti_win");
	elm_win_borderless_set(win, EINA_TRUE);
	elm_win_autodel_set(win, EINA_TRUE);
	evas_object_size_hint_weight_set(win, EVAS_HINT_EXPAND,	EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(win, EVAS_HINT_FILL, EVAS_HINT_FILL);

	efl_util_set_notification_window_level(win, EFL_UTIL_NOTIFICATION_LEVEL_DEFAULT);
	elm_win_prop_focus_skip_set(win, EINA_TRUE);
	elm_win_aux_hint_add(win, "wm.policy.win.user.geometry", "1");

	if (elm_win_wm_rotation_supported_get(win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(win, rots, 4);
	}
	evas_object_smart_callback_add(win, "wm,rotation,changed", _ui_rotation_wm_cb, NULL);
	evas_object_event_callback_add(win, EVAS_CALLBACK_RESIZE, _resize_cb, "win");

	wd = (struct Internal_Data *) calloc(1, sizeof(struct Internal_Data));
	if (!wd) {
		if (win) {
			evas_object_del(win);
		}
		return NULL;
	}
	evas_object_data_set(win, E_DATA_KEY, wd);
	wd->angle = 0;
	wd->orient = NOTI_ORIENT_TOP;
	evas_object_move(win, 0, NOTI_START_Y);
	elm_win_screen_size_get(win, NULL, NULL, &w, &h);

	wd->scr_w = w;
	wd->scr_h = h;
	wd->w = w;
	wd->h = NOTI_HEIGHT;

	evas_object_resize(win, w, wd->h);
	evas_object_event_callback_add(win, EVAS_CALLBACK_SHOW, _show, NULL);
	evas_object_event_callback_add(win, EVAS_CALLBACK_DEL, _del, NULL);
	evas_object_event_callback_add(win, EVAS_CALLBACK_RESIZE, _resized, NULL);

	return win;
}

HAPI void quickpanel_noti_win_content_set(Evas_Object *obj, Evas_Object *content)
{
	struct Internal_Data *wd;

	if (!obj) {
		return;
	}

	wd = evas_object_data_get(obj, E_DATA_KEY);
	if (!wd) {
		return;
	}

	if (wd->content && content != NULL) {
		evas_object_del(wd->content);
		wd->content = NULL;
	}

	wd->content = content;

	if (!content) {
		return;
	}

	evas_object_size_hint_weight_set(content, EVAS_HINT_EXPAND,	EVAS_HINT_EXPAND);
	evas_object_resize(obj, wd->w, wd->h);
	elm_win_resize_object_add(obj, content);
	evas_object_size_hint_min_set(content, wd->w, wd->h);
	evas_object_event_callback_add(content,	EVAS_CALLBACK_CHANGED_SIZE_HINTS, _content_changed_size_hints, obj);
}

HAPI void quickpanel_noti_win_resize(Evas_Object *obj, int btn_cnt)
{
	struct Internal_Data *wd;

	if (!obj) {
		return;
	}

	wd = evas_object_data_get(obj, E_DATA_KEY);
	if (!wd) {
		return;
	}

	evas_object_resize(obj, wd->w, wd->h);
}

HAPI void quickpanel_noti_win_orient_set(Evas_Object *obj, enum Noti_Orient orient)
{
	struct Internal_Data *wd = evas_object_data_get(obj, E_DATA_KEY);

	if (!wd) {
		return;
	}

	if (orient >= NOTI_ORIENT_LAST) {
		return;
	}
	switch (orient) {
	case NOTI_ORIENT_BOTTOM:
		evas_object_move(obj, 0, wd->scr_h - wd->h);
		wd->orient = NOTI_ORIENT_BOTTOM;
		break;
	case NOTI_ORIENT_TOP:
	default:
		evas_object_move(obj, 0, NOTI_START_Y);
		wd->orient = NOTI_ORIENT_TOP;
		break;
	}
}
