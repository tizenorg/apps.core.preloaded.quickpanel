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

#include <Elementary.h>

#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <sound_manager.h>
#include <E_DBus.h>

#include "media.h"
#include "quickpanel-ui.h"
#include "quickpanel_def.h"
#include "common_uic.h"
#include "common.h"
#include "modules.h"
#include "settings.h"
#include "setting_utils.h"
#include "setting_module_api.h"
#include "accessibility.h"
#include "pager.h"
#include "pager_common.h"

#define E_DATA_ICON_CLICKED_CB "clicked_cb"
#define E_DATA_ICON_ORIGINAL_OBJ "original_obj"

#define TAP_AND_DELAY_LONG 1.000

static struct _info {
	int down_x;
	Eina_Bool is_longpressed;
	Ecore_Timer *timer_longpress;
} s_info = {
	.down_x = 0,
	.is_longpressed = EINA_FALSE,
	.timer_longpress = NULL,
};

static Eina_Bool _icon_handler_longpress(void *data)
{
	DBG("");
	Evas_Object *obj = data;
	QP_Module_Setting *module = NULL;
	retif(obj == NULL, ECORE_CALLBACK_CANCEL, "invalid argument");

	struct appdata *ad = (struct appdata*)quickpanel_get_app_data();
	retif(ad == NULL, ECORE_CALLBACK_CANCEL, "application data is NULL");

	quickpanel_media_play_feedback();

	ecore_timer_del(s_info.timer_longpress);
	s_info.timer_longpress = NULL;
	s_info.is_longpressed = EINA_TRUE;

	module = evas_object_data_get(obj, E_DATA_MODULE_INFO);
	if (module != NULL) {
		if (module->handler_longpress != NULL) {
			if (module->name) {
				DBG("launch setting menu of %s", module->name);
			}
			module->handler_longpress(obj);

			elm_object_signal_emit(obj, "mouse,up,1", "background.super");
		}
	}

	return ECORE_CALLBACK_CANCEL;
}

static void _icon_mouse_up_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	retif(obj == NULL, , "invalid argument");

	if (s_info.timer_longpress != NULL) {
		ecore_timer_del(s_info.timer_longpress);
		s_info.timer_longpress = NULL;
		s_info.is_longpressed = EINA_FALSE;
	}
}

static void _icon_mouse_down_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	retif(obj == NULL, , "invalid argument");

	if (s_info.timer_longpress != NULL) {
		ecore_timer_del(s_info.timer_longpress);
		s_info.timer_longpress = NULL;
	}

	quickpanel_page_get_touched_pos(&(s_info.down_x), NULL);

	s_info.is_longpressed = EINA_FALSE;
	s_info.timer_longpress = ecore_timer_add(TAP_AND_DELAY_LONG,_icon_handler_longpress, obj);
}

#ifdef QP_SCREENREADER_ENABLE
static void _icon_focus_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *icon = NULL;
	Edje_Signal_Cb func = NULL;
	retif(obj == NULL, , "invalid argument");

	struct appdata *ad = (struct appdata*)quickpanel_get_app_data();
	retif(ad == NULL, , "application data is NULL");

	if (s_info.timer_longpress != NULL) {
		ecore_timer_del(s_info.timer_longpress);
		s_info.timer_longpress = NULL;
	}

	if (s_info.is_longpressed != EINA_TRUE) {
		quickpanel_media_play_feedback();
	}

	icon = evas_object_data_get(obj, E_DATA_ICON_ORIGINAL_OBJ);
	if (icon != NULL) {
		func = evas_object_data_get(icon, E_DATA_ICON_CLICKED_CB);

		if (func != NULL && s_info.is_longpressed != EINA_TRUE) {
			func(data, icon,  "mouse,clicked,1", "background.super");
		}
	}
}

static void _icon_focus_clicked_cb_without_feedback(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *icon = NULL;
	Edje_Signal_Cb func = NULL;
	retif(obj == NULL, , "invalid argument");

	struct appdata *ad = (struct appdata*)quickpanel_get_app_data();
	retif(ad == NULL, , "application data is NULL");

	if (s_info.timer_longpress != NULL) {
		ecore_timer_del(s_info.timer_longpress);
		s_info.timer_longpress = NULL;
	}

	icon = evas_object_data_get(obj, E_DATA_ICON_ORIGINAL_OBJ);
	if (icon != NULL) {
		func = evas_object_data_get(icon, E_DATA_ICON_CLICKED_CB);

		if (func != NULL && s_info.is_longpressed != EINA_TRUE) {
			func(data, icon,  "mouse,clicked,1", "background.super");
		}
	}
}

HAPI int quickpanel_setting_icon_click_cb_add(Evas_Object *icon, Edje_Signal_Cb func, void *data)
{
	retif(icon == NULL, QP_FAIL, "invalid parameter");
	retif(func == NULL, QP_FAIL, "invalid parameter");

	struct appdata *ad = (struct appdata*)quickpanel_get_app_data();
	retif(ad == NULL, QP_FAIL, "application data is NULL");

	elm_object_signal_callback_add(icon, "mouse,up,1", "background.super", _icon_mouse_up_cb, data);
	elm_object_signal_callback_add(icon, "mouse,down,1", "background.super", _icon_mouse_down_cb, data);

	evas_object_data_set(icon, E_DATA_ICON_CLICKED_CB, func);

#ifdef QP_SCREENREADER_ENABLE
	Evas_Object *ao = NULL;
	ao = quickpanel_accessibility_screen_reader_object_get(icon,
			SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", icon);
	if (ao != NULL) {
		evas_object_smart_callback_add(ao, "clicked",
				_icon_focus_clicked_cb, data);
		evas_object_data_set(ao, E_DATA_ICON_ORIGINAL_OBJ, icon);
	}
#endif

	return 0;
}

HAPI int quickpanel_setting_icon_click_cb_without_feedback_add(Evas_Object *icon, Edje_Signal_Cb func, void *data)
{
	retif(icon == NULL, QP_FAIL, "invalid parameter");
	retif(func == NULL, QP_FAIL, "invalid parameter");

	struct appdata *ad = (struct appdata*)quickpanel_get_app_data();
	retif(ad == NULL, QP_FAIL, "application data is NULL");

	elm_object_signal_callback_add(icon, "mouse,up,1", "background.super", _icon_mouse_up_cb, data);
	elm_object_signal_callback_add(icon, "mouse,down,1", "background.super", _icon_mouse_down_cb, data);

	evas_object_data_set(icon, E_DATA_ICON_CLICKED_CB, func);

#ifdef QP_SCREENREADER_ENABLE
	Evas_Object *ao = NULL;
	ao = quickpanel_accessibility_screen_reader_object_get(icon,
			SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", icon);
	if (ao != NULL) {
		evas_object_smart_callback_add(ao, "clicked",
				_icon_focus_clicked_cb_without_feedback, data);
		evas_object_data_set(ao, E_DATA_ICON_ORIGINAL_OBJ, icon);
	}
#endif

	return 0;
}

HAPI int quickpanel_setting_icon_click_cb_del(Evas_Object *icon, Edje_Signal_Cb func)
{
	retif(icon == NULL, QP_FAIL, "invalid parameter");
	retif(func == NULL, QP_FAIL, "invalid parameter");

	struct appdata *ad = (struct appdata*)quickpanel_get_app_data();
	retif(ad == NULL, QP_FAIL, "application data is NULL");

	elm_object_signal_callback_del(icon, "mouse,clicked,1", "background.super", func);

	return QP_OK;
}

HAPI void quickpanel_setting_icon_handler_longpress(const char *pkgname, void *data)
{
	int ret;

	ret = quickpanel_uic_launch_ug_by_appcontrol(pkgname, data);
	quickpanel_uic_launch_app_inform_result(pkgname, ret);
	quickpanel_uic_close_quickpanel(true, 1);
}
#endif

