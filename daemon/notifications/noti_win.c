/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Elementary.h>
#include <utilX.h>

#define NOTI_HEIGHT 50
#ifndef __UNUSED__
#define __UNUSED__ __attribute__((unused))
#endif
/* Using this macro to emphasize that some portion like stacking and
rotation handling are implemented for X based platform
*/

#ifdef HAVE_X
#include <Ecore_X.h>
#endif
#include "common.h"
#include "noti_win.h"

struct Internal_Data {
	Evas_Object *content;
	Ecore_Event_Handler *rotation_event_handler;
	Evas_Coord w;
	Evas_Coord h;
	int angle;
	enum Noti_Orient orient;
};

static const char *data_key = "_data";

static void _set_win_type_notification_level(Evas_Object *win)
{
	retif(win == NULL, , "invalid parameter");

	Ecore_X_Window w = elm_win_xwindow_get(win);

	if (w > 0) {
		ecore_x_icccm_hints_set(w, 0, ECORE_X_WINDOW_STATE_HINT_NONE, 0, 0,
			0, 0, 0);
		ecore_x_netwm_opacity_set(w, 0);

		ecore_x_netwm_window_type_set(w,
				ECORE_X_WINDOW_TYPE_NOTIFICATION);
		utilx_set_system_notification_level(ecore_x_display_get(), w,
				UTILX_NOTIFICATION_LEVEL_HIGH);
	}
}

static void _show(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj,
	void *event_info __UNUSED__)
{
	struct Internal_Data *wd = evas_object_data_get(obj, data_key);

	if (!wd)
		return;
	if (wd->content)
		evas_object_show(wd->content);
}

static void _content_hide(void *data, Evas *e __UNUSED__,
	Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
	evas_object_hide(data);
}

static void _content_changed_size_hints(void *data, Evas *e __UNUSED__,
	Evas_Object *obj, void *event_info __UNUSED__)
{
	Evas_Coord h;
	struct Internal_Data *wd = evas_object_data_get(data, data_key);

	if (!wd)
		return;

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

	if (!wd)
		return;
	if (sub == wd->content) {
		evas_object_event_callback_del(wd->content, EVAS_CALLBACK_HIDE,
			_content_hide);
		evas_object_event_callback_del(wd->content,
			EVAS_CALLBACK_CHANGED_SIZE_HINTS,
			_content_changed_size_hints);
		wd->content = NULL;
	}
}

static void _del(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj,
	void *event_info __UNUSED__)
{
	struct Internal_Data *wd = evas_object_data_get(obj, data_key);

	if (!wd)
		return;
	if (wd->rotation_event_handler)
		ecore_event_handler_del(wd->rotation_event_handler);
	evas_object_data_set(data, data_key, NULL);
	free(wd);
}

#ifdef HAVE_X
static void _update_geometry_on_rotation(Evas_Object *obj, int angle,
		int *x, int *y, int *w)
{
	Evas_Coord root_w, root_h;
	struct Internal_Data *wd = evas_object_data_get(obj, data_key);

	if (!wd)
		return;

	ecore_x_window_size_get(ecore_x_window_root_first_get(), &root_w,
		&root_h);

	/* rotate window */
	switch (angle) {
	case 90:
		*w = root_h;
		if (wd->orient == NOTI_ORIENT_BOTTOM)
			*x = root_w - wd->h;
		break;
	case 270:
		*w = root_h;
		if (!(wd->orient == NOTI_ORIENT_BOTTOM))
			*x = root_w - wd->h;
		break;
	case 180:
		*w = root_w;
		if (!wd->orient == NOTI_ORIENT_BOTTOM)
			*y = root_h - wd->h;
		break;
	case 0:
	default:
		*w = root_w;
		if (wd->orient == NOTI_ORIENT_BOTTOM)
			*y = root_h - wd->h;
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

	if (!wd)
		return;
	angle = elm_win_rotation_get(obj);
	if (angle % 90)
		return;
	angle %= 360;
	if (angle < 0)
		angle += 360;
	wd->angle = angle;

	_update_geometry_on_rotation(obj, wd->angle, &x, &y, &w);

	evas_object_move(obj, x, y);
	wd->w = w;
	evas_object_resize(obj, wd->w, wd->h);
}

static Eina_Bool _prop_change(void *data, int type __UNUSED__, void *event)
{
	Ecore_X_Event_Window_Property *ev;
	struct Internal_Data *wd = evas_object_data_get(data, data_key);

	if (!wd)
		return ECORE_CALLBACK_PASS_ON;
	ev = event;
	if (ev->atom == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE)
		if (ev->win == elm_win_xwindow_get(data))
			_win_rotated(data);
	return ECORE_CALLBACK_PASS_ON;
}
#endif

Evas_Object *noti_win_add(Evas_Object *parent)
{
	Evas_Object *win;
	Evas_Object *bg;
	struct Internal_Data *wd;
	Evas_Coord w = 0;

	win = elm_win_add(parent, "noti_win", ELM_WIN_NOTIFICATION);
	elm_win_alpha_set(win, EINA_TRUE);

	if (!win)
		return NULL;
	elm_win_title_set(win, "noti_win");
	elm_win_borderless_set(win, EINA_TRUE);
	elm_win_autodel_set(win, EINA_TRUE);
	evas_object_size_hint_weight_set(win, EVAS_HINT_EXPAND,
		EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(win, EVAS_HINT_FILL, EVAS_HINT_FILL);
	bg = elm_bg_add(win);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND,
		EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, bg);
	evas_object_show(bg);

	_set_win_type_notification_level(win);

	wd = (struct Internal_Data *) calloc(1, sizeof(struct Internal_Data));
	if (!wd)
		return NULL;
	evas_object_data_set(win, data_key, wd);
	wd->angle = 0;
	wd->orient = NOTI_ORIENT_TOP;
	evas_object_move(win, 0, 0);
#ifdef HAVE_X
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, NULL);
	evas_object_resize(win, w, NOTI_HEIGHT);
	wd->rotation_event_handler = ecore_event_handler_add(
		ECORE_X_EVENT_WINDOW_PROPERTY, _prop_change, win);
#endif
	wd->w = w;
	wd->h = NOTI_HEIGHT;
	evas_object_smart_callback_add(win, "sub-object-del", _sub_del, NULL);
	evas_object_event_callback_add(win, EVAS_CALLBACK_SHOW, _show, NULL);
	evas_object_event_callback_add(win, EVAS_CALLBACK_DEL, _del, NULL);
	return win;
}

void noti_win_content_set(Evas_Object *obj, Evas_Object *content)
{
	Evas_Coord h;
	struct Internal_Data *wd;

	if (!obj)
		return;
	wd = evas_object_data_get(obj, data_key);
	if (!wd)
		return;
	if (wd->content)
		evas_object_del(content);
	wd->content = content;
	if (content) {
		evas_object_size_hint_weight_set(wd->content, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
		elm_win_resize_object_add(obj, wd->content);
		evas_object_size_hint_min_get(wd->content, NULL, &h);
		if (h)
			wd->h = h;
		evas_object_size_hint_min_set(wd->content, wd->w, wd->h);
		evas_object_resize(obj, wd->w, wd->h);
		evas_object_event_callback_add(wd->content, EVAS_CALLBACK_HIDE,
				_content_hide, obj);
		evas_object_event_callback_add(wd->content,
				EVAS_CALLBACK_CHANGED_SIZE_HINTS,
				_content_changed_size_hints, obj);
	}
}

void noti_win_orient_set(Evas_Object *obj, enum Noti_Orient orient)
{
#ifdef HAVE_X
	Evas_Coord root_w, root_h;
#endif
	struct Internal_Data *wd = evas_object_data_get(obj, data_key);

	if (!wd)
		return;
	if (orient >= NOTI_ORIENT_LAST)
		return;
#ifdef HAVE_X
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &root_w,
		&root_h);
#endif
	switch (orient) {
	case NOTI_ORIENT_BOTTOM:
#ifdef HAVE_X
		evas_object_move(obj, 0, root_h - wd->h);
#endif
		wd->orient = NOTI_ORIENT_BOTTOM;
		break;
	case NOTI_ORIENT_TOP:
	default:
#ifdef HAVE_X
		evas_object_move(obj, 0, 0);
#endif
		wd->orient = NOTI_ORIENT_TOP;
		break;
	}
}
