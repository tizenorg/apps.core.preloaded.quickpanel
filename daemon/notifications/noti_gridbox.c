/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the License);
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

#include <Ecore_X.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "list_util.h"
#include "quickpanel_def.h"
#include "noti_gridbox.h"
#include "noti_box.h"

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

typedef struct _gridbox_info_animation {
	Evas_Object *gridbox;
	Evas_Object *item;

	void (*update_cb)(Evas_Object *list, void *data, int is_prepend);
	Evas_Object *container;
	void *noti;
	int pos;
} gridbox_info_animation;

static void _gridbox_layout_get_pos(int order, int *x, int *y, void *data) {
	gridbox_info_layout *info_layout = data;

	retif(data == NULL, , "invalid parameter");
	retif(x == NULL, , "invalid parameter");
	retif(y == NULL, , "invalid parameter");

	int n_per_row = info_layout->n_per_rows;

	int row = (order - 1) / n_per_row;
	int column = (order - 1) - (row * n_per_row);

	//DBG("order:%d r:%d c:%d", order, row, column);

	int row_x = info_layout->padding_left
			+ ((info_layout->child_w + info_layout->padding_between) * column);

	int row_y = info_layout->padding_top
			+ ((info_layout->child_h + info_layout->padding_between) * row);

	*x = row_x;
	*y = row_y;
}

static void _gridbox_layout(Evas_Object *o, Evas_Object_Box_Data *priv,
		void *data) {
	int n_children;
	int x, y, w, h;
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

	gridbox_info_layout *info_layout = (gridbox_info_layout *) data;

	n_children = eina_list_count(priv->children);
	DBG("layout function:%d", n_children);
	if (!n_children) {
		evas_object_size_hint_min_set(o, -1, 0);
		return;
	}

	//box geometry
	evas_object_geometry_get(o, &x, &y, &w, &h);

	num_padding_between = info_layout->n_per_rows / 2;
	num_padding_between += (info_layout->n_per_rows > 1 && (info_layout->n_per_rows % 2) > 0) ? 1 : 0;

	space_w = (info_layout->padding_left * 2) + (info_layout->padding_between * num_padding_between);
	child_w = (info_layout->limit_w - space_w) / info_layout->n_per_rows;

	info_layout->child_w = child_w;

	DBG("grid layout children pos:%d %d", info_layout->child_w, info_layout->child_h);

	int order_children = 1;
	EINA_LIST_FOREACH_SAFE(priv->children, l, l_next, opt)
	{
		_gridbox_layout_get_pos(order_children, &off_x, &off_y, info_layout);
		evas_object_move(opt->obj, x + off_x, y + off_y);
		evas_object_size_hint_min_set(opt->obj, info_layout->child_w,
				info_layout->child_h);
		evas_object_resize(opt->obj, info_layout->child_w,
				info_layout->child_h);
		order_children++;
	}

	evas_object_size_hint_min_set(o, -1,
			off_y + info_layout->child_h + info_layout->padding_bottom);
}

HAPI Evas_Object *gridbox_create(Evas_Object *parent, void *data) {

	retif(parent == NULL, NULL, "invalid parameter");
	retif(data == NULL, NULL, "invalid parameter");
	struct appdata *ad = data;
	Evas_Object *gridbox = NULL;

	gridbox_info_layout *info_layout_portrait = NULL;
	gridbox_info_layout *info_layout_landscape = NULL;

	info_layout_portrait = (gridbox_info_layout *) malloc(
			sizeof(gridbox_info_layout));
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

	info_layout_landscape = (gridbox_info_layout *) malloc(
			sizeof(gridbox_info_layout));
	retif(info_layout_landscape == NULL, NULL, "memory allocation failed");
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

	if (ad->angle == 270 || ad->angle == 90)
		elm_box_layout_set(gridbox, _gridbox_layout, info_layout_landscape,
				NULL);
	else
		elm_box_layout_set(gridbox, _gridbox_layout, info_layout_portrait,
				NULL);

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

HAPI void gridbox_remove(Evas_Object *gridbox) {

	retif(gridbox == NULL, , "invalid parameter");

	gridbox_info_layout *info_layout_portrait = evas_object_data_get(gridbox,
			E_DATA_LAYOUT_PORTRAIT);
	gridbox_info_layout *info_layout_landscape = evas_object_data_get(gridbox,
			E_DATA_LAYOUT_LANDSCAPE);

	gridbox_remove_all_item(gridbox, 0);
	evas_object_data_del(gridbox, E_DATA_LAYOUT_PORTRAIT);
	evas_object_data_del(gridbox, E_DATA_LAYOUT_LANDSCAPE);
	evas_object_data_del(gridbox, E_DATA_CB_DELETE_ITEM);
	evas_object_data_del(gridbox, E_DATA_APP_DATA);
	quickpanel_list_util_item_del_tag(gridbox);
	evas_object_del(gridbox);

	if (info_layout_portrait != NULL)
		free(info_layout_portrait);
	if (info_layout_landscape != NULL)
		free(info_layout_landscape);
}

HAPI void gridbox_set_item_deleted_cb(Evas_Object *gridbox,
		void(*deleted_cb)(void *data, Evas_Object *obj)) {
	retif(gridbox == NULL, , "invalid parameter");
	retif(deleted_cb == NULL, , "invalid parameter");

	evas_object_data_set(gridbox, E_DATA_CB_DELETE_ITEM, deleted_cb);
}

static void _gridbox_call_item_deleted_cb(Evas_Object *gridbox, void *data,
		Evas_Object *obj) {
	retif(gridbox == NULL, , "invalid parameter");

	void (*deleted_cb)(void *data, Evas_Object *obj) = NULL;

	deleted_cb = evas_object_data_get(gridbox, E_DATA_CB_DELETE_ITEM);

	if (deleted_cb != NULL) {
		deleted_cb(data, obj);
	}
}

HAPI void gridbox_add_item(Evas_Object *gridbox, Evas_Object *item, int is_prepend) {
	const char *signal = NULL;

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

	if (is_prepend == GRIDBOX_PREPEND)
		elm_box_pack_start(gridbox, item);
	else
		elm_box_pack_end(gridbox, item);
}

static void _gridbox_remove_item_anim_cb(void *data, Elm_Transit *transit) {
	DBG("");
	retif(data == NULL, , "invalid parameter");
	retif(transit == NULL, , "invalid parameter");

	gridbox_info_animation *info_animation = data;

	retif(info_animation->gridbox == NULL, , "invalid parameter");
	retif(info_animation->item == NULL, , "invalid parameter");

	DBG("remove:%p", info_animation->item);

	void *node = noti_box_node_get(info_animation->item);
	elm_box_unpack(info_animation->gridbox, info_animation->item);
	noti_box_remove(info_animation->item);
	_gridbox_call_item_deleted_cb(info_animation->gridbox,
			node, NULL);

	if (info_animation->update_cb != NULL) {
		retif(info_animation->container == NULL, , "invalid parameter");
		retif(info_animation->noti == NULL, , "invalid parameter");

		info_animation->update_cb(info_animation->container,
				info_animation->noti, info_animation->pos);
	}

	free(info_animation);
	info_animation = NULL;
}

HAPI void gridbox_remove_item(Evas_Object *gridbox, Evas_Object *item, int with_animation) {
	DBG("remove:%p", item);
	retif(gridbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");

	if (noti_box_get_status(item) == STATE_DELETING) {
		return ;
	}
	noti_box_set_status(item, STATE_DELETING);

	if (with_animation == 1) {
		gridbox_info_animation *info_animation = (gridbox_info_animation *) malloc(
				sizeof(gridbox_info_animation));
		if (info_animation == NULL)
			return;
		info_animation->gridbox = gridbox;
		info_animation->item = item;
		info_animation->update_cb = NULL;
		info_animation->container = NULL;
		info_animation->noti = NULL;
		info_animation->pos = 0;

		Elm_Transit *transit = elm_transit_add();
		//Fade in and out with layout object.
		elm_transit_object_add(transit, item);
		elm_transit_effect_fade_add(transit);
		elm_transit_duration_set(transit, 0.7);
		elm_transit_del_cb_set(transit, _gridbox_remove_item_anim_cb,
				info_animation);
		elm_transit_go(transit);
	} else {
		void *node = noti_box_node_get(item);
		elm_box_unpack(gridbox, item);
		noti_box_remove(item);
		_gridbox_call_item_deleted_cb(gridbox,
				node, NULL);
	}
}

HAPI void gridbox_remove_all_item(Evas_Object *gridbox, int with_animation) {
	DBG("");
	retif(gridbox == NULL, , "invalid parameter");

	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj;
	Eina_List *item_list = elm_box_children_get(gridbox);

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj)
	{
		if (obj != NULL) {
			// call deleted callback
			gridbox_remove_item(gridbox, obj, with_animation);
		}
	}
}

HAPI void gridbox_update_item(Evas_Object *gridbox, Evas_Object *item) {

	retif(gridbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");
}

HAPI void gridbox_remove_and_add_item(Evas_Object *gridbox, Evas_Object *item
		,void (*update_cb)(Evas_Object *list, void *data, int is_prepend)
		,void *container, void *data, int pos) {

	retif(gridbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");
	retif(update_cb == NULL, , "invalid parameter");
	retif(container == NULL, , "invalid parameter");
	retif(data == NULL, , "invalid parameter");

	if (noti_box_get_status(item) == STATE_DELETING) {
		return ;
	}
	noti_box_set_status(item, STATE_DELETING);

	gridbox_info_animation *info_animation = (gridbox_info_animation *) malloc(
			sizeof(gridbox_info_animation));
	if (info_animation == NULL)
		return;
	info_animation->gridbox = gridbox;
	info_animation->item = item;
	info_animation->update_cb = update_cb;
	info_animation->container = container;
	info_animation->noti = data;
	info_animation->pos = pos;

	Elm_Transit *transit = elm_transit_add();
	//Fade in and out with layout object.
	elm_transit_object_add(transit, item);
	elm_transit_effect_fade_add(transit);
	elm_transit_duration_set(transit, 0.4);
	elm_transit_del_cb_set(transit, _gridbox_remove_item_anim_cb,
			info_animation);
	elm_transit_go(transit);
}

HAPI void gridbox_finalize_rotation_cb(void *data) {
	retif(data == NULL, , "invalid parameter");
	Evas_Object *gridbox = data;

	elm_box_recalculate(gridbox);
}

HAPI void gridbox_rotation(Evas_Object *gridbox, int angle) {
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

	DBG("all count:%d", eina_list_count (item_list));

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj)
	{
		if (obj != NULL) {
			elm_object_signal_emit(obj, signal, "box.prog");
			edje_object_message_signal_process(_EDJ(obj));
			elm_layout_sizing_eval(obj);
			DBG("set to %s, %x", signal, obj);
		}
	}

	if (angle == 270 || angle == 90) {
		elm_box_layout_set(gridbox, _gridbox_layout, info_layout_landscape,
				NULL);

#if 0
		layout_data = elm_box_transition_new(0.0, _gridbox_layout,
				info_layout_portrait, NULL, _gridbox_layout,
				info_layout_landscape, NULL, gridbox_finalize_rotation_cb,
				gridbox);
#endif
	} else {
		elm_box_layout_set(gridbox, _gridbox_layout, info_layout_portrait,
				NULL);
#if 0
		layout_data = elm_box_transition_new(0.0, _gridbox_layout,
				info_layout_landscape, NULL, _gridbox_layout,
				info_layout_portrait, NULL, gridbox_finalize_rotation_cb,
				gridbox);
#endif
	}

#if 0
	elm_box_layout_set(gridbox, elm_box_layout_transition, layout_data,
			elm_box_transition_free);
#endif
	DBG("Angle  Rotation  is %d", angle);
}
