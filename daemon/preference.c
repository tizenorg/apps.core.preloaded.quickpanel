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
#include <fcntl.h>
#include <unistd.h>

#include <iniparser.h>
#include <Elementary.h>

#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "preference.h"
#include "common.h"


#include "quickpanel-ui.h"

#define FILE_PREFERENCE DATADIR_RW"/preference.ini"

static const char *_default_preference_get(const char *key)
{
	retif(key == NULL, NULL, "invalid parameter");

	if (strcmp(key, PREF_BRIGHTNESS) == 0) {
		return "OFF";
	} else if (strcmp(key, PREF_QUICKSETTING_ORDER) == 0){
		return "wifi,gps,sound,rotate,bluetooth,mobile_data,assisitvelight,u_power_saving,wifi_hotspot,flightmode";
	} else if (strcmp(key, PREF_QUICKSETTING_FEATURED_NUM) == 0){
		return "11";
	} else if (strcmp(key, PREF_SHORTCUT_ENABLE) == 0){
		return "ON";
	} else if (strcmp(key, PREF_SHORTCUT_EARPHONE_ORDER) == 0){
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

static inline int _is_file_exist(void)
{

	if (access(FILE_PREFERENCE, O_RDWR) == 0) {
		return 1;
	}

	return 0;
}

static void _default_file_create(void)
{
	FILE	*fp = NULL ;

	fp = fopen(FILE_PREFERENCE, "w");
	retif(fp == NULL, , "fatal:failed to create preference file");

	fprintf(fp, "\n\
			[%s]\n\
			%s = %s ;\n\
			%s = %s ;\n\
			%s = %s ;\n\
			%s = %s ;\n\
			%s = %s ;\n\
			\n"
			, PREF_SECTION
			, PREF_BRIGHTNESS_KEY, _default_preference_get(PREF_BRIGHTNESS)
			, PREF_QUICKSETTING_ORDER_KEY, _default_preference_get(PREF_QUICKSETTING_ORDER)
			, PREF_QUICKSETTING_FEATURED_NUM_KEY, _default_preference_get(PREF_QUICKSETTING_FEATURED_NUM)
			, PREF_SHORTCUT_ENABLE_KEY, _default_preference_get(PREF_SHORTCUT_ENABLE)
			, PREF_SHORTCUT_EARPHONE_ORDER_KEY, _default_preference_get(PREF_SHORTCUT_EARPHONE_ORDER)
		   );

	fclose(fp);
}

HAPI int quickpanel_preference_get(const char *key, char *value)
{
	int ret = QP_OK;
	dictionary	*ini = NULL;
	const char *value_r = NULL;
	retif(key == NULL, QP_FAIL, "Invalid parameter!");
	retif(value == NULL, QP_FAIL, "Invalid parameter!");

	ini = iniparser_load(FILE_PREFERENCE);
	if (ini == NULL) {
		DBG("failed to load ini file");
		_default_file_create();
		value_r = _default_preference_get(key);
		goto END;
	}

#if defined(WINSYS_X11)
	value_r = iniparser_getstr(ini, key);
	if (value_r == NULL) {
		value_r = _default_preference_get(key);
		if (_key_validation_check(key) == 1) {
			_default_file_create();
		}
		goto END;
	} else {
		DBG("get:[%s]", value_r);
	}
#endif

END:
	if (value_r != NULL) {
		strcpy(value, value_r);
		ret = QP_OK;
	}

	if (ini != NULL) {
		iniparser_freedict(ini);
	}

	return ret;
}

HAPI const char *quickpanel_preference_default_get(const char *key)
{
	retif(key == NULL, NULL, "Invalid parameter!");

	return _default_preference_get(key);
}

HAPI int quickpanel_preference_set(const char *key, char *value)
{
	int ret = QP_FAIL;
	FILE *fp = NULL;
	dictionary	*ini = NULL;
	retif(key == NULL, QP_FAIL, "Invalid parameter!");
	retif(value == NULL, QP_FAIL, "Invalid parameter!");

	if (_is_file_exist() == 0) {
		_default_file_create();
	}

	ini = iniparser_load(FILE_PREFERENCE);
	retif(ini == NULL, QP_FAIL, "failed to load ini file");
	
#if defined(WINSYS_X11)
	if (iniparser_setstr(ini, (char *)key, value) == 0) {
		ret = QP_OK;
	} else {
		ERR("failed to write %s=%s", key, value);
	}
#endif

	fp = fopen(FILE_PREFERENCE, "w");
	if (fp != NULL) {
		iniparser_dump_ini(ini, fp);
		fclose(fp);
	}

	iniparser_freedict(ini);

	return ret;
}
