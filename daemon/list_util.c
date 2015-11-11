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
#include <stdlib.h>

#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "list_util.h"
#include "vi_manager.h"

#define E_DATA_ITEM_LABEL_H "QP_ITEM_DATA"

struct _qp_item_data {
	qp_item_type_e type;
	void *data;
};

static Eina_Bool _anim_init_cb(void *data);
static Eina_Bool _anim_job_cb(void *data);
static Eina_Bool _anim_done_cb(void *data);

static void _viewer_freeze(Evas_Object *viewer)
{
	int freezed_count = 0;
	retif(viewer == NULL, , "Invalid parameter!");

	freezed_count = elm_object_scroll_freeze_get(viewer);

	if (freezed_count <= 0) {
		elm_object_scroll_freeze_push(viewer);
	}
}

static void _viewer_unfreeze(Evas_Object *viewer)
{
	int i = 0, freezed_count = 0;
	retif(viewer == NULL, , "Invalid parameter!");

	freezed_count = elm_object_scroll_freeze_get(viewer);

	for (i = 0 ; i < freezed_count; i++) {
		elm_object_scroll_freeze_pop(viewer);
	}
}

HAPI qp_item_data *quickpanel_list_util_item_new(qp_item_type_e type, void *data)
{
	qp_item_data *qid;

	qid = malloc(sizeof(*qid));
	if (!qid) {
		ERR("fail to alloc qid");
		return NULL;
	}

	qid->type = type;
	qid->data = data;

	return qid;
}

HAPI void quickpanel_list_util_item_del(qp_item_data *qid)
{
	free(qid);
}

HAPI void quickpanel_list_util_item_set_tag(Evas_Object *item, qp_item_data *qid)
{
	retif(item == NULL, , "invalid parameter");
	retif(qid == NULL, , "invalid parameter");

	evas_object_data_set(item, E_DATA_ITEM_LABEL_H, qid);
}

HAPI void quickpanel_list_util_item_del_tag(Evas_Object *item)
{
	retif(item == NULL, , "invalid parameter");

	qp_item_data *qid = evas_object_data_get(item, E_DATA_ITEM_LABEL_H);

	if (qid != NULL) {
		evas_object_data_del(item, E_DATA_ITEM_LABEL_H);
		free(qid);
	}
}

HAPI void *quickpanel_list_util_item_get_data(qp_item_data *qid)
{
	void *user_data = NULL;

	if (!qid) {
		return NULL;
	}

	user_data = qid->data;

	return user_data;
}

HAPI void quickpanel_list_util_item_set_data(qp_item_data *qid, void *data)
{
	if (!qid) {
		return;
	}	

	qid->data = data;
}

HAPI int quickpanel_list_util_item_compare(const void *data1, const void *data2)
{
	int diff = 0;
	qp_item_data *qid1 = NULL;
	qp_item_data *qid2 = NULL;
	const Evas_Object *eo1 = data1;
	const Evas_Object *eo2 = data2;

	if (!eo1) {
		INFO("eo1 is NULL");
		return -1;
	}

	if (!eo2) {
		INFO("eo2 is NULL");
		return 1;
	}

	qid1 = evas_object_data_get(eo1, E_DATA_ITEM_LABEL_H);
	qid2 = evas_object_data_get(eo2, E_DATA_ITEM_LABEL_H);

	if (!qid1) {
		INFO("qid1 is NULL");
		return -1;
	}

	if (!qid2) {
		INFO("qid2 is NULL");
		return 1;
	}

	/* elm_genlist sort is not working as i expected */
	if (qid1->type == qid2->type) {
		return 1;
	}


	diff = qid1->type - qid2->type;
	return diff;
}

static qp_item_type_e _get_item_type(qp_item_data *item_data)
{
	retif(item_data == NULL, QP_ITEM_TYPE_NONE, "a invalid data");

	return item_data->type;
}

static int _item_compare(const void *data1, const void *data2)
{
	int diff = 0;
	const Evas_Object *eo1 = data1;
	const qp_item_data *qid1 = NULL;
	const qp_item_data *qid2 = data2;

	if (!data1) {
		INFO("data1 is NULL");
		return -1;
	}

	if (!data2) {
		INFO("data2 is NULL");
		return 1;
	}

	qid1 = evas_object_data_get(eo1, E_DATA_ITEM_LABEL_H);

	if (!qid1) {
		INFO("qid1 is NULL");
		return -1;
	}

	diff = qid1->type - qid2->type;

	return diff;
}

static void _list_util_layout_get_coord(Evas_Object *container, Evas_Object *first, int *coord_x, int *coord_y)
{
	int x = 0;
	int y = 0;
	int h = 0;
	int off_y = 0;
	qp_item_type_e item_type = QP_ITEM_TYPE_NONE;
	struct appdata *ad = quickpanel_get_app_data();

	retif(container == NULL, , "invalid parameter");
	retif(ad == NULL, , "invalid data.");

	Eina_List *list_tmp = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;
	Evas_Object *obj = NULL;
	Eina_List *item_list = elm_box_children_get(container);

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
		if (obj != NULL) {
			item_type = quickpanel_list_util_item_type_get(obj);
			if (item_type == QP_ITEM_TYPE_ONGOING_NOTI
					|| item_type == QP_ITEM_TYPE_NOTI) {
				list_tmp = elm_box_children_get(obj);
				if (list_tmp != NULL) {
					if (eina_list_count(list_tmp) != 0 ) {
						evas_object_geometry_get(obj, NULL, NULL, NULL, &h);
					}
					eina_list_free(list_tmp);
				}
			} else {
				evas_object_geometry_get(obj, NULL, NULL, NULL, &h);
			}

			off_y += h;
			h = 0;
			if (obj == first) {
				break;
			}
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

	if (item_list != NULL) {
		eina_list_free(item_list);
	}
}

Evas_Object *_list_util_get_first(Evas_Object *list, Evas_Object *new_obj)
{
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj = NULL;
	Evas_Object *first = NULL;
	Eina_List *item_list = elm_box_children_get(list);

	qp_item_data *item_data = NULL;

	retif(list == NULL, NULL, "invalid parameter");
	retif(new_obj == NULL, NULL, "invalid parameter");

	item_data = evas_object_data_get(new_obj, E_DATA_ITEM_LABEL_H);
	retif(item_data == NULL, NULL, "invalid parameter");

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
		if (obj != NULL) {
			if (_item_compare(obj, item_data) > 0) {
				break;
			}
		}

		first = obj;
	}

	if (item_list != NULL) {
		eina_list_free(item_list);
	}

	return first;
}

HAPI qp_item_type_e quickpanel_list_util_item_type_get(Evas_Object *item)
{
	qp_item_data *qid = NULL;
	retif(item == NULL, QP_ITEM_TYPE_NONE, "invalid parameter");

	qid = evas_object_data_get(item, E_DATA_ITEM_LABEL_H);
	if (qid != NULL) {
		return _get_item_type(qid);
	}

	return QP_ITEM_TYPE_NONE;
}

HAPI void quickpanel_list_util_item_unpack_by_object(Evas_Object *list , Evas_Object *item, int is_unpack_only, int is_hide)
{
	QP_VI *vi = NULL;
	qp_item_data *qid = NULL;
	retif(list == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");

	qid = evas_object_data_get(item, E_DATA_ITEM_LABEL_H);
	vi = quickpanel_vi_new_with_data(
			VI_OP_DELETE,
			_get_item_type(qid),
			list,
			item,
			_anim_init_cb,
			_anim_job_cb,
			_anim_done_cb,
			_anim_done_cb,
			vi,
			NULL,
			is_unpack_only,
			is_hide);
	quickpanel_vi_start(vi);
}

HAPI void quickpanel_list_util_sort_insert(Evas_Object *list, Evas_Object *new_obj)
{

	QP_VI *vi = NULL;
	qp_item_data *qid = NULL;
	retif(list == NULL, , "invalid parameter");
	retif(new_obj == NULL, , "invalid parameter");

	qid = evas_object_data_get(new_obj, E_DATA_ITEM_LABEL_H);
	vi = quickpanel_vi_new_with_data(
			VI_OP_INSERT,
			_get_item_type(qid),
			list,
			new_obj,
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

HAPI Elm_Transit *quickpanel_list_util_get_reorder_transit(Evas_Object *item, Elm_Transit *transit, int distance)
{
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj = NULL;
	Eina_List *item_list = NULL;
	int is_start_relayout = 0;
	Elm_Transit *transit_layout = NULL;
	Evas_Object *container = NULL;

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, NULL, "invalid parameter");

	container = ad->list;
	retif(container == NULL, NULL, "invalid parameter");
	retif(item == NULL, NULL, "invalid parameter");

	item_list = elm_box_children_get(container);

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
		if (obj == item) {
			is_start_relayout = 1;
		} else if (obj != NULL && is_start_relayout == 1) {
			transit_layout = elm_transit_add();
			if (transit_layout != NULL) {
				elm_transit_effect_translation_add(transit_layout, 0, 0, 0, distance);
				elm_transit_object_add(transit_layout, obj);
				elm_transit_duration_set(transit_layout,
						quickpanel_vim_get_duration(VI_OP_REORDER));
				elm_transit_tween_mode_set(transit_layout,
						quickpanel_vim_get_tweenmode(VI_OP_REORDER));
				elm_transit_objects_final_state_keep_set(transit_layout, EINA_TRUE);
				elm_transit_event_enabled_set(transit_layout, EINA_TRUE);
				if (transit != NULL) {
					elm_transit_chain_transit_add(transit, transit_layout);
				} else {
					elm_transit_go(transit_layout);
				}
			}
		}
	}

	if (item_list != NULL) {
		eina_list_free(item_list);
	}

	return transit_layout;
}

static void _anim_init_insert(void *data)
{
	QP_VI *vi = data;
	int coord_x = 0, coord_y = 0;
	retif(vi == NULL, , "invalid parameter");

	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	Evas_Object *container = vi->container;
	Evas_Object *item = vi->target;
	Evas_Object *first = NULL;

	evas_object_clip_set(item, evas_object_clip_get(container));
	evas_object_color_set(item, 0, 0, 0, 0);

	first = _list_util_get_first(container, item);
	_list_util_layout_get_coord(container, first, &coord_x, &coord_y);
	evas_object_move(item, coord_x, coord_y);
}

static void _anim_job_insert(void *data)
{

	QP_VI *vi = data;
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj = NULL;
	Eina_List *item_list = NULL;
	int is_start_relayout = 0;
	int item_width, item_height = 0;
	int coord_x = 0, coord_y = 0;
	Elm_Transit *transit_layout = NULL;
	Elm_Transit *transit_fadein = NULL;
	Evas_Object *container = NULL;
	Evas_Object *item = NULL;
	Evas_Object *first = NULL;

	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	container = vi->container;
	item = vi->target;
	item_list = elm_box_children_get(container);

	first = _list_util_get_first(container, item);
	_list_util_layout_get_coord(container, first, &coord_x, &coord_y);
	evas_object_move(item, coord_x, coord_y);
	is_start_relayout = (first == NULL) ? 1 : 0;

	evas_object_geometry_get(item, NULL, NULL, &item_width, &item_height);
	if (item_width == 0 && item_height == 0) {
		ERR("failed to get a size of item %d %d", item_width, item_height);
		evas_object_size_hint_min_get (item, &item_width, &item_height);
	}

	if (vi->item_type == QP_ITEM_TYPE_ONGOING_NOTI) {
		if (item_list != NULL) {
			eina_list_free(item_list);
		}
		return;
	}

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
		if (obj == first) {
			is_start_relayout = 1;
		} else if (obj != NULL && is_start_relayout == 1) {
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
				ERR("Failed to create a transit");
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
			elm_transit_go(transit_fadein);
		}
	} else {
		if (transit_layout != NULL) elm_transit_del(transit_layout);
		quickpanel_vi_done(vi);
		ERR("Failed to create all the transit");
	}
}

static void _anim_done_insert(void *data)
{
	QP_VI *vi = data;
	retif(data == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	Evas_Object *container = vi->container;
	Evas_Object *item = vi->target;
	Evas_Object *first = _list_util_get_first(container, item);

	evas_object_color_set(item, 255, 255, 255, 255);

	if (first == NULL) {
		elm_box_pack_start(container, item);
	} else {
		elm_box_pack_after(container, item, first);
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
		elm_transit_tween_mode_set(transit_fadeout,
				quickpanel_vim_get_tweenmode(VI_OP_DELETE));
		elm_transit_duration_set(transit_fadeout,
				quickpanel_vim_get_duration(VI_OP_DELETE));
		elm_transit_go(transit_fadeout);

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
					elm_transit_chain_transit_add(transit_fadeout, transit_layout);
				} else {
					ERR("Failed to create a transit");
				}
			}
		}
	}

	if (item_list != NULL) {
		eina_list_free(item_list);
	}

	if (transit_layout != NULL) {
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
	QP_VI *vi = data;

	retif(vi == NULL, , "invalid parameter");
	retif(vi->container == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	Evas_Object *container = vi->container;
	Evas_Object *item = vi->target;

	elm_box_unpack(container, item);

	if (vi->extra_flag_2 == 1) {
		evas_object_move(item, -10000, -10000);
		quickpanel_vi_object_event_freeze_set(item, EINA_FALSE);
	}
	if (vi->extra_flag_1 == 0 && item != NULL) {
		evas_object_del(item);
		item = NULL;
	}
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

HAPI void quickpanel_list_util_scroll_freeze_set(Eina_Bool is_freeze)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid data.");
	retif(ad->scroller == NULL, , "invalid data.");

	if (is_freeze == EINA_TRUE) {
		_viewer_freeze(ad->scroller);
	} else {
		_viewer_unfreeze(ad->scroller);
	}
}
