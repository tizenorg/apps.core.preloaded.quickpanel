/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.tizenopensource.org/license
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

struct _qp_item_data {
	qp_item_type_e type;
	void *data;
};

static qp_item_count g_qp_item_count = {
	.group = 0,
	.ongoing = 0,
	.noti = 0,
	.minicontrol = 0,
};

qp_item_data *quickpanel_list_util_item_new(qp_item_type_e type, void *data)
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

void *quickpanel_list_util_item_get_data(qp_item_data *qid)
{
	void *user_data = NULL;

	if (!qid)
		return NULL;

	user_data = qid->data;

	return user_data;
}

int quickpanel_list_util_item_compare(const void *data1, const void *data2)
{
	int diff = 0;
	qp_item_data *qid1 = NULL;
	qp_item_data *qid2 = NULL;
	const Elm_Object_Item *it1 = data1;
	const Elm_Object_Item *it2 = data2;

	if (!it1) {
		INFO("it1 is NULL");
		return -1;
	}

	if (!it2) {
		INFO("it2 is NULL");
		return 1;
	}

	qid1 = elm_object_item_data_get(it1);
	qid2 = elm_object_item_data_get(it2);

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

void quickpanel_list_util_item_del_by_type(Evas_Object *list,
				const Elm_Object_Item *refer_item,
				qp_item_type_e type)
{
	const Elm_Object_Item *start_item = NULL;

	if (!list)
		return;

	if (refer_item)
		start_item = refer_item;
	else
		start_item = elm_genlist_first_item_get(list);

	while (start_item) {
		qp_item_data *qid = NULL;
		const Elm_Object_Item *next = NULL;

		next = elm_genlist_item_next_get(start_item);
		qid = elm_object_item_data_get(start_item);
		if (!qid) {
			ERR("fail to get qid, continue loop");
			continue;
		}

		if (qid->type > type)
			break;
		else if (qid->type == type)
			elm_object_item_del((Elm_Object_Item *)start_item);

		start_item = next;
	}

	return;
}

void quickpanel_list_util_item_update_by_type(Evas_Object *list,
				Elm_Object_Item *refer_item,
				qp_item_type_e type)
{
	Elm_Object_Item *start_item = NULL;

	if (!list)
		return;

	if (refer_item)
		start_item = refer_item;
	else
		start_item = elm_genlist_first_item_get(list);

	while (start_item) {
		qp_item_data *qid = NULL;
		Elm_Object_Item *next = NULL;

		next = elm_genlist_item_next_get(start_item);
		qid = elm_object_item_data_get(start_item);
		if (!qid) {
			ERR("fail to get qid, continue loop");
			continue;
		}

		if (qid->type > type)
			break;
		else if (qid->type == type) {
			elm_genlist_item_fields_update(start_item, "*", ELM_GENLIST_ITEM_FIELD_ALL);
		}

		start_item = next;
	}

	return;
}

Elm_Object_Item *quickpanel_list_util_find_item_by_type(Evas_Object *list,
				void *data,
				Elm_Object_Item *refer_item,
				qp_item_type_e type)
{
	Elm_Object_Item *start_item = NULL;
	Elm_Object_Item *found = NULL;

	if (!list)
		return NULL;

	if (refer_item)
		start_item = refer_item;
	else
		start_item = elm_genlist_first_item_get(list);

	while (start_item) {
		qp_item_data *qid = NULL;
		Elm_Object_Item *next = NULL;
		void *user_data = NULL;

		next = elm_genlist_item_next_get(start_item);
		qid = elm_object_item_data_get(start_item);
		if (!qid) {
			ERR("fail to get qid, continue loop");
			continue;
		}

		if (qid->type > type)
			break;
		else if (qid->type == type) {
			user_data = quickpanel_list_util_item_get_data(qid);
			if (user_data == data) {
				found = start_item;
				break;
			}
		}

		start_item = next;
	}

	return found;
}

static int __item_compare(const void *data1, const void *data2)
{
	int diff = 0;
	const Elm_Object_Item *it1 = data1;
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

	qid1 = elm_object_item_data_get(it1);

	if (!qid1) {
		INFO("qid1 is NULL");
		return -1;
	}

	diff = qid1->type - qid2->type;

	return diff;
}


Elm_Object_Item *quickpanel_list_util_sort_insert(Evas_Object *list,
					const Elm_Genlist_Item_Class *itc,
					const void *item_data,
					Elm_Object_Item *parent,
					Elm_Genlist_Item_Type type,
					Evas_Smart_Cb func,
					const void *func_data)
{
	Elm_Object_Item *it = NULL;
	Elm_Object_Item *first = NULL;

	retif(!list, NULL, "list is NULL");
	retif(!itc, NULL, "itc is NULL");
	retif(!item_data, NULL, "item_data is NULL");

	first = elm_genlist_first_item_get(list);
	while (first) {
		if (__item_compare(first, item_data) >= 0)
			break;

		first = elm_genlist_item_next_get(first);
	}

	if (!first)
		it = elm_genlist_item_append(list, itc, item_data, parent,
				type, func, func_data);
	else
		it = elm_genlist_item_insert_before(list, itc, item_data,
				parent, first, type, func, func_data);

	if (it != NULL) {
		quickpanel_list_util_add_count((qp_item_data *)item_data);
	}

	return it;
}

qp_item_count *quickpanel_list_util_get_item_count(void)
{
	return &g_qp_item_count;
}

void quickpanel_list_util_add_count(qp_item_data *qid)
{
	retif(qid == NULL, , "qid is NULL");

	switch(qid->type)
	{
		case  QP_ITEM_TYPE_ONGOING_NOTI:
			g_qp_item_count.ongoing++;
			break;
		case  QP_ITEM_TYPE_NOTI_GROUP:
			g_qp_item_count.group++;
			break;
		case  QP_ITEM_TYPE_NOTI:
			g_qp_item_count.noti++;
			break;
		case  QP_ITEM_TYPE_MINICTRL_TOP:
		case  QP_ITEM_TYPE_MINICTRL_MIDDLE:
		case  QP_ITEM_TYPE_MINICTRL_LOW:
			g_qp_item_count.minicontrol++;
			break;
	}

	DBG("(type:%d)\nnum_ongoing:%d, num_group:%d, num_noti:%d, num_minicontrol:%d"
			, qid->type
			, g_qp_item_count.ongoing
			, g_qp_item_count.group
			, g_qp_item_count.noti
			, g_qp_item_count.minicontrol);
}

void quickpanel_list_util_del_count(qp_item_data *qid)
{
	retif(qid == NULL, , "qid is NULL");

	quickpanel_list_util_del_count_by_itemtype(qid->type);
}

void quickpanel_list_util_del_count_by_itemtype(qp_item_type_e type)
{
	switch(type)
	{
		case  QP_ITEM_TYPE_ONGOING_NOTI:
			g_qp_item_count.ongoing = (g_qp_item_count.ongoing <= 0) ? 0 : g_qp_item_count.ongoing - 1;
			break;
		case  QP_ITEM_TYPE_NOTI_GROUP:
			g_qp_item_count.group = (g_qp_item_count.group <= 0) ? 0 : g_qp_item_count.group - 1;
			break;
		case  QP_ITEM_TYPE_NOTI:
			g_qp_item_count.noti = (g_qp_item_count.noti <= 0) ? 0 : g_qp_item_count.noti - 1;
			break;
		case  QP_ITEM_TYPE_MINICTRL_TOP:
		case  QP_ITEM_TYPE_MINICTRL_MIDDLE:
		case  QP_ITEM_TYPE_MINICTRL_LOW:
			g_qp_item_count.minicontrol = (g_qp_item_count.minicontrol <= 0) ? 0 : g_qp_item_count.minicontrol - 1;
			break;
	}

	DBG("(type:%d)\nnum_ongoing:%d, num_group:%d, num_noti:%d, num_minicontrol:%d"
			, type
			, g_qp_item_count.ongoing
			, g_qp_item_count.group
			, g_qp_item_count.noti
			, g_qp_item_count.minicontrol);
}
