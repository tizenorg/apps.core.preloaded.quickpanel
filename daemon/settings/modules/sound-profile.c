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
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <feedback.h>
#include <sound_manager.h>
#include <E_DBus.h>

#include "common_uic.h"
#include "common.h"
#include "quickpanel-ui.h"
#include "settings.h"
#include "setting_utils.h"
#include "setting_module_api.h"
#include "settings_icon_common.h"

#define BUTTON_LABEL _("IDS_QP_BUTTON2_SOUND_ABB")
#define BUTTON_ICON_SND_NORMAL "quick_icon_sn_vf.png"
#define BUTTON_ICON_SND_HIGHLIGHT "quick_icon_sn_vf.png"
#define BUTTON_ICON_MUTE_NORMAL "quick_icon_sf_vf.png"
#define BUTTON_ICON_VIB_HIGHLIGHT "quick_icon_sf_vn.png"
#define PACKAGE_SETTING_MENU "setting-profile-efl"
#define SAM_LOG_FEATURE_SOUND "ST0C"

static int g_check_press = 0;

static void _mouse_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);

static const char *_label_get(void)
{
	return BUTTON_LABEL;
}

static const char *_icon_get(qp_setting_icon_image_type type)
{
	if (type == QP_SETTING_ICON_NORMAL) {
		return BUTTON_ICON_SND_NORMAL;
	} else if (type == QP_SETTING_ICON_HIGHLIGHT) {
		return BUTTON_ICON_SND_HIGHLIGHT;
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

static void _play_snd_job_cb(void *data)
{
	if (feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_SILENT_OFF) != FEEDBACK_ERROR_NONE) {
		ERR("failed to play a sound");
	}
}

static void _play_vib_job_cb(void *data)
{
	if (feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_VIBRATION_ON) != FEEDBACK_ERROR_NONE) {
		ERR("failed to play a vibration");
	}
}

static void _view_update(Evas_Object *view, int state, int flag_extra_1, int flag_extra_2)
{
	int icon_state;
	int ret = -1;
	int sound_status = 1;
	int vibration_status = 1;
	Evas_Object *image = NULL;
	const char *text = NULL;
	const char *img_path = NULL;

	retif(view == NULL, , "Invalid parameter!");

	/* Get sound status */
	ret = vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &sound_status);
	/* If fail, set sound on status */
	if (ret != 0) {
		ERR("failed get VCONFKEY_SETAPPL_SOUND_STATUS_BOOL:%d", ret);
		sound_status = 1;
	}

	/* Get vibration status */
	ret = vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL,
			&vibration_status);
	/* If fail, set vibration on status */
	if (ret != 0) {
		ERR("failed get VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL:%d", ret);
		vibration_status = 1;
	}

	INFO("sound : %d, vibration : %d", sound_status, vibration_status);

	if (sound_status == 1 && vibration_status == 1) {
		/* Sound & vibration profile */
		icon_state = ICON_VIEW_STATE_ON;
		text = _("IDS_QP_BUTTON2_SOUND_ABB");
		img_path = BUTTON_ICON_SND_HIGHLIGHT;
	} else if (sound_status == 0 && vibration_status == 1) {
		/* Vibration profile */
		icon_state = ICON_VIEW_STATE_ON;
		text = _("IDS_QP_BUTTON2_VIBRATE");
		img_path = BUTTON_ICON_VIB_HIGHLIGHT;
	} else if (sound_status == 1 && vibration_status == 0) {
		/*  Sound profile */
		icon_state = ICON_VIEW_STATE_ON;
		text = _("IDS_QP_BUTTON2_SOUND_ABB");
		img_path = BUTTON_ICON_SND_HIGHLIGHT;
	} else {
		/*  Mute profile */
		icon_state = ICON_VIEW_STATE_OFF;
		text = _("IDS_QP_BUTTON2_MUTE_ABB");
		img_path = BUTTON_ICON_MUTE_NORMAL;
	}

	quickpanel_setting_icon_state_set(view, icon_state);
	quickpanel_setting_icon_text_set(view, text, icon_state);
	image = quickpanel_setting_icon_image_new(view, img_path);
	quickpanel_setting_icon_content_set(view, image);

	if (quickpanel_uic_is_opened() && g_check_press) {
		if (sound_status == 1 && vibration_status == 0) {
			ecore_job_add(_play_snd_job_cb, NULL);
			g_check_press = 0;
		} else if (sound_status == 0 && vibration_status == 1) {
			ecore_job_add(_play_vib_job_cb, NULL);
			g_check_press = 0;
		}
	}
}

static void _status_update(QP_Module_Setting *module, int flag_extra_1, int flag_extra_2)
{
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_icon_view_update(module,
			quickpanel_setting_module_icon_state_get(module),
			FLAG_VALUE_VOID);
}

static void _mouse_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	int ret = 0;
	int sound_status = 1;
	int vibration_status = 1;
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	/* Get sound profile */
	ret = vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL,
			&sound_status);
	retif(ret != 0, , "failed to get sound status(%d)", ret);

	ret = vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL,
			&vibration_status);
	retif(ret != 0, ,"failed to get vibration status(%d)", ret);

	INFO("sound : %d, vibration : %d", sound_status, vibration_status);

	g_check_press = 1;

	if (sound_status == 1 && vibration_status == 1) {
		ret = vconf_set_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, 0);
		msgif(ret != 0, "failed set VCONFKEY_SETAPPL_SOUND_STATUS_BOOL:%d", ret);
	} else  if (sound_status == 1 && vibration_status == 0) {
		ret = vconf_set_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, 1);
		msgif(ret != 0, "failed set VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL:%d", ret);
		ret = vconf_set_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, 0);
		msgif(ret != 0, "failed set VCONFKEY_SETAPPL_SOUND_STATUS_BOOL:%d", ret);
	} else if (sound_status == 0 && vibration_status == 1) {
		ret = vconf_set_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, 0);
		msgif(ret != 0, "failed set VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL:%d", ret);
		/*insert log for mute mode on state */
		//_log_manager_insert_log(SAM_LOG_FEATURE_SOUND, "MUTE", NULL);
	} else if (sound_status == 0 && vibration_status == 0) {
		ret = vconf_set_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, 1);
		msgif(ret != 0, "failed set VCONFKEY_SETAPPL_SOUND_STATUS_BOOL:%d", ret);
	}
}

static void _soundprofile_vconf_cb(keynode_t *node, void *data)
{
	_status_update(data, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
}

static int _register_module_event_handler(void *data)
{
	int ret = 0;

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL,
			_soundprofile_vconf_cb, data);
	msgif(ret != 0,
			"failed to vconf_notify_key_changed [%s] -[%d]",
			VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, ret);

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL,
			_soundprofile_vconf_cb, data);
	msgif(ret != 0,
			"failed to vconf_notify_key_changed [%s] -[%d]",
			VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, ret);

	return QP_OK;
}

static int _unregister_module_event_handler(void *data)
{
	int ret = 0;

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL,
			_soundprofile_vconf_cb);
	msgif(ret != 0,
			"failed to vconf_ignore_key_changed [%s] -[%d]",
			VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, ret);

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL,
			_soundprofile_vconf_cb);
	msgif(ret != 0,
			"failed to vconf_ignore_key_changed [%s] -[%d]",
			VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, ret);

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
	_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
}

static void _refresh(void *data)
{
	QP_Module_Setting *module = (QP_Module_Setting *)data;
	retif(module == NULL, , "Invalid parameter!");

	quickpanel_setting_module_icon_view_update_text(module);
	_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
}


QP_Module_Setting sound = {
	.name 				= "sound",
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
	.is_disable_feedback = 1,
};
