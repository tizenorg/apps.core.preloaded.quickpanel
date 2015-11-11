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


#ifndef __SETTING_MODULE_API_H__
#define __SETTING_MODULE_API_H__

#define FLAG_VALUE_VOID 0xDEADDEAD

#define FLAG_ENABLE 1
#define FLAG_DISABLE 0

#define FLAG_TURN_ON 1
#define FLAG_TURN_OFF 0

typedef enum _qp_setting_icon_container_type {
	QP_SETTING_ICON_CONTAINER_NONE = -1,
	QP_SETTING_ICON_CONTAINER_FEATURED = 0,
	QP_SETTING_ICON_CONTAINER_ALL_LIST,
} qp_setting_icon_container_type;

extern Evas_Object *quickpanel_setting_module_icon_create(QP_Module_Setting *module, Evas_Object *parent);
extern void quickpanel_setting_module_icon_add(QP_Module_Setting *module, Evas_Object *icon, qp_setting_icon_container_type container_type);
extern void quickpanel_setting_module_icon_remove(QP_Module_Setting *module, Evas_Object *icon);
extern void quickpanel_setting_module_icon_state_set(QP_Module_Setting *module, int state);
extern int quickpanel_setting_module_icon_state_get(QP_Module_Setting *module);
extern Evas_Object *quickpanel_setting_module_icon_get(QP_Module_Setting *module, qp_setting_icon_container_type container_type);
extern void quickpanel_setting_module_icon_view_update(QP_Module_Setting *module, int flag_extra_1, int flag_extra_2);
extern void quickpanel_setting_module_icon_view_update_text(QP_Module_Setting *module);
extern void quickpanel_setting_module_icon_status_update(QP_Module_Setting *module, int flag_extra_1, int flag_extra_2);
extern int quickpanel_setting_module_is_icon_clickable(QP_Module_Setting *module);
extern void quickpanel_setting_module_icon_timer_add(QP_Module_Setting *module);
extern void quickpanel_setting_module_icon_timer_del(QP_Module_Setting *module);
extern void quickpanel_setting_module_progress_mode_set(QP_Module_Setting *module, int is_enable, int is_request_on);
extern void quickpanel_setting_module_icon_destroy(QP_Module_Setting *module, Evas_Object *icon);

extern QP_Module_Setting *quickpanel_setting_module_get_from_icon(Evas_Object *icon);

#endif /* __SETTING_MODULE_API_H__ */
