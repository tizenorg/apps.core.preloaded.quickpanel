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

#include <app.h>
#include <tethering.h>
#include <wifi.h>
#include <vconf.h>

#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "common.h"
#include "quickpanel-ui.h"
#include "settings.h"
#include "setting_utils.h"
#include "setting_module_api.h"
#include "settings_icon_common.h"

#define E_DATA_POPUP_MODULE_ITEM "mobule_item"
#define BUTTON_LABEL _("IDS_ST_BUTTON2_WI_FI_ABB")
#define BUTTON_ICON_NORMAL "quick_icon_wifi.png"
#define BUTTON_ICON_HIGHLIGHT NULL
#define BUTTON_ICON_DIM NULL
#define PACKAGE_SETTING_MENU "wifi-efl-ug-lite"
#define PACKAGE_SETTING_MENU_PTN "wifi-efl-ug"	// for redwood partner

static int _wifi_on(void *data, const char *popup_txt);
static int _wifi_off(void);
static int _wifi_is_on(bool *is_on);
static void _wifi_state_changed_cb(wifi_device_state_e state, void *user_data);
static void _mouse_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _reset_icon(QP_Module_Setting *module);

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
	if (quickpanel_common_ui_is_package_exist(PACKAGE_SETTING_MENU)) {
		quickpanel_setting_icon_handler_longpress(PACKAGE_SETTING_MENU, NULL);
	} else {
		DBG("No package[%s] try[%s]", PACKAGE_SETTING_MENU, PACKAGE_SETTING_MENU_PTN);
		quickpanel_setting_icon_handler_longpress(PACKAGE_SETTING_MENU_PTN, NULL);
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

static void _status_update(QP_Module_Setting *module, int wifi_status, int flag_extra_2)
{
	ERR("");
	int ret = -1;
	bool is_on = 0;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_icon_timer_del(module);

	if (wifi_status == FLAG_VALUE_VOID) {
		ret = _wifi_is_on(&is_on);
	} else {
		ret = 0;
		if (wifi_status == WIFI_DEVICE_STATE_ACTIVATED) {
			is_on = true;
		} else {
			is_on = false;
		}
	}

	if (ret == 0 && is_on == true) {
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

static void _mouse_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	int ret = 0;
	int is_on = 0;
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	if (quickpanel_setting_module_is_icon_clickable(module) == 0) {
		return;
	}

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_ON) {
		is_on = FLAG_TURN_OFF;
		ret = _wifi_off();
	} else {
		is_on = FLAG_TURN_ON;
		ret = _wifi_on(module,
				_("IDS_WIFI_POP_BOTH_WI_FI_AND_MOBILE_AP_CANNOT_BE_ACTIVATED_AT_THE_SAME_TIME_DEACTIVATE_MOBILE_AP_Q"));
	}

	if (ret != 0) {
		ERR("wifi op failed:%d", ret);
		return;
	}

	if (is_on == 1) {
		quickpanel_setting_module_progress_mode_set(module, FLAG_ENABLE, FLAG_TURN_ON);
	} else {
		quickpanel_setting_module_progress_mode_set(module, FLAG_ENABLE, FLAG_TURN_OFF);
	}
	quickpanel_setting_module_icon_timer_add(module);
}

static int _register_module_event_handler(void *data)
{
	int ret = 0;

	ret = wifi_initialize();
	msgif(ret < 0, "[ERR] wifi_initialize()");

	ret = wifi_set_device_state_changed_cb(_wifi_state_changed_cb, data);
	if (ret != 0) {
		ERR("wifi cb register failed:%d", ret);
	} else {
		ERR("wifi op register done:%d", ret);
	}

	return QP_OK;
}

static int _unregister_module_event_handler(void *data)
{
	int ret = 0;

	ret = wifi_unset_device_state_changed_cb();
	if (ret != 0) {
		ERR("wifi cb unregister failed:%d", ret);
	} else {
		ERR("wifi op unregister done:%d", ret);
	}

	ret = wifi_deinitialize();
	msgif(ret != WIFI_ERROR_NONE, "Fail to deactivate Wi-Fi device");

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

static void _wifi_activated_cb(wifi_error_e result, void *user_data)
{
	if (result == WIFI_ERROR_NONE) {
		/* succeed to turn on Wi-Fi */
	} else {
		/* failed to turn on Wi-Fi */
	}
}

static void _wifi_deactivated_cb(wifi_error_e result, void* user_data)
{
	if (result == WIFI_ERROR_NONE) {
		/* succeed to turn off Wi-Fi */
	} else {
		/* failed to turn off Wi-Fi */
	}
}

static void _tethering_disabled_cb(tethering_error_e error, tethering_type_e type, tethering_disabled_cause_e code, void *data)
{
	int ret;
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	tethering_h th = (tethering_h)(module->loader->extra_handler_1);
	retif(th == NULL, , "th is NULL");
	tethering_destroy(th);
	module->loader->extra_handler_1 = NULL;

	if (error != TETHERING_ERROR_NONE) {
		/* failed to disable wifi tethering */
		return;
	}

	ret = wifi_activate_with_wifi_picker_tested(_wifi_activated_cb, NULL);
	if (ret != WIFI_ERROR_NONE) {
		ERR("Fail to activate Wi-Fi device [%d]\n", ret);
		return;
	}
}

static bool _tethering_disable(tethering_type_e type, void *data)
{
	tethering_h th = NULL;
	tethering_error_e ret = TETHERING_ERROR_NONE;
	QP_Module_Setting *module = (QP_Module_Setting *)data;

	retif(module == NULL, false, "Invalid parameter!");

	/* disable wifi tethering */
	ret = tethering_create(&th);
	if (ret != TETHERING_ERROR_NONE) {
		/* failed to create tethering handle */
		return false;
	}

	ret = tethering_set_disabled_cb(th, type,
			_tethering_disabled_cb, module);
	if (ret != TETHERING_ERROR_NONE) {
		/* failed to set disabled callback */
		tethering_destroy(th);
		return false;
	}

	module->loader->extra_handler_1 = th;
	quickpanel_setting_module_progress_mode_set(module, FLAG_ENABLE, FLAG_TURN_ON);
	quickpanel_setting_module_icon_timer_add(module);

	ret = tethering_disable(th, type);
	if (ret != TETHERING_ERROR_NONE) {
		ERR("fail to tethering_disable()");
		return false;
	}

	return true;
}

static void _tethering_wifi_reply_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
	QP_Module_Setting *module = (QP_Module_Setting *) user_data;
	char *resp_type = NULL;

	DBG("Result[%d]", result);
	if (result == APP_CONTROL_RESULT_SUCCEEDED && reply) {
		app_control_get_extra_data(reply, "_SYSPOPUP_RESP_", &resp_type);
		if (resp_type) {
			DBG("Response[%s]", resp_type);
			if (!strcmp("RESP_TETHERING_TYPE_WIFI_OFF", resp_type)) {
				_tethering_disable(TETHERING_TYPE_WIFI, user_data);
			}

			free(resp_type);
		}
	}

	_reset_icon(module);
}

static void _tethering_off_popup(Evas_Object *win, void *data, int type, const char *popup_txt)
{
	app_control_h service = NULL;
	int ret = APP_CONTROL_ERROR_NONE;

	ret = app_control_create(&service);
	if (ret != APP_CONTROL_ERROR_NONE) {
		ERR("Failed to create app_control[%d]", ret);
		return;
	}

	app_control_add_extra_data(service, "_SYSPOPUP_TITLE_", "mobileap");
	if (type == TETHERING_TYPE_WIFI) {
		app_control_add_extra_data(service, "_SYSPOPUP_CONTENT_", "TETHERING_TYPE_WIFI");
	} else {
		app_control_add_extra_data(service, "_SYSPOPUP_CONTENT_", "TETHERING_TYPE_WIFI_AP");
	}
	app_control_add_extra_data(service, "_SYSPOPUP_TYPE_", "popup_user_resp");
	app_control_add_extra_data(service, "_AP_NAME_", "none");

	app_control_set_app_id(service, "net.netpopup");

	ret = app_control_send_launch_request(service, _tethering_wifi_reply_cb, data);
	if (ret != APP_CONTROL_ERROR_NONE) {
		ERR("Failed to send launch request[%d]", ret);
	}

	app_control_destroy(service);
}

static int _wifi_on(void *data, const char *popup_txt)
{
	int ret = -1;
	struct appdata *ad = quickpanel_get_app_data();
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(ad == NULL, ret, "Invalid parameter!");
	retif(module == NULL, ret, "Invalid parameter!");

	/* Check wifi tethering status */
	if (tethering_is_enabled(NULL, TETHERING_TYPE_WIFI)) {
		_tethering_off_popup(ad->win, data, TETHERING_TYPE_WIFI, popup_txt);
		return -1;
	}

	ret = wifi_activate_with_wifi_picker_tested(_wifi_activated_cb, NULL);
	if (ret != WIFI_ERROR_NONE) {
		ERR("Fail to activate Wi-Fi device [%d]\n", ret);
		return -1;
	}

	return 0;
}

static int _wifi_off(void)
{
	int ret;

	ret = wifi_deactivate(_wifi_deactivated_cb, NULL);
	if (ret != WIFI_ERROR_NONE) {
		ERR("Fail to activate Wi-Fi device [%d]\n", ret);
		return -1;
	}

	return 0;
}

static int _wifi_is_on(bool *is_on)
{
	int ret;

	ret = wifi_is_activated(is_on);
	if (ret != WIFI_ERROR_NONE) {
		ERR("Fail to get Wi-Fi device status [%d]\n", ret);
		return -1;
	}

	return 0;
}

/*
   Set Wi-Fi status changed callback
   - needs to update your Wi-Fi status.
 */
static void _wifi_state_changed_cb(wifi_device_state_e state, void *user_data)
{
	ERR("state:%d", state);

	if (state == WIFI_DEVICE_STATE_ACTIVATED) {
		/* Wi-Fi is activated(On) - change your Wi-Fi status */
		_status_update(user_data, state, FLAG_VALUE_VOID);
	} else if (state == WIFI_DEVICE_STATE_DEACTIVATED) {
		/* Wi-Fi is deactivated(Off) - change your Wi-Fi status */
		_status_update(user_data, state, FLAG_VALUE_VOID);
	} else {
		/* Ignore */
	}
}

static void _reset_icon(QP_Module_Setting *module) {
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
		ret = _wifi_on(module, _("IDS_WIFI_POP_BOTH_WI_FI_AND_MOBILE_AP_CANNOT_BE_ACTIVATED_AT_THE_SAME_TIME_DEACTIVATE_MOBILE_AP_Q"));

		if (ret == 0) {
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
		ret = _wifi_off();

		if (ret == 0) {
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

QP_Module_Setting wifi = {
	.name 				= "wifi",
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
