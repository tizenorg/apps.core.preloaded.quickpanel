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
#include <app_common.h>

#include "preference.h"
#include "common.h"


#include "quickpanel-ui.h"

#define PREFERENCE_FILE_NAME "preference.ini"

static char *_get_preference_file_path()
{
	char *data_path = NULL;
	char file_path[MAX_FILE_PATH_LEN] = {0, };


	data_path = app_get_data_path();
	retif(data_path == NULL, NULL, "failed to app_get_data_path()");

	snprintf(file_path, sizeof(file_path), "%s%s", data_path, PREFERENCE_FILE_NAME);
	free(data_path);

	return strdup(file_path);
}

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
	char *file_path = _get_preference_file_path();
	retif(file_path == NULL, 0, "failed to _get_preference_file_path()");

	if (access(file_path, O_RDWR) == 0) {
		return 1;
	}

	if (file_path != NULL) {
		free(file_path);
	}

	return 0;
}

static void _default_file_create(void)
{
	FILE	*fp = NULL ;
	char *file_path = _get_preference_file_path();
	retif(file_path == NULL, , "failed to _get_preference_file_path()");

	fp = fopen(file_path, "w");
	retif(fp == NULL, , "fatal:failed to create preference file %s", file_path);

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

	if (file_path != NULL) {
		free(file_path);
	}
}

HAPI int quickpanel_preference_get(const char *key, char *value)
{
	int ret = QP_OK;
	dictionary	*ini = NULL;
	const char *value_r = NULL;
	char *file_path = NULL;
	retif(key == NULL, QP_FAIL, "Invalid parameter!");
	retif(value == NULL, QP_FAIL, "Invalid parameter!");

	file_path = _get_preference_file_path();
	retif(file_path == NULL, QP_FAIL, "failed to _get_preference_file_path()");

	ini = iniparser_load(file_path);
	if (ini == NULL) {
		DBG("failed to load ini file");
		_default_file_create();
		value_r = _default_preference_get(key);
		goto END;
	}

	value_r = iniparser_getstring(ini, key, NULL);
	if (value_r == NULL) {
		value_r = _default_preference_get(key);
		if (_key_validation_check(key) == 1) {
			_default_file_create();
		}
	} else {
		DBG("get:[%s]", value_r);
	}


END:
	if (value_r != NULL) {
		strncpy(value, value_r, strlen(value_r));
		ret = QP_OK;
	}

	if (ini != NULL) {
		iniparser_freedict(ini);
	}

	if (file_path != NULL) {
		free(file_path);
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

	char *file_path = _get_preference_file_path();
	retif(file_path == NULL, QP_FAIL, "failed to _get_preference_file_path()");

	ini = iniparser_load(file_path);
	retif(ini == NULL, QP_FAIL, "failed to load ini file");

	if (iniparser_set(ini, (char *)key, value) == 0) {
		ret = QP_OK;
	} else {
		ERR("failed to write %s=%s", key, value);
	}

	fp = fopen(file_path, "w");
	if (fp != NULL) {
		iniparser_dump_ini(ini, fp);
		fclose(fp);
	}

	iniparser_freedict(ini);

	if (file_path != NULL) {
		free(file_path);
	}

	return ret;
}



