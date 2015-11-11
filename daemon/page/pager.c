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

#include "common.h"
#include "pager.h"
#include "pager_common.h"
#include "quickpanel-ui.h"
#include "quickpanel_def.h"

static int _init(void *data);
static void _init_job_cb(void *data);
static int _fini(void *data);
static int _resume(void *data);
static void _opened(void *data);
static void _closed(void *data);
static void _refresh(void *data);
static inline void _page_mapbuf_enable_set(Evas_Object *box, int is_enable);

#define ENABLE_MAPBUF 0

typedef enum _qp_pager_state_type {
	PAGER_STATE_NOT_READY = 0,
	PAGER_STATE_IDLE,
	PAGER_STATE_WILL_SCROLL,
	PAGER_STATE_SCROLLING,
	PAGER_STATE_FINISHED_SCROLLING,
} qp_pager_state_type;

QP_Module pager = {
	.name = "pager",
	.init = _init,
	.init_job_cb = _init_job_cb,
	.fini = _fini,
	.resume = _resume,
	.qp_opened = _opened,
	.qp_closed = _closed,
	.refresh = _refresh,
};

static struct info {
	qp_pager_state_type state;
	int last_page;
	int is_in_edge;

	Evas_Object *view_scroller;
	Evas_Object *view_box;

	Ecore_Event_Handler *hdl_move;
	Ecore_Event_Handler *hdl_up;
	Ecore_Event_Handler *hdl_down;

	Ecore_Timer *timer_scroll_adj;

	int scroll_start_from;
} s_info = {
	.state = PAGER_STATE_NOT_READY,
	.last_page = PAGE_IDX_MAIN,
	.is_in_edge = 0,

	.view_scroller = NULL,
	.view_box = NULL,

	.hdl_move = NULL,
	.hdl_up = NULL,
	.hdl_down = NULL,

	.timer_scroll_adj = NULL,

	.scroll_start_from = -1,
};

static inline void _set_state(qp_pager_state_type state)
{
	if (state == PAGER_STATE_IDLE) {
		_page_mapbuf_enable_set(s_info.view_box, 0);
	} else  if (state == PAGER_STATE_WILL_SCROLL || state == PAGER_STATE_SCROLLING) {
		_page_mapbuf_enable_set(s_info.view_box, 1);
	}
	s_info.state = state;
}

static inline qp_pager_state_type _get_state(void)
{
	return s_info.state;
}

static inline int _last_page_get(void)
{
	return s_info.last_page;
}

static inline void _page_show(int page_index)
{
	elm_scroller_page_show(s_info.view_scroller, page_index, 0);
	s_info.last_page = page_index;
}

static Eina_Bool _page_adjust_timer_cb(void *data)
{
	int index = 0;

	if (s_info.timer_scroll_adj != NULL) {
		ecore_timer_del(s_info.timer_scroll_adj);
		s_info.timer_scroll_adj = NULL;
	}

	elm_scroller_current_page_get(s_info.view_scroller, &index, NULL);
	elm_scroller_page_bring_in(s_info.view_scroller, index, 0);
	s_info.last_page = index;

	return ECORE_CALLBACK_CANCEL;
}

static inline void _page_adjust(int is_use_timer)
{
	if (s_info.timer_scroll_adj != NULL) {
		ecore_timer_del(s_info.timer_scroll_adj);
		s_info.timer_scroll_adj = NULL;
	}

	if (is_use_timer) {
		s_info.timer_scroll_adj = ecore_timer_add(0.1, _page_adjust_timer_cb, NULL);
	} else {
		_page_adjust_timer_cb(NULL);
	}
}

static inline int _current_page_index_get(void)
{
	int index = 0;

	elm_scroller_current_page_get(s_info.view_scroller, &index, NULL);

	return index;
}

static inline Evas_Object *_current_page_get(void)
{
	int index = 0;
	int list_cnt = 0;
	Eina_List *list = NULL;
	static int last_page = -1;
	Evas_Object *obj = NULL;

	elm_scroller_current_page_get(s_info.view_scroller, &index, NULL);

	if (last_page != index) {
		DBG("current selected page:%d", index);
		last_page = index;
	}

	list = elm_box_children_get(s_info.view_box);
	if (list != NULL) {
		list_cnt = eina_list_count(list);

		if (index < list_cnt) {
			obj = (Evas_Object *)eina_list_nth(list, index);
		}
		eina_list_free(list);
	}

	return obj;
}

#if ENABLE_MAPBUF
static void _mapbuf_job_cb(void *data)
{
	Eina_List *list = NULL;
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *box = s_info.view_box;
	Evas_Object *item = NULL;
	int is_enable = (int)data;
	QP_Page_Handler *page_handler = NULL;
	retif(box == NULL, , "invalid parameter");

	list = elm_box_children_get(box);
	retif(list == NULL, , "empty box");

	SDBG("mapbuf enable:%d", is_enable);

	EINA_LIST_FOREACH_SAFE(list, l, l_next, item) {
		page_handler = quickpanel_page_handler_get(item);
		if (page_handler != NULL) {
			if (page_handler->mapbuf_enable_set != NULL) {
				if (elm_config_access_get() == EINA_FALSE) {
					if (is_enable == 1) {
						page_handler->mapbuf_enable_set(EINA_TRUE);
					} else {
						page_handler->mapbuf_enable_set(EINA_FALSE);
					}
				} else {
					page_handler->mapbuf_enable_set(EINA_FALSE);
				}
			}
		}
	}

	eina_list_free(list);
}
#endif

static inline void _page_mapbuf_enable_set(Evas_Object *box, int is_enable)
{
#if ENABLE_MAPBUF
	ecore_job_add(_mapbuf_job_cb, (void *)is_enable);
#endif
}

static inline void _page_resize(Evas_Object *box, int width, int height, const char *signal)
{
	Eina_List *list = NULL;
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *item = NULL;
	QP_Page_Handler *page_handler = NULL;
	retif(box == NULL, , "invalid parameter");

	list = elm_box_children_get(box);
	retif(list == NULL, , "empty box");

	EINA_LIST_FOREACH_SAFE(list, l, l_next, item) {
		page_handler = quickpanel_page_handler_get(item);

		if (page_handler != NULL) {
			if (page_handler->content_resize != NULL) {
				page_handler->content_resize(width, height, signal);
			}
		}
	}

	eina_list_free(list);
}

static inline void _page_rotation(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retif(ad == NULL, , "Invalid parameter!");

	if (ad->angle == 90 || ad->angle == 270) {
		_page_resize(s_info.view_box, ad->win_height, ad->win_width - ELM_SCALE_SIZE((QP_DATE_H + QP_HANDLE_H)), "landscape");
	} else {
		_page_resize(s_info.view_box, ad->win_width, ad->win_height - ELM_SCALE_SIZE((QP_DATE_H + QP_HANDLE_H)), "portrait");
	}
}

static Eina_Bool _up_cb(void *data, int type, void *event)
{
	Evas_Object *page = _current_page_get();
	QP_Page_Handler *page_handler = NULL;
	retif(page == NULL, EINA_FALSE, "Invalid parameter!");

	if (_get_state() == PAGER_STATE_WILL_SCROLL) {
		_set_state(PAGER_STATE_SCROLLING);
		_page_adjust(1);
	}

	page_handler = quickpanel_page_handler_get(page);
	retif(page_handler == NULL, EINA_FALSE, "no page handler found");

	if (page_handler->up_cb != NULL) {
		page_handler->up_cb(event, NULL);
	}

	if (_get_state() == PAGER_STATE_SCROLLING) {
		_page_adjust(1);
	}

	return EINA_TRUE;
}

static Eina_Bool _down_cb(void *data, int type, void *event)
{
	Evas_Object *page = _current_page_get();
	QP_Page_Handler *page_handler = NULL;
	retif(page == NULL, EINA_FALSE, "Invalid parameter!");

	page_handler = quickpanel_page_handler_get(page);
	retif(page_handler == NULL, EINA_FALSE, "no page handler found");

	if (page_handler->down_cb != NULL) {
		page_handler->down_cb(event, NULL);
	}

	return EINA_TRUE;
}

static void _scroller_anim_start_cb(void *data, Evas_Object *scroller, void *event_info)
{
	Evas_Object *page = _current_page_get();
	QP_Page_Handler *page_handler = NULL;
	retif(page == NULL, , "Invalid parameter!");

	s_info.scroll_start_from = _current_page_index_get();

	if (_get_state() == PAGER_STATE_IDLE) {
		_set_state(PAGER_STATE_WILL_SCROLL);

		page_handler = quickpanel_page_handler_get(page);
		retif(page_handler == NULL, , "no page handler found");

		if (page_handler->scroll_start_cb != NULL) {
			page_handler->scroll_start_cb(event_info, NULL);
		}
	}
}

static void _scroller_anim_stop_cb(void *data, Evas_Object *scroller, void *event_info)
{
	Evas_Object *page = _current_page_get();
	QP_Page_Handler *page_handler = NULL;
	retif(page == NULL, , "Invalid parameter!");

	if (s_info.is_in_edge == 1 || _get_state() == PAGER_STATE_FINISHED_SCROLLING) {
		_set_state(PAGER_STATE_IDLE);

		page_handler = quickpanel_page_handler_get(page);
		retif(page_handler == NULL, , "no page handler found");

		if (page_handler->scroll_done_cb != NULL) {
			page_handler->scroll_done_cb(event_info, NULL);
		}

		_page_mapbuf_enable_set(s_info.view_box, 0);
	}
}

static void _scroller_edge_cb(void *data, Evas_Object *scroller, void *event_info)
{
	if (_get_state() == PAGER_STATE_WILL_SCROLL
			|| _get_state() == PAGER_STATE_SCROLLING) {
		_set_state(PAGER_STATE_FINISHED_SCROLLING);
		s_info.is_in_edge = 1;
		_page_adjust(0);
	}
}

static void _scroller_scroll_cb(void *data, Evas_Object *scroller, void *event_info)
{
	s_info.is_in_edge = 0;
}

/*****************************************************************************
 *
 * Util functions
 *
 *****************************************************************************/
static int _init(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");
	retif(s_info.view_scroller == NULL, QP_FAIL, "Invalid parameter!");

	s_info.hdl_up = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, _up_cb
			, s_info.view_scroller);
	s_info.hdl_down = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, _down_cb
			, s_info.view_scroller);

	evas_object_smart_callback_add(s_info.view_scroller, "scroll,drag,start",
			_scroller_anim_start_cb, s_info.view_scroller);
	evas_object_smart_callback_add(s_info.view_scroller, "scroll,anim,stop",
			_scroller_anim_stop_cb, s_info.view_scroller);
	evas_object_smart_callback_add(s_info.view_scroller, "scroll,drag,stop",
			_scroller_anim_stop_cb, s_info.view_scroller);
	evas_object_smart_callback_add(s_info.view_scroller, "scroll",
			_scroller_scroll_cb, s_info.view_scroller);

	evas_object_smart_callback_add(s_info.view_scroller, "edge,left",
			_scroller_edge_cb, s_info.view_scroller);
	evas_object_smart_callback_add(s_info.view_scroller, "edge,right",
			_scroller_edge_cb, s_info.view_scroller);

	_set_state(PAGER_STATE_IDLE);

	return QP_OK;
}

static void _init_job_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retif(ad == NULL, , "Invalid parameter!");
	retif(s_info.view_scroller == NULL, , "Invalid parameter!");

	_page_rotation(ad);

	_page_show(PAGE_IDX_MAIN);
	evas_object_show(s_info.view_scroller);
}

static int _fini(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	if (s_info.hdl_up != NULL) {
		ecore_event_handler_del(s_info.hdl_up);
	}
	if (s_info.hdl_down != NULL) {
		ecore_event_handler_del(s_info.hdl_down);
	}

	return QP_OK;
}

static int _resume(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	return QP_OK;
}

static void _opened(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	_page_mapbuf_enable_set(s_info.view_box, 0);

	quickpanel_page_editing_icon_visible_status_update();
}

static void _closed(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");
	retif(s_info.view_scroller == NULL, , "Invalid parameter!");

	_page_show(PAGE_IDX_MAIN);
	_page_mapbuf_enable_set(s_info.view_box, 0);

	quickpanel_page_editing_icon_visible_status_update();
}

static void _refresh(void *data) {
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");
	retif(s_info.view_box == NULL, , "Invalid parameter!");
	retif(s_info.view_scroller == NULL, , "Invalid parameter!");

	_page_rotation(ad);
}

static void _scroller_resized_cb(void *data, Evas * e,
		Evas_Object * obj, void *event_info)
{
	_page_show(PAGE_IDX_MAIN);
	evas_object_event_callback_del(s_info.view_scroller, EVAS_CALLBACK_RESIZE, _scroller_resized_cb);
}

HAPI Evas_Object *quickpanel_pager_new(Evas_Object *parent, void *data)
{
	Evas_Object *box = NULL;
	Evas_Object *scroller = NULL;

	retif(parent == NULL, NULL, "failed to memory allocation");

	if (s_info.view_scroller != NULL && s_info.view_box != NULL) {
		return s_info.view_scroller;
	}

	scroller = elm_scroller_add(parent);
	retif(!scroller, NULL, "fail to add scroller");
	elm_scroller_bounce_set(scroller, EINA_TRUE, EINA_TRUE);
	elm_scroller_propagate_events_set(scroller, EINA_FALSE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_scroller_page_relative_set (scroller, 1.0, 0.0);

	box = elm_box_add(scroller);
	if (!box) {
		ERR("fail to add box");
		if (scroller != NULL) {
			evas_object_del(scroller);
			scroller = NULL;
		}
		return EINA_FALSE;
	}
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(box, EINA_TRUE);
	evas_object_show(box);

	elm_object_content_set(scroller, box);

	s_info.view_scroller = scroller;
	s_info.view_box = box;

	evas_object_event_callback_add(s_info.view_scroller, EVAS_CALLBACK_RESIZE, _scroller_resized_cb, NULL);

	return scroller;
}

HAPI void quickpanel_pager_destroy(void)
{
	if (s_info.view_box != NULL) {
		elm_box_unpack_all(s_info.view_box);
		evas_object_del(s_info.view_box);
		s_info.view_box = NULL;
	}
	if (s_info.view_scroller != NULL) {
		evas_object_del(s_info.view_scroller);
		s_info.view_scroller = NULL;
	}
}

HAPI Evas_Object *quickpanel_pager_view_get(const char *view_name)
{
	retif(view_name == NULL, NULL, "invalid parameter");

	if (strcmp(view_name, "SCROLLER") == 0) {
		return s_info.view_scroller;
	} else if (strcmp(view_name, "BOX") == 0) {
		return s_info.view_box;
	}

	return NULL;
}

HAPI int quickpanel_pager_current_page_get(void)
{
	int index = 0;

	elm_scroller_current_page_get(s_info.view_scroller, &index, NULL);

	return index;
}

HAPI void quickpanel_pager_page_set(int page_index, int need_resize)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid parameter");

	if (need_resize) {
		_page_rotation(ad);
	}
	_page_show(page_index);

	evas_object_show(s_info.view_scroller);
}

HAPI void quickpanel_pager_mapbuf_set(int is_enable)
{
	_page_mapbuf_enable_set(s_info.view_box, is_enable);
}
