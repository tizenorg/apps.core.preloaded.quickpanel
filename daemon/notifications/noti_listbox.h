/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the License);
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

#ifndef __QUICKPANEL_NOTI_LISTBOX_H__
#define __QUICKPANEL_NOTI_LISTBOX_H__

#define LISTBOX_PREPEND 1
#define LISTBOX_APPEND 0

Evas_Object *listbox_create(Evas_Object *parent, void *data);
void listbox_remove(Evas_Object *listbox);
void listbox_add_item(Evas_Object *listbox, Evas_Object *item, int is_prepend);
void listbox_remove_item(Evas_Object *listbox, Evas_Object *item, int with_animation);
void listbox_rotation(Evas_Object *listbox, int angle);
void listbox_remove_and_add_item(Evas_Object *listbox, Evas_Object *item
		,void (*update_cb)(Evas_Object *list, void *data, int is_prepend)
		,void *container, void *data, int pos);
void listbox_remove_all_item(Evas_Object *listbox, int with_animation);
void listbox_set_item_deleted_cb(Evas_Object *listbox,
		void(*deleted_cb)(void *data, Evas_Object *obj));
void listbox_update(Evas_Object *listbox);
void listbox_update_item(Evas_Object *listbox, Evas_Object *item);
#endif
