/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __QUICKPANEL_NOTI_BOX_H__
#define __QUICKPANEL_NOTI_BOX_H__

#include <notification.h>

#define STATE_NORMAL 1
#define STATE_DELETING 0

#define E_DATA_NOTI_BOX_H "noti_box"
#define E_DATA_CB_SELECTED_ITEM "noti_box_cb_selected"
#define E_DATA_CB_BUTTON_1 "noti_box_cb_button_1"
#define E_DATA_CB_DELETED_ITEM "noti_box_cb_deleted"

typedef struct _noti_box_h {
	int status;
	void *data;
	notification_ly_type_e  layout;
} noti_box_h;

Evas_Object *noti_box_create(Evas_Object *parent, notification_ly_type_e layout);
void noti_box_node_set(Evas_Object *noti_box, void *data);
void *noti_box_node_get(Evas_Object *noti_box);
void noti_box_remove(Evas_Object *noti_box);
void noti_box_set_item_selected_cb(Evas_Object *noti_box,
		void(*selected_cb)(void *data, Evas_Object *obj));
void noti_box_set_item_button_1_cb(Evas_Object *noti_box,
		void(*button_1_cb)(void *data, Evas_Object *obj));
void noti_box_set_item_deleted_cb(Evas_Object *noti_box,
		void(*deleted_cb)(void *data, Evas_Object *obj));
int noti_box_get_status(Evas_Object *noti_box);
void noti_box_set_status(Evas_Object *noti_box, int status);

#endif
