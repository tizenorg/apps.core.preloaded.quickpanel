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


#include <stdlib.h>
#include <glib.h>
#include <Elementary.h>

#include <vconf.h>
#include <notification.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "common.h"
#include "quickpanel-ui.h"
#include "quickpanel_def.h"
#include "modules.h"
#include "settings.h"
#include "setting_utils.h"
#include "settings_ipc.h"
#include "pager.h"
#include "pager_common.h"
#include "preference.h"

#ifdef QP_SCREENREADER_ENABLE
#include "accessibility.h"
#endif

#ifdef QP_EMERGENCY_MODE_ENABLE
#include "emergency_mode.h"
#endif

static int quickpanel_settings_init(void *data);
static int quickpanel_settings_fini(void *data);
static int quickpanel_settings_suspend(void *data);
static int quickpanel_settings_resume(void *data);
static void quickpanel_settings_lang_changed(void *data);
static void quickpanel_settings_reflesh(void *data);
static Eina_Bool _module_is_enabled(QP_Module_Setting *module);

extern QP_Module_Setting wifi;
extern QP_Module_Setting gps;
extern QP_Module_Setting bluetooth;
extern QP_Module_Setting sound;
extern QP_Module_Setting rotate;

QP_Module settings = {
	.name = "settings",
	.init = quickpanel_settings_init,
	.fini = quickpanel_settings_fini,
	.suspend = quickpanel_settings_suspend,
	.resume = quickpanel_settings_resume,
	.lang_changed = quickpanel_settings_lang_changed,
	.refresh = quickpanel_settings_reflesh,
};

static struct _info {
	GHashTable *module_table;
	QP_Module_Setting *modules[];
} s_info = {
	.module_table = NULL,
	.modules = {
		&wifi,
		&gps,
		&sound,
		&rotate,
		&bluetooth,
		NULL,
	},
};

static void _module_init(QP_Module_Setting *module)
{
	if (module->init != NULL) {
		module->loader = (QP_Setting_Loaded_Item *)calloc(1, sizeof(QP_Setting_Loaded_Item));
		module->init(module);
		module->is_loaded = EINA_TRUE;
	}
}

static void _module_fini(QP_Module_Setting *module)
{
	if (module->fini != NULL) {
		module->fini(module);
		if (module->loader != NULL) {
			free(module->loader);
			module->loader = NULL;
			module->is_loaded = EINA_FALSE;
		}
	}
}

static int _module_count_get(void)
{
	int i, cnt = 0;

	for (i = 0; s_info.modules[i] != NULL; i++) {
		cnt++;
	}

	return cnt;
}

static QP_Module_Setting *_module_get_by_name(const char *name)
{
	retif(name == NULL, NULL, "invalid parameter");
	retif(s_info.module_table == NULL, NULL, "invalid parameter");

	return g_hash_table_lookup(s_info.module_table, name);
}

static Eina_Bool _module_is_enabled(QP_Module_Setting *module) {
	retif(module == NULL, EINA_FALSE, "invalid parameter");
	retif(module->name == NULL, EINA_FALSE, "invalid parameter");

	if (strcmp(module->name, MODULE_BLANK) == 0) {
		return EINA_FALSE;
	}
	if (module->supported_get) {
		if (module->supported_get() == 0)
			return EINA_FALSE;
	}
	return EINA_TRUE;
}

static char *_preference_get(const char *key)
{
	char line[PREF_LEN_VALUE_MAX + 1] = {0,};

	if (quickpanel_preference_get(key, line) == QP_OK) {
		DBG("quicksetting order from file:%s", line);
		return strdup(line);
	}

	return NULL;
}

static int quickpanel_settings_init(void *data)
{
	int i;
	int mod_count = 0;
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	mod_count = _module_count_get();
	if (s_info.module_table != NULL) {
		g_hash_table_remove_all(s_info.module_table);
		g_hash_table_destroy(s_info.module_table);
		s_info.module_table = NULL;
	}
	s_info.module_table = g_hash_table_new_full(g_str_hash, g_str_equal,
			(GDestroyNotify)g_free,
			NULL);
	if (s_info.module_table != NULL) {
		for (i = 0; i < mod_count; i++) {
			if (s_info.modules[i]->supported_get != NULL) {
				if (s_info.modules[i]->supported_get() == 0) {
					continue;
				}
			}

			if (s_info.modules[i]->init != NULL && s_info.modules[i]->name != NULL) {
				ERR("quickbutton %s is initialized", s_info.modules[i]->name);
				DBG("quickbutton %s is initialized", s_info.modules[i]->name);
				g_hash_table_insert(s_info.module_table,
						g_strdup(s_info.modules[i]->name),
						s_info.modules[i]);
				_module_init(s_info.modules[i]);
			}
		}
	} else {
		ERR("failed to create module has table");
		return QP_FAIL;
	}

	quickpanel_settings_ipc_init(ad);

	return QP_OK;
}

static int quickpanel_settings_fini(void *data)
{
	int i;
	int ret = 0;
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	quickpanel_settings_ipc_fini(ad);

	for (i = 0; s_info.modules[i] != NULL; i++) {
		if (_module_is_enabled(s_info.modules[i]) == EINA_TRUE) {
			_module_fini(s_info.modules[i]);
		}
	}

	if (s_info.module_table) {
		g_hash_table_remove_all(s_info.module_table);
		g_hash_table_destroy(s_info.module_table);
		s_info.module_table = NULL;
	}

	return ret;
}

static int quickpanel_settings_suspend(void *data)
{
	int i;
	int ret = 0;
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; s_info.modules[i] != NULL; i++) {
		if (_module_is_enabled(s_info.modules[i]) == EINA_TRUE) {
			if ((s_info.modules[i])->suspend != NULL) {
				(s_info.modules[i])->suspend(s_info.modules[i]);
			}
		}
	}

	return ret;
}

static int quickpanel_settings_resume(void *data)
{
	int i;
	int ret = 0;
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; s_info.modules[i] != NULL; i++) {
		if (_module_is_enabled(s_info.modules[i]) == EINA_TRUE) {
			if ((s_info.modules[i])->resume != NULL) {
				(s_info.modules[i])->resume(s_info.modules[i]);
			}
		}
	}

	return ret;
}

static void quickpanel_settings_lang_changed(void *data)
{
	int i;
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	for (i = 0; s_info.modules[i] != NULL; i++) {
		if (_module_is_enabled(s_info.modules[i]) == EINA_TRUE) {
			if ((s_info.modules[i])->lang_changed != NULL) {
				(s_info.modules[i])->lang_changed(s_info.modules[i]);
			}
		}
	}
}

static void quickpanel_settings_reflesh(void *data)
{
	int i;
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	for (i = 0; s_info.modules[i] != NULL; i++) {
		if (_module_is_enabled(s_info.modules[i]) == EINA_TRUE) {
			if ((s_info.modules[i])->refresh != NULL) {
				(s_info.modules[i])->refresh(s_info.modules[i]);
			}
		}
	}
}

HAPI int quickpanel_settings_featured_list_validation_check(char *order)
{
	int i = 0, is_valid = 0;
	int order_count = 0;
	gchar **order_split = NULL;
	QP_Module_Setting *mod = NULL;
	retif(order == NULL, is_valid, "Invalid parameter!");

	if (s_info.module_table == NULL) {
		return is_valid;
	}

	order_split = g_strsplit(order, ",", 0);

	if (order_split != NULL) {
		order_count = g_strv_length(order_split);

		if (order_count >= QP_SETTING_NUM_MINIMUM_ICON) {
			for (i = 0; i < order_count; i++) {
				mod = _module_get_by_name(order_split[i]);
				if (mod != NULL) {
					is_valid = 1;
				} else {
					is_valid = 0;
					break;
				}
			}
		}

		g_strfreev(order_split);
	}

	return is_valid;
}

HAPI void quickpanel_settings_featured_list_get(Eina_List **list)
{
	int i = 0, seq_count = 0;
	int num_featured = 0;
	int seq_added_count = 0;
	gchar **params = NULL;
	QP_Module_Setting *module = NULL;
	retif(list == NULL, , "invalid data.");
	char *sequence = _preference_get(PREF_QUICKSETTING_ORDER);
	const char *default_sequence = quickpanel_preference_default_get(PREF_QUICKSETTING_ORDER);

	char *num_featured_str =  _preference_get(PREF_QUICKSETTING_FEATURED_NUM);
	const char *default_num_featured_str = quickpanel_preference_default_get(PREF_QUICKSETTING_FEATURED_NUM);

	if (sequence != NULL) {
		params = g_strsplit(sequence, ",", 0);
		free(sequence);
	} else {
		params = g_strsplit(default_sequence, ",", 0);
	}

	if (num_featured_str != NULL) {
		num_featured = atoi(num_featured_str);
		free(num_featured_str);
	} else {
		if (default_num_featured_str != NULL) {
			num_featured = atoi(default_num_featured_str);
		} else {
			num_featured = QP_SETTING_NUM_TOTAL_ICON;
		}
	}

	if (params != NULL) {
		seq_count = g_strv_length(params);

		for (i = 0; i < seq_count; i++) {
			if (seq_added_count >= num_featured){
				break;
			}

			module = _module_get_by_name(params[i]);
			if (module != NULL) {
				if (_module_is_enabled(module) == EINA_TRUE) {
					*list = eina_list_append (*list, module);
					seq_added_count++;
				}
			}
		}

		g_strfreev(params);
	}
}

HAPI void quickpanel_settings_all_list_get(Eina_List **list)
{
	int i = 0, seq_count = 0;
	gchar **params = NULL;
	QP_Module_Setting *module = NULL;
	retif(list == NULL, , "invalid data.");
	char *sequence = _preference_get(PREF_QUICKSETTING_ORDER);
	const char *default_sequence = quickpanel_preference_default_get(PREF_QUICKSETTING_ORDER);

	if (sequence != NULL) {
		params = g_strsplit(sequence, ",", 0);
		free(sequence);
	} else if (default_sequence != NULL){
		params = g_strsplit(default_sequence, ",", 0);
	}

	if (params != NULL) {
		seq_count = g_strv_length(params);

		for (i = 0; i < seq_count; i++) {
			module = _module_get_by_name(params[i]);
			if (module != NULL) {
				if (_module_is_enabled(module) == EINA_TRUE) {
					*list = eina_list_append (*list, module);
				}
			}
		}

		g_strfreev(params);
	}
}

HAPI void quickpanel_setting_save_list_to_file(Eina_List *list, int num_featured)
{
	Eina_List *l;
	Eina_List *l_next;
	QP_Module_Setting *module = NULL;
	char buf[32] = {0,};
	retif(list == NULL, , "invalid parameter");

	int is_first = 1;

	char *base = NULL;
	char *temp = NULL;

	EINA_LIST_FOREACH_SAFE(list, l, l_next, module) {
		if (module == NULL){
			continue;
		}
		if (module->name == NULL) {
			continue;
		}
		if (_module_is_enabled(module) == EINA_FALSE) {
			continue;
		}

		if (is_first == 1) {
			base = g_strdup(module->name);
			is_first = 0;
		} else {
			temp = g_strconcat(base, ",", module->name, NULL);
			if (base != NULL) g_free(base);
			base = temp;
			temp = NULL;
		}
	}

	if (base != NULL) {
		if (quickpanel_preference_set(PREF_QUICKSETTING_ORDER, base) == QP_FAIL) {
			ERR("failed to write quicksetting order");
		}
		g_free(base);
		snprintf(buf, sizeof(buf) - 1, "%d", num_featured);
		if (quickpanel_preference_set(PREF_QUICKSETTING_FEATURED_NUM, buf) == QP_FAIL) {
			ERR("failed to write quicksetting featured num");
		}
	}
}

HAPI QP_Module_Setting *quickpanel_settings_module_get_by_name(const char *name)
{
	return _module_get_by_name(name);
}

HAPI int quickpanel_settings_module_count_get(void)
{
	return _module_count_get();
}
