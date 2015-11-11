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


#ifndef _QP_LIST_UTIL_DEF_
#define _QP_LIST_UTIL_DEF_

typedef enum {
	QP_ITEM_TYPE_NONE = -1,
	QP_ITEM_TYPE_SETTING = 0,
	QP_ITEM_TYPE_BRIGHTNESS,
	QP_ITEM_TYPE_VOICE_CONTOL,
	QP_ITEM_TYPE_FACTORY,
	QP_ITEM_TYPE_MULTIWINDOW,
	QP_ITEM_TYPE_EARJACK,
	QP_ITEM_TYPE_MINICTRL_ONGOING,
	QP_ITEM_TYPE_MINICTRL_MIDDLE,
	QP_ITEM_TYPE_ONGOING_NOTI_GROUP,
	QP_ITEM_TYPE_ONGOING_NOTI,
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

extern qp_item_data *quickpanel_list_util_item_new(qp_item_type_e type, void *data);
extern void quickpanel_list_util_item_del(qp_item_data *qid);
extern void quickpanel_list_util_item_set_tag(Evas_Object *item, qp_item_data *qid);
extern void quickpanel_list_util_item_del_tag(Evas_Object *item);

extern void *quickpanel_list_util_item_get_data(qp_item_data *qid);
extern void quickpanel_list_util_item_set_data(qp_item_data *qid, void *data);
extern int quickpanel_list_util_item_compare(const void *data1, const void *data2);
extern qp_item_type_e quickpanel_list_util_item_type_get(Evas_Object *item);

extern void quickpanel_list_util_item_unpack_by_object(Evas_Object *list , Evas_Object *item, int is_unpack_only, int is_hide);
extern void quickpanel_list_util_sort_insert(Evas_Object *list, Evas_Object *new_obj);

extern Elm_Transit *quickpanel_list_util_get_reorder_transit(Evas_Object *item, Elm_Transit *transit, int distance);
extern void quickpanel_list_util_scroll_freeze_set(Eina_Bool is_freeze);

#endif /* _QP_LIST_UTIL_DEF_ */

