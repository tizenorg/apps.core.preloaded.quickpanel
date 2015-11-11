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


#ifndef __QUICKPANEL_SETTINGS_GRIDBOX_H__
#define __QUICKPANEL_SETTINGS_GRIDBOX_H__

#define SETTINGS_GRIDBOX_PREPEND 1
#define SETTINGS_GRIDBOX_APPEND 0
#define SETTINGS_GRIDBOX_ITEM_ICON "item_icon"
#define SETTINGS_GRIDBOX_ITEM_DIVIDER "item_divider"

extern Evas_Object *quickpanel_settings_gridbox_create(Evas_Object *parent, void *data);
extern void quickpanel_settings_gridbox_remove(Evas_Object *gridbox);
extern void quickpanel_settings_gridbox_item_add(Evas_Object *gridbox, Evas_Object *item, const char *item_type, int is_prepend);
extern void quickpanel_settings_gridbox_item_remove(Evas_Object *gridbox, Evas_Object *item);
extern void quickpanel_settings_gridbox_item_remove_all(Evas_Object *gridbox);
extern void quickpanel_settings_gridbox_rotation(Evas_Object *gridbox, int angle);
extern int quickpanel_settings_gridbox_item_count_get(Evas_Object *gridbox);
extern int quickpanel_settings_gridbox_item_index_get(Evas_Object *gridbox, int touch_x, int touch_y);
extern void quickpanel_settings_gridbox_unpack_all(Evas_Object *gridbox);

#endif
