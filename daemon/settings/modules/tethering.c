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


#include <vconf.h>
#include <tethering.h>
#include <glib.h>
#include <bundle_internal.h>
#include <net_connection.h>
#include <syspopup_caller.h>
#include "common.h"
#include "quickpanel-ui.h"
#include "settings.h"
#include "setting_utils.h"
#include "setting_module_api.h"

#define MOBILE_AP_SYSPOPUP_NAME		"mobileap-syspopup"
#define BUTTON_LABEL 			_("IDS_ST_BUTTON2_WI_FI_NTETHERING")
#define BUTTON_ICON_NORMAL		"quick_icon_wifi_tethering.png"
#define BUTTON_ICON_HIGHLIGHT		NULL
#define BUTTON_ICON_DIM NULL
#define PACKAGE_SETTING_MENU		"setting-mobileap-efl"

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
	quickpanel_setting_icon_handler_longpress(PACKAGE_SETTING_MENU, NULL);
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
	retif(module == NULL, , "Invalid parameter!");
	retif(module->loader->extra_handler_1 == NULL, , "Invalid parameter!");

	quickpanel_setting_module_icon_timer_del(module);

	if (tethering_is_enabled(module->loader->extra_handler_1, TETHERING_TYPE_WIFI)) {
		quickpanel_setting_module_icon_state_set(module, ICON_VIEW_STATE_ON);
	} else {
		quickpanel_setting_module_icon_state_set(module, ICON_VIEW_STATE_OFF);
	}

	quickpanel_setting_module_progress_mode_set(module, FLAG_DISABLE, FLAG_TURN_OFF);
	quickpanel_setting_module_icon_timer_del(module);

	quickpanel_setting_module_icon_view_update(module,
			quickpanel_setting_module_icon_state_get(module),
			FLAG_VALUE_VOID);
}

static void _tethering_enabled_cb(tethering_error_e result, tethering_type_e type, bool is_requested, void *user_data)
{
	QP_Module_Setting *module = (QP_Module_Setting *)user_data;
	retif(module == NULL, , "Invalid parameter!");

	retif(type != TETHERING_TYPE_WIFI, , "Another type of tethering is enabled - type:%d", type);

	if (result != TETHERING_ERROR_NONE) {
		if (is_requested == TRUE) {
			quickpanel_setting_module_progress_mode_set(module, FLAG_DISABLE, FLAG_TURN_OFF);
			quickpanel_setting_module_icon_timer_del(module);
		}
		_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);

		WARN("Failed to enable tethering - error:%x", result);
		return;
	}

	if (is_requested == TRUE) {
		quickpanel_setting_module_progress_mode_set(module, FLAG_DISABLE, FLAG_TURN_OFF);
		quickpanel_setting_module_icon_timer_del(module);
	}
	_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
	WARN("WIFI tethering is enabled - type:%d", type);

	return;
}

static void _tethering_disabled_cb(tethering_error_e result, tethering_type_e type, tethering_disabled_cause_e cause, void *user_data)
{
	QP_Module_Setting *module = (QP_Module_Setting *)user_data;
	retif(module == NULL, , "Invalid parameter!");
	retif(module->loader == NULL, , "Invalid parameter!");

	if (result != TETHERING_ERROR_NONE && type == TETHERING_TYPE_WIFI) {
		quickpanel_setting_module_progress_mode_set(module, FLAG_DISABLE, FLAG_TURN_OFF);
		quickpanel_setting_module_icon_timer_del(module);
		_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);

		WARN("Failed to disable tethering - error:%x", result);
		return;
	}

	if (type == TETHERING_TYPE_WIFI) {
		quickpanel_setting_module_progress_mode_set(module, FLAG_DISABLE, FLAG_TURN_OFF);
		quickpanel_setting_module_icon_timer_del(module);
		_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
		WARN("WIFI tethering is disabled - cause:%x", cause);
	} else {
		WARN("Ignored tethering event - result:%x type:%d cause:%x", result, type, cause);
	}

	return;
}

static int _tethering_enabled_set(void *data, Eina_Bool state)
{
	QP_Module_Setting *module = (QP_Module_Setting *)data;

	if (module == NULL || module->loader->extra_handler_1 == NULL) {
		ERR("invalid parameter\n");
		return QP_FAIL;
	}

	if (state) {
		if (tethering_is_enabled(NULL, TETHERING_TYPE_WIFI) == FALSE) {
			// for checking pre-conditions, popup will be provided by mobileap-syspopup
			bundle *b = bundle_create();
			if (!b) {
				ERR("Failed to create bundle.");
				return QP_FAIL;
			}
			bundle_add(b, "msg", "Confirm WiFi tethering on");
			syspopup_launch(MOBILE_AP_SYSPOPUP_NAME, b);
			bundle_free(b);
		}
	} else {
		if (tethering_is_enabled(NULL, TETHERING_TYPE_WIFI) == TRUE) {
			bundle *b = bundle_create();
			if (!b) {
				ERR("Failed to create bundle.");
				return QP_FAIL;
			}
			bundle_add(b, "msg", "Confirm WiFi tethering off");
			syspopup_launch(MOBILE_AP_SYSPOPUP_NAME, b);
			bundle_free(b);
		}
	}

	return QP_OK;
}

static void _mouse_clicked_cb(void *data,
		Evas_Object *obj, const char *emission, const char *source)
{
	int ret = 0;
	int is_on = 0;
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	if (quickpanel_setting_module_is_icon_clickable(module) == 0) {
		DBG("Icon is not clickable");
		return;
	}

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_OFF) {
		ret = _tethering_enabled_set(module, EINA_TRUE);
		if (ret != QP_OK) {
			ERR("Failed to enable tethering");
			return;
		}
		is_on = 1;
	} else {
		ret = _tethering_enabled_set(module, EINA_FALSE);
		if (ret != QP_OK) {
			ERR("Failed to disable tethering");
			return;
		}
		is_on = 0;
	}

	if (ret == QP_OK) {
		if (is_on == 1) {
			quickpanel_setting_module_progress_mode_set(module, FLAG_ENABLE, FLAG_TURN_ON);
		} else {
			quickpanel_setting_module_progress_mode_set(module, FLAG_ENABLE, FLAG_TURN_OFF);
		}
		quickpanel_setting_module_icon_timer_add(module);
	}
	return;
}

static int _register_module_event_handler(void *data)
{
	tethering_error_e ret = TETHERING_ERROR_NONE;
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, ret, "Invalid parameter!");

	ret = tethering_create(&(module->loader->extra_handler_1));
	msgif(ret != TETHERING_ERROR_NONE, "fail to create tethering handler");

	ret = tethering_set_enabled_cb(module->loader->extra_handler_1,
			TETHERING_TYPE_WIFI, _tethering_enabled_cb, data);
	msgif(ret != TETHERING_ERROR_NONE, "fail to register enabled callback");

	ret = tethering_set_disabled_cb(module->loader->extra_handler_1,
			TETHERING_TYPE_WIFI, _tethering_disabled_cb, data);
	msgif(ret != TETHERING_ERROR_NONE, "fail to register disabled callback");

	return QP_OK;
}

static int _unregister_module_event_handler(void *data)
{
	tethering_error_e ret =TETHERING_ERROR_NONE;
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, ret, "Invalid parameter!");

	if (module->loader->extra_handler_1 != NULL) {

		tethering_destroy(module->loader->extra_handler_1);
		module->loader->extra_handler_1 = NULL;
	}

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

static void _reset_icon(QP_Module_Setting *module)
{
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_progress_mode_set(module, FLAG_DISABLE, FLAG_VALUE_VOID);
	_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
}

static void _handler_on(void *data)
{
	tethering_error_e ret = TETHERING_ERROR_NONE;
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_progress_mode_set(module, FLAG_DISABLE, FLAG_TURN_OFF);
	quickpanel_setting_module_icon_timer_del(module);

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_OFF) {
		ret = _tethering_enabled_set(module, EINA_TRUE);

		if (ret == QP_OK) {
			quickpanel_setting_module_progress_mode_set(module, FLAG_ENABLE, FLAG_TURN_ON);
			quickpanel_setting_module_icon_timer_add(module);
		} else {
			ERR("op failed:%d", ret);
		}
	} else {
		ERR("the button is already turned on");
		_reset_icon(module);
	}
}

static void _handler_off(void *data)
{
	int ret = TETHERING_ERROR_NONE;
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_progress_mode_set(module, FLAG_DISABLE, FLAG_TURN_OFF);
	quickpanel_setting_module_icon_timer_del(module);

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_ON) {
		ret = _tethering_enabled_set(module, EINA_FALSE);
		if (ret == QP_OK) {
			quickpanel_setting_module_progress_mode_set(module, FLAG_ENABLE, FLAG_TURN_OFF);
			quickpanel_setting_module_icon_timer_add(module);
		} else {
			ERR("op failed:%d", ret);
		}
	} else {
		ERR("the button is already turned off");
		_reset_icon(module);
	}
}

static void _handler_progress_on(void *data)
{
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_progress_mode_set(module, FLAG_ENABLE, FLAG_TURN_ON);
}

static void _handler_progress_off(void *data)
{
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	_reset_icon(module);
}

static void _handler_progress_reset(void *data)
{
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	_reset_icon(module);
}

static int _handler_ipc(const char *command, void *data)
{
	int i = 0;
	retif(data == NULL, EINA_FALSE, "item data is NULL");
	retif(command == NULL, EINA_FALSE, "command is NULL");

	static Setting_Activity_Handler __table_handler[] = {
		{
			.command = "on",
			.handler = _handler_on,
		},
		{
			.command = "off",
			.handler = _handler_off,
		},
		{
			.command = "progress_on",
			.handler = _handler_progress_on,
		},
		{
			.command = "progress_off",
			.handler = _handler_progress_off,
		},
		{
			.command = "progress_reset",
			.handler = _handler_progress_reset,
		},
		{
			.command = NULL,
			.handler = NULL,
		},
	};

	for (i = 0; __table_handler[i].command; i++) {
		if (strcasecmp(__table_handler[i].command, command)) {
			continue;
		}

		if (__table_handler[i].handler != NULL) {
			DBG("process:%s", command);
			__table_handler[i].handler(data);
		}
		break;
	}

	return EINA_TRUE;
}

QP_Module_Setting tethering = {
	.name 				= "wifi_hotspot",
	.init				= _init,
	.fini 				= _fini,
	.lang_changed 			= _lang_changed,
	.refresh 			= _refresh,
	.icon_get 			= _icon_get,
	.label_get 			= _label_get,
	.view_update        = _view_update,
	.status_update		= _status_update,
	.handler_longpress	= _long_press_cb,
	.handler_ipc        = _handler_ipc,
	.handler_press		= _mouse_clicked_cb,
};
