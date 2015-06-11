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
#include <system_settings.h>
#include <bundle_internal.h>
#include "common.h"
#include "quickpanel-ui.h"
#include "settings.h"
#include "setting_utils.h"
#include "setting_module_api.h"


#define BUTTON_LABEL _("IDS_ST_BUTTON2_MOBILE_NDATA")
#define BUTTON_ICON_NORMAL "quick_icon_mobile_data.png"
#define BUTTON_ICON_HIGHLIGHT NULL
#define BUTTON_ICON_DIM NULL
#define PACKAGE_SETTING_MENU "setting-network-efl"
#define SYSPOPUP_NAME "mode-syspopup"

#ifndef VCONFKEY_SETAPPL_MOBILE_DATA_ON_REMINDER
#define VCONFKEY_SETAPPL_MOBILE_DATA_ON_REMINDER "db/setting/network/mobile_data_on_reminder"
#endif
#ifndef VCONFKEY_SETAPPL_MOBILE_DATA_OFF_REMINDER
#define VCONFKEY_SETAPPL_MOBILE_DATA_OFF_REMINDER "db/setting/network/mobile_data_off_reminder"
#endif



static int _is_simcard_inserted(void);
static int _is_in_flightmode(void);
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
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid data.");

	if (_is_in_flightmode() == 1) {
		quickpanel_setting_create_timeout_popup(ad->win, _("IDS_SCP_BODY_UNABLE_TO_CONNECT_TO_MOBILE_NETWORKS_WHILE_FLIGHT_MODE_IS_ENABLED_CONNECT_TO_A_WI_FI_NETWORK_INSTEAD_OR_DISABLE_FLIGHT_MODE_AND_TRY_AGAIN"));
		return;
	}

	if (_is_simcard_inserted() == 0) {
		quickpanel_setting_create_timeout_popup(ad->win, _("IDS_ST_BODY_INSERT_SIM_CARD_TO_ACCESS_NETWORK_SERVICES"));
		return;
	}

	quickpanel_setting_icon_handler_longpress(PACKAGE_SETTING_MENU, NULL);
#endif
}

static int _need_display_popup(int is_on)
{
	int ret = -1, status = 0;

	if (is_on == 1) {
		ret = vconf_get_bool(VCONFKEY_SETAPPL_MOBILE_DATA_ON_REMINDER,
				&status);
		msgif(ret != 0, "failed to get VCONFKEY_SETAPPL_MOBILE_DATA_ON_REMINDER %d %d", ret, is_on);
		if (ret == 0 && status == 1) {
			return 1;
		}
	} else {
		ret = vconf_get_bool(VCONFKEY_SETAPPL_MOBILE_DATA_OFF_REMINDER,
				&status);
		msgif(ret != 0, "failed to get VCONFKEY_SETAPPL_MOBILE_DATA_OFF_REMINDER %d %d", ret, is_on);
		if (ret == 0 && status == 1) {
			return 1;
		}
	}

	return 0;
}

static void _turn_on(int is_on)
{
	int ret = 0;

	if (is_on) {
		ret = system_settings_set_value_bool(SYSTEM_SETTINGS_KEY_3G_DATA_NETWORK_ENABLED, 1);
		msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to set SYSTEM_SETTINGS_KEY_3G_DATA_NETWORK_ENABLED %d %d", ret, is_on);
	} else {
		ret = system_settings_set_value_bool(SYSTEM_SETTINGS_KEY_3G_DATA_NETWORK_ENABLED, 0);
		msgif(ret != SYSTEM_SETTINGS_ERROR_NONE,"failed to set SYSTEM_SETTINGS_KEY_3G_DATA_NETWORK_ENABLED: %d %d", ret, is_on);
	}
}

static void _turn_on_with_popup(int is_on)
{
	bundle *b = NULL;
	b = bundle_create();
	if (b != NULL) {
		if (is_on) {
			bundle_add(b, "_MODE_SYSTEM_POPUP_TYPE_", "MODE_SYSTEM_MOBILEDATA_ON");
		} else {
			bundle_add(b, "_MODE_SYSTEM_POPUP_TYPE_", "MODE_SYSTEM_MOBILEDATA_OFF");
		}
		syspopup_launch(SYSPOPUP_NAME, b);
		bundle_free(b);
	} else {
		ERR("failed to create a bundle");
	}
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


static int _is_simcard_inserted(void)
{
	int ret_1 = QP_FAIL;
	int ret_2 = QP_FAIL;
	int sim_status_1 = VCONFKEY_TELEPHONY_SIM_UNKNOWN;
	int sim_status_2 = VCONFKEY_TELEPHONY_SIM_UNKNOWN;

	ret_1 = vconf_get_int(VCONFKEY_TELEPHONY_SIM_SLOT, &sim_status_1);
	msgif(ret_1 != QP_OK, "failed to get the VCONFKEY_TELEPHONY_SIM_SLOT : %d", ret_1);

	ret_2 = vconf_get_int(VCONFKEY_TELEPHONY_SIM_SLOT2, &sim_status_2);
	msgif(ret_2 != QP_OK, "failed to get the VCONFKEY_TELEPHONY_SIM_SLOT2 : %d", ret_2);

	INFO("MOBILE DATA SIM CARD: %d %d", sim_status_1, sim_status_2);

	if ((ret_1 == QP_OK && sim_status_1 == VCONFKEY_TELEPHONY_SIM_INSERTED) ||
		(ret_2 == QP_OK && sim_status_2 == VCONFKEY_TELEPHONY_SIM_INSERTED)) {
		return 1;
	}

	return 0;
}

static int _is_in_flightmode(void)
{
	int ret = QP_FAIL, flight_mode = 0;

#ifdef HAVE_X
	ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, &flight_mode);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to get the SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE : %d", ret);
#endif
	if (ret == QP_OK && flight_mode == 1) {
		return 1;
	}

	return 0;
}

static void _status_update(QP_Module_Setting *module, int flag_extra_1, int flag_extra_2)
{
	int ret = 0;
	int status = 0;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_icon_timer_del(module);

	if (quickpanel_uic_is_emul() == 1) {
		status = 1;
	} else if (_is_in_flightmode() == 1) {
		status = 0;
	} else if (_is_simcard_inserted() == 0) {
		status = 0;
	} else {
		ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_3G_DATA_NETWORK_ENABLED, &status);
		msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "fail to get SYSTEM_SETTINGS_KEY_3G_DATA_NETWORK_ENABLED:%d", ret);
	}

	if (status == 1) {
		quickpanel_setting_module_icon_state_set(module, ICON_VIEW_STATE_ON);
	} else {
		quickpanel_setting_module_icon_state_set(module, ICON_VIEW_STATE_OFF);
	}

	quickpanel_setting_module_icon_view_update(module,
			quickpanel_setting_module_icon_state_get(module),
			FLAG_VALUE_VOID);
}

static void _mouse_clicked_cb(void *data,
		Evas_Object *obj, const char *emission, const char *source)
{
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	if (quickpanel_setting_module_is_icon_clickable(module) == 0) {
		return;
	}

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid data.");

	if (quickpanel_uic_is_emul() == 1) {
		quickpanel_setting_create_timeout_popup(ad->win, _NOT_LOCALIZED("Unsupported."));
		return;
	}

	if (_is_in_flightmode() == 1) {
		quickpanel_setting_create_timeout_popup(ad->win, _("IDS_SCP_BODY_UNABLE_TO_CONNECT_TO_MOBILE_NETWORKS_WHILE_FLIGHT_MODE_IS_ENABLED_CONNECT_TO_A_WI_FI_NETWORK_INSTEAD_OR_DISABLE_FLIGHT_MODE_AND_TRY_AGAIN"));
		return;
	}

	if (_is_simcard_inserted() == 0) {
		quickpanel_setting_create_timeout_popup(ad->win, _("IDS_ST_BODY_INSERT_SIM_CARD_TO_ACCESS_NETWORK_SERVICES"));
		return;
	}

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_ON) {
		/* disable */
		if (_need_display_popup(0) == 1) {
			_turn_on_with_popup(0);
		} else {
			_turn_on(0);
		}
	} else {
		/* enable */
		if (_need_display_popup(1) == 1) {
			_turn_on_with_popup(1);
		} else {
			_turn_on(1);
		}
	}
}

static void _mobiledata_vconf_cb(keynode_t *node,
		void *data)
{
	_status_update(data, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
}

static int _register_module_event_handler(void *data)
{
	int ret = 0;

	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_3G_DATA_NETWORK_ENABLED, _mobiledata_vconf_cb, data);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to notify key(%s) : %d", SYSTEM_SETTINGS_KEY_3G_DATA_NETWORK_ENABLED, ret);

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,_mobiledata_vconf_cb, data);
	msgif(ret != 0, "failed to notify key(%s) : %d", VCONFKEY_TELEPHONY_SIM_SLOT, ret);
#ifdef HAVE_X
	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, _mobiledata_vconf_cb, data);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to notify key(%s) : %d", SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, ret);
#endif
	return QP_OK;
}

static int _unregister_module_event_handler(void *data)
{
	int ret = 0;

	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_3G_DATA_NETWORK_ENABLED);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to ignore key(%s) : %d", SYSTEM_SETTINGS_KEY_3G_DATA_NETWORK_ENABLED, ret);

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,_mobiledata_vconf_cb);
	msgif(ret != 0, "failed to ignore key(%s) : %d", VCONFKEY_TELEPHONY_SIM_SLOT, ret);

#ifdef HAVE_X
	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to ignore key(%s) : %d", SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, ret);
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
		ret = system_settings_set_value_bool(SYSTEM_SETTINGS_KEY_3G_DATA_NETWORK_ENABLED, 1);

		if (ret == SYSTEM_SETTINGS_ERROR_NONE) {
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
		ret = vconf_set_bool(VCONFKEY_3G_ENABLE, 0);

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

QP_Module_Setting mobile_data = {
	.name 				= "mobile_data",
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
