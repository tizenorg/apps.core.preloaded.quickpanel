/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _QP_LIST_UTIL_DEF_
#define _QP_LIST_UTIL_DEF_

#include <Elementary.h>

typedef enum {
	QP_ITEM_TYPE_NONE = -1,
	QP_ITEM_TYPE_SETTING = 0,
	QP_ITEM_TYPE_TOGGLE,
	QP_ITEM_TYPE_BRIGHTNESS,
	QP_ITEM_TYPE_ONGOING_NOTI,
	QP_ITEM_TYPE_MINICTRL_ONGOING,
	QP_ITEM_TYPE_MINICTRL_TOP,
	QP_ITEM_TYPE_MINICTRL_MIDDLE,
	QP_ITEM_TYPE_MINICTRL_LOW,
	QP_ITEM_TYPE_NOTI_GROUP,
	QP_ITEM_TYPE_NOTI,
	QP_ITEM_TYPE_MAX,
} qp_item_type_e;

typedef struct _qp_item_data qp_item_data;
typedef struct _qp_item_count {
	int group;
	int ongoing;
	int noti;
	int minicontrol;
} qp_item_count;


qp_item_data *quickpanel_list_util_item_new(qp_item_type_e type, void *data);

qp_item_type_e quickpanel_list_util_item_get_item_type(qp_item_data *qid);
void *quickpanel_list_util_item_get_data(qp_item_data *qid);
void quickpanel_list_util_item_set_data(qp_item_data *qid, void *data);

int quickpanel_list_util_item_compare(const void *data1, const void *data2);

void quickpanel_list_util_item_del_by_type(Evas_Object *list,
				const Elm_Object_Item *refer_item,
				qp_item_type_e type);

void quickpanel_list_util_item_update_by_type(Evas_Object *list,
				Elm_Object_Item *refer_item,
				qp_item_type_e type);

Elm_Object_Item *quickpanel_list_util_find_item_by_type(Evas_Object *list,
					void *data,
					Elm_Object_Item *refer_item,
					qp_item_type_e type);

Elm_Object_Item *quickpanel_list_util_sort_insert(Evas_Object *list,
					const Elm_Genlist_Item_Class *itc,
					const void *item_data,
					Elm_Object_Item *parent,
					Elm_Genlist_Item_Type type,
					Evas_Smart_Cb func,
					const void *func_data);

qp_item_count *quickpanel_list_util_get_item_count(void);
void quickpanel_list_util_add_count(qp_item_data *qid);
void quickpanel_list_util_del_count(qp_item_data *qid);
void quickpanel_list_util_del_count_by_itemtype(qp_item_type_e type);

#endif /* _QP_LIST_UTIL_DEF_ */

