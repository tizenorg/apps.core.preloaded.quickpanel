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


#ifndef __QUICKPANEL_GRIDBOX_H__
#define __QUICKPANEL_GRIDBOX_H__

#define GRIDBOX_PREPEND 1
#define GRIDBOX_APPEND 0

Evas_Object *quickpanel_noti_gridbox_create(Evas_Object *parent, void *data);
void quickpanel_noti_gridbox_remove(Evas_Object *gridbox);
void quickpanel_noti_gridbox_add_item(Evas_Object *gridbox, Evas_Object *item, int is_prepend);
void quickpanel_noti_gridbox_remove_item(Evas_Object *gridbox, Evas_Object *item, int with_animation);
void quickpanel_noti_gridbox_rotation(Evas_Object *gridbox, int angle);
void quickpanel_noti_gridbox_remove_and_add_item(Evas_Object *gridbox, Evas_Object *item
		,void (*update_cb)(Evas_Object *list, void *data, int is_prepend)
		,void *container, void *data, int pos);
void quickpanel_noti_gridbox_remove_all_item(Evas_Object *gridbox, int with_animation);
void quickpanel_noti_gridbox_set_item_deleted_cb(Evas_Object *gridbox,
		void(*deleted_cb)(void *data, Evas_Object *obj));
int quickpanel_noti_gridbox_get_item_count(Evas_Object *gridbox);
int quickpanel_noti_gridbox_get_geometry(Evas_Object *gridbox,
		int *limit_h, int *limit_partial_h, int *limit_partial_w);
void quickpanel_noti_gridbox_update_item(Evas_Object *gridbox, Evas_Object *item);
void quickpanel_noti_gridbox_closing_trigger_set(Evas_Object *gridbox);
int quickpanel_noti_gridbox_get_item_exist(Evas_Object *gridbox, Evas_Object *box);
#endif
