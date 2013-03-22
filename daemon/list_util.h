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
	QP_ITEM_TYPE_BRIGHTNESS,
	QP_ITEM_TYPE_ONGOING_NOTI,
	QP_ITEM_TYPE_MINICTRL_ONGOING,
	QP_ITEM_TYPE_MINICTRL_TOP,
	QP_ITEM_TYPE_MINICTRL_MIDDLE,
	QP_ITEM_TYPE_MINICTRL_LOW,
	QP_ITEM_TYPE_NOTI_GROUP,
	QP_ITEM_TYPE_NOTI,
	QP_ITEM_TYPE_BAR,
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
void quickpanel_list_util_item_set_tag(Evas_Object *item, qp_item_data *qid);
void quickpanel_list_util_item_del_tag(Evas_Object *item);

void *quickpanel_list_util_item_get_data(qp_item_data *qid);
void quickpanel_list_util_item_set_data(qp_item_data *qid, void *data);
int quickpanel_list_util_item_compare(const void *data1, const void *data2);

void quickpanel_list_util_item_unpack_by_type(Evas_Object *list
		, qp_item_type_e type);
void quickpanel_list_util_item_unpack_by_object(Evas_Object *list
		, Evas_Object *item);

void quickpanel_list_util_sort_insert(Evas_Object *list,
					Evas_Object *new_obj);

#endif /* _QP_LIST_UTIL_DEF_ */

