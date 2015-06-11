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


#include "quickpanel-ui.h"
#include "common.h"
#include "list_util.h"
#include "quickpanel_def.h"
#include "noti_gridbox.h"
#include "noti_box.h"
#include "vi_manager.h"

#define E_DATA_LAYOUT_PORTRAIT "layout_portrait"
#define E_DATA_LAYOUT_LANDSCAPE "layout_landscape"
#define E_DATA_CB_DELETE_ITEM "cb_delete_item"
#define E_DATA_CB_REMOVED "cb_removed"
#define E_DATA_APP_DATA "app_data"

typedef struct _gridbox_info_layout {
	int n_per_rows;
	int padding_top;
	int padding_left;
	int padding_right;
	int padding_bottom;
	int padding_between;
	int child_w;
	int child_h;
	double scale;
	int limit_w;
} gridbox_info_layout;

static Eina_Bool _anim_init_cb(void *data);
static Eina_Bool _anim_job_cb(void *data);
static Eina_Bool _anim_done_cb(void *data);

static gridbox_info_layout *_gridbox_get_layout(Evas_Object *gridbox)
{
	struct appdata *ad = quickpanel_get_app_data();
	gridbox_info_layout *info_layout = NULL;

	retif(gridbox == NULL, NULL, "invalid parameter");
	retif(ad == NULL, NULL, "invalid data.");

	if (ad->angle == 270 || ad->angle == 90) {
		info_layout = evas_object_data_get(gridbox, E_DATA_LAYOUT_LANDSCAPE);
	} else {
		info_layout = evas_object_data_get(gridbox, E_DATA_LAYOUT_PORTRAIT);
	}

	return info_layout;
}

static void _gridbox_layout_get_pos(int order, int *x, int *y, void *data)
{
	gridbox_info_layout *info_layout = data;
	retif(info_layout == NULL, , "invalid parameter");

	int n_per_row = info_layout->n_per_rows;

	int row = (order - 1) / n_per_row;
	int column = (order - 1) - (row * n_per_row);

	int row_x = info_layout->padding_left
			+ ((info_layout->child_w + info_layout->padding_between) * column);

	int row_y = info_layout->padding_top
			+ ((info_layout->child_h + info_layout->padding_between) * row);

	if (x != NULL) {
		*x = row_x;
	}

	if (y != NULL) {
		*y = row_y;
	}
}

static void _gridbox_layout_get_coord(Evas_Object *gridbox, int num_child, int index,
		void *layout_data, int *coord_x, int *coord_y)
{
	int x, y, w, h;
	int off_x = 0, off_y = 0;
	int child_w;
	int space_w = 0;
	int num_padding_between = 0;
	struct appdata *ad = quickpanel_get_app_data();
	gridbox_info_layout *info_layout = NULL;

	retif(gridbox == NULL, , "invalid parameter");
	retif(ad == NULL, , "invalid data.");

	if (layout_data != NULL) {
		info_layout = (gridbox_info_layout *) layout_data;
	} else {
		info_layout = _gridbox_get_layout(gridbox);
	}
	retif(info_layout == NULL, , "invalid data.");

	//box geometry
	evas_object_geometry_get(gridbox, &x, &y, &w, &h);

	num_padding_between = info_layout->n_per_rows / 2;
	num_padding_between += (info_layout->n_per_rows > 1 && (info_layout->n_per_rows % 2) > 0) ? 1 : 0;

	space_w = (info_layout->padding_left * 2) + (info_layout->padding_between * num_padding_between);
	child_w = (info_layout->limit_w - space_w) / info_layout->n_per_rows;

	info_layout->child_w = child_w;
	_gridbox_layout_get_pos(index, &off_x, &off_y, info_layout);

	if (coord_x != NULL) {
		*coord_x = x + off_x;
	}
	if (coord_y != NULL) {
		*coord_y = y + off_y;
	}
}

static void _gridbox_layout(Evas_Object *o, Evas_Object_Box_Data *priv,
		void *data)
{
	int n_children;
	int x, y;
	int off_x = 0, off_y = 0;
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object_Box_Option *opt;
	int child_w;
	int space_w = 0;
	int num_padding_between = 0;

	retif(o == NULL, , "invalid parameter");
	retif(priv == NULL, , "invalid parameter");
	retif(data == NULL, , "invalid parameter");

	gridbox_info_layout *info_layout = _gridbox_get_layout(data);
	retif(info_layout == NULL, , "failed to get layout data");

	n_children = eina_list_count(priv->children);
	if (!n_children) {
		evas_object_size_hint_min_set(o, ELM_SCALE_SIZE(-1), ELM_SCALE_SIZE(0));
		return;
	}

	//box geometry
	evas_object_geometry_get(o, &x, &y, NULL, NULL);

	num_padding_between = info_layout->n_per_rows / 2;
	num_padding_between += (info_layout->n_per_rows > 1 && (info_layout->n_per_rows % 2) > 0) ? 1 : 0;

	space_w = (info_layout->padding_left * 2) + (info_layout->padding_between * num_padding_between);
	child_w = (info_layout->limit_w - space_w) / info_layout->n_per_rows;

	info_layout->child_w = child_w;

	int order_children = 1;
	EINA_LIST_FOREACH_SAFE(priv->children, l, l_next, opt) {
		_gridbox_layout_get_pos(order_children, &off_x, &off_y, info_layout);
		evas_object_move(opt->obj, x + off_x, y + off_y);
		evas_object_size_hint_min_set(opt->obj, info_layout->child_w,
				info_layout->child_h);
		evas_object_resize(opt->obj, info_layout->child_w,
				info_layout->child_h);
		order_children++;
	}

	evas_object_size_hint_min_set(o, ELM_SCALE_SIZE(-1), off_y + info_layout->child_h + info_layout->padding_bottom);
}

HAPI Evas_Object *quickpanel_noti_gridbox_create(Evas_Object *parent, void *data)
{

	retif(parent == NULL, NULL, "invalid parameter");
	retif(data == NULL, NULL, "invalid parameter");
	struct appdata *ad = data;
	Evas_Object *gridbox = NULL;

	gridbox_info_layout *info_layout_portrait = NULL;
	gridbox_info_layout *info_layout_landscape = NULL;

	info_layout_portrait = (gridbox_info_layout *) malloc(sizeof(gridbox_info_layout));
	retif(info_layout_portrait == NULL, NULL, "memory allocation failed");
	info_layout_portrait->padding_between = 12 * ad->scale;
	info_layout_portrait->padding_top = 0;
	info_layout_portrait->padding_left = 14 * ad->scale;
	info_layout_portrait->padding_bottom = 12 * ad->scale;
	info_layout_portrait->n_per_rows = 2;
	info_layout_portrait->child_w = 0; //340;
	info_layout_portrait->child_h = BOX_HEIGHT_P * ad->scale; //400;
	info_layout_portrait->limit_w = ad->win_width; //400;
	info_layout_portrait->scale = ad->scale;

	info_layout_landscape = (gridbox_info_layout *) malloc(sizeof(gridbox_info_layout));
	if (info_layout_landscape == NULL) {
		free(info_layout_portrait);
		ERR("memory allocation failed");
		return NULL;
	}
	info_layout_landscape->padding_between = 12 * ad->scale;
	info_layout_landscape->padding_top = 0;
	info_layout_landscape->padding_left = 14 * ad->scale;
	info_layout_landscape->padding_bottom = 12 * ad->scale;
	info_layout_landscape->n_per_rows = 3;
	info_layout_landscape->child_w = 0; //409;
	info_layout_landscape->child_h = BOX_HEIGHT_L * ad->scale; //400;
	info_layout_landscape->limit_w = ad->win_height; //400;
	info_layout_landscape->scale = ad->scale;

	gridbox = elm_box_add(parent);
	evas_object_size_hint_weight_set(gridbox, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(gridbox, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_box_layout_set(gridbox, _gridbox_layout, gridbox, NULL);
	evas_object_show(gridbox);

	evas_object_data_set(gridbox, E_DATA_LAYOUT_PORTRAIT, info_layout_portrait);
	evas_object_data_set(gridbox, E_DATA_LAYOUT_LANDSCAPE,
			info_layout_landscape);
	evas_object_data_set(gridbox, E_DATA_CB_DELETE_ITEM, NULL);
	evas_object_data_set(gridbox, E_DATA_APP_DATA, ad);

	qp_item_data *qid
		= quickpanel_list_util_item_new(QP_ITEM_TYPE_NOTI, NULL);
	quickpanel_list_util_item_set_tag(gridbox, qid);

	return gridbox;
}

HAPI void quickpanel_noti_gridbox_remove(Evas_Object *gridbox)
{
	retif(gridbox == NULL, , "invalid parameter");

	gridbox_info_layout *info_layout_portrait = evas_object_data_get(gridbox,
			E_DATA_LAYOUT_PORTRAIT);
	gridbox_info_layout *info_layout_landscape = evas_object_data_get(gridbox,
			E_DATA_LAYOUT_LANDSCAPE);

	quickpanel_noti_gridbox_remove_all_item(gridbox, 0);
	evas_object_data_del(gridbox, E_DATA_LAYOUT_PORTRAIT);
	evas_object_data_del(gridbox, E_DATA_LAYOUT_LANDSCAPE);
	evas_object_data_del(gridbox, E_DATA_CB_DELETE_ITEM);
	evas_object_data_del(gridbox, E_DATA_APP_DATA);
	quickpanel_list_util_item_del_tag(gridbox);
	evas_object_del(gridbox);
	gridbox = NULL;

	if (info_layout_portrait != NULL) {
		free(info_layout_portrait);
	}
	if (info_layout_landscape != NULL) {
		free(info_layout_landscape);
	}
}

HAPI void quickpanel_noti_gridbox_set_item_deleted_cb(Evas_Object *gridbox,
		void(*deleted_cb)(void *data, Evas_Object *obj))
{
	retif(gridbox == NULL, , "invalid parameter");
	retif(deleted_cb == NULL, , "invalid parameter");

	evas_object_data_set(gridbox, E_DATA_CB_DELETE_ITEM, deleted_cb);
}

static void _gridbox_call_item_deleted_cb(Evas_Object *gridbox, void *data,
		Evas_Object *obj)
	{
	retif(gridbox == NULL, , "invalid parameter");

	void (*deleted_cb)(void *data, Evas_Object *obj) = NULL;

	deleted_cb = evas_object_data_get(gridbox, E_DATA_CB_DELETE_ITEM);

	if (deleted_cb != NULL) {
		deleted_cb(data, obj);
	}
}

HAPI void quickpanel_noti_gridbox_add_item(Evas_Object *gridbox, Evas_Object *item, int is_prepend)
{
	QP_VI *vi = NULL;
	const char *signal = NULL;
	gridbox_info_layout *info_layout = NULL;

	retif(gridbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");

	struct appdata *ad = evas_object_data_get(gridbox, E_DATA_APP_DATA);
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

	info_layout = _gridbox_get_layout(gridbox);
	retif(info_layout == NULL, , "invalid parameter");

	_gridbox_layout_get_coord(gridbox, 0, 1, NULL, NULL, NULL);

	evas_object_size_hint_min_set(item, info_layout->child_w,
			info_layout->child_h);
	evas_object_resize(item, info_layout->child_w,
			info_layout->child_h);

	vi = quickpanel_vi_new_with_data(
			VI_OP_INSERT,
			QP_ITEM_TYPE_NOTI,
			gridbox,
			item,
			_anim_init_cb,
			_anim_job_cb,
			_anim_done_cb,
			_anim_done_cb,
			vi,
			NULL,
			is_prepend,
			0);
	quickpanel_vi_start(vi);
}

static void _anim_init_insert(void *data)
{
	QP_VI *vi = data;
	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	Evas_Object *gridbox = vi->container;
	Evas_Object *item = vi->target;

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid parameter");

	evas_object_clip_set(item, evas_object_clip_get(gridbox));
	evas_object_color_set(item, 0, 0, 0, 0);
}

static void _anim_job_insert(void *data)
{
	QP_VI *vi = data;
	int index = 1, index_child = 1;
	int is_prepend = 0;
	int coord_x, coord_y = 0;
	int coord_old_x, coord_old_y = 0;
	int coord_fix_x, coord_fix_y = 0;
	Evas_Object *gridbox = NULL;
	Evas_Object *item = NULL;
	Elm_Transit *transit_layout = NULL;
	Elm_Transit *transit_fadein = NULL;
	gridbox_info_layout *info_layout = NULL;

	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	gridbox = vi->container;
	item = vi->target;
	is_prepend = vi->extra_flag_1;

	info_layout = _gridbox_get_layout(gridbox);
	retif(info_layout == NULL, , "invalid parameter");

	if (is_prepend != 1) {
		index_child = quickpanel_noti_gridbox_get_item_count(gridbox);
	}
	_gridbox_layout_get_coord(gridbox, 0, index_child, NULL, &coord_x, &coord_y);
	evas_object_move(item, coord_x, coord_y);

	if (is_prepend == 1) {
		Eina_List *l;
		Eina_List *l_next;
		Evas_Object *obj;
		Eina_List *item_list = elm_box_children_get(gridbox);

		DBG("all count:%d", eina_list_count (item_list));

		EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
			if (obj != NULL) {
				transit_layout = elm_transit_add();
				if (transit_layout != NULL) {
					evas_object_geometry_get(obj, &coord_old_x, &coord_old_y, NULL, NULL);
					_gridbox_layout_get_coord(gridbox, 0, index + 1, NULL, &coord_x, &coord_y);

					coord_x = coord_x - coord_old_x;
					coord_y = coord_y - coord_old_y;
					coord_fix_x = (coord_x != 0) ? coord_x / coord_x : 0;
					coord_fix_y = (coord_y != 0) ? coord_y / coord_y : 0;
					elm_transit_effect_translation_add(transit_layout, 0, 0, coord_x + coord_fix_x, coord_y + coord_fix_y);
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
			index++;
		}

		if (item_list != NULL) {
			eina_list_free(item_list);
		}
	}

	transit_fadein = elm_transit_add();
	if (transit_fadein != NULL) {
		elm_transit_object_add(transit_fadein, item);
		elm_transit_effect_color_add(transit_fadein, 0, 0, 0, 0, 255, 255, 255, 255);
		elm_transit_duration_set(transit_fadein,
				quickpanel_vim_get_duration(VI_OP_INSERT));
		elm_transit_tween_mode_set(transit_fadein,
				quickpanel_vim_get_tweenmode(VI_OP_INSERT));
		elm_transit_del_cb_set(transit_fadein,
				quickpanel_vi_done_cb_for_transit, vi);
		if (transit_layout != NULL) {
			elm_transit_chain_transit_add(transit_layout, transit_fadein);
		} else {
			elm_transit_go(transit_fadein);
		}
	} else {
		ERR("failed to create a transit");
		quickpanel_vi_done(vi);
	}
}

static void _anim_done_insert(void *data)
{
	QP_VI *vi = data;
	retif(vi == NULL, , "invalid parameter");

	Evas_Object *gridbox = vi->container;
	Evas_Object *item = vi->target;
	int is_prepend = vi->extra_flag_1;

	retif(gridbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");

	evas_object_color_set(item, 255, 255, 255, 255);

	if (is_prepend == GRIDBOX_PREPEND) {
		elm_box_pack_start(gridbox, item);
	} else {
		elm_box_pack_end(gridbox, item);
	}
}

static void _anim_job_delete(void *data)
{
	QP_VI *vi = data;
	int coord_x, coord_y = 0;
	int coord_old_x, coord_old_y = 0;
	int coord_fix_x, coord_fix_y = 0;

	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	Elm_Transit *transit_layout = NULL;
	Elm_Transit *transit_fadein = NULL;
	Evas_Object *gridbox = vi->container;
	Evas_Object *item = vi->target;

	transit_fadein = elm_transit_add();
	if (transit_fadein != NULL) {
		elm_transit_object_add(transit_fadein, item);
		elm_transit_effect_color_add(transit_fadein, 255, 255, 255, 255, 0, 0, 0, 0);
		elm_transit_objects_final_state_keep_set(transit_fadein, EINA_TRUE);
		elm_transit_tween_mode_set(transit_fadein,
				quickpanel_vim_get_tweenmode(VI_OP_DELETE));
		elm_transit_duration_set(transit_fadein,
				quickpanel_vim_get_duration(VI_OP_DELETE));
		elm_transit_go(transit_fadein);
	} else {
		ERR("failed to create a transit");
	}

	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj;
	Eina_List *item_list = elm_box_children_get(gridbox);

	DBG("all count:%d", eina_list_count (item_list));

	int index_child = 1;
	int is_start_relayout = 0;
	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
		if (obj == item) {
			is_start_relayout = 1;
		} else if (obj != NULL && is_start_relayout == 1) {
			transit_layout = elm_transit_add();
			if (transit_layout != NULL) {
				evas_object_geometry_get(obj, &coord_old_x, &coord_old_y, NULL, NULL);
				_gridbox_layout_get_coord(gridbox, 0, index_child - 1, NULL, &coord_x, &coord_y);

				coord_x = coord_x - coord_old_x;
				coord_y = coord_y - coord_old_y;
				coord_fix_x = (coord_x != 0) ? coord_x/coord_x : 0;
				coord_fix_y = (coord_y != 0) ? coord_y/coord_y : 0;
				elm_transit_effect_translation_add(transit_layout, 0, 0, coord_x + coord_fix_x, coord_y + coord_fix_y);
				elm_transit_object_add(transit_layout, obj);
				elm_transit_duration_set(transit_layout,
						quickpanel_vim_get_duration(VI_OP_REORDER));
				elm_transit_tween_mode_set(transit_layout,
						quickpanel_vim_get_tweenmode(VI_OP_REORDER));
				elm_transit_objects_final_state_keep_set(transit_layout, EINA_TRUE);
				if (transit_fadein != NULL) {
					elm_transit_chain_transit_add(transit_fadein, transit_layout);
				}
			} else {
				ERR("failed to create a transit");
			}
		}
		index_child++;
	}

	if (item_list != NULL) {
		eina_list_free(item_list);
	}

	if (transit_layout != NULL) {
		elm_transit_del_cb_set(transit_layout, quickpanel_vi_done_cb_for_transit, vi);
	} else if (transit_fadein != NULL) {
		elm_transit_del_cb_set(transit_fadein, quickpanel_vi_done_cb_for_transit, vi);
	} else {
		ERR("failed to create a transit");
		quickpanel_vi_done(vi);
	}
}

static void _anim_done_delete(void *data)
{
	QP_VI *vi = data;
	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	Evas_Object *gridbox = vi->container;
	Evas_Object *item = vi->target;

	elm_box_unpack(gridbox, item);
	quickpanel_noti_box_remove(item);
	_gridbox_call_item_deleted_cb(gridbox, quickpanel_noti_box_node_get(item), NULL);
}

HAPI void quickpanel_noti_gridbox_remove_item(Evas_Object *gridbox, Evas_Object *item, int with_animation)
{
	QP_VI *vi = NULL;
	retif(gridbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");

	if (with_animation == 1) {
		vi = quickpanel_vi_new_with_data(
				VI_OP_DELETE,
				QP_ITEM_TYPE_NOTI,
				gridbox,
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
		void *node = quickpanel_noti_box_node_get(item);
		elm_box_unpack(gridbox, item);
		quickpanel_noti_box_remove(item);
		_gridbox_call_item_deleted_cb(gridbox,
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

	Evas_Object *gridbox = vi->container;

	item_list = elm_box_children_get(gridbox);
	retif(item_list == NULL, , "invalid parameter");

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
		if (obj != NULL) {
			DBG("try to remove:%p", obj);
			quickpanel_noti_gridbox_remove_item(gridbox, obj, EINA_TRUE);
		}
	}

	if (item_list != NULL) {
		eina_list_free(item_list);
	}
}

HAPI void quickpanel_noti_gridbox_remove_all_item(Evas_Object *gridbox, int with_animation)
{
	QP_VI *vi = NULL;
	retif(gridbox == NULL, , "invalid parameter");

	vi = quickpanel_vi_new_with_data(
			VI_OP_DELETE_ALL,
			QP_ITEM_TYPE_NOTI,
			gridbox,
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

static void _anim_job_update(void *data)
{
	QP_VI *vi = data;
	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	quickpanel_vi_done(data);
}

static void _anim_done_update(void *data)
{
	QP_VI *vi = data;
	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	Evas_Object *gridbox = vi->container;
	Evas_Object *item = vi->target;

	if (quickpanel_noti_gridbox_get_item_exist(gridbox, item) == 1) {
		quickpanel_noti_box_item_update(item);
	}
}

HAPI void quickpanel_noti_gridbox_update_item(Evas_Object *gridbox, Evas_Object *item)
{
	QP_VI *vi = NULL;
	retif(gridbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");

	vi = quickpanel_vi_new_with_data(
			VI_OP_UPDATE,
			QP_ITEM_TYPE_NOTI,
			gridbox,
			item,
			_anim_init_cb,
			_anim_job_cb,
			_anim_done_cb,
			_anim_done_cb,
			vi,
			NULL,
			0,
			0);

	retif(vi == NULL, , "quickpanel_vi_new_with_data returns NULL");
	vi->disable_interrupt_userevent = 1;
	vi->disable_freezing = 1;
	quickpanel_vi_start(vi);
}

HAPI void quickpanel_noti_gridbox_remove_and_add_item(Evas_Object *gridbox, Evas_Object *item
		,void (*update_cb)(Evas_Object *list, void *data, int is_prepend)
		,void *container, void *data, int pos)
{
	QP_VI *vi = NULL;
	retif(gridbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");
	retif(update_cb == NULL, , "invalid parameter");
	retif(container == NULL, , "invalid parameter");
	retif(data == NULL, , "invalid parameter");

	vi = quickpanel_vi_new_with_data(
			VI_OP_DELETE,
			QP_ITEM_TYPE_NOTI,
			gridbox,
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
}

HAPI void gridbox_finalize_rotation_cb(void *data)
{
	retif(data == NULL, , "invalid parameter");
	Evas_Object *gridbox = data;

	elm_box_recalculate(gridbox);
}

HAPI void quickpanel_noti_gridbox_rotation(Evas_Object *gridbox, int angle)
{
	const char *signal = NULL;

	retif(gridbox == NULL, , "invalid parameter");

	gridbox_info_layout *info_layout_portrait = evas_object_data_get(gridbox,
			E_DATA_LAYOUT_PORTRAIT);
	gridbox_info_layout *info_layout_landscape = evas_object_data_get(gridbox,
			E_DATA_LAYOUT_LANDSCAPE);

	retif(info_layout_portrait == NULL || info_layout_landscape == NULL, ,
			"gridbox is crashed");

	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj;
	Eina_List *item_list = elm_box_children_get(gridbox);

	if (angle == 270 || angle == 90) {
		signal = "box.landscape";
	} else {
		signal = "box.portrait";
	}

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
		if (obj != NULL) {
			elm_object_signal_emit(obj, signal, "box.prog");
			edje_object_message_signal_process(_EDJ(obj));
			DBG("set to %s, %x", signal, obj);
		}
	}

	if (item_list != NULL) {
		eina_list_free(item_list);
	}

	elm_box_layout_set(gridbox, _gridbox_layout, gridbox, NULL);
	DBG("rotation angle is %d", angle);
}

HAPI int quickpanel_noti_gridbox_get_item_count(Evas_Object *gridbox)
{
	int item_count = 0;
	Eina_List *items = NULL;
	retif(gridbox == NULL, 0, "invalid parameter");

	if ((items = elm_box_children_get(gridbox)) != NULL) {
		item_count = eina_list_count(items);
		eina_list_free(items);
		return item_count;
	} else {
		return 0;
	}
}

HAPI int quickpanel_noti_gridbox_get_item_exist(Evas_Object *gridbox, Evas_Object *box)
{
	int ret = 0;
	Eina_List *items = NULL;
	retif(gridbox == NULL, 0, "invalid parameter");

	if ((items = elm_box_children_get(gridbox)) != NULL) {
		if (eina_list_data_find(items, box) != NULL) {
			ret = 1;
		}
		eina_list_free(items);
	}

	return ret;
}

static Eina_Bool _anim_init_cb(void *data)
{
	int i = 0;
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
	int i = 0;
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
			.op_type = VI_OP_UPDATE,
			.handler = _anim_job_update,
		},
		{
			.op_type = VI_OP_NONE,
			.handler = NULL,
		},
	};

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
	int i = 0;
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
			.op_type = VI_OP_UPDATE,
			.handler = _anim_done_update,
		},
		{
			.op_type = VI_OP_NONE,
			.handler = NULL,
		},
	};

	for (i = 0; anim_done_table[i].op_type != VI_OP_NONE; i++) {
		if (anim_done_table[i].op_type != vi->op_type) {
			continue;
		}

		anim_done_table[i].handler(vi);
		break;
	}

	return EINA_TRUE;
}

HAPI int quickpanel_noti_gridbox_get_geometry(Evas_Object *gridbox,
		int *limit_h, int *limit_partial_h, int *limit_partial_w)
{

	int count = 0;
	int num_last = 0;
	int x = 0, y = 0, w = 0, h = 0;
	Eina_List *list = NULL;
	gridbox_info_layout *info_layout = NULL;

	retif(gridbox == NULL, 0, "invalid parameter");
	retif(limit_h == NULL, 0, "invalid parameter");
	retif(limit_partial_h == NULL, 0, "invalid parameter");
	retif(limit_partial_w == NULL, 0, "invalid parameter");
	evas_object_geometry_get(gridbox, &x, &y, &w, &h);

	info_layout = _gridbox_get_layout(gridbox);
	retif(info_layout == NULL, 0, "invalid parameter");

	list = elm_box_children_get(gridbox);
	if (list != NULL) {
		count = eina_list_count(list);
		num_last = count % info_layout->n_per_rows;
		eina_list_free(list);
	} else {
		num_last = 0;
	}

	*limit_h =  y + h;
	if (num_last > 0) {
		*limit_partial_h = *limit_h - info_layout->child_h;
		*limit_partial_w = num_last * info_layout->child_w;
	} else {
		*limit_partial_h = *limit_h;
		*limit_partial_w = 0;
	}

	return 1;
}

static void _notibox_deleted_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	quickpanel_uic_close_quickpanel(EINA_FALSE, EINA_FALSE);
}

HAPI void quickpanel_noti_gridbox_closing_trigger_set(Evas_Object *gridbox)
{
	Evas_Object *item = NULL;
	Eina_List *items = NULL;
	retif(gridbox == NULL, , "invalid parameter");

	if ((items = elm_box_children_get(gridbox)) != NULL) {
		item = eina_list_nth(items, 0);
		if (item != NULL) {
			evas_object_event_callback_add(item,
					EVAS_CALLBACK_DEL, _notibox_deleted_cb, NULL);
		}
		eina_list_free(items);
	}
}
