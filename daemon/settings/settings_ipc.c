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
#include <E_DBus.h>
#include <glib.h>

#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "quickpanel_def.h"
#include "common.h"
#include "modules.h"
#include "settings.h"
#include "setting_utils.h"
#include "settings_view_all.h"
#include "settings_view_featured.h"

static struct _info {
	E_DBus_Signal_Handler *hdl_activity;
	E_DBus_Signal_Handler *hdl_editing;
} s_info = {
	.hdl_activity = NULL,
	.hdl_editing = NULL,
};

static void _handler_activity(void *data, DBusMessage *msg)
{
	int ret = 0;
	DBusError err;
	char *module = NULL;
	char *command = NULL;
	QP_Module_Setting *mod = NULL;
	retif(data == NULL || msg == NULL, , "Invalid parameter!");

	dbus_error_init(&err);
	ret = dbus_message_get_args(msg, &err,
			DBUS_TYPE_STRING, &module,
			DBUS_TYPE_STRING, &command,
			DBUS_TYPE_INVALID);
	retif(ret == 0, , "dbus_message_get_args error");
	retif(module == NULL, , "Failed to get module");
	retif(command == NULL, , "Failed to get command");

	if (dbus_error_is_set(&err)) {
		ERR("Dbus err: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	mod = quickpanel_settings_module_get_by_name(module);
	if (mod != NULL) {
		DBG("module:%s, command:%s", module, command);
		if (mod->handler_ipc != NULL) {
			if (mod->is_loaded == EINA_TRUE && mod->loader != NULL) {
				mod->handler_ipc(command, mod);
			} else {
				ERR("module:%s isn't loaded");
			}
		} else {
			ERR("module:%s don't have IPC handler");
		}
	} else {
		ERR("failed to lookup module:%s", module);
	}
}

static void _handler_editing(void *data, DBusMessage *msg)
{
	int i = 0;
	int ret = 0, is_error = 0;
	DBusError err;
	char *key = NULL;
	char *order = NULL;
	int num_featured = 0;
	int order_count = 0;
	gchar **order_split = NULL;
	Eina_List *list_active = NULL;
	QP_Module_Setting *mod = NULL;
	retif(data == NULL || msg == NULL, , "Invalid parameter!");

	dbus_error_init(&err);
	ret = dbus_message_get_args(msg, &err,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_STRING, &order,
			DBUS_TYPE_INT32, &num_featured,
			DBUS_TYPE_INVALID);
	retif(ret == 0, , "dbus_message_get_args error");
	retif(key == NULL, , "Failed to get key");
	retif(order == NULL, , "Failed to get value");

	if (dbus_error_is_set(&err)) {
		ERR("dbus err: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	if (strcmp(key, "quicksetting_order") == 0) {
		DBG("order:%s %d", order, num_featured);
		if (quickpanel_settings_featured_list_validation_check(order) == 1) {
			order_split = g_strsplit(order, ",", 0);
			if (order_split != NULL) {
				order_count = g_strv_length(order_split);
				DBG("count of quicksettings:%d", order_count);

				for (i = 0; i < order_count; i++) {
					mod = quickpanel_settings_module_get_by_name(order_split[i]);
					if (mod != NULL && mod->init != NULL) {
						list_active = eina_list_append (list_active, mod);
					} else {
						ERR("failed to get quicksetting:%s", order_split[i]);
						is_error = 1;
					}
				}

				if (is_error == 0) {
					if (list_active != NULL) {
						quickpanel_setting_view_featured_reload(list_active, num_featured);
						quickpanel_setting_view_all_reload(list_active);
						quickpanel_setting_save_list_to_file(list_active, num_featured);
						eina_list_free(list_active);
					}
				}
				g_strfreev(order_split);
			}
		} else {
			ERR("setting order validation check failed, igonore this signal");
		}
	}
}

static void _settings_ipc_init(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");
	retif(ad->dbus_connection == NULL, , "Invalid parameter!");

	s_info.hdl_activity =
		e_dbus_signal_handler_add(ad->dbus_connection, NULL,
				QP_DBUS_PATH,
				QP_DBUS_NAME,
				QS_DBUS_SIG_ACTIVITY,
				_handler_activity, ad);
	msgif(s_info.hdl_activity == NULL, "fail to add size signal");

	s_info.hdl_editing =
		e_dbus_signal_handler_add(ad->dbus_connection, NULL,
				QP_DBUS_PATH,
				QP_DBUS_NAME,
				QS_DBUS_SIG_EDITING,
				_handler_editing, ad);
	msgif(s_info.hdl_editing == NULL, "fail to add size signal");
}

static void _settings_ipc_fini(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");
	retif(ad->dbus_connection == NULL, , "Invalid parameter!");

	if (s_info.hdl_activity != NULL) {
		e_dbus_signal_handler_del(ad->dbus_connection,
				s_info.hdl_activity);
		s_info.hdl_activity = NULL;
	}

	if (s_info.hdl_editing != NULL) {
		e_dbus_signal_handler_del(ad->dbus_connection,
				s_info.hdl_editing);
		s_info.hdl_editing = NULL;
	}
}

/*****************************************************************************
 *
 * Util functions
 *
 *****************************************************************************/
HAPI int quickpanel_settings_ipc_init(void *data)
{
	_settings_ipc_init(data);

	return QP_OK;
}

HAPI int quickpanel_settings_ipc_fini(void *data)
{
	_settings_ipc_fini(data);

	return QP_OK;
}
