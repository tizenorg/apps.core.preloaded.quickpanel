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

#include <bluetooth.h>
#include <vconf.h>
#include <bluetooth_internal.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "common.h"
#include "quickpanel-ui.h"
#include "settings.h"
#include "setting_utils.h"
#include "setting_module_api.h"
#include "settings_icon_common.h"

#define BUTTON_LABEL _("IDS_ST_BUTTON2_BLUETOOTH_ABB")
#define BUTTON_ICON_NORMAL "quick_icon_bluetooth.png"
#define BUTTON_ICON_HIGHLIGHT NULL
#define BUTTON_ICON_DIM NULL
#define PACKAGE_SETTING_MENU "ug-bluetooth-efl"

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
	int ret = 0;
	bt_adapter_state_e adapter_state = BT_ADAPTER_DISABLED;
	retif(module == NULL, , "Invalid parameter!");

	ret = bt_adapter_get_state(&adapter_state);
	if (ret != BT_ERROR_NONE) {
		DBG("bt_adapter_get_state failed [%d]", ret);
	}

	if (adapter_state == BT_ADAPTER_ENABLED) {
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
	int ret;
	int is_on = 0;
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	if (quickpanel_setting_module_is_icon_clickable(module) == 0) {
		return;
	}

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_ON) {
		ret = bt_adapter_disable();
		retif(ret != BT_ERROR_NONE, , "failed to disable BT adapter");

		is_on = 0;
	} else {
		ret = bt_adapter_enable();
		retif(ret != BT_ERROR_NONE, , "failed to enable BT adapter");

		is_on = 1;
	}

	if (is_on == 1) {
		quickpanel_setting_module_progress_mode_set(module, FLAG_ENABLE, FLAG_TURN_ON);
	} else {
		quickpanel_setting_module_progress_mode_set(module, FLAG_ENABLE, FLAG_TURN_OFF);
	}
	quickpanel_setting_module_icon_timer_add(module);
}

static void _bluetooth_status_changed_cb(int result, bt_adapter_state_e adapter_state, void *user_data)
{
	QP_Module_Setting *module = (QP_Module_Setting *)user_data;
	retif(module == NULL, , "Invalid parameter!");

	INFO("bluetooth state : %d", adapter_state);
	quickpanel_setting_module_icon_timer_del(module);

	if (result != BT_ERROR_NONE) {
		ERR("BT adapter operation is failed");
		_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
		return;
	}

	_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
}

static int _register_module_event_handler(void *data)
{
	int ret = 0;

	ret = bt_initialize();
	msgif(ret != BT_ERROR_NONE, "bt_initialize failed");

	ret = bt_adapter_set_state_changed_cb(_bluetooth_status_changed_cb, data);
	msgif(ret != BT_ERROR_NONE, "bt_adapter_set_state_changed_cb failed");

	return QP_OK;
}

static int _unregister_module_event_handler(void *data)
{
	int ret = 0;

	ret = bt_adapter_unset_state_changed_cb();
	msgif(ret != BT_ERROR_NONE, "bt_adapter_unset_state_changed_cb failed");

	ret = bt_deinitialize();
	msgif(ret != BT_ERROR_NONE, "bt_deinitialize failed");

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

QP_Module_Setting bluetooth = {
	.name				= "bluetooth",
	.init				= _init,
	.fini				= _fini,
	.lang_changed		= _lang_changed,
	.refresh			= _refresh,
	.icon_get			= _icon_get,
	.label_get			= _label_get,
	.view_update		= _view_update,
	.status_update		= _status_update,
	.handler_longpress	= _long_press_cb,
	.handler_press		= _mouse_clicked_cb,
};
