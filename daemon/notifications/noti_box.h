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


#ifndef __QUICKPANEL_NOTI_BOX_H__
#define __QUICKPANEL_NOTI_BOX_H__

#include <notification.h>
#include "vi_manager.h"

#define E_DATA_NOTI_BOX_H "noti_box"
#define E_DATA_CB_SELECTED_ITEM "noti_box_cb_selected"
#define E_DATA_CB_BUTTON_1 "noti_box_cb_button_1"
#define E_DATA_CB_DELETED_ITEM "noti_box_cb_deleted"
#define E_DATA_NOTI_BOX_MB_BG "noti_box_mb"

typedef enum _qp_notibox_state_type {
	NOTIBOX_STATE_NORMAL = 0,
	NOTIBOX_STATE_GETSTURE_WAIT,
	NOTIBOX_STATE_GETSTURE_CANCELED,
	NOTIBOX_STATE_DELETED,
} qp_notibox_state_type;

typedef struct _noti_box_h {
	int status;
	int priv_id;
	void *noti_node;
	notification_ly_type_e  layout;

	QP_VI *vi;
	Ecore_Animator *animator;

	int obj_w;
	int obj_h;
	int press_x;
	int press_y;
	int distance;
	int need_to_cancel_press;
	qp_notibox_state_type state;
} noti_box_h;

Evas_Object *quickpanel_noti_box_create(Evas_Object *parent, notification_ly_type_e layout);
void quickpanel_noti_box_node_set(Evas_Object *noti_box, void *noti_node);
void *quickpanel_noti_box_node_get(Evas_Object *noti_box);
void quickpanel_noti_box_remove(Evas_Object *noti_box);
void quickpanel_noti_box_set_item_selected_cb(Evas_Object *noti_box,
		void(*selected_cb)(void *data, Evas_Object *obj));
void quickpanel_noti_box_set_item_button_1_cb(Evas_Object *noti_box,
		void(*button_1_cb)(void *data, Evas_Object *obj));
void quickpanel_noti_box_set_item_deleted_cb(Evas_Object *noti_box,
		void(*deleted_cb)(void *data, Evas_Object *obj));
void quickpanel_noti_box_item_dragging_cancel(Evas_Object *noti_box);
void quickpanel_noti_box_item_update(Evas_Object *noti_box);
#endif
