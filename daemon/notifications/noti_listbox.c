/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the License);
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
#include "noti_listbox.h"
#include "noti_list_item.h"

#define E_DATA_LAYOUT_PORTRAIT "layout_portrait"
#define E_DATA_LAYOUT_LANDSCAPE "layout_landscape"
#define E_DATA_CB_DELETE_ITEM "cb_delete_item"
#define E_DATA_CB_REMOVED "cb_removed"
#define E_DATA_APP_DATA "app_data"

typedef struct _listbox_info_layout {
	int n_per_rows;
	int padding_top;
	int padding_left;
	int padding_right;
	int padding_bottom;
	int padding_between;
	int child_w;
	int child_h;
	double scale;
} listbox_info_layout;

typedef struct _listbox_info_animation {
	Evas_Object *listbox;
	Evas_Object *item;

	void (*update_cb)(Evas_Object *list, void *data, int is_prepend);
	Evas_Object *container;
	void *noti;
	int pos;
} listbox_info_animation;

Evas_Object *listbox_create(Evas_Object *parent, void *data) {
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

	qp_item_data *qid
		= quickpanel_list_util_item_new(QP_ITEM_TYPE_ONGOING_NOTI, NULL);
	quickpanel_list_util_item_set_tag(listbox, qid);

	return listbox;
}

void listbox_remove(Evas_Object *listbox) {

	retif(listbox == NULL, , "invalid parameter");

	listbox_remove_all_item(listbox, 0);
	evas_object_data_del(listbox, E_DATA_CB_DELETE_ITEM);
	evas_object_data_del(listbox, E_DATA_APP_DATA);
	quickpanel_list_util_item_del_tag(listbox);
	evas_object_del(listbox);
}

void listbox_set_item_deleted_cb(Evas_Object *listbox,
		void(*deleted_cb)(void *data, Evas_Object *obj)) {
	retif(listbox == NULL, , "invalid parameter");
	retif(deleted_cb == NULL, , "invalid parameter");

	evas_object_data_set(listbox, E_DATA_CB_DELETE_ITEM, deleted_cb);
}

static void _listbox_call_item_deleted_cb(Evas_Object *listbox, void *data,
		Evas_Object *obj) {
	retif(listbox == NULL, , "invalid parameter");

	void (*deleted_cb)(void *data, Evas_Object *obj) = NULL;

	deleted_cb = evas_object_data_get(listbox, E_DATA_CB_DELETE_ITEM);

	if (deleted_cb != NULL) {
		deleted_cb(data, obj);
	}
}

void listbox_add_item(Evas_Object *listbox, Evas_Object *item, int is_prepend) {
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

	if (is_prepend == LISTBOX_PREPEND)
		elm_box_pack_start(listbox, item);
	else
		elm_box_pack_end(listbox, item);
}

static void _listbox_remove_item_anim_cb(void *data, Elm_Transit *transit) {
	DBG("");
	retif(data == NULL, , "invalid parameter");
	retif(transit == NULL, , "invalid parameter");

	listbox_info_animation *info_animation = data;

	retif(info_animation->listbox == NULL, , "invalid parameter");
	retif(info_animation->item == NULL, , "invalid parameter");

	DBG("remove:%p", info_animation->item);
	void *node = noti_list_item_node_get(info_animation->item);
	elm_box_unpack(info_animation->listbox, info_animation->item);
	noti_list_item_remove(info_animation->item);
	_listbox_call_item_deleted_cb(info_animation->listbox,
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

void listbox_remove_item(Evas_Object *listbox, Evas_Object *item, int with_animation) {
	retif(listbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");

	if (noti_list_item_get_status(item) == STATE_DELETING) {
		return ;
	}
	noti_list_item_set_status(item, STATE_DELETING);

	DBG("remove:%p", item);

	if (with_animation == 1) {
		listbox_info_animation *info_animation = (listbox_info_animation *) malloc(
				sizeof(listbox_info_animation));
		if (info_animation == NULL)
			return;
		info_animation->listbox = listbox;
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
		elm_transit_del_cb_set(transit, _listbox_remove_item_anim_cb,
				info_animation);
		elm_transit_go(transit);
	} else {
		DBG("%p", item);
		void *node = noti_list_item_node_get(item);
		elm_box_unpack(listbox, item);
		noti_list_item_remove(item);
		_listbox_call_item_deleted_cb(listbox,
				node, NULL);
	}
}

void listbox_remove_all_item(Evas_Object *listbox, int with_animation) {
	DBG("");
	retif(listbox == NULL, , "invalid parameter");

	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj;
	Eina_List *item_list = elm_box_children_get(listbox);

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj)
	{
		if (obj != NULL) {
			// call deleted callback
			listbox_remove_item(listbox, obj, with_animation);
		}
	}
}

void listbox_update(Evas_Object *listbox) {
	retif(listbox == NULL, , "invalid parameter");

	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj;
	Eina_List *item_list = elm_box_children_get(listbox);

	DBG("all count:%d", eina_list_count(item_list));

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj)
	{
		if (obj != NULL) {
			noti_list_item_update(obj);
		}
	}
}

void listbox_update_item(Evas_Object *listbox, Evas_Object *item) {
	retif(listbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");

	noti_list_item_update(item);
}

void listbox_remove_and_add_item(Evas_Object *listbox, Evas_Object *item
		,void (*update_cb)(Evas_Object *list, void *data, int is_prepend)
		,void *container, void *data, int pos) {

	retif(listbox == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");
	retif(update_cb == NULL, , "invalid parameter");
	retif(container == NULL, , "invalid parameter");
	retif(data == NULL, , "invalid parameter");

	if (noti_list_item_get_status(item) == STATE_DELETING) {
		return ;
	}
	noti_list_item_set_status(item, STATE_DELETING);

	listbox_info_animation *info_animation = (listbox_info_animation *) malloc(
			sizeof(listbox_info_animation));
	if (info_animation == NULL)
		return;
	info_animation->listbox = listbox;
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
	elm_transit_del_cb_set(transit, _listbox_remove_item_anim_cb,
			info_animation);
	elm_transit_go(transit);
}

static void listbox_finalize_rotation_cb(void *data) {
	retif(data == NULL, , "invalid parameter");
	Evas_Object *listbox = data;

	elm_box_recalculate(listbox);
}

void listbox_rotation(Evas_Object *listbox, int angle) {
	const char *signal = NULL;

	retif(listbox == NULL, , "invalid parameter");

	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj;
	Eina_List *item_list = elm_box_children_get(listbox);

	if (angle == 270 || angle == 90) {
		signal = "box.landscape";
	} else {
		signal = "box.portrait";
	}

	DBG("all count:%d", eina_list_count(item_list));

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj)
	{
		if (obj != NULL) {
			elm_object_signal_emit(obj, signal, "box.prog");
			edje_object_message_signal_process(_EDJ(obj));
			elm_layout_sizing_eval(obj);
			DBG("set to %s, %x", signal, obj);
		}
	}

	DBG("Angle  Rotation  is %d", angle);
}
