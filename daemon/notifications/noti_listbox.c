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

#include <vconf.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <notification.h>
#include <system_settings.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "common_uic.h"
#include "list_util.h"
#include "quickpanel_def.h"
#include "noti_listbox.h"
#include "vi_manager.h"
#include "noti_node.h"
#include "noti_list_item.h"
#include "noti.h"

#define E_DATA_LAYOUT_PORTRAIT "layout_portrait"
#define E_DATA_LAYOUT_LANDSCAPE "layout_landscape"
#define E_DATA_CB_DELETE_ITEM "cb_delete_item"
#define E_DATA_CB_REMOVED "cb_removed"
#define E_DATA_APP_DATA "app_data"
#define E_DATA_IS_HIDED "hided"

static Eina_Bool _anim_init_cb(void *data);
static Eina_Bool _anim_job_cb(void *data);
static Eina_Bool _anim_done_cb(void *data);

static void _listbox_flag_set(Evas_Object *container, const char *key, int value)
{
	retif(container == NULL, , "invalid parameter");
	retif(key == NULL, , "invalid parameter");

	evas_object_data_set(container, key, (void *)(long)value);
}

static int _listbox_flag_get(Evas_Object *container, const char *key)
{
	retif(container == NULL, 0, "invalid parameter");
	retif(key == NULL, 0, "invalid parameter");

	return (int)(long)evas_object_data_get(container, key);
}

static int _listbox_layout_item_valid(Evas_Object *container, Evas_Object *item)
{
	int ret = 0;
	Eina_List *list = NULL;
	retif(container == NULL, 0, "invalid parameter");
	list = elm_box_children_get(container);
	retif(list == NULL, 0, "invalid parameter. containter[%p]", container);

	if (eina_list_data_find(list, item) != NULL) {
		ret = 1;
	}

	eina_list_free(list);

	return ret;
}

static void _listbox_layout_get_coord(Evas_Object *container, int insert_position, int *coord_x, int *coord_y, Evas_Object *noti_section)
{
	int x, y, h;
	int off_y = 0;
	struct appdata *ad = quickpanel_get_app_data();

	retif(container == NULL, , "invalid parameter");
	retif(ad == NULL, , "invalid data.");

	if (insert_position == 0) {
		Eina_List *l;
		Eina_List *l_next;
		Evas_Object *obj = NULL;
		Eina_List *item_list = elm_box_children_get(container);
		noti_node_item *node = NULL;
		notification_type_e type = NOTIFICATION_TYPE_NONE;

		EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
			node = quickpanel_noti_list_item_node_get(obj);
			if (node) {
				notification_h noti = node->noti;
				if (noti) {
					notification_get_type(noti, &type);
					if (type == NOTIFICATION_TYPE_NOTI) {
						break;
					} else {
						evas_object_geometry_get(obj, NULL, NULL, NULL, &h);
						off_y += h;
					}
				}
			} else {
				evas_object_geometry_get(obj, NULL, NULL, NULL, &h);
				off_y += h;
			}
		}

		if (item_list != NULL) {
			eina_list_free(item_list);
		}
	}
	else if (insert_position == 1) {
		Eina_List *l;
		Eina_List *l_next;
		Evas_Object *obj = NULL;
		Eina_List *item_list = elm_box_children_get(container);

		EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
			if (obj != NULL) {
				evas_object_geometry_get(obj, NULL, NULL, NULL, &h);
				off_y += h;
			}
			if (obj == noti_section) {
				break;
			}
		}

		if (item_list != NULL) {
			eina_list_free(item_list);
		}
	}

	evas_object_geometry_get(container, &x, &y, NULL, &h);
	if (off_y == 0 || y == 0) {
		ERR("Failed get a valid height offset : %d %d", off_y, y);
	}

	if (coord_x != NULL) {
		*coord_x = x;
	}
	if (coord_y != NULL) {
		*coord_y = y + off_y;
	}
}

static void _listbox_layout_size_get(Evas_Object *container, int *w, int *h)
{
	int h_temp = 0;
	int w_item = 0, h_item = 0;
	Evas_Object *obj = NULL;
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid data.");
	retif(container == NULL, , "invalid parameter");

	Eina_List *item_list = elm_box_children_get(container);

	EINA_LIST_FREE(item_list, obj)
	{
		if (obj != NULL) {
			evas_object_geometry_get(obj, NULL, NULL, NULL, &h_temp);
			h_item += h_temp;
		}
	}
	evas_object_geometry_get(container, NULL, NULL, &w_item, NULL);

	if (w != NULL) {
		*w = w_item;
	}
	if (h != NULL) {
		*h = h_item;
	}
}

HAPI Evas_Object *quickpanel_noti_listbox_create(Evas_Object *parent, void *data, qp_item_type_e item_type)
{

	struct appdata *ad = data;
	Evas_Object *listbox = NULL;

	retif(parent == NULL, NULL, "invalid parameter");
	retif(data == NULL, NULL, "invalid parameter");

	listbox = elm_box_add(parent);
	evas_object_size_hint_weight_set(listbox, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(listbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(listbox, EINA_FALSE);
	evas_object_show(listbox);

	evas_object_data_set(listbox, E_DATA_CB_DELETE_ITEM, NULL);
	evas_object_data_set(listbox, E_DATA_APP_DATA, ad);
	_listbox_flag_set(listbox, E_DATA_IS_HIDED, 0);

	qp_item_data *qid
		= quickpanel_list_util_item_new(item_type, NULL);
	quickpanel_list_util_item_set_tag(listbox, qid);

	return listbox;
}

HAPI void quickpanel_noti_listbox_remove(Evas_Object *listbox)
{
	retif(listbox == NULL, , "invalid parameter");

	quickpanel_noti_listbox_remove_all_item(listbox, 0);
	evas_object_data_del(listbox, E_DATA_CB_DELETE_ITEM);
	evas_object_data_del(listbox, E_DATA_APP_DATA);
	quickpanel_list_util_item_del_tag(listbox);
	evas_object_del(listbox);
	listbox = NULL;
}

HAPI void quickpanel_noti_listbox_set_item_deleted_cb(Evas_Object *listbox, void(*deleted_cb)(void *data, Evas_Object *obj))
{
	retif(listbox == NULL, , "invalid parameter");
	retif(deleted_cb == NULL, , "invalid parameter");

	evas_object_data_set(listbox, E_DATA_CB_DELETE_ITEM, deleted_cb);
}

static void _listbox_call_item_deleted_cb(Evas_Object *listbox, void *data, Evas_Object *obj)
{
	retif(listbox == NULL, , "invalid parameter");

	void (*deleted_cb)(void *data, Evas_Object *obj) = NULL;

	deleted_cb = evas_object_data_get(listbox, E_DATA_CB_DELETE_ITEM);

	if (deleted_cb != NULL) {
		deleted_cb(data, obj);
	}
}

HAPI void quickpanel_noti_listbox_add_item(Evas_Object *listbox, Evas_Object *item, int insert_pos, Evas_Object *noti_section)
{
	QP_VI *vi = NULL;
	const char *signal = NULL;
	retif(listbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");

	struct appdata *ad = evas_object_data_get(listbox, E_DATA_APP_DATA);

	if (ad != NULL) {
		if (ad->angle == 270 || ad->angle == 90) {
			signal = "box.landscape";
		} else {
			signal = "box.portrait";
		}
	}

	DBG("set to %s, %x", signal, item);

	elm_object_signal_emit(item, signal, "box.prog");
	edje_object_message_signal_process(_EDJ(item));
	elm_layout_sizing_eval(item);

	vi = quickpanel_vi_new_with_data(
			VI_OP_INSERT,
			QP_ITEM_TYPE_ONGOING_NOTI,
			listbox,
			item,
			_anim_init_cb,
			_anim_job_cb,
			_anim_done_cb,
			_anim_done_cb,
			vi,
			noti_section,
			insert_pos,
			0);
	quickpanel_vi_start(vi);
}

HAPI void quickpanel_noti_listbox_remove_item(Evas_Object *listbox, Evas_Object *item, int with_animation)
{
	QP_VI *vi = NULL;
	retif(listbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");


	DBG("remove:%p", item);

	if (with_animation == 1) {
		vi = quickpanel_vi_new_with_data(
				VI_OP_DELETE,
				QP_ITEM_TYPE_ONGOING_NOTI,
				listbox,
				item,
				_anim_init_cb,
				_anim_job_cb,
				_anim_done_cb,
				_anim_done_cb,
				vi,
				NULL,
				0,
				0);
		quickpanel_vi_start(vi);
	} else {
		DBG("%p", item);
		void *node = quickpanel_noti_list_item_node_get(item);
		elm_box_unpack(listbox, item);
		quickpanel_noti_list_item_remove(item);
		_listbox_call_item_deleted_cb(listbox,
				node, NULL);
	}
}

static void _anim_job_delete_all(void *data)
{
	QP_VI *vi = data;
	retif(vi == NULL, , "invalid parameter");

	quickpanel_vi_done(vi);
}

static void _anim_done_delete_all(void *data)
{
	QP_VI *vi = data;
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj = NULL;
	Eina_List *item_list = NULL;

	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");

	Evas_Object *listbox = vi->container;

	item_list = elm_box_children_get(listbox);
	retif(item_list == NULL, , "invalid parameter");

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
		if (obj != NULL) {
			DBG("try to remove:%p", obj);
			quickpanel_noti_listbox_remove_item(listbox, obj, EINA_TRUE);
		}
	}

	if (item_list != NULL) {
		eina_list_free(item_list);
	}
}

HAPI void quickpanel_noti_listbox_remove_all_item(Evas_Object *listbox, int with_animation)
{
	QP_VI *vi = NULL;
	retif(listbox == NULL, , "invalid parameter");

	vi = quickpanel_vi_new_with_data(
			VI_OP_DELETE_ALL,
			QP_ITEM_TYPE_ONGOING_NOTI,
			listbox,
			NULL,
			_anim_init_cb,
			_anim_job_cb,
			_anim_done_cb,
			_anim_done_cb,
			vi,
			NULL,
			0,
			0);
	quickpanel_vi_start(vi);
}

HAPI void quickpanel_noti_listbox_update(Evas_Object *listbox)
{
	retif(listbox == NULL, , "invalid parameter");

	Evas_Object *obj;
	Eina_List *item_list = elm_box_children_get(listbox);

	EINA_LIST_FREE(item_list, obj)
	{
		quickpanel_noti_list_item_update(obj);
	}
}

HAPI void quickpanel_noti_listbox_items_visibility_set(Evas_Object *listbox, int is_visible)
{
	retif(listbox == NULL, , "invalid parameter");

	_listbox_flag_set(listbox, E_DATA_IS_HIDED, is_visible);
}

HAPI void quickpanel_noti_listbox_update_item(Evas_Object *listbox, Evas_Object *item)
{
	retif(listbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");

	if (_listbox_layout_item_valid(listbox, item)) {
		quickpanel_noti_list_item_update(item);
	}
}

HAPI void quickpanel_noti_listbox_rotation(Evas_Object *listbox, int angle)
{
	int h = 0;
	Evas_Object *obj = NULL;
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid parameter");
	retif(listbox == NULL, , "invalid parameter");

	Eina_List *item_list = elm_box_children_get(listbox);

	DBG("items in listbox:%d", eina_list_count(item_list));

	EINA_LIST_FREE(item_list, obj)
	{
		if (obj != NULL) {
			evas_object_geometry_get(obj, NULL, NULL, NULL, &h);
			if (angle == 270 || angle == 90) {
				evas_object_resize(obj, ad->win_height, h);
			} else {
				evas_object_resize(obj, ad->win_width, h);
			}
			elm_layout_sizing_eval(obj);
		}
	}

	_listbox_layout_size_get(listbox, NULL, &h);

	if (angle == 270 || angle == 90) {
		evas_object_resize(listbox, ad->win_height, h);
	} else {
		evas_object_resize(listbox, ad->win_width, h);
	}
	DBG("listbox has been rotated to %d", angle);
}

HAPI int quickpanel_noti_listbox_get_item_count(Evas_Object *listbox)
{
	int item_count = 0;
	Eina_List *items = NULL;
	retif(listbox == NULL, 0, "invalid parameter");

	if ((items = elm_box_children_get(listbox)) != NULL) {
		item_count = eina_list_count(items);
		eina_list_free(items);
		return item_count;
	} else {
		return 0;
	}
}

static void _anim_init_insert(void *data)
{

	QP_VI *vi = data;
	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	Evas_Object *container = vi->container;
	Evas_Object *item = vi->target;

	evas_object_clip_set(item, evas_object_clip_get(container));
	evas_object_color_set(item, 0, 0, 0, 0);
}

static void _anim_job_insert(void *data)
{

	QP_VI *vi = data;
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj = NULL;
	Eina_List *item_list = NULL;
	int item_width, item_height = 0;
	int coord_x = 0, coord_y = 0;
	int insert_position = 0;
	Elm_Transit *transit_layout_parent = NULL;
	Elm_Transit *transit_layout = NULL;
	Elm_Transit *transit_fadein = NULL;
	Evas_Object *container = NULL;
	Evas_Object *item = NULL;
	int flag = 0;
	noti_node_item *node = NULL;
	notification_type_e type = NOTIFICATION_TYPE_NONE;

	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	container = vi->container;
	item = vi->target;
	insert_position = vi->extra_flag_1;
	item_list = elm_box_children_get(container);

	_listbox_layout_get_coord(container, insert_position, &coord_x, &coord_y, (Evas_Object *)vi->extra_data_2);
	evas_object_move(item, coord_x, coord_y);

	evas_object_geometry_get(item, NULL, NULL, &item_width, &item_height);
	if (item_width == 0 && item_height == 0) {
		ERR("failed to get a size of item %d %d", item_width, item_height);
		evas_object_size_hint_min_get (item, &item_width, &item_height);
	}

	transit_layout_parent = quickpanel_list_util_get_reorder_transit(container, NULL, item_height);

	if (insert_position) {
		EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
			if (obj == (Evas_Object *)vi->extra_data_2) {
				flag = 1;
			}
			else if (flag == 1) {
				transit_layout = elm_transit_add();
				if (transit_layout != NULL) {
					elm_transit_effect_translation_add(transit_layout, 0, 0, 0, item_height);
					elm_transit_object_add(transit_layout, obj);
					elm_transit_duration_set(transit_layout,
							quickpanel_vim_get_duration(VI_OP_REORDER));
					elm_transit_tween_mode_set(transit_layout,
							quickpanel_vim_get_tweenmode(VI_OP_REORDER));
					elm_transit_objects_final_state_keep_set(transit_layout, EINA_TRUE);

					elm_transit_go(transit_layout);
				} else {
					ERR("failed to create a transit");
				}
			}
		}
	}
	else if (insert_position == 0) {
		EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
			node = quickpanel_noti_list_item_node_get(obj);
			if (node) {
				notification_h noti = node->noti;
				if (noti) {
					notification_get_type(noti, &type);
					if (type == NOTIFICATION_TYPE_NOTI) {
						flag = 1;
					}
				}
			}
			if (flag == 1) {
				transit_layout = elm_transit_add();
				if (transit_layout != NULL) {
					elm_transit_effect_translation_add(transit_layout, 0, 0, 0, item_height);
					elm_transit_object_add(transit_layout, obj);
					elm_transit_duration_set(transit_layout,
							quickpanel_vim_get_duration(VI_OP_REORDER));
					elm_transit_tween_mode_set(transit_layout,
							quickpanel_vim_get_tweenmode(VI_OP_REORDER));
					elm_transit_objects_final_state_keep_set(transit_layout, EINA_TRUE);

					elm_transit_go(transit_layout);
				} else {
					ERR("failed to create a transit");
				}
			}
		}
	}

	if (item_list != NULL) {
		eina_list_free(item_list);
	}

	transit_fadein = elm_transit_add();
	if (transit_fadein != NULL) {
		elm_transit_object_add(transit_fadein, item);
		elm_transit_effect_color_add(transit_fadein, 0, 0, 0, 0, 255, 255, 255, 255);
		elm_transit_duration_set(transit_fadein,
				quickpanel_vim_get_duration(VI_OP_INSERT));
		elm_transit_tween_mode_set(transit_fadein,
				quickpanel_vim_get_tweenmode(VI_OP_INSERT));
		elm_transit_del_cb_set(transit_fadein, quickpanel_vi_done_cb_for_transit, vi);

		if (transit_layout != NULL) {
			elm_transit_chain_transit_add(transit_layout, transit_fadein);
		} else {
			if (transit_layout_parent != NULL) {
				elm_transit_chain_transit_add(transit_layout_parent, transit_fadein);
			} else {
				elm_transit_go(transit_fadein);
			}
		}
	} else {
		ERR("Failed to create all the transit");
		quickpanel_vi_done(vi);
	}
}

static void _anim_done_insert(void *data)
{

	QP_VI *vi = data;
	int inset_position = 0;
	Eina_List *l;
	Eina_List *l_next;
	Eina_List *item_list = NULL;
	Evas_Object *obj = NULL;
	noti_node_item *node = NULL;
	notification_type_e type = NOTIFICATION_TYPE_NONE;
	int flag = 0;

	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	Evas_Object *container = vi->container;
	Evas_Object *item = vi->target;
	inset_position = vi->extra_flag_1;
	item_list = elm_box_children_get(container);

	evas_object_color_set(item, 255, 255, 255, 255);

	if (inset_position == LISTBOX_PREPEND) {
		EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
			if (obj == (Evas_Object *)vi->extra_data_2) {
				elm_box_pack_after(container, item, obj);
				break;
			}
		}
	} else if (inset_position == LISTBOX_APPEND) {
		EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
			node = quickpanel_noti_list_item_node_get(obj);
			if (node) {
				notification_h noti = node->noti;
				if (noti) {
					notification_get_type(noti, &type);
					if (type == NOTIFICATION_TYPE_NOTI) {
						//node_first_noti = node;
						elm_box_pack_before(container, item, obj);
						flag = 1;
						break;
					}
				}
			}
		}
		if (flag == 0) {
			elm_box_pack_end(container, item);
		}
	} else {
		int ongoing_count = quickpanel_noti_get_type_count(NOTIFICATION_TYPE_ONGOING);
		DBG("NOTI INSERT AT: %d", ongoing_count);

		if (ongoing_count == 0) {
			DBG("NOTI INSERT START");
			elm_box_pack_start(container, item);
		} else {
			Eina_List *items = elm_box_children_get(container);
			if (!items) {
				ERR("Failed to recieve container items, adding new notification to end of the list");
				elm_box_pack_end(container, item);
				return;
			}

			Evas_Object *before = eina_list_nth (items, ongoing_count - 1);
			if (!before) {
				ERR("Failed to recieve preceding item, adding new notification to end of the list");
				elm_box_pack_end(container, item);
				return;
			}

			DBG("NOTI INSERT BEFORE: %p", before);
			elm_box_pack_after(container, item, before);
		}
	}
}

static void _anim_job_delete(void *data)
{
	QP_VI *vi = data;
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj = NULL;
	Eina_List *item_list = NULL;
	int is_start_relayout = 0;
	int item_width, item_height = 0;
	Elm_Transit *transit_layout_parent = NULL;
	Elm_Transit *transit_layout = NULL;
	Elm_Transit *transit_fadeout = NULL;
	Evas_Object *container = NULL;
	Evas_Object *item = NULL;

	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	container = vi->container;
	item = vi->target;
	item_list = elm_box_children_get(container);

	evas_object_geometry_get(item, NULL, NULL, &item_width, &item_height);
	if (item_width == 0 && item_height == 0) {
		ERR("failed to get a size of item %d %d", item_width, item_height);
		evas_object_size_hint_min_get (item, &item_width, &item_height);
	}

	transit_fadeout = elm_transit_add();
	if (transit_fadeout != NULL) {
		elm_transit_object_add(transit_fadeout, item);
		elm_transit_effect_color_add(transit_fadeout, 255, 255, 255, 255, 0, 0, 0, 0);
		elm_transit_objects_final_state_keep_set(transit_fadeout, EINA_TRUE);
		elm_transit_duration_set(transit_fadeout, quickpanel_vim_get_duration(VI_OP_DELETE));
		elm_transit_go(transit_fadeout);
	} else {
		ERR("failed to create a transit");
	}

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
		if (obj == item) {
			is_start_relayout = 1;
		} else if (obj != NULL && is_start_relayout == 1) {
			transit_layout = elm_transit_add();
			if (transit_layout != NULL) {
				elm_transit_effect_translation_add(transit_layout, 0, 0, 0, -item_height);
				elm_transit_object_add(transit_layout, obj);
				elm_transit_duration_set(transit_layout,
						quickpanel_vim_get_duration(VI_OP_REORDER));
				elm_transit_tween_mode_set(transit_layout,
						quickpanel_vim_get_tweenmode(VI_OP_REORDER));
				elm_transit_objects_final_state_keep_set(transit_layout, EINA_TRUE);
				if (transit_fadeout != NULL) {
					elm_transit_chain_transit_add(transit_fadeout, transit_layout);
				}
			} else {
				ERR("failed to create a transit");
			}
		}
	}

	if (item_list != NULL) {
		eina_list_free(item_list);
	}

	transit_layout_parent = quickpanel_list_util_get_reorder_transit(container,
			transit_fadeout, -item_height);

	if (transit_layout_parent != NULL) {
		elm_transit_del_cb_set(transit_layout_parent, quickpanel_vi_done_cb_for_transit,
				vi);
	} else if (transit_layout != NULL) {
		elm_transit_del_cb_set(transit_layout, quickpanel_vi_done_cb_for_transit,
				vi);
	} else if (transit_fadeout != NULL) {
		elm_transit_del_cb_set(transit_fadeout, quickpanel_vi_done_cb_for_transit,
				vi);
	} else {
		ERR("Failed to create all the transit");
		quickpanel_vi_done(vi);
	}
}

static void _anim_done_delete(void *data)
{
	int w = 0, h = 0;
	QP_VI *vi = data;

	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	Evas_Object *container = vi->container;
	Evas_Object *item = vi->target;

	elm_box_unpack(container, item);
	quickpanel_noti_list_item_remove(item);
	_listbox_call_item_deleted_cb(container,
			quickpanel_noti_list_item_node_get(item), NULL);

	if (_listbox_flag_get(container, E_DATA_IS_HIDED) == 1) {
		_listbox_layout_size_get(container, &w, &h);
		evas_object_resize(container, w, h);
	}
}

static Eina_Bool _anim_init_cb(void *data)
{
	QP_VI *vi = data;
	retif(vi == NULL, EINA_FALSE, "invalid parameter");

	static qp_vi_op_table anim_init_table[] = {
		{
			.op_type = VI_OP_INSERT,
			.handler = _anim_init_insert,
		},
		{
			.op_type = VI_OP_NONE,
			.handler = NULL,
		},
	};

	int i = 0;
	for (i = 0; anim_init_table[i].op_type != VI_OP_NONE; i++) {
		if (anim_init_table[i].op_type != vi->op_type) {
			continue;
		}

		anim_init_table[i].handler(vi);
		break;
	}

	return EINA_TRUE;
}

static Eina_Bool _anim_job_cb(void *data)
{
	QP_VI *vi = data;
	retif(vi == NULL, EINA_FALSE, "invalid parameter");

	static qp_vi_op_table anim_job_table[] = {
		{
			.op_type = VI_OP_INSERT,
			.handler = _anim_job_insert,
		},
		{
			.op_type = VI_OP_DELETE,
			.handler = _anim_job_delete,
		},
		{
			.op_type = VI_OP_DELETE_ALL,
			.handler = _anim_job_delete_all,
		},
		{
			.op_type = VI_OP_NONE,
			.handler = NULL,
		},
	};

	int i = 0;
	for (i = 0; anim_job_table[i].op_type != VI_OP_NONE; i++) {
		if (anim_job_table[i].op_type != vi->op_type) {
			continue;
		}
		anim_job_table[i].handler(vi);
		break;
	}

	return EINA_TRUE;
}

static Eina_Bool _anim_done_cb(void *data)
{
	QP_VI *vi = data;
	retif(vi == NULL, EINA_FALSE, "invalid parameter");

	static qp_vi_op_table anim_done_table[] = {
		{
			.op_type = VI_OP_INSERT,
			.handler = _anim_done_insert,
		},
		{
			.op_type = VI_OP_DELETE,
			.handler = _anim_done_delete,
		},
		{
			.op_type = VI_OP_DELETE_ALL,
			.handler = _anim_done_delete_all,
		},
		{
			.op_type = VI_OP_NONE,
			.handler = NULL,
		},
	};

	int i = 0;
	for (i = 0; anim_done_table[i].op_type != VI_OP_NONE; i++) {
		if (anim_done_table[i].op_type != vi->op_type) {
			continue;
		}

		anim_done_table[i].handler(vi);
		break;
	}

	return EINA_TRUE;
}

HAPI int quickpanel_noti_listbox_get_geometry(Evas_Object *listbox, int *limit_h, int *limit_partial_h, int *limit_partial_w)
{

	int x = 0, y = 0, w = 0, h = 0;

	retif(listbox == NULL, 0, "invalid parameter");
	retif(limit_h == NULL, 0, "invalid parameter");
	retif(limit_partial_h == NULL, 0, "invalid parameter");
	retif(limit_partial_w == NULL, 0, "invalid parameter");
	evas_object_geometry_get(listbox, &x, &y, &w, &h);

	*limit_h =  y + h;
	*limit_partial_h = *limit_h;
	*limit_partial_w = 0;

	return 1;
}

static void _notibox_deleted_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	quickpanel_uic_close_quickpanel(EINA_FALSE, EINA_FALSE);
}

HAPI void quickpanel_noti_listbox_closing_trigger_set(Evas_Object *listbox)
{
	Evas_Object *item = NULL;
	Eina_List *items = NULL;
	retif(listbox == NULL, , "invalid parameter");

	if ((items = elm_box_children_get(listbox)) != NULL) {
		item = eina_list_nth(items, 0);
		if (item != NULL) {
			evas_object_event_callback_add(item,
					EVAS_CALLBACK_DEL, _notibox_deleted_cb, NULL);
		}
		eina_list_free(items);
	}
}
