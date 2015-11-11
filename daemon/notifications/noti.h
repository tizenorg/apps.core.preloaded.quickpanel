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


#ifndef __NOTI_H__
#define __NOTI_H__

#define NOTI_PRESS_BG 0
#define NOTI_PRESS_BUTTON_1 1

#define QP_PRELOAD_NOTI_ICON_PATH "/usr/apps/org.tizen.quickpanel/shared/res/noti_icons"

extern int quickpanel_noti_get_count(void);
extern int quickpanel_noti_get_geometry(int *limit_h, int *limit_partial_h, int *limit_partial_w);
extern void quickpanel_noti_closing_trigger_set(void);

extern noti_node_item *quickpanel_noti_node_get_by_priv_id(int priv_id);
extern noti_node_item *quickpanel_noti_node_get_first_noti(void);

extern void quickpanel_noti_set_clear_all_status();

extern void quickpanel_noti_on_clear_all_clicked(void *data, Evas_Object *obj, void *info);
extern int quickpanel_noti_get_type_count(notification_type_e noti_type);
extern void quickpanel_noti_init_noti_section(void);
extern void quickpanel_noti_update_by_system_time_changed_setting_cb(system_settings_key_e  key, void *data);
extern void quickpanel_noti_update_by_system_time_changed_vconf_cb(keynode_t *key, void *data);

#endif
