/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
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

#include <Elementary.h>
#include <stdlib.h>

#include "common.h"
#include "list_util.h"

#define E_DATA_ITEM_LABEL_H "QP_ITEM_DATA"

struct _qp_item_data {
	qp_item_type_e type;
	void *data;
};

HAPI qp_item_data *quickpanel_list_util_item_new(qp_item_type_e type, void *data)
{
	qp_item_data *qid = NULL;

	qid = malloc(sizeof(struct _qp_item_data));
	if (!qid) {
		ERR("fail to alloc qid");
		return NULL;
	}

	qid->type = type;
	qid->data = data;

	return qid;
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

	if (!qid)
		return NULL;

	user_data = qid->data;

	return user_data;
}

HAPI void quickpanel_list_util_item_set_data(qp_item_data *qid, void *data)
{
	if (!qid)
		return ;

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
	if (qid1->type == qid2->type)
		return 1;

	diff = qid1->type - qid2->type;
	return diff;
}

HAPI void quickpanel_list_util_item_unpack_by_type(Evas_Object *list
		, qp_item_type_e type)
{
	retif(list == NULL, , "invalid parameter");

	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj;
	Eina_List *item_list = elm_box_children_get(list);

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj)
	{
		if (obj != NULL) {
			// call deleted callback
			qp_item_data *qid = evas_object_data_get(obj, E_DATA_ITEM_LABEL_H);
			if (qid != NULL) {
				if (qid->type == type) {
					elm_box_unpack(list, obj);
				}
			}
		}
	}
}

HAPI void quickpanel_list_util_item_unpack_by_object(Evas_Object *list
		, Evas_Object *item)
{
	retif(list == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");

	elm_box_unpack(list, item);
}

static int __item_compare(const void *data1, const void *data2)
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


HAPI void quickpanel_list_util_sort_insert(Evas_Object *list,
					Evas_Object *new_obj)
{
	retif(list == NULL, , "invalid parameter");
	retif(new_obj == NULL, , "invalid parameter");

	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj = NULL;
	Evas_Object *first = NULL;
	Eina_List *item_list = elm_box_children_get(list);
	qp_item_data *item_data = NULL;

	item_data = evas_object_data_get(new_obj, E_DATA_ITEM_LABEL_H);
	retif(item_data == NULL, , "invalid parameter");

	INFO("current entry count in list:%d", eina_list_count(item_list));
	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj)
	{
		if (obj != NULL) {
			if (__item_compare(obj, item_data) >= 0)
				break;
		}

		first = obj;
	}

	if (first == NULL) {
		elm_box_pack_start(list, new_obj);
	} else {
		elm_box_pack_after(list, new_obj, first);
	}
}
