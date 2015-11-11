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
#include <vconf.h>
#include <syspopup_caller.h>
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

#define BUTTON_LABEL _("IDS_QP_BUTTON2_U_POWER_NSAVING_ABB")
#define BUTTON_ICON_NORMAL "quick_icon_ultra_power_saving.png"
#define BUTTON_ICON_HIGHLIGHT NULL
#define BUTTON_ICON_DIM NULL
#define PACKAGE_SETTING_MENU "setting-powersaving-efl"
#define SYSPOPUP_NAME "mode-syspopup"

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
		bundle_add(kb, "power_saving", "upsm");
		quickpanel_setting_icon_handler_longpress(PACKAGE_SETTING_MENU, kb);
		bundle_free(kb);
	} else {
		ERR("failed to create the bunlde");
	}
#endif
}

#if 0
static int _is_need_to_show_disclaimer(void) {
	int ret = 0, status = 0;

	ret = vconf_get_bool(VCONFKEY_SETAPPL_UPSM_DO_NOT_SHOW_DISCLAIMER, &status);
	if (ret == 0 && status) {
		return 0;
	}

	return 1;
}
#endif

static void _turn_on(int is_on)
{
	bundle *b = NULL;

	//if(_is_need_to_show_disclaimer() == 1)
	{
		b = bundle_create();
		if (b != NULL) {
			bundle_add(b, "viewtype", "disc");

			quickpanel_setting_icon_handler_longpress(PACKAGE_SETTING_MENU, b);
			bundle_free(b);
		}
	}
	/*else
	  {
	  b = bundle_create();
	  if (b != NULL) {
	  if (is_on) {
	  bundle_add(b, "_MODE_SYSTEM_POPUP_TYPE_", "POPUP_EMERGENCY_PSMODE_SETTING");
	  } else {
	  bundle_add(b, "_MODE_SYSTEM_POPUP_TYPE_", "POPUP_NORMAL_PSMODE");
	  }
	  syspopup_launch(SYSPOPUP_NAME, b);
	  bundle_free(b);
	  } else {
	  ERR("failed to create a bundle");
	  }
	  }*/
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
	int status = 0;
	retif(module == NULL, , "Invalid parameter!");

	ret = vconf_get_int(VCONFKEY_SETAPPL_PSMODE, &status);
	msgif(ret != 0, "fail to get VCONFKEY_SETAPPL_PSMODE:%d", ret);

	if (status == SETTING_PSMODE_EMERGENCY) {
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
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_ON) {
		_turn_on(0);
	} else {
		_turn_on(1);
	}
}

static void _powersave_vconf_cb(keynode_t *node, void *data)
{
	_status_update(data, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
}

static int _register_module_event_handler(void *data)
{
	int ret = 0;

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_PSMODE,
			_powersave_vconf_cb, data);
	msgif(ret != 0, "failed to notify key(VCONFKEY_SETAPPL_PSMODE) : %d", ret);

	return QP_OK;
}

static int _unregister_module_event_handler(void *data)
{
	int ret = 0;

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_PSMODE,
			_powersave_vconf_cb);
	msgif(ret != 0, "failed to ignore key(VCONFKEY_SETAPPL_PSMODE) : %d", ret);

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

QP_Module_Setting u_power_saving = {
	.name 				= "u_power_saving",
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
