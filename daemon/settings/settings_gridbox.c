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
#include "common.h"
#include "list_util.h"
#include "quickpanel_def.h"
#include "settings_gridbox.h"
#include "pager.h"

#define E_DATA_LAYOUT_PORTRAIT "layout_portrait"
#define E_DATA_LAYOUT_LANDSCAPE "layout_landscape"
#define E_DATA_ITEM_TYPE "item_type"
#define E_DATA_POS_INFO "pos_info"
#define DIVIDER_TOUCH_W 20

typedef struct _info_layout {
	int n_per_rows;
	int padding_top;
	int padding_left;
	int padding_right;
	int padding_bottom;
	int padding_between_h;
	int padding_between_v;
	int child_w;
	int child_h;
	double scale;
	int limit_w;
} info_layout;

typedef struct _info_position {
	int index;
	int offset_x;
	int offset_y;
	int width;
	int height;
} info_position;

static Eina_List *_position_info_add(Eina_List *list, int is_icon, int index, int offset_x, int offset_y, int width, int height)
{
	info_position *pos_info = (info_position *) calloc(1, sizeof(info_position));
	retif(pos_info == NULL, NULL, "failed to allocate memory");

	pos_info->index = index;

	if (is_icon == 1) {
		if (offset_x <= 0) {
			pos_info->offset_x = offset_x;
			pos_info->width = width - DIVIDER_TOUCH_W;
		} else {
			pos_info->offset_x = offset_x + DIVIDER_TOUCH_W;
			pos_info->width = width - DIVIDER_TOUCH_W - DIVIDER_TOUCH_W;
		}
	} else {
		if (offset_x <= 0) {
			pos_info->offset_x = offset_x;
			pos_info->width = width - DIVIDER_TOUCH_W;
		} else {
			pos_info->offset_x = offset_x - DIVIDER_TOUCH_W;
			pos_info->width = width + DIVIDER_TOUCH_W + DIVIDER_TOUCH_W;
		}
	}

	pos_info->offset_y = offset_y;
	pos_info->height = height;

	return eina_list_append(list, pos_info);
}

static void _position_info_clear(Eina_List *list)
{
	info_position *pos_info = NULL;

	EINA_LIST_FREE(list, pos_info) {
		if (pos_info != NULL) {
			free(pos_info);
			pos_info = NULL;
		}
	}
}

static Eina_List *_position_info_get(Evas_Object *gridbox)
{
	return evas_object_data_get(gridbox, E_DATA_POS_INFO);
}

static void _position_info_set(Evas_Object *gridbox, Eina_List *list)
{
	evas_object_data_set(gridbox, E_DATA_POS_INFO, list);
}

static info_layout *_get_layout(Evas_Object *gridbox)
{
	struct appdata *ad = quickpanel_get_app_data();
	info_layout *info_layout = NULL;

	retif(ad == NULL, NULL, "invalid data.");
	retif(gridbox == NULL, NULL, "invalid parameter");

	if (ad->angle == 270 || ad->angle == 90) {
		info_layout = evas_object_data_get(gridbox, E_DATA_LAYOUT_LANDSCAPE);
	} else {
		info_layout = evas_object_data_get(gridbox, E_DATA_LAYOUT_PORTRAIT);
	}

	return info_layout;
}

static int _item_is_icon(Evas_Object *icon)
{
	const char *item_type = NULL;
	retif(icon == NULL, 1, "invalid parameter");

	item_type = evas_object_data_get(icon, E_DATA_ITEM_TYPE);
	retif(item_type == NULL, 1, "invalid parameter");

	if (strcmp(item_type, SETTINGS_GRIDBOX_ITEM_ICON) == 0) {
		return 1;
	}

	return 0;
}

static void _item_pos_get(int order, int *x, int *y, void *data)
{
	info_layout *info_layout = data;
	retif(info_layout == NULL, , "invalid parameter");

	int n_per_row = info_layout->n_per_rows;

	int row = (order - 1) / n_per_row;
	int column = (order - 1) - (row * n_per_row);

	int row_x = info_layout->padding_left
		+ ((info_layout->child_w + info_layout->padding_between_h) * column);

	int row_y = info_layout->padding_top
		+ ((info_layout->child_h + info_layout->padding_between_v) * row);

	if (x != NULL) {
		*x = row_x;
	}

	if (y != NULL) {
		*y = row_y;
	}
}

static inline void _item_move_and_resize(Evas_Object *item, int x, int y, int w, int h)
{
	evas_object_move(item, x, y);
	evas_object_size_hint_min_set(item, w, h);
	evas_object_resize(item, w, h);
}

static void _layout_cb(Evas_Object *o, Evas_Object_Box_Data *priv, void *data)
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

	info_layout *info_layout = _get_layout(data);
	Eina_List *list_pos_info = _position_info_get(data);

	n_children = eina_list_count(priv->children);
	DBG("layout function:%d", n_children);
	if (!n_children) {
		evas_object_size_hint_min_set(o, ELM_SCALE_SIZE(-1), ELM_SCALE_SIZE(0));
		return;
	}

	//box geometry
	evas_object_geometry_get(o, &x, &y, NULL, NULL);

	num_padding_between = info_layout->n_per_rows >> 1;
	num_padding_between += (info_layout->n_per_rows > 1 && (info_layout->n_per_rows % 2) > 0) ? 1 : 0;

	space_w = (info_layout->padding_left * 2) + (info_layout->padding_between_h * num_padding_between);
	child_w = (info_layout->limit_w - space_w) / info_layout->n_per_rows;

	info_layout->child_w = child_w;

	if (list_pos_info != NULL) {
		_position_info_clear(list_pos_info);
		_position_info_set(data, NULL);
		list_pos_info = NULL;
	}

	int order_obj = 0;
	int order_children = 1;
	int order_divider = 1;
	Evas_Object *btn_previous = NULL;

	EINA_LIST_FOREACH_SAFE(priv->children, l, l_next, opt) {
		if (_item_is_icon(opt->obj)) {
			_item_pos_get(order_children, &off_x, &off_y, info_layout);
			_item_move_and_resize(opt->obj, x + off_x, y + off_y,
					info_layout->child_w, info_layout->child_h);
			order_children++;
			list_pos_info =
				_position_info_add(list_pos_info, 1, order_obj, off_x, off_y, info_layout->child_w, info_layout->child_h);
			if (btn_previous != NULL && opt->obj != NULL) {
				elm_object_focus_next_object_set(opt->obj, btn_previous, ELM_FOCUS_LEFT);
				elm_object_focus_next_object_set(btn_previous, opt->obj, ELM_FOCUS_RIGHT);
			}
			btn_previous = opt->obj;
		} else {
			_item_pos_get(order_children - 1, &off_x, &off_y, info_layout);
			_item_move_and_resize(opt->obj, x + off_x + info_layout->child_w, y + off_y,
					info_layout->padding_between_h, info_layout->child_h);
			if ((order_divider % info_layout->n_per_rows) == 0) {
				evas_object_hide(opt->obj);
			} else {
				evas_object_show(opt->obj);
			}
			order_divider++;
			list_pos_info =
				_position_info_add(list_pos_info, 0, order_obj, off_x + info_layout->child_w, off_y,
						info_layout->padding_between_h, info_layout->child_h);
		}
		order_obj++;
	}

	_position_info_set(data, list_pos_info);
}

static void _deleted_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("deleted_cb");
	Eina_List *list = NULL;
	Evas_Object *gridbox = obj;
	retif(gridbox == NULL, , "invalid parameter");

	info_layout *info_layout_portrait = evas_object_data_get(gridbox,
			E_DATA_LAYOUT_PORTRAIT);
	info_layout *info_layout_landscape = evas_object_data_get(gridbox,
			E_DATA_LAYOUT_LANDSCAPE);

	list = _position_info_get(gridbox);
	_position_info_clear(list);
	_position_info_set(gridbox, NULL);

	quickpanel_settings_gridbox_item_remove_all(gridbox);
	evas_object_data_del(gridbox, E_DATA_LAYOUT_PORTRAIT);
	evas_object_data_del(gridbox, E_DATA_LAYOUT_LANDSCAPE);

	if (info_layout_portrait != NULL) {
		free(info_layout_portrait);
	}
	if (info_layout_landscape != NULL) {
		free(info_layout_landscape);
	}
}

HAPI Evas_Object *quickpanel_settings_gridbox_create(Evas_Object *parent, void *data)
{
	retif(parent == NULL, NULL, "invalid parameter");
	retif(data == NULL, NULL, "invalid parameter");
	struct appdata *ad = data;
	Evas_Object *gridbox = NULL;

	info_layout *info_layout_portrait = NULL;
	info_layout *info_layout_landscape = NULL;

	info_layout_portrait = (info_layout *) calloc(1,
			sizeof(info_layout));
	retif(info_layout_portrait == NULL, NULL, "memory allocation failed");
	info_layout_portrait->padding_between_h = 2 * ad->scale;
	info_layout_portrait->padding_between_v = 1 * ad->scale;
	info_layout_portrait->padding_top = 0;
	info_layout_portrait->padding_left = 1;
	info_layout_portrait->padding_bottom = 0;
	info_layout_portrait->n_per_rows = 8;
	info_layout_portrait->child_w = 0;

	info_layout_portrait->child_h = QP_SETTING_ICON_MIN_WH_WVGA * ad->scale;

	info_layout_portrait->limit_w = ad->win_width;
	info_layout_portrait->scale = ad->scale;

	info_layout_landscape = (info_layout *) calloc(1, sizeof(info_layout));
	if (info_layout_landscape == NULL) {
		free(info_layout_portrait);
		ERR("memory allocation failed");
		return NULL;
	}
	info_layout_landscape->padding_between_h = 2 * ad->scale;
	info_layout_landscape->padding_between_v = 1 * ad->scale;
	info_layout_landscape->padding_top = 0;
	info_layout_landscape->padding_left = 1;
	info_layout_landscape->padding_bottom = 0;
	info_layout_landscape->n_per_rows = 10;
	info_layout_landscape->child_w = 0;

	info_layout_landscape->child_h = QP_SETTING_ICON_MIN_WH_WVGA * ad->scale;

	info_layout_landscape->limit_w = ad->win_height;
	info_layout_landscape->scale = ad->scale;

	gridbox = elm_box_add(parent);
	evas_object_size_hint_weight_set(gridbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(gridbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(gridbox, EINA_TRUE);

	elm_box_layout_set(gridbox, _layout_cb, gridbox, NULL);

	evas_object_data_set(gridbox, E_DATA_LAYOUT_PORTRAIT, info_layout_portrait);
	evas_object_data_set(gridbox, E_DATA_LAYOUT_LANDSCAPE, info_layout_landscape);
	evas_object_event_callback_add(gridbox, EVAS_CALLBACK_DEL, _deleted_cb, NULL);

	evas_object_show(gridbox);

	return gridbox;
}

HAPI void quickpanel_settings_gridbox_remove(Evas_Object *gridbox)
{
	retif(gridbox == NULL, , "invalid parameter");

	quickpanel_settings_gridbox_item_remove_all(gridbox);
	evas_object_del(gridbox);
	gridbox = NULL;
}

HAPI void quickpanel_settings_gridbox_item_add(Evas_Object *gridbox, Evas_Object *item, const char *item_type, int is_prepend)
{
	evas_object_data_set(item, E_DATA_ITEM_TYPE, item_type);

	if (is_prepend == SETTINGS_GRIDBOX_PREPEND) {
		elm_box_pack_start(gridbox, item);
	} else {
		elm_box_pack_end(gridbox, item);
	}
}

HAPI void quickpanel_settings_gridbox_item_remove(Evas_Object *gridbox, Evas_Object *item)
{
	retif(gridbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");

	elm_box_unpack(gridbox, item);
	evas_object_del(item);
	item = NULL;
}

HAPI void quickpanel_settings_gridbox_item_remove_all(Evas_Object *gridbox)
{
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj = NULL;
	Eina_List *item_list = NULL;

	retif(gridbox == NULL, , "invalid parameter");

	item_list = elm_box_children_get(gridbox);
	retif(item_list == NULL, , "invalid parameter");

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
		if (obj != NULL) {
			quickpanel_settings_gridbox_item_remove(gridbox, obj);
		}
	}

	eina_list_free(item_list);
}

HAPI void quickpanel_settings_gridbox_rotation(Evas_Object *gridbox, int angle)
{
	const char *signal = NULL;

	retif(gridbox == NULL, , "invalid parameter");

	info_layout *info_layout_portrait = evas_object_data_get(gridbox,
			E_DATA_LAYOUT_PORTRAIT);
	info_layout *info_layout_landscape = evas_object_data_get(gridbox,
			E_DATA_LAYOUT_LANDSCAPE);

	retif(info_layout_portrait == NULL || info_layout_landscape == NULL, ,
			"gridbox is crashed");

	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj;
	Eina_List *item_list = elm_box_children_get(gridbox);
	retif(item_list == NULL, , "invalid parameter");

	if (angle == 270 || angle == 90) {
		signal = "icon.landscape";
	} else {
		signal = "icon.portrait";
	}

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
		if (obj != NULL) {
			elm_object_signal_emit(obj, signal, "quickpanl.prog");
			edje_object_message_signal_process(_EDJ(obj));
		}
	}
}

HAPI int quickpanel_settings_gridbox_item_count_get(Evas_Object *gridbox)
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

HAPI int quickpanel_settings_gridbox_item_index_get(Evas_Object *gridbox, int touch_x, int touch_y)
{
	Eina_List *l;
	Eina_List *l_next;
	Eina_List *list_pos_info = NULL;
	info_position *pos_info = NULL;
	int x = 0, y = 0, w = 0, h = 0;

	evas_object_geometry_get(gridbox, &x, &y, &w, &h);
	list_pos_info = _position_info_get(gridbox);

	if (touch_x >= x && touch_x <= x + w && touch_y >= y && touch_y <= y + h) {
		touch_x = touch_x - x;
		touch_y = touch_y - y;
		EINA_LIST_FOREACH_SAFE(list_pos_info, l, l_next, pos_info) {
			if (pos_info != NULL) {
				if (touch_x >= pos_info->offset_x && touch_x <= pos_info->offset_x + pos_info->width
						&& touch_y >= pos_info->offset_y && touch_y <= pos_info->offset_y + pos_info->height) {
					return pos_info->index;
				}
			}
		}
	}
	return -1;
}

HAPI void quickpanel_settings_gridbox_unpack_all(Evas_Object *gridbox)
{
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *node = NULL;
	Eina_List *list = NULL;
	retif(gridbox == NULL, , "invalid parameter");

	list = elm_box_children_get(gridbox);
	retif(list == NULL, , "empty list");

	elm_box_unpack_all(gridbox);

	EINA_LIST_FOREACH_SAFE(list, l, l_next, node) {
		if (node != NULL) {
			if (_item_is_icon(node) == 0) {
				evas_object_del(node);
				node = NULL;
			}
		}
	}

	eina_list_free(list);
}
