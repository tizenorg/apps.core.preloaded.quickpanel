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

#ifndef __SETTING_H__
#define __SETTING_H__

#define MODULE_BLANK "blank"
#define FILE_QP_BUTTON_ORDER_INI DATADIR_RW"/qp_setting_order.ini"
#define E_DATA_MODULE_INFO "module_info"

#define ICON_VIEW_STATE_OFF 0
#define ICON_VIEW_STATE_ON 1
#define ICON_VIEW_STATE_DIM 2

#define STATE_ICON_NOT_LOADED 0
#define STATE_ICON_IDLE 1
#define STATE_ICON_BUSY 2

#define QS_DBUS_SIG_ACTIVITY "ACTIVITY"
#define QS_DBUS_SIG_EDITING "EDITING"

typedef enum _qp_setting_icon_state_type {
	QP_SETTING_ICON_STATE_NOT_LOADED,
	QP_SETTING_ICON_STATE_IDLE,
	QP_SETTING_ICON_STATE_BUSY,
} qp_setting_icon_state_type;

typedef enum _qp_setting_icon_image_type {
	QP_SETTING_ICON_NORMAL,
	QP_SETTING_ICON_HIGHLIGHT,
	QP_SETTING_ICON_DIM,
} qp_setting_icon_image_type;

typedef  struct _Setting_Activity_Handler {
	char *command;
	void (*handler)(void *data);
} Setting_Activity_Handler;

typedef struct _QP_Module_Setting QP_Module_Setting;
typedef struct _QP_Setting_Loaded_Item QP_Setting_Loaded_Item;

struct _QP_Module_Setting {
	char *name;
	int is_disable_feedback;

	/* func */
	int (*init) (void *);
	int (*fini) (void *);
	int (*suspend) (void *);
	int (*resume) (void *);
	int (*hib_enter) (void *);
	int (*hib_leave) (void *);
	void (*lang_changed) (void *);
	void (*refresh) (void *);
	void (*qp_opened) (void *);
	void (*qp_closed) (void *);

	const char *(*label_get) (void);
	const char *(*icon_get) (qp_setting_icon_image_type type);
	int (*supported_get) (void);
	void (*view_update)(Evas_Object *, int, int, int);
	void (*status_update)(QP_Module_Setting *, int, int);

	int (*handler_ipc)(const char *, void *);
	Edje_Signal_Cb handler_press;
	void (*handler_longpress) (void *);

	/* do not modify this area */
	/* internal data */
	Eina_Bool is_loaded;
	QP_Setting_Loaded_Item *loader;
	Eina_List *view_list;
};

struct _QP_Setting_Loaded_Item {
	QP_Module_Setting *module;
	void *app_data;
	int state_icon;

	Evas_Object *view;
	Ecore_Timer *timer;
	int state;
	void *extra_handler_1;
};

extern void quickpanel_setting_save_list_to_file(Eina_List *list, int num_featured);
extern int quickpanel_settings_featured_list_validation_check(char *order);
extern void quickpanel_settings_featured_list_get(Eina_List **list);
extern void quickpanel_settings_all_list_get(Eina_List **list);

extern QP_Module_Setting *quickpanel_settings_module_get_by_name(const char *name);
extern int quickpanel_settings_module_count_get(void);

#endif
