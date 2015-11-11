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
#include <stdbool.h>
#include <glib.h>

#include <vconf.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <notification.h>
#include <system_settings.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "common_uic.h"
#include "pager.h"
#include "pager_common.h"
#include "list_util.h"
#include "noti_node.h"
#include "vi_manager.h"
#include "setting_utils.h"
#include "settings.h"
#include "settings_view_featured.h"
#include "noti.h"

#ifdef QP_EMERGENCY_MODE_ENABLE
#include "emergency_mode.h"
#endif

#define FICKUP_TIME_LIMIT 150
#define FICKUP_DISTANCE_LIMIT 160

static void _mapbuf_enable_set(Eina_Bool is_enable);
static void _content_resize(int width, int height, const char *signal);
static int _up_cb(void *event_info, void *data);
static int _down_cb(void *event_info, void *data);
static int _scroll_start_cb(void *event_info, void *data);
static int _scroll_done_cb(void *event_info, void *data);
static int _page_changed_cb(void *event_info, void *data);

static struct info {
	Evas_Object *mapbuf;
	Evas_Object *view;
	Evas_Object *view_scroller;
	Evas_Object *view_box;

	int flick_press_x;
	int flick_press_y;
	int flick_available;
	int flick_time;
} s_info = {
	.mapbuf = NULL,
	.view = NULL,
	.view_scroller = NULL,
	.view_box = NULL,

	.flick_press_x = 0,
	.flick_press_y = 0,
	.flick_available = 0,
	.flick_time = 0,
};

static QP_Page_Handler page_handler  = {
	.status = 0,
	.name = NULL,
	/* func */
	.mapbuf_enable_set = _mapbuf_enable_set,
	.content_resize = _content_resize,
	.down_cb = _down_cb,
	.up_cb = _up_cb,
	.scroll_start_cb = _scroll_start_cb,
	.scroll_done_cb = _scroll_done_cb,
	.page_changed_cb = _page_changed_cb,
};

static void _mapbuf_enable_set(Eina_Bool is_enable)
{
	Evas_Coord y;

	if (s_info.mapbuf != NULL) {
		elm_mapbuf_enabled_set(s_info.mapbuf, is_enable);
	}

	if (is_enable) {
		evas_object_geometry_get(s_info.view_scroller, NULL, &y, NULL, NULL);
		evas_object_move(s_info.view, 0, y);
	}
}

static void _content_resize(int width, int height, const char *signal)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid parameter");

	if (s_info.view != NULL) {
		elm_object_signal_emit(s_info.view, signal, "prog");
		evas_object_size_hint_min_set(s_info.view, width, height);
	}
}

void static _flick_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	int limit_h = 0;
	int limit_partial_h = 0;
	int limit_partial_w = 0;
	Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)event_info;
	retif(ev == NULL, , "event_info is NULL");

	s_info.flick_press_x = ev->canvas.x;
	s_info.flick_press_y = ev->canvas.y;
	s_info.flick_time = ev->timestamp;

	quickpanel_noti_get_geometry(&limit_h, &limit_partial_h, &limit_partial_w);

	if (s_info.flick_press_y > limit_h) {
		s_info.flick_available = 1;
	} else {
		if (s_info.flick_press_x > limit_partial_w && s_info.flick_press_y > limit_partial_h) {
			s_info.flick_available = 1;
		} else {
			s_info.flick_available = 0;
		}
	}
}

static void  _flick_mouse_move_cb(void* data, Evas* e, Evas_Object* obj, void* event_info)
{
	int delta_y = 0;
	Evas_Event_Mouse_Move* ev = event_info;
	retif(ev == NULL, , "event_info is NULL");

	if (s_info.flick_available == 0) {
		return;
	}
	if (ev->cur.output.y > ev->prev.output.y) {
		s_info.flick_available = 0;
		return;
	}
	if (abs(ev->cur.output.x - ev->prev.output.x) > 40) {
		s_info.flick_available = 0;
		return;
	}
	if (ev->timestamp - s_info.flick_time > FICKUP_TIME_LIMIT) {
		s_info.flick_available = 0;
		return;
	}

	delta_y = s_info.flick_press_y - ev->cur.output.y;

	if (delta_y > FICKUP_DISTANCE_LIMIT) {
		ERR("closed by flick up base area");
		quickpanel_uic_close_quickpanel(false, 0);
	}
}

static int _up_cb(void *event_info, void *data)
{
	quickpanel_page_scroll_hold_set(EINA_TRUE);
	quickpanel_page_scroll_freeze_set(EINA_FALSE);

	return QP_OK;
}

static int _down_cb(void *event_info, void *data)
{
	int x = 0, y = 0;
	static int settings_y = -1, settings_h = -1;

#ifdef QP_EMERGENCY_MODE_ENABLE
	if (quickpanel_emergency_mode_is_on()) {
		return QP_OK;
	}
#endif

	if (settings_y == -1 || settings_h == -1) {
		Evas_Object *obj_settings = NULL;
		struct appdata *ad = quickpanel_get_app_data();
		if (ad != NULL && ad->ly != NULL) {
			obj_settings = quickpanel_setting_box_get(ad->ly);
			if (obj_settings != NULL) {
				evas_object_geometry_get(obj_settings, NULL, &settings_y, NULL, &settings_h);
			}
		}
	}

	quickpanel_page_get_touched_pos(&x, &y);

	if (y >= settings_y && y <= settings_y + settings_h) {
		if (quickpanel_settings_is_in_left_edge() == EINA_TRUE) {
			quickpanel_page_scroll_hold_set(EINA_FALSE);
		}
	} else {
		quickpanel_page_scroll_freeze_set(EINA_TRUE);
	}

	return QP_OK;
}

static int _scroll_start_cb(void *event_info, void *data)
{
	quickpanel_vim_set_state_suspend();

	return QP_OK;
}

static int _scroll_done_cb(void *event_info, void *data)
{
	quickpanel_vim_set_state_ready();

	return QP_OK;
}

static int _page_changed_cb(void *event_info, void *data)
{
	quickpanel_setting_view_featured_initial_focus_set();

	return QP_OK;
}

HAPI Evas_Object *quickpanel_page_base_create(Evas_Object *parent, void *data)
{
	Evas_Object *mapbuf = NULL;
	Evas_Object *view = NULL;

	retif(parent == NULL, NULL, "invalid parameter");

	if (s_info.view != NULL) {
		return s_info.view;
	}

	mapbuf = elm_mapbuf_add(parent);
	elm_mapbuf_enabled_set(mapbuf, EINA_FALSE);
	elm_mapbuf_smooth_set(mapbuf, EINA_FALSE);

	evas_object_size_hint_weight_set(mapbuf, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(mapbuf, EVAS_HINT_FILL, EVAS_HINT_FILL);

	view = quickpanel_uic_load_edj(mapbuf, DEFAULT_EDJ, "quickpanel/base", 0);
	retif(view == NULL, NULL, "failed to load base layout");

	Evas_Object *scroller = elm_scroller_add(parent);
	retif(!scroller, NULL, "fail to add scroller");
	//elm_object_style_set(scroller, "default");
	elm_object_style_set(scroller, "bg/default");
	elm_scroller_bounce_set(scroller, EINA_TRUE, EINA_TRUE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(scroller);

	Evas_Object *box = elm_box_add(scroller);
	if (!box) {
		ERR("fail to add box");
		if (scroller != NULL) {
			evas_object_del(scroller);
			scroller = NULL;
		}
		return NULL;
	}
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, 0);
	elm_box_horizontal_set(box, EINA_FALSE);

	elm_object_content_set(scroller, box);
	elm_object_part_content_set(view, "qp.base.list.swallow", scroller);
	evas_object_show(scroller);

	Evas_Object *bg_touch = elm_bg_add(view);
	if (bg_touch != NULL) {
		evas_object_color_set(bg_touch, 255, 255, 255, 250);
		//evas_object_color_set(bg_touch, 0, 0, 0, 0);
		evas_object_event_callback_add(bg_touch,
				EVAS_CALLBACK_MOUSE_DOWN, _flick_mouse_down_cb, NULL);
		evas_object_event_callback_add(bg_touch,
				EVAS_CALLBACK_MOUSE_MOVE, _flick_mouse_move_cb, NULL);
		elm_object_part_content_set(view, "background.touch", bg_touch);
	}


	quickpanel_page_handler_set(mapbuf, &page_handler);

	elm_object_content_set(mapbuf, view);
	evas_object_show(mapbuf);


	s_info.mapbuf = mapbuf;
	s_info.view = view;
	s_info.view_scroller = scroller;
	s_info.view_box = box;

#ifdef QP_EMERGENCY_MODE_ENABLE
	if (quickpanel_emergency_mode_is_on()) {
		quickpanel_page_scroll_freeze_set(EINA_TRUE);
	}
#endif

	return s_info.mapbuf;
}

HAPI Evas_Object *quickpanel_page_base_view_get(const char *view_name)
{
	retif(view_name == NULL, NULL, "invalid parameter");

	if (strcmp(view_name, "LAYOUT") == 0) {
		return s_info.view;
	} else if (strcmp(view_name, "SCROLLER") == 0) {
		return s_info.view_scroller;
	} else if (strcmp(view_name, "BOX") == 0) {
		return s_info.view_box;
	}

	return NULL;
}

HAPI void quickpanel_page_base_focus_allow_set(Eina_Bool is_enable)
{
	elm_object_tree_focus_allow_set(s_info.mapbuf, is_enable);
}
