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


#ifndef __QUICKPANEL_NOTI_LISTBOX_H__
#define __QUICKPANEL_NOTI_LISTBOX_H__

#define LISTBOX_INSERT_AFTER -1
#define LISTBOX_PREPEND 1
#define LISTBOX_APPEND 0

extern Evas_Object *quickpanel_noti_listbox_create(Evas_Object *parent, void *data, qp_item_type_e item_type);
extern void quickpanel_noti_listbox_remove(Evas_Object *listbox);
extern void quickpanel_noti_listbox_add_item(Evas_Object *listbox, Evas_Object *item, int is_prepend, Evas_Object *noti_section);
extern void quickpanel_noti_listbox_remove_item(Evas_Object *listbox, Evas_Object *item, int with_animation);
extern void quickpanel_noti_listbox_rotation(Evas_Object *listbox, int angle);
extern void quickpanel_noti_listbox_remove_all_item(Evas_Object *listbox, int with_animation);
extern void quickpanel_noti_listbox_set_item_deleted_cb(Evas_Object *listbox, void(*deleted_cb)(void *data, Evas_Object *obj));
extern void quickpanel_noti_listbox_update(Evas_Object *listbox);
extern void quickpanel_noti_listbox_update_item(Evas_Object *listbox, Evas_Object *item);
extern int quickpanel_noti_listbox_get_item_count(Evas_Object *listbox);
extern void quickpanel_noti_listbox_items_visibility_set(Evas_Object *listbox, int is_visible);
extern int quickpanel_noti_listbox_get_geometry(Evas_Object *listbox, int *limit_h, int *limit_partial_h, int *limit_partial_w);
extern void quickpanel_noti_listbox_closing_trigger_set(Evas_Object *listbox);

#endif
