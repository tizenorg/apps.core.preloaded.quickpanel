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
#include <syspopup_caller.h>
#include <app_control.h>
#include <locations.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "common.h"
#include "quickpanel-ui.h"
#include "settings.h"
#include "setting_utils.h"
#include "setting_module_api.h"
#include "settings_icon_common.h"


#define BUTTON_LABEL _("IDS_QP_BUTTON2_LOCATION_ABB")
#define BUTTON_ICON_NORMAL "quick_icon_location.png"
#define BUTTON_ICON_HIGHLIGHT NULL
#define BUTTON_ICON_DIM NULL
#define PACKAGE_SETTING_MENU "org.tizen.setting-location"
#define OPERATION_SETTING_MENU "http://tizen.org/appcontrol/operation/configure/location"
#define PACKAGE_SYSPOPUP "gps-syspopup"

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

static void _syspopup_launch(int is_on)
{
	syspopup_launch(PACKAGE_SYSPOPUP, NULL);
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
	bool status = 0;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_progress_mode_set(module, FLAG_DISABLE, FLAG_TURN_OFF);
	quickpanel_setting_module_icon_timer_del(module);

	ret = location_manager_is_enabled_method(LOCATIONS_METHOD_HYBRID, &status);
	msgif(ret != 0, "fail to get LOCATIONS_METHOD_HYBRID:%d", ret);

	if (status == true) {
		quickpanel_setting_module_icon_state_set(module, ICON_VIEW_STATE_ON);
	} else {
		quickpanel_setting_module_icon_state_set(module, ICON_VIEW_STATE_OFF);
	}

	quickpanel_setting_module_icon_view_update(module,
			quickpanel_setting_module_icon_state_get(module),
			FLAG_VALUE_VOID);
}

static void _mouse_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	int ret = 0;
	bool enable = 0;
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	if (quickpanel_setting_module_is_icon_clickable(module) == 0) {
		return;
	}

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_OFF) {
		_syspopup_launch(quickpanel_setting_module_icon_state_get(module));
	} else {
		// Use my location off
		ret = location_manager_is_enabled_method(LOCATIONS_METHOD_HYBRID, &enable);
		if (ret == false) {
			if (enable == true) {
				ret = location_manager_enable_method(LOCATIONS_METHOD_HYBRID, false);
				if (ret != 0) {
					ERR("Failed to set LOCATIONS_METHOD_HYBRID[%d]", ret);
				}
			}
		} else {
			ERR("Failed to get Use my location[%d]", ret);
		}
		// GPS off
		ret = location_manager_is_enabled_method(LOCATIONS_METHOD_GPS, &enable);
		if (ret == false) {
			if (enable == true) {
				ret = location_manager_enable_method(LOCATIONS_METHOD_GPS, false);
				if (ret != 0) {
					ERR("Failed to set LOCATIONS_METHOD_GPS [%d]", ret);
				}
			}
		} else {
			ERR("Failed to get GPS[%d]", ret);
		}
		// Wireless networks off
		ret = location_manager_is_enabled_method(LOCATIONS_METHOD_WPS, &enable);
		if (ret == false) {
			if (enable == true) {
				ret = location_manager_enable_method(LOCATIONS_METHOD_WPS, false);
				if (ret != 0) {
					ERR("Failed to set LOCATIONS_METHOD_WPS [%d]", ret);
				}
			}
		} else {
			ERR("Failed to get network[%d]", ret);
		}
	}
}

static void _gps_vconf_cb(keynode_t *node, void *data)
{
	_status_update(data, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
}

static int _register_module_event_handler(void *data)
{
	int ret = 0;

	ret = vconf_notify_key_changed(VCONFKEY_LOCATION_USE_MY_LOCATION,
			_gps_vconf_cb, data);
	msgif(ret != 0, "failed to notify key(VCONFKEY_LOCATION_USE_MY_LOCATION) : %d", ret);

	return QP_OK;
}

static int _unregister_module_event_handler(void *data)
{
	int ret = 0;

	ret = vconf_ignore_key_changed(VCONFKEY_LOCATION_USE_MY_LOCATION,
			_gps_vconf_cb);
	msgif(ret != 0, "failed to ignore key(VCONFKEY_LOCATION_USE_MY_LOCATION) : %d", ret);

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
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_progress_mode_set(module, FLAG_DISABLE, FLAG_TURN_OFF);
	quickpanel_setting_module_icon_timer_del(module);

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_OFF) {
		quickpanel_setting_module_progress_mode_set(module, FLAG_ENABLE, FLAG_TURN_ON);
		quickpanel_setting_module_icon_timer_add(module);
	} else {
		ERR("the button is already turned on");
		_reset_icon(module);
	}
}

static void _handler_off(void *data)
{
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_progress_mode_set(module, FLAG_DISABLE, FLAG_TURN_OFF);
	quickpanel_setting_module_icon_timer_del(module);

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_ON) {
		quickpanel_setting_module_progress_mode_set(module, FLAG_ENABLE, FLAG_TURN_OFF);
		quickpanel_setting_module_icon_timer_add(module);
	} else {
		ERR("the button is already turned off");
		_reset_icon(module);
	}
}

static void _handler_progress_on(void *data)
{
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_progress_mode_set(module, FLAG_ENABLE, FLAG_VALUE_VOID);
}

static void _handler_progress_off(void *data)
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

QP_Module_Setting gps = {
	.name 				= "gps",
	.init				= _init,
	.fini 				= _fini,
	.lang_changed 		= _lang_changed,
	.refresh 			= _refresh,
	.icon_get 			= _icon_get,
	.label_get 			= _label_get,
	.view_update        = _view_update,
	.status_update		= _status_update,
	.handler_longpress		= _long_press_cb,
	.handler_ipc        = _handler_ipc,
	.handler_press		= _mouse_clicked_cb,
};
