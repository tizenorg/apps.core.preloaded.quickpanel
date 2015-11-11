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
#include <string.h>
#include <glib.h>

#include <vconf.h>
#include <notification.h>
#include <notification_internal.h>
#include <notification_ongoing_flag.h>
#include <system_settings.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "list_util.h"
#include "quickpanel_def.h"
#include "vi_manager.h"
#include "noti_node.h"
#include "noti_list_item.h"
#include "noti.h"
#include "noti_util.h"
#include "animated_icon.h"

#ifdef QP_SCREENREADER_ENABLE
#include "accessibility.h"
#endif

#ifdef QP_ANIMATED_IMAGE_ENABLE
#include "animated_image.h"
#endif

extern Noti_View_H noti_view_h;
extern Noti_View_H ongoing_noti_view_h;

#define THRESHOLD_DRAGGING_TIME_LIMIT 1.0
#define LIMIT_ZOOM_RATIO 0.57
#define LIMIT_FADEOUT_RATIO 0.1
#define THRESHOLD_DELETE_START 80
#define THRESHOLD_DELETE_START_Y_LIMIT 60
#define THRESHOLD_DISTANCE (300)

static struct _info {
	int item_debug_step;
	Noti_View_H *view_handlers[NOTIFICATION_LY_MAX + 1];
} s_info = {
	.item_debug_step = 0,
	.view_handlers = {
		NULL,
		&noti_view_h,
		&noti_view_h,
		&noti_view_h,
		&ongoing_noti_view_h,
		&ongoing_noti_view_h,
		NULL,
	},
};

static int _is_item_deletable_by_gesture(noti_list_item_h *handler)
{
	notification_type_e type = NOTIFICATION_TYPE_NONE;
	notification_ly_type_e ly_type = NOTIFICATION_LY_NONE;

	retif(handler == NULL, 0, "Invalid parameter!");
	retif(handler->noti_node == NULL, 0, "Invalid parameter!");
	retif(handler->noti_node->noti == NULL, 0, "Invalid parameter!");
	bool ongoing_flag = false;

	notification_h noti = handler->noti_node->noti;

	notification_get_type(noti, &type);
	notification_get_layout(noti, &ly_type);
	notification_get_ongoing_flag(noti, &ongoing_flag);

	if( (type == NOTIFICATION_TYPE_ONGOING && ongoing_flag) ||
			(type == NOTIFICATION_TYPE_ONGOING && ly_type == NOTIFICATION_LY_ONGOING_PROGRESS)) {
		return 0;
	}

	return 1;
}

static void _item_handler_set(Evas_Object *item, noti_list_item_h *handler)
{
	retif(item == NULL, , "Invalid parameter!");
	retif(handler == NULL, , "Invalid parameter!");

	evas_object_data_set(item, E_DATA_NOTI_LIST_ITEM_H, handler);
}

static noti_list_item_h *_item_handler_get(Evas_Object *item)
{
	retif(item == NULL, NULL, "Invalid parameter!");

	return evas_object_data_get(item, E_DATA_NOTI_LIST_ITEM_H);
}

static noti_node_item *_get_noti_node(Evas_Object *item)
{
	retif(item == NULL, NULL, "invalid parameter");

	noti_list_item_h *handler = _item_handler_get(item);
	retif(handler == NULL, NULL, "invalid parameter");

	return quickpanel_noti_node_get_by_priv_id(handler->priv_id);
}

static void _response_callback_call(Evas_Object *item, const char *emission)
{
	static double time_called = 0.0;
	retif(item == NULL, , "invalid parameter");
	retif(emission == NULL, , "invalid parameter");

	if (time_called == 0.0) {
		time_called = ecore_loop_time_get();
	} else {
		if ((ecore_loop_time_get() - time_called) < 0.4) {
			DBG("click rejected");
			return;
		}
		time_called = ecore_loop_time_get();
	}

	response_cb cb = NULL;
	noti_list_item_h *handler = _item_handler_get(item);
	noti_node_item *noti_node = _get_noti_node(item);
	if (handler != NULL && noti_node != NULL) {
		if (strncmp(emission,"selected", strlen("selected")) == 0) {
			if (handler->need_to_cancel_press > 0) {
				handler->need_to_cancel_press = 0;
				return;
			}

			cb = handler->selected_cb;
			if (cb != NULL) {
				cb(noti_node, item);
			}
		}
		if (strncmp(emission,"button_1", strlen("button_1")) == 0) {
			if (handler->need_to_cancel_press > 0) {
				handler->need_to_cancel_press = 0;
				return;
			}

			cb = handler->button_1_cb;
			if (cb != NULL) {
				cb(noti_node, item);
			}
		}
		if (strncmp(emission,"deleted", strlen("deleted")) == 0) {
			cb = handler->deleted_cb;
			if (cb != NULL) {
				cb(noti_node, item);
			}
		}
	}
}

static void _signal_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
	retif(data == NULL, , "invalid parameter");
	retif(o == NULL, , "invalid parameter");
	retif(emission == NULL, , "invalid parameter");

	_response_callback_call(o, emission);
}

void static _mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	int w = 0, h = 0;
	noti_list_item_h *handler = NULL;
	Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)event_info;
	retif(ev == NULL, , "event_info is NULL");

	handler = _item_handler_get(obj);
	retif(handler == NULL, , "handler is NULL");

	evas_object_geometry_get(obj, NULL, NULL, &w, &h);

	handler->press_x = ev->canvas.x;
	handler->press_y = ev->canvas.y;
	handler->obj_w = w;
	handler->obj_h = h;
	handler->state = NOTILISTITEM_STATE_NORMAL;

	s_info.item_debug_step = 1;
	SDBG("mouse down:%d %d %d", handler->obj_w, handler->obj_h, handler->state);

	if (handler->vi != NULL) {
		quickpanel_vi_user_event_del(handler->vi);
		handler->vi = NULL;
	}

	handler->need_to_cancel_press = 0;
}

static void _mouse_move_cb(void* data, Evas* e, Evas_Object* obj, void* event_info)
{
	int delta_x = 0;
	static int vi_start_x = 0;
	static int delta_prev = -1;
	int x = 0, y = 0;
	int w = 0, h = 0;
	noti_list_item_h *handler = NULL;
	Evas_Map *map = NULL;
	Evas_Event_Mouse_Move* ev = event_info;
	QP_VI *vi = NULL;
	retif(ev == NULL, , "event_info is NULL");

	handler = _item_handler_get(obj);
	retif(handler == NULL, , "handler is NULL");

	if (handler->state == NOTILISTITEM_STATE_GETSTURE_CANCELED) {
		DBG("deletion has been canceled");
		return;
	}

	evas_object_geometry_get(obj, &x, &y, &w, &h);
	delta_x = ev->cur.output.x - handler->press_x;

	if (s_info.item_debug_step == 1) {
		SDBG("mouse move:%d %d %d", delta_x, vi_start_x, handler->state);
		s_info.item_debug_step = 2;
	}

	if (handler->state == NOTILISTITEM_STATE_NORMAL && _is_item_deletable_by_gesture(handler) == 1) {
		if (abs(delta_x) >= THRESHOLD_DELETE_START) {
			DBG("start a deletion");
			handler->state = NOTILISTITEM_STATE_GETSTURE_WAIT;

			vi_start_x = delta_x;

			vi = quickpanel_vi_new_with_data(
					VI_OP_DELETE,
					QP_ITEM_TYPE_NOTI,
					NULL,
					obj,
					NULL,
					NULL,
					NULL,
					NULL,
					vi,
					NULL,
					0,
					0);
			handler->vi = vi;
			handler->need_to_cancel_press = 1;
			quickpanel_vi_user_event_add(vi);
		}
	} else if (handler->state == NOTILISTITEM_STATE_GETSTURE_WAIT) {
		if (delta_prev != delta_x) {
			map = evas_map_new(4);
			if (map != NULL) {
				evas_map_util_points_populate_from_object(map, obj);
				evas_map_util_points_populate_from_geometry(map, x + delta_x - vi_start_x, y, w, h, 0);
				evas_object_map_enable_set(obj, EINA_TRUE);
				evas_object_map_set(obj, map);
				evas_map_free(map);
				quickpanel_list_util_scroll_freeze_set(EINA_TRUE);
			}
			delta_prev = delta_x;
		}
	}

	handler->distance = delta_x;
}

static void _mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	int x = 0;
	noti_list_item_h *handler;

	handler = _item_handler_get(obj);
	retif(handler == NULL, , "handler is NULL");

	quickpanel_list_util_scroll_freeze_set(EINA_FALSE);

	if (s_info.item_debug_step == 2) {
		SDBG("mouse up:%d", handler->state);
		s_info.item_debug_step = 3;
	}

	if (handler->state == NOTILISTITEM_STATE_GETSTURE_WAIT) {
		if (abs(handler->distance) >= (THRESHOLD_DISTANCE - 10) && _is_item_deletable_by_gesture(handler) == 1) {
			x = abs(handler->distance) - THRESHOLD_DELETE_START;

			if (handler->distance > 0) {
				Elm_Transit *transit_flick;

				evas_object_map_set(obj, NULL);
				transit_flick = elm_transit_add();
				if (transit_flick != NULL) {
					elm_transit_effect_translation_add(transit_flick, x, 0, 480, 0);
					elm_transit_object_add(transit_flick, obj);
					elm_transit_duration_set(transit_flick, 0.25 * (480 - x ) / 480);
					elm_transit_tween_mode_set(transit_flick, ELM_TRANSIT_TWEEN_MODE_LINEAR);
					elm_transit_objects_final_state_keep_set(transit_flick, EINA_TRUE);
					elm_transit_go(transit_flick);

					_response_callback_call(obj, "deleted");
				}
			} else if (handler->distance < 0) {
				Elm_Transit *transit_flick;

				evas_object_map_set(obj, NULL);
				transit_flick = elm_transit_add();
				if (transit_flick != NULL) {
					elm_transit_effect_translation_add(transit_flick, -x, 0, -480, 0);
					elm_transit_object_add(transit_flick, obj);
					elm_transit_duration_set(transit_flick, 0.25 * ( 480 - x ) / 480);
					elm_transit_tween_mode_set(transit_flick, ELM_TRANSIT_TWEEN_MODE_LINEAR);
					elm_transit_objects_final_state_keep_set(transit_flick, EINA_TRUE);
					elm_transit_go(transit_flick);

					_response_callback_call(obj, "deleted");
				}
			}
		} else {
			evas_object_map_enable_set(obj, EINA_FALSE);
		}

		if (handler->vi != NULL) {
			quickpanel_vi_user_event_del(handler->vi);
			handler->vi = NULL;
		}
	} else if (handler->state == NOTILISTITEM_STATE_GETSTURE_CANCELED) {
		evas_object_map_enable_set(obj, EINA_FALSE);

		if (handler->vi != NULL) {
			quickpanel_vi_user_event_del(handler->vi);
			handler->vi = NULL;
		}
	}

	handler->state = NOTILISTITEM_STATE_NORMAL;
}

static Evas_Event_Flags _flick_end_cb(void *data, void *event_info)
{
	int x = 0;
	noti_list_item_h *handler = NULL;
	Evas_Object *view = NULL;
	Elm_Transit *transit_flick = NULL;
	Elm_Gesture_Momentum_Info *info = (Elm_Gesture_Momentum_Info *)event_info;

	view = (Evas_Object *)data;
	handler = _item_handler_get(view);

	if (handler != NULL) {
		handler->state = NOTILISTITEM_STATE_GETSTURE_CANCELED;

		if (_is_item_deletable_by_gesture(handler) != 1) {
			return EVAS_EVENT_FLAG_NONE;
		}

		x = abs(handler->distance) - THRESHOLD_DELETE_START;
	}

	if (info->x2 - info->x1 > 50) {
		DBG("Flick event is occurred to right.");
		evas_object_map_set(view, NULL);
		transit_flick = elm_transit_add();
		if (transit_flick != NULL) {
			elm_transit_effect_translation_add(transit_flick, x, 0, 480, 0);
			elm_transit_object_add(transit_flick, view);
			elm_transit_duration_set(transit_flick, 0.25 * (480 - x ) /480);
			elm_transit_tween_mode_set(transit_flick, ELM_TRANSIT_TWEEN_MODE_LINEAR);
			elm_transit_objects_final_state_keep_set(transit_flick, EINA_TRUE);
			elm_transit_go(transit_flick);

			_response_callback_call(view, "deleted");
		}
	} else if (info->x1 - info->x2 > 50) {
		DBG("Flick event is occurred to left.");
		evas_object_map_set(view, NULL);
		transit_flick = elm_transit_add();
		if (transit_flick != NULL) {
			elm_transit_effect_translation_add(transit_flick, -x, 0, -480, 0);
			elm_transit_object_add(transit_flick, view);
			elm_transit_duration_set(transit_flick, 0.25 * ( 480 - x ) / 480);
			elm_transit_tween_mode_set(transit_flick, ELM_TRANSIT_TWEEN_MODE_LINEAR);
			elm_transit_objects_final_state_keep_set(transit_flick, EINA_TRUE);
			elm_transit_go(transit_flick);

			_response_callback_call(view, "deleted");
		}
	}

	return EVAS_EVENT_FLAG_NONE;
}

HAPI Evas_Object *quickpanel_noti_list_item_create(Evas_Object *parent, notification_h noti)
{
	Evas_Object *view = NULL;
	retif(noti == NULL, NULL, "invalid parameter");

	notification_ly_type_e layout = NOTIFICATION_LY_NOTI_EVENT_SINGLE;
	notification_get_layout(noti, &layout);

	retif(s_info.view_handlers[layout] == NULL, NULL, "invalid parameter");
	retif(s_info.view_handlers[layout]->create == NULL, NULL, "invalid parameter");

	view = s_info.view_handlers[layout]->create(noti, parent);
	if (view != NULL) {
		noti_list_item_h *handler = (noti_list_item_h *) malloc(sizeof(noti_list_item_h));
		retif(handler == NULL, NULL, "failed to allocate a memory");

		memset(handler, 0, sizeof(noti_list_item_h));

		handler->layout = layout;
		handler->status = STATE_NORMAL;
		handler->noti_node = NULL;
		handler->state = NOTILISTITEM_STATE_NORMAL;

		_item_handler_set(view, handler);

		Evas_Object *focus = quickpanel_accessibility_ui_get_focus_object(view);
		elm_object_part_content_set(view, "focus", focus);

		//add event
		elm_object_signal_callback_add(view,
				"selected",
				"edje",
				_signal_cb,
				parent
				);

		//add event
		elm_object_signal_callback_add(view,
				"button_1",
				"edje",
				_signal_cb,
				parent
				);

		//add event
		elm_object_signal_callback_add(view,
				"deleted",
				"edje",
				_signal_cb,
				parent
				);

		DBG("created box:%p", view);

		evas_object_event_callback_add(view, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, NULL);
		evas_object_event_callback_add(view, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move_cb, NULL);
		evas_object_event_callback_add(view, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, NULL);

		Evas_Object *gl = elm_gesture_layer_add(parent);
		elm_gesture_layer_flick_time_limit_ms_set(gl, 300);
		elm_gesture_layer_attach(gl, view);

		elm_gesture_layer_cb_set(gl, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_END, _flick_end_cb, view);
	} else {
		ERR("failed to create notification view(%s)"
				, s_info.view_handlers[layout]->name);
	}

	return view;
}

HAPI void quickpanel_noti_list_item_update(Evas_Object *item)
{
	retif(item == NULL, , "invalid parameter");

	noti_list_item_h *handler = _item_handler_get(item);
	if (handler != NULL) {
		retif(s_info.view_handlers[handler->layout] == NULL, , "invalid parameter");
		retif(s_info.view_handlers[handler->layout]->update == NULL, , "invalid parameter");

		noti_node_item *noti_node = _get_noti_node(item);
		s_info.view_handlers[handler->layout]->update(noti_node, handler->layout, item);
	}
}

HAPI void quickpanel_noti_list_item_remove(Evas_Object *item)
{
	retif(item == NULL, , "invalid parameter");

	noti_list_item_h *handler = _item_handler_get(item);
	if (handler != NULL) {
		retif(s_info.view_handlers[handler->layout] == NULL, , "invalid parameter");

		if (s_info.view_handlers[handler->layout] != NULL) {
			if (s_info.view_handlers[handler->layout]->remove != NULL) {
				noti_node_item *noti_node = _get_noti_node(item);
				s_info.view_handlers[handler->layout]->remove(noti_node, handler->layout, item);
			}
		}

		free(handler);
	}

	evas_object_data_del(item, E_DATA_NOTI_LIST_ITEM_H);
	evas_object_del(item);
	item = NULL;
}

HAPI void quickpanel_noti_list_item_set_status(Evas_Object *item, int status)
{
	retif(item == NULL, , "invalid parameter");

	noti_list_item_h *handler = _item_handler_get(item);
	if (handler != NULL) {
		handler->status = status;
	}
}

HAPI int quickpanel_noti_list_item_get_status(Evas_Object *item)
{
	retif(item == NULL, STATE_NORMAL, "invalid parameter");

	noti_list_item_h *handler = _item_handler_get(item);
	if (handler != NULL) {
		return handler->status;
	}

	return STATE_DELETING;
}

HAPI void quickpanel_noti_list_item_node_set(Evas_Object *item, noti_node_item *noti_node)
{
	int priv_id = 0;
	retif(item == NULL, , "invalid parameter");
	retif(noti_node == NULL, , "invalid parameter");

	noti_list_item_h *handler = _item_handler_get(item);
	if (handler != NULL) {
		handler->noti_node = noti_node;
		notification_get_id(handler->noti_node->noti, NULL, &priv_id);
		handler->priv_id = priv_id;
		quickpanel_noti_list_item_update(item);
	}
}

HAPI void *quickpanel_noti_list_item_node_get(Evas_Object *item)
{
	retif(item == NULL, NULL, "invalid parameter");

	noti_node_item *noti_node = _get_noti_node(item);
	if (noti_node != NULL) {
		return noti_node;
	}

	return NULL;
}

#ifdef QP_SCREENREADER_ENABLE
static void _focus_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *item = data;
	retif(item == NULL, , "invalid parameter");

	_response_callback_call(item, "selected");
}
#endif

HAPI void quickpanel_noti_list_item_set_item_selected_cb(Evas_Object *item, response_cb callback)
{
	retif(item == NULL, , "invalid parameter");
	retif(callback == NULL, , "invalid parameter");

	noti_list_item_h *handler = _item_handler_get(item);
	if (handler != NULL) {
		handler->selected_cb = callback;
	}

#ifdef QP_SCREENREADER_ENABLE
	Evas_Object *ao = NULL;
	ao = quickpanel_accessibility_screen_reader_object_get(item,
			SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", item);
	if (ao != NULL) {
		evas_object_smart_callback_add(ao, "clicked", _focus_selected_cb, item);
	}
#endif
}

HAPI void quickpanel_noti_list_item_set_item_button_1_cb(Evas_Object *item, response_cb callback)
{
	retif(item == NULL, , "invalid parameter");
	retif(callback == NULL, , "invalid parameter");

	noti_list_item_h *handler = _item_handler_get(item);
	if (handler != NULL) {
		handler->button_1_cb = callback;
	}
}

HAPI void quickpanel_noti_list_item_set_item_deleted_cb(Evas_Object *item, response_cb callback)
{
	retif(item == NULL, , "invalid parameter");
	retif(callback == NULL, , "invalid parameter");

	noti_list_item_h *handler = _item_handler_get(item);
	if (handler != NULL) {
		handler->deleted_cb = callback;
	}
}

HAPI noti_list_item_h *quickpanel_noti_list_item_handler_get(Evas_Object *item)
{
	return _item_handler_get(item);
}
