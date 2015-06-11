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


#include <app.h>
#include <device/led.h>
#include <vconf.h>

#include "common.h"
#include "quickpanel-ui.h"
#include "settings.h"
#include "setting_utils.h"
#include "setting_module_api.h"


#define E_DATA_POPUP_MODULE_ITEM "mobule_item"
#define BUTTON_LABEL _("IDS_ST_BUTTON2_TORCH_ABB")
#define BUTTON_ICON_NORMAL "quick_icon_torch.png"

static void _status_update(QP_Module_Setting *module, int light_status, int flag_extra_2);

static const char *_label_get(void)
{
	return BUTTON_LABEL;
}

static void _on_vconf_assetive_light_changed(keynode_t *node, void *user_data)
{
	Eina_Bool mode = EINA_FALSE;

	if (!node) {
		ERR("node == NULL");
		return;
	}

#ifdef HAVE_X
	mode = node->value.b;
#endif

	quickpanel_setting_module_icon_state_set(user_data, mode);
	_status_update(user_data, mode, FLAG_VALUE_VOID);
}

static int _init(void *data)
{
	vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TORCH_LIGHT, _on_vconf_assetive_light_changed, data);

	return QP_OK;
}

static void _set_assetive_light_mode(Eina_Bool mode)
{
	int max_brightness;
	int ret = device_flash_get_max_brightness(&max_brightness);

	if (ret != 0) {
		ERR("TORCH LIGHT CHANGE: ret != 0 -> %d", ret);
		return;
	} else {
		ERR("TORCH LIGHT OK[%d]", max_brightness);
	}

	int ret_set = -1;

	if(mode == EINA_TRUE) {
		ret_set = device_flash_set_brightness(max_brightness);
		if (ret_set != 0) {
			ERR("Failed to set brightness(%d)[%d]", max_brightness, ret_set);
		} else {
			if (vconf_set_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TORCH_LIGHT, 1) < 0) {
				ERR("Failed to set tourch light vconf key");
			}
		}
	} else {
		ret_set = device_flash_set_brightness(0);
		if (ret_set != 0) {
			ERR("Failed to set brightness(0)[%d]", ret_set);
		} else {
			if (vconf_set_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TORCH_LIGHT, 0) < 0) {
				ERR("Failed to set tourch light vconf key");
			}
		}
	}
}

static void _view_update(Evas_Object *view, int state, int flag_extra_1, int flag_extra_2)
{
	Evas_Object *image = NULL;
	const char *icon_path = NULL;

	quickpanel_setting_icon_state_set(view, state);
	icon_path = BUTTON_ICON_NORMAL;
	image = quickpanel_setting_icon_image_new(view, icon_path);
	quickpanel_setting_icon_content_set(view, image);
	quickpanel_setting_icon_text_set(view, BUTTON_LABEL, state);
}

static void _status_update(QP_Module_Setting *module, int light_status, int flag_extra_2)
{
	int ret = -1;
	retif(module == NULL, , "Invalid parameter!");

	int brightness = 0;
	ret = device_flash_get_brightness(&brightness);
	if (ret != 0) {
		ERR("Failed to get brightness[%d]", ret);
	}

	if (brightness > 0) {
		quickpanel_setting_module_icon_state_set(module, ICON_VIEW_STATE_ON);
	} else {
		quickpanel_setting_module_icon_state_set(module, ICON_VIEW_STATE_OFF);
	}

	quickpanel_setting_module_icon_view_update(module,
			quickpanel_setting_module_icon_state_get(module), FLAG_VALUE_VOID);
}

static void _mouse_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	if (quickpanel_setting_module_icon_state_get(module) == ICON_VIEW_STATE_ON) {
		_set_assetive_light_mode(FLAG_TURN_OFF);
	} else {
		_set_assetive_light_mode(FLAG_TURN_ON);
	}

	_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
}

static void _lang_changed(void *data)
{
	QP_Module_Setting *module = (QP_Module_Setting *) data;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_icon_view_update_text(module);
}

static void _refresh(void *data)
{
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_icon_view_update_text(module);
}

static int _fini(void *data)
{
	return QP_OK;
}

QP_Module_Setting assistive_light =
{
	.name				= "assisitvelight",
	.init				= _init,
	.fini				= _fini,
	.lang_changed 		= _lang_changed,
	.refresh			= _refresh,
	.icon_get			= NULL,
	.label_get			= _label_get,
	.supported_get		= NULL,
	.view_update		= _view_update,
	.status_update		= _status_update,
	.handler_longpress  = NULL,
	.handler_press		= _mouse_clicked_cb,
};

