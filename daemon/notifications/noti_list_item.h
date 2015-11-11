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


#ifndef __QUICKPANEL_NOTI_LIST_ITEM_H__
#define __QUICKPANEL_NOTI_LIST_ITEM_H__

#define STATE_NORMAL 1
#define STATE_DELETING 0

#define E_DATA_NOTI_LIST_ITEM_H "noti_list_item"

typedef Evas_Object *(*creater_cb)(notification_h, Evas_Object *);
typedef void (*action_cb)(noti_node_item *, notification_ly_type_e, Evas_Object *);
typedef void (*response_cb)(noti_node_item *, Evas_Object *);

typedef enum _qp_notilistitem_state_type {
	NOTILISTITEM_STATE_NORMAL = 0,
	NOTILISTITEM_STATE_GETSTURE_WAIT,
	NOTILISTITEM_STATE_GETSTURE_CANCELED,
	NOTILISTITEM_STATE_DELETED,
} qp_notilistitem_state_type;

typedef struct _Noti_View_H {
	char *name;

	/* func */
	creater_cb create;
	action_cb update;
	action_cb remove;
} Noti_View_H;

typedef struct _noti_list_item_h {
	int status;
	int priv_id;
	notification_ly_type_e layout;
	noti_node_item *noti_node;

	response_cb selected_cb;
	response_cb button_1_cb;
	response_cb deleted_cb;

	QP_VI *vi;
	Ecore_Animator *animator;

	int obj_w;
	int obj_h;
	int press_x;
	int press_y;
	int distance;
	int need_to_cancel_press;
	qp_notilistitem_state_type state;
} noti_list_item_h;

extern Evas_Object *quickpanel_noti_list_item_create(Evas_Object *parent, notification_h noti);
extern void quickpanel_noti_list_item_update(Evas_Object *item);
extern void quickpanel_noti_list_item_remove(Evas_Object *item);

extern void quickpanel_noti_list_item_node_set(Evas_Object *item, noti_node_item *noti_node);
extern void *quickpanel_noti_list_item_node_get(Evas_Object *item);
extern int quickpanel_noti_list_item_get_status(Evas_Object *item);
extern void quickpanel_noti_list_item_set_status(Evas_Object *item, int status);

extern void quickpanel_noti_list_item_set_item_selected_cb(Evas_Object *item, response_cb selected_cb);
extern void quickpanel_noti_list_item_set_item_button_1_cb(Evas_Object *item, response_cb callback);
extern void quickpanel_noti_list_item_set_item_deleted_cb(Evas_Object *item, response_cb callback);

extern noti_list_item_h *quickpanel_noti_list_item_handler_get(Evas_Object *item);
#endif
