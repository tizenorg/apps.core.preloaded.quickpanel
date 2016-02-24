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
#include <glib.h>
#include <string.h>
#include <stdbool.h>

#include <minicontrol-viewer.h>
#include <minicontrol-internal.h>
#include <bundle.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "common.h"
#include "common_uic.h"
#include "quickpanel-ui.h"
#include "quickpanel_def.h"
#include "list_util.h"
#include "quickpanel_debug_util.h"
#include "minictrl.h"
#include "vi_manager.h"

#ifdef QP_SCREENREADER_ENABLE
#include "accessibility.h"
#endif

#define MINICONTROL_TYPE_STR_VIEWER "::[viewer="
#define MINICONTROL_TYPE_STR_QUICKPANEL "QUICKPANEL"
#define MINICONTROL_TYPE_STR_LOCKSCREEN "LOCKSCREEN"
#define MINICONTROL_TYPE_STR_ONGOING "_ongoing]"
#define MINICONTROL_VIEW_DATA "MINICONTROL_VIEW_DATA"

#define THRESHOLD_DELETE_START 30
#define THRESHOLD_DELETE_START_Y_LIMIT 60
#define THRESHOLD_DISTANCE (200)
#define THRESHOLD_DISTANCE_LOCK (500)

#define MINICONTROL_BUNDLE_KEY_WIDTH "width"
#define MINICONTROL_BUNDLE_KEY_HEIGHT "height"

#define BUNDLE_BUFFER_LENGTH 100

typedef enum _gesture_state_type {
	STATE_NORMAL = 0,
	STATE_GESTURE_WAIT,
	STATE_GESTURE_CANCELED,
	STATE_DELETED,
} gesture_state_type;

struct _viewer_item {
	char *name;
	unsigned int width;
	unsigned int height;
	Evas_Object *viewer;
	void *data;

	//for flick gesture
	QP_VI *vi;
	int obj_w;
	int obj_h;
	int press_x;
	int press_y;
	int distance;
	int need_to_cancel_press;
	gesture_state_type state;
	int deletable;
};

static struct _info {
	GHashTable *prov_table;
} s_info = {
	.prov_table = NULL,
};

void _minictrl_sendview_rotation_event(const char* name, int angle);

static int _viewer_check(const char *name)
{
	char *pos_start = NULL;

	if (!name) {
		ERR("Name is NULL");
		return 0;
	}

	if ((pos_start = strstr(name, MINICONTROL_TYPE_STR_VIEWER)) != NULL) {
		if (strstr(pos_start, MINICONTROL_TYPE_STR_QUICKPANEL) != NULL) {
			return 1;
		} else {
			return 0;
		}
	} else if (strstr(name, MINICONTROL_TYPE_STR_LOCKSCREEN) != NULL) {
		return 0;
	}

	return 1;
}

static void _viewer_unfreeze(Evas_Object *viewer)
{
	int i = 0, freezed_count = 0;

	if (!viewer) {
		ERR("Invalid parameter");
		return;
	}

	freezed_count = elm_object_scroll_freeze_get(viewer);

	for (i = 0 ; i < freezed_count; i++) {
		elm_object_scroll_freeze_pop(viewer);
	}
}

static Evas_Object *_get_minictrl_obj(Evas_Object *layout)
{
	if (!layout) {
		ERR("Invalid parameter");
		return NULL;
	}

	return elm_object_part_content_get(layout, "elm.icon");
}

static void _viewer_set_size(Evas_Object *layout, void *data, int width, int height)
{
	Evas_Object *viewer;
	struct appdata *ad;
	int max_width;
	int resized_width;
	int is_landscape;

	if (!layout || !data || width < 0 || height < 0) {
		ERR("Invalid parameters (%p, %p, %d, %d)", layout, data, width, height);
		return;
	}

	viewer = _get_minictrl_obj(layout);
	if (!viewer) {
		ERR("Unable to get the 'viewer'");
		return;
	}

	ad = data;

	if (ad->angle == 0 || ad->angle == 180) {
		is_landscape = 0;
	} else {
		is_landscape = 1;
	}

	if (width > ad->win_width) {
		ERR("MC Size is not valid. it is larger than window size: %dx%d (%dx%d) %d", width, height, ad->win_width, ad->win_height, ad->angle);
	}

	max_width  = is_landscape ? ad->win_height : ad->win_width;
	resized_width = (width > max_width) ? max_width : width;

	SERR("minicontroller view is resized to w:%d/%d(%d) h:%d Landscape[%d]", resized_width, max_width, width, height, is_landscape);

	evas_object_size_hint_min_set(viewer, resized_width, height);
	evas_object_size_hint_max_set(viewer, resized_width, height);
}

static void _viewer_item_free(struct _viewer_item *item)
{
	struct appdata *ad;

	ad = quickpanel_get_app_data();
	if (!ad || !ad->list || !item) {
		ERR("Invalid paramter %p, %p, %p", ad, ad ? ad->list : NULL, item);
		return;
	}

	free(item->name);

	if (item->viewer) {
		quickpanel_list_util_item_unpack_by_object(ad->list, item->viewer, 0, 0);
		quickpanel_list_util_item_del_tag(item->viewer);
		evas_object_del(item->viewer);
	}

	free(item);
}

static bool _check_deletable(Evas_Object *obj)
{
	struct _viewer_item *vit;

	vit = evas_object_data_get(obj, MINICONTROL_VIEW_DATA);
	if (vit) {
		return vit->deletable;
	}

	return TRUE;
}

static void _mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ev;
	struct _viewer_item *vit;

	vit = evas_object_data_get(obj, MINICONTROL_VIEW_DATA);
	ev = (Evas_Event_Mouse_Down *)event_info;

	if (!ev || !vit) {
		ERR("ev %p, vit %p");
		return;
	}

	evas_object_geometry_get(obj, NULL, NULL, &vit->obj_w, &vit->obj_h);

	vit->press_x = ev->canvas.x;
	vit->press_y = ev->canvas.y;
	vit->state = STATE_NORMAL;

	SDBG("mouse down:%d %d %d", vit->obj_w, vit->obj_h, vit->state);

	if (vit->vi != NULL) {
		quickpanel_vi_user_event_del(vit->vi);
		vit->vi = NULL;
	}

	vit->need_to_cancel_press = 0;
}

static void _mouse_move_cb(void* data, Evas* e, Evas_Object* obj, void* event_info)
{
	static int vi_start_x = 0;
	static int delta_prev = -1;
	int delta_x;
	int x;
	int y;
	int w;
	int h;
	Evas_Event_Mouse_Move* ev;
	struct _viewer_item *vit;
	struct appdata *ad;

	ad = data;
	ev = event_info;
	vit = evas_object_data_get(obj, MINICONTROL_VIEW_DATA);
	if (!ev || !vit || !ad) {
		ERR("ev: %p, vit: %p, ad: %p", ev, vit, ad);
		return;
	}

	if (vit->state == STATE_GESTURE_CANCELED) {
		DBG("deletion has been canceled");
		return;
	}

	if (!_check_deletable(obj)) {
		DBG("vit->deletable is false");
		return;
	}

	evas_object_geometry_get(obj, &x, &y, &w, &h);
	delta_x = (ev->cur.output.x - vit->press_x) / 2;

	switch (vit->state) {
	case STATE_NORMAL:
		if (abs(delta_x) >= THRESHOLD_DELETE_START) {
			QP_VI *vi;

			DBG("start a deletion");
			vit->state = STATE_GESTURE_WAIT;

			vi_start_x = delta_x;

			vi = quickpanel_vi_new_with_data(
					VI_OP_DELETE,
					QP_ITEM_TYPE_NOTI,
					NULL,
					obj,
					NULL,
					NULL,
					NULL,
					NULL, /* _drag_cancel_cb, */
					NULL, /* vi == null */
					NULL,
					0,
					0);

			if (vi) {
				vit->vi = vi;
				quickpanel_vi_user_event_add(vi);
			} else {
				ERR("Unable to create a 'vi'");
			}

			vit->need_to_cancel_press = 1;
		}
		break;
	case STATE_GESTURE_WAIT:
		if (delta_prev != delta_x) {
			Evas_Map *map;

			map = evas_map_new(4);
			if (map != NULL) {
				evas_map_util_points_populate_from_object(map, obj);
				evas_map_util_points_populate_from_geometry(map, x + delta_x - vi_start_x, y, w, h, 0);
				evas_object_map_enable_set(obj, EINA_TRUE);
				evas_object_map_set(obj, map);
				evas_map_free(map);
				_viewer_unfreeze(ad->scroller);
			}
			delta_prev = delta_x;
		}
		break;
	default:
		break;
	}

	vit->distance = delta_x;
}

static void _minictrl_remove(const char *name, void *data)
{
	DBG("_minictrl_remove [%s]", name);

	minicontrol_viewer_send_event(name, MINICONTROL_EVENT_REQUEST_HIDE, NULL);

	if (s_info.prov_table) {
		if (g_hash_table_remove(s_info.prov_table, name)) {
			DBG("success to remove %s", name);
			if (!data) {
				ERR("data is NULL");
				/**
				 * @todo
				 * Oh, this function doesn't handles "data".
				 * Why does this has to check its existence??
				 */
				return;
			}
		} else {
			WARN("unknown provider name : %s", name);
		}
	}
}

static void _mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	struct _viewer_item *vit;
	struct appdata *ad;
	int swipe_distance;

	ad = data;
	vit = evas_object_data_get(obj, MINICONTROL_VIEW_DATA);
	if (!vit || !ad) {
		ERR("vit: %p, ad: %p", vit, ad);
		return;
	}

	_viewer_unfreeze(ad->scroller);

	if (!_check_deletable(obj)) {
		swipe_distance = THRESHOLD_DISTANCE_LOCK;
	} else {
		swipe_distance = THRESHOLD_DISTANCE;
	}

	switch (vit->state) {
	case STATE_GESTURE_WAIT:
		if (abs(vit->distance) >= (swipe_distance - 10)) {
			Elm_Transit *transit_flick;
			int x;

			x = abs(vit->distance) - THRESHOLD_DELETE_START;

			if (vit->distance > 0) {
				evas_object_map_set(obj, NULL);
				transit_flick = elm_transit_add();
				if (transit_flick != NULL) {
					elm_transit_effect_translation_add(transit_flick, x, 0, 480, 0);
					elm_transit_object_add(transit_flick, obj);
					elm_transit_duration_set(transit_flick, 0.25 * (480 - x ) / 480);
					elm_transit_tween_mode_set(transit_flick, ELM_TRANSIT_TWEEN_MODE_LINEAR);
					elm_transit_objects_final_state_keep_set(transit_flick, EINA_TRUE);
					elm_transit_go(transit_flick);
					_minictrl_remove(vit->name, vit->data);
				}
			} else if (vit->distance < 0) {
				evas_object_map_set(obj, NULL);
				transit_flick = elm_transit_add();
				if (transit_flick != NULL) {
					elm_transit_effect_translation_add(transit_flick, -x, 0, -480, 0);
					elm_transit_object_add(transit_flick, obj);
					elm_transit_duration_set(transit_flick, 0.25 * ( 480 - x ) / 480);
					elm_transit_tween_mode_set(transit_flick, ELM_TRANSIT_TWEEN_MODE_LINEAR);
					elm_transit_objects_final_state_keep_set(transit_flick, EINA_TRUE);
					elm_transit_go(transit_flick);
					_minictrl_remove(vit->name, vit->data);
				}
			}
		} else {
			evas_object_map_enable_set(obj, EINA_FALSE);
		}

		if (vit->vi != NULL) {
			quickpanel_vi_user_event_del(vit->vi);
			vit->vi = NULL;
		}
		break;
	case STATE_GESTURE_CANCELED:
		evas_object_map_enable_set(obj, EINA_FALSE);

		if (vit->vi != NULL) {
			quickpanel_vi_user_event_del(vit->vi);
			vit->vi = NULL;
		}
		break;
	default:
		break;
	}

	vit->state = STATE_NORMAL;
}

static Evas_Object *_minictrl_create_view(struct appdata *ad, const char *name)
{
	Evas_Object *layout;
	Evas_Object *viewer;
	Evas_Object *focus;

	if (!ad || !ad->list || !name) {
		ERR("Invalid parameters: %p %p %p", ad, ad ? ad->list : NULL, name);
		return NULL;
	}

	layout = elm_layout_add(ad->list);
	if (!layout) {
		ERR("Unable to create a layout");
		return NULL;
	}

	elm_layout_file_set(layout, DEFAULT_EDJ, "quickpanel/minictrl/default");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	viewer = minicontrol_viewer_add(layout, name);
	if (!viewer) {
		ERR("fail to add viewer - %s", name);
		evas_object_del(layout);
		return NULL;
	}
	elm_object_focus_allow_set(viewer, EINA_TRUE);
	elm_object_part_content_set(layout, "elm.icon", viewer);

	evas_object_event_callback_add(viewer, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, ad);
	evas_object_event_callback_add(viewer, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move_cb, ad);
	evas_object_event_callback_add(viewer, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, ad);

	focus = quickpanel_accessibility_ui_get_focus_object(layout);
	elm_object_part_content_set(layout, "focus", focus);
#ifdef QP_SCREENREADER_ENABLE
	Evas_Object *ao;
	ao = quickpanel_accessibility_screen_reader_object_get(layout, SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", layout);
	if (ao != NULL) {
		elm_access_info_cb_set(ao, ELM_ACCESS_TYPE, quickpanel_accessibility_info_cb, _NOT_LOCALIZED("Mini controller"));
	}
#endif

	return layout;
}

static int _minictrl_is_ongoing(const char *str)
{
	if (str == NULL) {
		return 0;
	}

	if (strstr(str, MINICONTROL_TYPE_STR_ONGOING) != NULL) {
		return 1;
	} else {
		return 0;
	}
}

static void _minictrl_add(const char *name, unsigned int width, unsigned int height, void *data)
{
	qp_item_data *qid = NULL;
	struct _viewer_item *vit = NULL;
	qp_item_type_e type;
	struct appdata *ad;
	Evas_Object *viewer = NULL;

	if (!name || !data) {
		ERR("name: %p, data: %p", name, data);
		return;
	}

	ad = data;
	if (!ad->list) {
		ERR("List is null");
		return;
	}

	if (s_info.prov_table) {
		struct _viewer_item *found;

		found = g_hash_table_lookup(s_info.prov_table, name);
		if (found) {
			ERR("already have it : %s", name);
			return;
		}
	} else {
		ERR("s_info.prov_table is NULL");
		return;
	}

	/* elm_plug receives 'server_del' event,
	 * if it repeats connect and disconnect frequently.
	 *
	 */
	viewer = _minictrl_create_view(ad, name);
	if (!viewer) {
		ERR("Failed to create view[%s]", name);
		return;
	}

	_viewer_set_size(viewer, ad, width, height);
	quickpanel_uic_initial_resize(viewer,
			(height > QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT)
			? height : QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT);

	vit = malloc(sizeof(*vit));
	if (!vit) {
		ERR("fail to alloc vit");
		evas_object_del(viewer);
		return;
	}

	if (_minictrl_is_ongoing(name) == 1) {
		type = QP_ITEM_TYPE_MINICTRL_ONGOING;
	} else {
		type = QP_ITEM_TYPE_MINICTRL_MIDDLE;
	}

	qid = quickpanel_list_util_item_new(type, vit);
	if (!qid) {
		ERR("fail to alloc vit");
		evas_object_del(viewer);
		free(vit);
		return;
	}

	vit->name = strdup(name);
	if (!vit->name) {
		ERR("strdup: %d", errno);
		quickpanel_list_util_item_del(qid);
		evas_object_del(viewer);
		free(vit);
		return;
	}
	vit->width = width;
	vit->height = height;
	vit->viewer = viewer;
	vit->data = data;
	vit->deletable = 1;
	quickpanel_list_util_item_set_tag(vit->viewer, qid);
	quickpanel_list_util_sort_insert(ad->list, vit->viewer);
	evas_object_data_set(_get_minictrl_obj(viewer), MINICONTROL_VIEW_DATA, vit);

	g_hash_table_insert(s_info.prov_table, g_strdup(name), vit);

	DBG("success to add minicontrol %s", name);
	_minictrl_sendview_rotation_event(vit->name, ad->angle);
}

static void _anim_init_resize(void *data)
{
	QP_VI *vi;
	Evas_Object *item;

	vi = data;
	if (!vi) {
		ERR("Invalid parameter");
		return;
	}

	item = vi->target;
	if (!item) {
		ERR("Invalid target");
		return;
	}

	evas_object_color_set(item, 0, 0, 0, 0);
}

static Eina_Bool _anim_init_cb(void *data)
{
	QP_VI *vi;
	int i;
	static qp_vi_op_table anim_init_table[] = {
		{
			.op_type = VI_OP_RESIZE,
			.handler = _anim_init_resize,
		},
		{
			.op_type = VI_OP_NONE,
			.handler = NULL,
		},
	};
	
	vi = data;
	if (!vi) {
		ERR("Invalid parameter");
		return EINA_FALSE;
	}

	for (i = 0; anim_init_table[i].op_type != VI_OP_NONE; i++) {
		if (anim_init_table[i].op_type != vi->op_type) {
			continue;
		}

		anim_init_table[i].handler(vi);
		break;
	}

	return EINA_TRUE;
}

static void _reorder_transit_del_cb(void *data, Elm_Transit *transit)
{
	QP_VI *vi;
	Evas_Object *item;
	struct appdata *ad;

	vi = data;
	if (!vi) {
		ERR("vi is null");
		return;
	}

	item = vi->target;
	if (!item) {
		ERR("Target is null");
		return;
	}

	ad = quickpanel_get_app_data();
	if (!ad) {
		ERR("ad is null");
		return;
	}

	_viewer_set_size(item, ad, vi->extra_flag_1, vi->extra_flag_2);
	quickpanel_uic_initial_resize(item,
			(vi->extra_flag_2 > QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT)
			? vi->extra_flag_2 : QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT);
}

static void _anim_job_resize(void *data)
{
	Elm_Transit *transit_layout_parent;
	struct _viewer_item *viewer_item;
	Elm_Transit *transit_fadein;
	struct appdata *ad;
	Evas_Object *item;
	int to_w, to_h;
	QP_VI *vi;

	vi = data;
	ad = quickpanel_get_app_data();
	if (!ad || !vi || !vi->target || !vi->extra_data_2) {
		ERR("Invalid parameters: %p %p %p %p", ad, vi, vi ? vi->target : NULL, vi ? vi->extra_data_2 : NULL);
		return;
	}

	item = vi->target;
	to_w = vi->extra_flag_1;
	to_h = vi->extra_flag_2;
	viewer_item = vi->extra_data_2;

	transit_layout_parent = quickpanel_list_util_get_reorder_transit(viewer_item->viewer, NULL, to_h - viewer_item->height);
	if (transit_layout_parent != NULL) {
		elm_transit_del_cb_set(transit_layout_parent, _reorder_transit_del_cb, vi);
	} else {
		_viewer_set_size(item, ad, to_w, to_h);
		quickpanel_uic_initial_resize(item,
				(to_h > QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT)
				? to_h : QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT);
	}

	transit_fadein = elm_transit_add();
	if (transit_fadein != NULL) {
		elm_transit_object_add(transit_fadein, item);
		elm_transit_effect_color_add(transit_fadein, 0, 0, 0, 0, 255, 255, 255, 255);
		elm_transit_duration_set(transit_fadein, 0.35);
		elm_transit_tween_mode_set(transit_fadein, quickpanel_vim_get_tweenmode(VI_OP_INSERT));
		elm_transit_del_cb_set(transit_fadein, quickpanel_vi_done_cb_for_transit, vi);
		elm_transit_objects_final_state_keep_set(transit_fadein, EINA_TRUE);

		if (transit_layout_parent != NULL) {
			elm_transit_chain_transit_add(transit_layout_parent, transit_fadein);
			elm_transit_go(transit_layout_parent);
		} else {
			elm_transit_go(transit_fadein);
		}
	} else {
		ERR("Failed to create all the transit");
		quickpanel_vi_done(vi);
	}
}

static Eina_Bool _anim_job_cb(void *data)
{
	QP_VI *vi;
	int i;
	static qp_vi_op_table anim_job_table[] = {
		{
			.op_type = VI_OP_RESIZE,
			.handler = _anim_job_resize,
		},
		{
			.op_type = VI_OP_NONE,
			.handler = NULL,
		},
	};

	vi = data;
	if (!vi) {
		ERR("Invalid parameter");
		return EINA_FALSE;
	}

	for (i = 0; anim_job_table[i].op_type != VI_OP_NONE; i++) {
		if (anim_job_table[i].op_type != vi->op_type) {
			continue;
		}

		anim_job_table[i].handler(vi);
		break;
	}

	return EINA_TRUE;
}

static void _anim_done_resize(void *data)
{
	QP_VI *vi;
	struct _viewer_item *viewer_item;
	struct appdata *ad;
	Evas_Object *item;

	vi = data;
	if (!vi) {
		ERR("Invalid parameter");
		return;
	}

	ad = quickpanel_get_app_data();
	if (!ad) {
		ERR("Invalid ad");
		return;
	}

	item = vi->target;
	if (!item) {
		ERR("Invalid target");
		return;
	}

	viewer_item = vi->extra_data_2;
	if (!viewer_item) {
		ERR("viewer_item is null");
		return;
	}

	viewer_item->width = vi->extra_flag_1;
	viewer_item->height = vi->extra_flag_2;

	_viewer_set_size(item, ad, viewer_item->width, viewer_item->height);
	quickpanel_uic_initial_resize(item,
			(viewer_item->height > QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT)
			? viewer_item->height : QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT);
	evas_object_color_set(item, 255, 255, 255, 255);
}

static Eina_Bool _anim_done_cb(void *data)
{
	QP_VI *vi;
	int i;
	static qp_vi_op_table anim_done_table[] = {
		{
			.op_type = VI_OP_RESIZE,
			.handler = _anim_done_resize,
		},
		{
			.op_type = VI_OP_NONE,
			.handler = NULL,
		},
	};

	vi = data;
	if (!vi) {
		ERR("Invalid parameter");
		return EINA_FALSE;
	}

	for (i = 0; anim_done_table[i].op_type != VI_OP_NONE; i++) {
		if (anim_done_table[i].op_type != vi->op_type) {
			continue;
		}

		anim_done_table[i].handler(vi);
		break;
	}

	return EINA_TRUE;
}

static void _minictrl_resize_vi(Evas_Object *list, struct _viewer_item *item, int to_w, int to_h)
{
	QP_VI *vi;

	if (!list || !item) {
		ERR("Invalid parameter: list: %p, item: %p", list, item);
		return;
	}

	vi = quickpanel_vi_new_with_data(
			VI_OP_RESIZE,
			QP_ITEM_TYPE_MINICTRL_MIDDLE,
			list,
			item->viewer,
			_anim_init_cb,
			_anim_job_cb,
			_anim_done_cb,
			_anim_done_cb,
			NULL, /* vi == NULL */
			item,
			to_w,
			to_h);

	if (vi) {
		quickpanel_vi_start(vi);
	} else {
		ERR("Unable to create 'vi'");
	}
}

static void _minictrl_update(const char *name, unsigned int width, unsigned int height, void *data)
{
	struct appdata *ad = data;
	struct _viewer_item *found = NULL;

	if (!s_info.prov_table || !ad) {
		ERR("name: %s, table: %p, ad: %p", name, s_info.prov_table, ad);
		return;
	}

	found = g_hash_table_lookup(s_info.prov_table, name);
	if (!found) {
		WARN("unknown provider name : %s", name);
		return;
	}

	if (found->viewer) {
		if (found->height != height || found->width != width) {
			_minictrl_resize_vi(ad->list, found, width, height);
		} else {
			_viewer_set_size(found->viewer, ad, width, height);
			quickpanel_uic_initial_resize(found->viewer,
					(height > QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT)
					? height : QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT);
		}
	}
}

static void _minictrl_lock(const char *name)
{
	struct _viewer_item *found;

	if (!s_info.prov_table) {
		ERR("table is empty: %s", name);
		return;
	}

	DBG("minictrl_lock %s", name);
	found = g_hash_table_lookup(s_info.prov_table, name);
	if (!found) {
		WARN("unknown provider name : %s", name);
		return;
	}

	if (found->viewer) {
		struct _viewer_item *vit;

		vit = evas_object_data_del(_get_minictrl_obj(found->viewer), MINICONTROL_VIEW_DATA);
		if (vit) {
			vit->deletable = 0;
			evas_object_data_set(_get_minictrl_obj(found->viewer), MINICONTROL_VIEW_DATA, vit);
		} else {
			WARN("vit is NULL");
		}
	}
}

static void _mctrl_viewer_event_cb(minicontrol_event_e event, const char *name, bundle *event_arg, void *data)
{
	struct appdata *ad;
	int ret;
	int *width;
	int *height;
	int _width;
	int _height;
	size_t bundle_size;

	if (!data || !name) {
		ERR("Invalid parameter");
		return;
	}

	ad = data;

	if (_viewer_check(name) == 0) {
		ERR("%s: ignored", name);
		return;
	}

	if ((int)event == MINICONTROL_EVENT_REQUEST_LOCK) {
		/**
		 * This event type is extra one. not in the enumeration list.
		 */
		_minictrl_lock(name);
	} else {
		switch (event) {
		case MINICONTROL_EVENT_START:
			ret = bundle_get_byte(event_arg, MINICONTROL_BUNDLE_KEY_WIDTH, (void **)&width, &bundle_size);
			if (ret != BUNDLE_ERROR_NONE || bundle_size != sizeof(int)) {
				ERR("Failed to get bundle value(width) %d : %d", ret, bundle_size);
				_width = 0;
				width = &_width;
			}

			ret = bundle_get_byte(event_arg, MINICONTROL_BUNDLE_KEY_HEIGHT, (void **)&height, &bundle_size);
			if (ret != BUNDLE_ERROR_NONE || bundle_size != sizeof(int)) {
				ERR("Failed to get bundle value(height) : %d", ret);
				_height = 0;
				height = &_height;
			}

			DBG("Name: %s, Size: %dx%d", name, *width, *height);
			_minictrl_add(name, *width, *height, data);
			break;
		case MINICONTROL_EVENT_RESIZE:
			ret = bundle_get_byte(event_arg, MINICONTROL_BUNDLE_KEY_WIDTH, (void **)&width, &bundle_size);
			if (ret != BUNDLE_ERROR_NONE || bundle_size != sizeof(int)) {
				ERR("Failed to get bundle value(width) %d : %d", ret, bundle_size);
				_width = 0;
				width = &_width;
			}

			ret = bundle_get_byte(event_arg, MINICONTROL_BUNDLE_KEY_HEIGHT, (void **)&height, &bundle_size);
			if (ret != BUNDLE_ERROR_NONE || bundle_size != sizeof(int)) {
				ERR("Failed to get bundle value(height) : %d", ret);
				_height = 0;
				height = &_height;
			}

			DBG("Name: %s, Size: %dx%d", name, *width, *height);
			_minictrl_update(name, *width, *height, data);
			break;
		case MINICONTROL_EVENT_STOP:
			_minictrl_remove(name, data);
			break;
		case MINICONTROL_EVENT_REQUEST_HIDE:
			quickpanel_uic_close_quickpanel(true, 0);
			break;
		case MINICONTROL_EVENT_REQUEST_ANGLE:
			if (ad->list != NULL) {
				SERR("need to broadcasting angle by %s ", name, event);
				_minictrl_sendview_rotation_event(name, ad->angle);
			}
			break;
		default:
			break;
		}
	}
}

static int _init(void *data)
{
	minicontrol_error_e ret;

	if (!data) {
		ERR("Invalid parameter");
		return QP_FAIL;
	}

	s_info.prov_table = g_hash_table_new_full(g_str_hash, g_str_equal,
			(GDestroyNotify)g_free,
			(GDestroyNotify)_viewer_item_free);

	ret = minicontrol_viewer_set_event_cb(_mctrl_viewer_event_cb, data);
	if (ret != MINICONTROL_ERROR_NONE) {
		ERR("fail to minicontrol_viewer_set_event_cb()- %d", ret);
		return QP_FAIL;
	}

	return QP_OK;
}

static int _fini(void *data)
{
	minicontrol_error_e ret;

	ret = minicontrol_viewer_unset_event_cb();

	if (ret != MINICONTROL_ERROR_NONE) {
		ERR("fail to minicontrol_viewer_unset_event_cb()- %d", ret);
	}

	if (s_info.prov_table) {
		g_hash_table_destroy(s_info.prov_table);
		s_info.prov_table = NULL;
	}

	return QP_OK;
}

static int _suspend(void *data)
{
	struct appdata *ad;

	ad = data;
	if (!ad) {
		ERR("Invalid parameter");
		return QP_FAIL;
	}

	if (ad->list != NULL) {
		_viewer_unfreeze(ad->scroller);
	}

	return QP_OK;
}

static int _resume(void *data)
{
	struct appdata *ad;

	ad = data;
	if (!ad) {
		ERR("Invalid parameter");
		return QP_FAIL;
	}

	if (ad->list != NULL) {
		_viewer_unfreeze(ad->scroller);
	}

	return QP_OK;
}

HAPI void quickpanel_minictrl_rotation_report(int angle)
{
	bundle *event_arg_bundle;

	if (s_info.prov_table == NULL) {
		return;
	}

	if (g_hash_table_size(s_info.prov_table) <= 0) {
		return;
	}

	GHashTableIter iter;
	gpointer key, value;

	g_hash_table_iter_init (&iter, s_info.prov_table);
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		SINFO("minicontrol name:%s rotation:%d", key, angle);
		event_arg_bundle = bundle_create();
		if (event_arg_bundle) {
			char bundle_value_buffer[BUNDLE_BUFFER_LENGTH] = { 0, };
			snprintf(bundle_value_buffer, sizeof(bundle_value_buffer) - 1, "%d", angle);
			bundle_add_str(event_arg_bundle, "angle", bundle_value_buffer);
			minicontrol_viewer_send_event(key, MINICONTROL_EVENT_REPORT_ANGLE, event_arg_bundle);
			bundle_free(event_arg_bundle);
		}
	}
}

void _minictrl_sendview_rotation_event(const char* name, int angle)
{
	bundle *event_arg_bundle;

	if (!name) {
		ERR("Invalid parameter");
		return;
	}

	if (s_info.prov_table == NULL) {
		return;
	}

	if (g_hash_table_size(s_info.prov_table) <= 0) {
		return;
	}

	SINFO("minicontrol name:%s rotation:%d", name, angle);
	event_arg_bundle = bundle_create();
	if (event_arg_bundle) {
		char bundle_value_buffer[BUNDLE_BUFFER_LENGTH] = { 0, };

		snprintf(bundle_value_buffer, sizeof(bundle_value_buffer) - 1, "%d", angle);
		bundle_add_str(event_arg_bundle, "angle", bundle_value_buffer);
		minicontrol_viewer_send_event(name, MINICONTROL_EVENT_REPORT_ANGLE, event_arg_bundle);
		bundle_free(event_arg_bundle);
	}
}

static void _minictrl_send_view_event_cb(gpointer key, gpointer value, gpointer user_data)
{
	if (!key) {
		ERR("Key is null");
		return;
	}

	bundle *event_arg_bundle;

	event_arg_bundle = bundle_create();
	if (event_arg_bundle) {
		minicontrol_viewer_event_e event;

		event = (minicontrol_viewer_event_e)user_data;
		minicontrol_viewer_send_event(key, event, event_arg_bundle);
		bundle_free(event_arg_bundle);
	}
}

static void _minictrl_opened(void *data)
{
	DBG("");

	g_hash_table_foreach(s_info.prov_table, _minictrl_send_view_event_cb, (gpointer)MINICONTROL_VIEWER_EVENT_SHOW);
}

static void _minictrl_closed(void *data)
{
	DBG("");
	g_hash_table_foreach(s_info.prov_table, _minictrl_send_view_event_cb, (gpointer)MINICONTROL_VIEWER_EVENT_HIDE);
}

QP_Module minictrl = {
	.name = "minictrl",
	.init = _init,
	.fini = _fini,
	.suspend = _suspend,
	.resume = _resume,
	.hib_enter = NULL,
	.hib_leave = NULL,
	.lang_changed = NULL,
	.refresh = NULL,
	.get_height = NULL,
	.qp_opened = _minictrl_opened,
	.qp_closed = _minictrl_closed,
};

/* End of a file */
