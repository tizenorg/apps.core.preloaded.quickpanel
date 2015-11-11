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

#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "common_uic.h"
#include "common.h"
#include "pager.h"
#include "pager_common.h"
#include "list_util.h"
#include "vi_manager.h"
#include "settings_gridbox.h"
#include "settings_view_all.h"
#include "quickpanel_def.h"

static void _mapbuf_enable_set(Eina_Bool is_enable);
static void _content_resize(int width, int height, const char *signal);
static int _up_cb(void *event_info, void *data);
static int _down_cb(void *event_info, void *data);
static int _scroll_start_cb(void *event_info, void *data);
static int _scroll_done_cb(void *event_info, void *data);
static int _page_changed_cb(void *event_info, void *data);

static struct info {
	int is_scroll_freezed;
	Evas_Object *mapbuf;
	Evas_Object *view;
	Evas_Object *scroller;
	Evas_Object *layout;
	Evas_Object *view_section_1;
	Evas_Object *view_active_buttons;
} s_info = {
	.is_scroll_freezed = 0,
	.mapbuf = NULL,
	.view = NULL,
	.scroller = NULL,
	.layout = NULL,
	.view_section_1 = NULL,
	.view_active_buttons = NULL,
};

static QP_Page_Handler page_handler  = {
	.status = 0,
	.name = NULL,

	.mapbuf_enable_set = _mapbuf_enable_set,
	.content_resize = _content_resize,
	.down_cb = _down_cb,
	.up_cb = _up_cb,
	.scroll_start_cb = _scroll_start_cb,
	.scroll_done_cb = _scroll_done_cb,
	.page_changed_cb = _page_changed_cb,
};

static inline void _scroll_hold(Evas_Object *viewer)
{
	int hold_count = 0;
	retif(viewer == NULL, , "Invalid parameter!");

	hold_count = elm_object_scroll_hold_get(viewer);

	if (hold_count <= 0) {
		elm_object_scroll_hold_push(viewer);
	}
}

static inline void _scroll_unhold(Evas_Object *viewer)
{
	int i = 0, hold_count = 0;
	retif(viewer == NULL, , "Invalid parameter!");

	hold_count = elm_object_scroll_hold_get(viewer);

	for (i = 0 ; i < hold_count; i++) {
		elm_object_scroll_hold_pop(viewer);
	}
}

static void _mapbuf_enable_set(Eina_Bool is_enable)
{
	Evas_Coord y;

	if (s_info.mapbuf != NULL) {
		elm_mapbuf_enabled_set(s_info.mapbuf, is_enable);
	}

	if (is_enable) {
		evas_object_geometry_get(s_info.mapbuf, NULL, &y, NULL, NULL);
		evas_object_move(s_info.view, 0, y);
	}
}

static void _content_resize(int width, int height, const char *signal)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid parameter");

	if (s_info.view != NULL) {
		elm_object_signal_emit(s_info.view, signal, "prog");
		evas_object_size_hint_min_set(s_info.view, ELM_SCALE_SIZE(width), ELM_SCALE_SIZE(height));
	}
	if (s_info.layout != NULL) {
		elm_object_signal_emit(s_info.layout, signal, "prog");
		if (strcmp(signal, "portrait") == 0) {
			evas_object_size_hint_min_set(s_info.layout, ELM_SCALE_SIZE(width), ELM_SCALE_SIZE(height));
		} else {
			evas_object_size_hint_min_set(s_info.layout, ELM_SCALE_SIZE(width), ELM_SCALE_SIZE(height));
		}
	}
}

static int _up_cb(void *event_info, void *data)
{
	if (s_info.is_scroll_freezed == 0) {
		quickpanel_page_scroll_hold_set(EINA_TRUE);
	}

	return QP_OK;
}

static int _down_cb(void *event_info, void *data)
{
	if (s_info.is_scroll_freezed == 0) {
		quickpanel_page_scroll_hold_set(EINA_FALSE);
	}

	return QP_OK;
}

static int _scroll_start_cb(void *event_info, void *data)
{
	return QP_OK;
}

static int _scroll_done_cb(void *event_info, void *data)
{
	quickpanel_vim_set_state_suspend();

	return QP_OK;
}

static int _page_changed_cb(void *event_info, void *data)
{
	return QP_OK;
}

static void _deleted_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("deleted view");
	if (s_info.view_active_buttons != NULL) {
		quickpanel_settings_gridbox_remove(s_info.view_active_buttons);
	}
	if (s_info.view_section_1 != NULL) {
		evas_object_del(s_info.view_section_1);
	}
	if (s_info.layout != NULL) {
		evas_object_del(s_info.layout);
	}
	if (s_info.scroller != NULL) {
		evas_object_del(s_info.scroller);
	}
	if (s_info.view != NULL) {
		evas_object_del(s_info.view);
	}

	s_info.mapbuf = NULL;
	s_info.view = NULL;
	s_info.scroller = NULL;
	s_info.layout = NULL;
	s_info.view_section_1 = NULL;
	s_info.view_active_buttons = NULL;
}

HAPI Evas_Object *quickpanel_page_setting_all_create(Evas_Object *parent, void *data)
{
	Evas_Object *mapbuf = NULL;
	Evas_Object *view = NULL;
	Evas_Object *scroller = NULL;
	Evas_Object *layout = NULL;

	retif(parent == NULL, NULL, "invalid parameter");

	if (s_info.view == NULL) {
		mapbuf = elm_mapbuf_add(parent);
		elm_mapbuf_enabled_set(mapbuf, EINA_FALSE);
		elm_mapbuf_smooth_set(mapbuf, EINA_FALSE);

		view = quickpanel_uic_load_edj(mapbuf, DEFAULT_EDJ, "quickpanel/page_setting_all_base", 0);
		retif(view == NULL, NULL, "failed to load editing layout");

		scroller = elm_scroller_add(view);
		retif(!scroller, NULL, "fail to add scroller");
		elm_object_style_set(scroller, "list_effect");
		elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_TRUE);
		elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
		evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_fill_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_show(scroller);

		layout = quickpanel_uic_load_edj(scroller, DEFAULT_EDJ, "quickpanel/page_setting_all", 0);
		retif(layout == NULL, NULL, "failed to load editing layout");

		quickpanel_page_handler_set(mapbuf, &page_handler);
		evas_object_event_callback_add(mapbuf, EVAS_CALLBACK_DEL, _deleted_cb, NULL);

		elm_object_content_set(scroller, layout);
		elm_object_part_content_set(view, "object.layout", scroller);

		elm_object_content_set(mapbuf, view);
		evas_object_show(mapbuf);

		s_info.mapbuf = mapbuf;
		s_info.view = view;
		s_info.scroller = scroller;
		s_info.layout = layout;
	}

	return s_info.mapbuf;
}

HAPI Evas_Object *quickpanel_page_setting_all_view_get(const char *view_name)
{
	retif(view_name == NULL, NULL, "invalid parameter");

	if (strcmp(view_name, "VIEW") == 0) {
		return s_info.view;
	} else if (strcmp(view_name, "LAYOUT") == 0) {
		return s_info.layout;
	} else if (strcmp(view_name, "SECTION.1") == 0) {
		return s_info.view_section_1;
	} else if (strcmp(view_name, "ACTIVE.BUTTONS") == 0) {
		return s_info.view_active_buttons;
	}

	return NULL;
}

HAPI void quickpanel_page_setting_all_view_set(const char *view_name, Evas_Object *view)
{
	retif(s_info.view == NULL, , "invalid parameter");
	retif(s_info.layout == NULL, , "invalid parameter");
	retif(view_name == NULL, , "invalid parameter");
	retif(view == NULL, , "invalid parameter");

	if (strcmp(view_name, "SECTION.1") == 0) {
		elm_object_part_content_set(s_info.layout, "object.section.1", view);
		s_info.view_section_1 = view;
	} else if (strcmp(view_name, "ACTIVE.BUTTONS") == 0) {
		elm_object_part_content_set(s_info.layout, "object.active.buttons", view);
		s_info.view_active_buttons = view;
	}
}

HAPI void quickpanel_page_setting_all_freeze_set(Eina_Bool is_freeze)
{
	if (is_freeze == EINA_TRUE) {
		s_info.is_scroll_freezed = 1;
		quickpanel_page_scroll_freeze_set(EINA_TRUE);
		_scroll_hold(s_info.scroller);
	} else {
		s_info.is_scroll_freezed = 0;
		quickpanel_page_scroll_freeze_set(EINA_FALSE);
		_scroll_unhold(s_info.scroller);
	}
}

HAPI Eina_Bool quickpanel_page_setting_all_is_page_showed(void)
{
	if (quickpanel_pager_current_page_get() == PAGE_IDX_EDITING) {
		return EINA_TRUE;
	}

	return EINA_FALSE;
}

HAPI void quickpanel_page_setting_all_focus_allow_set(Eina_Bool is_enable)
{
	elm_object_tree_focus_allow_set(s_info.mapbuf, is_enable);
}
