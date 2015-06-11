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
#include <syspopup_caller.h>
#include <tapi_common.h>
#include <ITapiSim.h>
#include <ITapiModem.h>
#include <TapiUtility.h>
#include <system_settings.h>
#include <bundle_internal.h>
#include "common.h"
#include "quickpanel-ui.h"
#include "settings.h"
#include "setting_utils.h"
#include "setting_module_api.h"


#define BUTTON_LABEL _("IDS_ST_BUTTON2_FLIGHT_NMODE")
#define BUTTON_ICON_NORMAL "quick_icon_flightmode.png"
#define BUTTON_ICON_HIGHLIGHT NULL
#define BUTTON_ICON_DIM NULL
#define PACKAGE_SETTING_MENU "setting-flightmode-efl"
#define SYSPOPUP_NAME "mode-syspopup"

static Eina_Bool fly_icon_is_locked = EINA_FALSE;
static Ecore_Timer *timer = NULL;

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

static Eina_Bool _unlock_fly_icon(void *data)
{
	fly_icon_is_locked = EINA_FALSE;
	ecore_timer_del(timer);
	timer = NULL;

	return ECORE_CALLBACK_CANCEL;
}

static void _long_press_cb(void *data)
{
#ifdef PACKAGE_SETTING_MENU
	if (fly_icon_is_locked == EINA_TRUE) {
		LOGD("Fly icon is locked");
		return;
	}
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
	LOGD("");
	int ret = 0;
	int status = 0;
	retif(module == NULL, , "Invalid parameter!");

#ifdef HAVE_X
	ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, &status);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "fail to get VCONFKEY_TELEPHONY_FLIGHT_MODE:%d", ret);
#endif

	if (status == 1) {
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

static void _tapi_flight_mode_cb(TapiHandle *handle, int result, void *data, void *user_data)
{
	ERR("flight mode result:%d", result);
	_status_update(user_data, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
}

static int _tapi_flight_mode_set(int on, void *data)
{
	LOGD("");
	int ret = QP_OK;
	int ret_t = TAPI_API_SUCCESS;
	TapiHandle *tapi_handle = NULL;
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, QP_FAIL, "Invalid parameter!");

	tapi_handle = tel_init(NULL);
	retif(tapi_handle == NULL, QP_FAIL, "failed to initialized tapi handler");

	if (on == 1) {
		ret_t = tel_set_flight_mode(tapi_handle,
						TAPI_POWER_FLIGHT_MODE_ENTER, _tapi_flight_mode_cb, data);
		if (ret_t != TAPI_API_SUCCESS) {
			ret = QP_FAIL;
			ERR("tel_set_flight_mode enter error:%d", ret_t);
		}
	} else {
		ret_t = tel_set_flight_mode(tapi_handle,
						TAPI_POWER_FLIGHT_MODE_LEAVE, _tapi_flight_mode_cb, data);
		if (ret_t != TAPI_API_SUCCESS) {
			ret = QP_FAIL;
			ERR("tel_set_flight_mode leave error:%d", ret_t);
		}
	}

	if ((ret_t = tel_deinit(tapi_handle)) != TAPI_API_SUCCESS) {
		ERR("failed to deinitialized tapi handler:%d", ret_t);
	}

	return ret;
}

static void _turn_on(int is_on)
{
	LOGD("");
	bundle *b = NULL;
	b = bundle_create();
	if (b != NULL) {
		if (is_on) {
			bundle_add(b, "_MODE_SYSTEM_POPUP_TYPE_", "MODE_SYSTEM_FLIGHTMODE_ON");
		} else {
			bundle_add(b, "_MODE_SYSTEM_POPUP_TYPE_", "MODE_SYSTEM_FLIGHTMODE_OFF");
		}
		syspopup_launch(SYSPOPUP_NAME, b);
		bundle_free(b);
	} else {
		ERR("failed to create a bundle");
	}

	timer = ecore_timer_add(1.0, _unlock_fly_icon, NULL);
}

static void _mouse_clicked_cb(void *data,
		Evas_Object *obj, const char *emission, const char *source)
{
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	LOGD("");
	retif(module == NULL, , "Invalid parameter!");

	if (fly_icon_is_locked == EINA_TRUE) {
		LOGD("Fly icon is locked");
		return;
	}

	if (quickpanel_setting_module_is_icon_clickable(module) == 0) {
		LOGD("Fly icon is not clickable");
		return;
	}

	fly_icon_is_locked = EINA_TRUE;

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_OFF) {
		_turn_on(1);
	} else {
		_turn_on(0);
	}
}

static void _tapi_flight_mode_vconf_cb(keynode_t *node, void *data)
{
	_status_update(data, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
}

static int _register_module_event_handler(void *data)
{
	int ret = 0;

#ifdef HAVE_X
	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, _tapi_flight_mode_vconf_cb, data);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to notify key(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE) : %d", ret);
#endif

	return QP_OK;
}

static int _unregister_module_event_handler(void *data)
{
	int ret = 0;

#ifdef HAVE_X
	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to ignore key(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE) : %d", ret);
#endif

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
	int ret = 0;
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_progress_mode_set(module, FLAG_DISABLE, FLAG_TURN_OFF);
	quickpanel_setting_module_icon_timer_del(module);

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_OFF) {
		ret = _tapi_flight_mode_set(1, module);

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
	int ret = 0;
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_progress_mode_set(module, FLAG_DISABLE, FLAG_TURN_OFF);
	quickpanel_setting_module_icon_timer_del(module);

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_ON) {
		ret = _tapi_flight_mode_set(0, module);

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
		if (strcmp(__table_handler[i].command, command)) {
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

QP_Module_Setting flightmode = {
	.name 				= "flightmode",
	.init				= _init,
	.fini 				= _fini,
	.lang_changed 		= _lang_changed,
	.refresh 			= _refresh,
	.icon_get 			= _icon_get,
	.label_get 			= _label_get,
	.view_update        = _view_update,
	.status_update		= _status_update,
	.handler_longpress	= _long_press_cb,
	.handler_ipc        = _handler_ipc,
	.handler_press		= _mouse_clicked_cb,
};
