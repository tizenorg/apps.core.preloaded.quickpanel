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


#include <stdio.h>
#include <app_preference.h>
#include <Elementary.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>
#include <app_common.h>

#include "preference.h"
#include "common.h"
#include "quickpanel-ui.h"


static const char *_default_preference_get(const char *key)
{
	retif(key == NULL, NULL, "invalid parameter");

	if (strcmp(key, PREF_BRIGHTNESS) == 0) {
		return "OFF";
	} else if (strcmp(key, PREF_QUICKSETTING_ORDER) == 0) {
		return "wifi,gps,sound,rotate,bluetooth,mobile_data,assisitvelight,u_power_saving,wifi_hotspot,flightmode";
	} else if (strcmp(key, PREF_QUICKSETTING_FEATURED_NUM) == 0) {
		return "11";
	} else if (strcmp(key, PREF_SHORTCUT_ENABLE) == 0) {
		return "ON";
	} else if (strcmp(key, PREF_SHORTCUT_EARPHONE_ORDER) == 0) {
		return "org.tizen.music-player,org.tizen.videos,org.tizen.phone,srfxzv8GKR.YouTube,org.tizen.voicerecorder";
	}

	return NULL;
}

static inline int _key_validation_check(const char *key)
{
	if (strcmp(key, PREF_BRIGHTNESS) == 0) {
		return 1;
	} else if (strcmp(key, PREF_QUICKSETTING_ORDER) == 0){
		return 1;
	} else if (strcmp(key, PREF_QUICKSETTING_FEATURED_NUM) == 0){
		return 1;
	} else if (strcmp(key, PREF_SHORTCUT_ENABLE) == 0){
		return 1;
	} else if (strcmp(key, PREF_SHORTCUT_EARPHONE_ORDER) == 0){
		return 1;
	}

	return 0;
}

HAPI int quickpanel_preference_get(const char *key, char **value)
{
	bool existing = false;

	retif(key == NULL, QP_FAIL, "Invalid parameter!");
	retif(value == NULL, QP_FAIL, "Invalid parameter!");

	if (preference_is_existing(key, &existing) == PREFERENCE_ERROR_NONE && !existing
		&& _key_validation_check(key)) {
		DBG("preference does not exist");
		/* Create preference */
		if (preference_set_string(PREF_BRIGHTNESS, _default_preference_get(PREF_BRIGHTNESS)) != PREFERENCE_ERROR_NONE) {
			DBG("preference set error %s", PREF_BRIGHTNESS);
			return QP_FAIL;
		}

		if (preference_set_string(PREF_QUICKSETTING_ORDER, _default_preference_get(PREF_QUICKSETTING_ORDER)) != PREFERENCE_ERROR_NONE) {
			DBG("preference set error %s", PREF_QUICKSETTING_ORDER);
			return QP_FAIL;
		}

		if (preference_set_string(PREF_QUICKSETTING_FEATURED_NUM, _default_preference_get(PREF_QUICKSETTING_FEATURED_NUM)) != PREFERENCE_ERROR_NONE) {
			DBG("preference set error %s", PREF_QUICKSETTING_FEATURED_NUM);
			return QP_FAIL;
		}

		if (preference_set_string(PREF_SHORTCUT_ENABLE, _default_preference_get(PREF_SHORTCUT_ENABLE)) != PREFERENCE_ERROR_NONE) {
			DBG("preference set error %s", PREF_SHORTCUT_ENABLE);
			return QP_FAIL;
		}

		if (preference_set_string(PREF_SHORTCUT_EARPHONE_ORDER, _default_preference_get(PREF_SHORTCUT_EARPHONE_ORDER)) != PREFERENCE_ERROR_NONE) {
			DBG("preference set error %s", PREF_SHORTCUT_EARPHONE_ORDER);
			return QP_FAIL;
		}
	}

	if ( preference_get_string(key, value) != PREFERENCE_ERROR_NONE)  {
		DBG("preference_get_string error : key(%s)", key);
		return QP_FAIL;
	}

	return QP_OK;
}

HAPI const char *quickpanel_preference_default_get(const char *key)
{
	retif(key == NULL, NULL, "Invalid parameter!");

	return _default_preference_get(key);
}

HAPI int quickpanel_preference_set(const char *key, char *value)
{
	retif(key == NULL, QP_FAIL, "Invalid parameter!");
	retif(value == NULL, QP_FAIL, "Invalid parameter!");

	if (preference_set_string(key, value) == PREFERENCE_ERROR_NONE) {
		DBG("quickpanel_preference_set  key[%s] value [%s]", key, value);
	} else {
		DBG("quickpanel_preference_set  failed key[%s]", key, value);
		return QP_FAIL;
	}

	return QP_OK;
}
