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

#ifndef __QUICKPANEL_GRIDBOX_H__
#define __QUICKPANEL_GRIDBOX_H__

#define GRIDBOX_PREPEND 1
#define GRIDBOX_APPEND 0

Evas_Object *gridbox_create(Evas_Object *parent, void *data);
void gridbox_remove(Evas_Object *gridbox);
void gridbox_add_item(Evas_Object *gridbox, Evas_Object *item, int is_prepend);
void gridbox_remove_item(Evas_Object *gridbox, Evas_Object *item, int with_animation);
void gridbox_rotation(Evas_Object *gridbox, int angle);
void gridbox_remove_and_add_item(Evas_Object *gridbox, Evas_Object *item
		,void (*update_cb)(Evas_Object *list, void *data, int is_prepend)
		,void *container, void *data, int pos);
void gridbox_remove_all_item(Evas_Object *gridbox, int with_animation);
void gridbox_set_item_deleted_cb(Evas_Object *gridbox,
		void(*deleted_cb)(void *data, Evas_Object *obj));
#endif
