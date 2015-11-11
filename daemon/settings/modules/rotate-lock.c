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

#include <vconf.h>
#include <system_settings.h>
#include <bundle_internal.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "common.h"
#include "quickpanel-ui.h"
#include "settings.h"
#include "setting_utils.h"
#include "setting_module_api.h"
#include "settings_icon_common.h"

#define BUTTON_LABEL _("IDS_ST_BUTTON2_AUTO_NROTATE")
#define BUTTON_ICON_NORMAL "quick_icon_auto_rotate.png"
#define BUTTON_ICON_HIGHLIGHT NULL
#define BUTTON_ICON_DIM NULL
#define PACKAGE_SETTING_MENU "setting-display-efl"

static void _mouse_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);

static const char *_label_get(void)
{
	return BUTTON_LABEL;
}

static const char *_icon_get(qp_setting_icon_image_type type)
{
	if (type == QP_SETTING_ICON_NORMAL) {
		return BUTTON_ICON_NORMAL;
	} else if (type == QP_SETTING_ICON_HIGHLIGHT) {
		return BUTTON_ICON_HIGHLIGHT;
	} else if (type == QP_SETTING_ICON_DIM) {
#ifdef BUTTON_ICON_DIM
		return BUTTON_ICON_DIM;
#endif
	}

	return NULL;
}

static void _long_press_cb(void *data)
{
#ifdef PACKAGE_SETTING_MENU
	bundle *kb = bundle_create();
	if (kb != NULL) {
		bundle_add(kb, "viewtype", "main");
		quickpanel_setting_icon_handler_longpress(PACKAGE_SETTING_MENU, kb);
		bundle_free(kb);
	} else {
		ERR("failed to create the bunlde");
	}
#endif
}

static void _view_update(Evas_Object *view, int state, int flag_extra_1, int flag_extra_2)
{
	Evas_Object *image = NULL;
	const char *icon_path = NULL;

	quickpanel_setting_icon_state_set(view, state);

	if (state == ICON_VIEW_STATE_ON) {
#ifdef BUTTON_ICON_HIGHLIGHT
		icon_path = BUTTON_ICON_HIGHLIGHT;
#endif
	} else if (state == ICON_VIEW_STATE_DIM) {
#ifdef BUTTON_ICON_DIM
		icon_path = BUTTON_ICON_DIM;
#endif
	} else {
		icon_path = BUTTON_ICON_NORMAL;
	}

	if (icon_path == NULL) {
		icon_path = BUTTON_ICON_NORMAL;
	}
	image = quickpanel_setting_icon_image_new(view, icon_path);
	quickpanel_setting_icon_content_set(view, image);
	quickpanel_setting_icon_text_set(view, BUTTON_LABEL, state);
}

static void _status_update(QP_Module_Setting *module, int flag_extra_1, int flag_extra_2)
{
	int ret = 0;
	bool status = false;
	retif(module == NULL, , "Invalid parameter!");

	ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_DISPLAY_SCREEN_ROTATION_AUTO, &status);
	msgif(ret !=  SYSTEM_SETTINGS_ERROR_NONE , "failed to notify key SYSTEM_SETTINGS_KEY_DISPLAY_SCREEN_ROTATION_AUTO : %d", ret);

	if (status == true) {
		quickpanel_setting_module_icon_state_set(module, ICON_VIEW_STATE_ON);
	} else {
		quickpanel_setting_module_icon_state_set(module, ICON_VIEW_STATE_OFF);
	}

	quickpanel_setting_module_icon_view_update(module, quickpanel_setting_module_icon_state_get(module),FLAG_VALUE_VOID);
}

static void _mouse_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	int ret = 0;
	bool status = false;

	ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_DISPLAY_SCREEN_ROTATION_AUTO, &status);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to notify key SYSTEM_SETTINGS_KEY_DISPLAY_SCREEN_ROTATION_AUTO : %d", ret);

	ret = system_settings_set_value_bool(SYSTEM_SETTINGS_KEY_DISPLAY_SCREEN_ROTATION_AUTO, !status );
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to notify key SYSTEM_SETTINGS_KEY_DISPLAY_SCREEN_ROTATION_AUTO : %d", ret);
}

static void _autorotation_vconf_cb(system_settings_key_e key, void *data)
{
	_status_update(data, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
}

static int _register_module_event_handler(void *data)
{
	int ret = 0;

	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_DISPLAY_SCREEN_ROTATION_AUTO, _autorotation_vconf_cb, data);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to notify key(SYSTEM_SETTINGS_KEY_DISPLAY_SCREEN_ROTATION_AUTO) : %d", ret);

	return QP_OK;
}

static int _unregister_module_event_handler(void *data)
{
	int ret = 0;

	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_DISPLAY_SCREEN_ROTATION_AUTO);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to ignore key(SYSTEM_SETTINGS_KEY_DISPLAY_SCREEN_ROTATION_AUTO) : %d", ret);

	return QP_OK;
}

/****************************************************************************
 *
 * Quickpanel Item functions
 *
 ****************************************************************************/
static int _init(void *data)
{
	int ret = QP_OK;

	ret = _register_module_event_handler(data);

	return ret;
}

static int _fini(void *data)
{
	int ret = QP_OK;

	ret = _unregister_module_event_handler(data);

	return ret;
}

static void _lang_changed(void *data)
{
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_icon_view_update_text(module);
}

static void _refresh(void *data)
{
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_icon_view_update_text(module);
}

QP_Module_Setting rotate = {
	.name 				= "rotate",
	.init				= _init,
	.fini 				= _fini,
	.lang_changed 		= _lang_changed,
	.refresh 			= _refresh,
	.icon_get 			= _icon_get,
	.label_get 			= _label_get,
	.view_update        = _view_update,
	.status_update		= _status_update,
	.handler_longpress		= _long_press_cb,
	.handler_press		= _mouse_clicked_cb,
};
