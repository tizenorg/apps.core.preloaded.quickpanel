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


#include <syspopup_caller.h>

#include <package_manager.h>
#include <bundle_internal.h>
#include <notification.h>
#include <notification_internal.h>
#include <notification_list.h>

#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "common.h"
#include "modules.h"
#include "datetime.h"
#include "emergency_mode.h"
#include "quickpanel-ui.h"

#ifdef QP_SETTING_ENABLE
extern QP_Module settings_view_featured;
#endif
#ifdef QP_BRIGHTNESS_ENABLE
extern QP_Module brightness_ctrl;
#endif

#define SETTING_SYSPOPUP "mode-syspopup"
#define BT_SHARE_DAEMON "/usr/bin/bluetooth-share"
#define BT_SHARE_SERVER "bluetooth-share-opp-server"
#define BT_SHARE_CLIENT "bluetooth-share-opp-client"
#define SCREEN_SHOT "shot-tizen"

static struct _info {
	int is_enabled;
	Eina_List *permitted_apps;
} s_info = {
	.is_enabled = 0,
	.permitted_apps = NULL,
};

static void _delete_unpermitted_app(void)
{
	notification_list_h noti_list = NULL;
	notification_list_h list_traverse = NULL;
	notification_h noti = NULL;

	notification_get_list(NOTIFICATION_TYPE_NONE, -1, &noti_list);

	list_traverse = noti_list;

	while (list_traverse != NULL) {
		noti = notification_list_get_data(list_traverse);

		quickpanel_emergency_mode_notification_filter(noti, 1);

		list_traverse = notification_list_get_next(list_traverse);
	}

	if (noti_list != NULL) {
		notification_free_list(noti_list);
		noti_list = NULL;
	}
}

static void _emergency_mode_start(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	if (s_info.is_enabled) {
		return;
	}

	quickpanel_datetime_datentime_event_set(0);
#ifdef QP_SETTING_ENABLE
	if (settings_view_featured.fini != NULL) {
		settings_view_featured.fini(ad);
	}
#endif
#ifdef QP_BRIGHTNESS_ENABLE
	if (brightness_ctrl.fini != NULL) {
		brightness_ctrl.fini(ad);
	}
#endif

	_delete_unpermitted_app();
	s_info.is_enabled = 1;
	ERR("emergency mode is enabled");
}

static void _emergency_mode_stop(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	if (!s_info.is_enabled) {
		return;
	}

	quickpanel_datetime_datentime_event_set(1);

#ifdef QP_SETTING_ENABLE
	if (settings_view_featured.init != NULL) {
		settings_view_featured.init(ad);
	}
	if (settings_view_featured.init_job_cb != NULL) {
		settings_view_featured.init_job_cb(ad);
	}
#endif
#ifdef QP_BRIGHTNESS_ENABLE
	if (brightness_ctrl.init != NULL) {
		brightness_ctrl.init(ad);
	}
#endif

	_delete_unpermitted_app();
	s_info.is_enabled = 0;
	ERR("emergency mode is disabled");
}

static void _vconf_cb(keynode_t *node, void *data)
{
	int mode = 0;

	if (vconf_get_int(VCONFKEY_SETAPPL_PSMODE, &mode) == 0) {
		if (mode == SETTING_PSMODE_EMERGENCY) {
			_emergency_mode_start(data);
		} else {
			_emergency_mode_stop(data);
		}
	} else {
		ERR("failed to get the value of VCONFKEY_SETAPPL_PSMODE");
	}
}

static bool _app_list_cb(package_info_h handle, void *user_data)
{
	char *appid = NULL;
	char *permitted_appid = NULL;

	/* NEED TO CHANGE */
	package_info_get_package(handle, &appid);

	permitted_appid = strdup(appid);

	s_info.permitted_apps = eina_list_append(s_info.permitted_apps, permitted_appid);
	DBG("%s is permitted.", permitted_appid);

	return 0;
}


static int _register_permitted_apps(void)
{
	DBG("");
	int ret = 0;
	package_manager_filter_h handle;

	s_info.permitted_apps = eina_list_append(s_info.permitted_apps, BT_SHARE_DAEMON);
	s_info.permitted_apps = eina_list_append(s_info.permitted_apps, SCREEN_SHOT);
	s_info.permitted_apps = eina_list_append(s_info.permitted_apps, BT_SHARE_SERVER);
	s_info.permitted_apps = eina_list_append(s_info.permitted_apps, BT_SHARE_CLIENT);

	ret = package_manager_filter_create(&handle);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		return -1;
	}
	
#if defined(WINSYS_X11)
	/* NEED TO CHANGE */
	ret = package_manager_filter_add_bool(handle, PMINFO_APPINFO_PROP_APP_SUPPORT_MODE, 1);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		package_manager_filter_destroy(handle);
		return -1;
	}
#endif

	ret = package_manager_filter_foreach_package_info(handle, _app_list_cb, NULL);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		package_manager_filter_destroy(handle);
		return -1;
	}

	package_manager_filter_destroy(handle);
	return 0;

}

static int _delete_permitted_apps(void)
{
	Eina_List *list = NULL;
	char *appid = NULL;

	if (!s_info.permitted_apps) {
		EINA_LIST_FOREACH(s_info.permitted_apps, list, appid)
			free(appid);
		eina_list_free(s_info.permitted_apps);
		s_info.permitted_apps = NULL;
	}

	return 0;
}

HAPI void quickpanel_emergency_mode_init(void *data)
{
	int ret = 0;
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	ret = _register_permitted_apps();
	msgif(ret !=0, "failed to register permitted apps");

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_PSMODE,
			_vconf_cb, ad);
	msgif(ret != 0, "failed to notify key(VCONFKEY_SETAPPL_PSMODE) : %d", ret);

	if (quickpanel_emergency_mode_is_on()) {
		s_info.is_enabled = 1;
	}
}

HAPI void quickpanel_emergency_mode_fini(void *data)
{
	int ret = 0;
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	ret = _delete_permitted_apps();
	msgif(ret !=0, "failed to delete permitted apps");

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_PSMODE,	_vconf_cb);
	msgif(ret != 0, "failed to ignore key(VCONFKEY_SETAPPL_PSMODE) : %d", ret);
}

HAPI int quickpanel_emergency_mode_is_permitted_app(const char *appid)
{
	int i = 0;
	int count = 0;
	char *permitted_app = NULL;
	retif(appid == NULL, 0, "Invalid parameter!");

	count = eina_list_count(s_info.permitted_apps);
	for(i = 0; i < count; i++) {
		permitted_app = (char *)eina_list_nth(s_info.permitted_apps, i);
		if (permitted_app != NULL && strcmp(permitted_app, appid) == 0) {
			return 1;
		}
	}

	return 0;
}

HAPI int quickpanel_emergency_mode_is_on(void)
{
	int mode = 0;

	if (vconf_get_int(VCONFKEY_SETAPPL_PSMODE, &mode) == 0) {
		if (mode == SETTING_PSMODE_EMERGENCY) {
			return 1;
		}
	}

	return 0;
}

HAPI int quickpanel_emergency_mode_notification_filter(notification_h noti, int is_delete)
{
	int priv_id = 0;
	char *pkgname = NULL;

	notification_get_pkgname(noti, &pkgname);
	notification_get_id(noti, NULL, &priv_id);

	DBG("Emergency mode filter is called: %s", pkgname);
	if (!quickpanel_emergency_mode_is_permitted_app(pkgname)) {
		if (is_delete) {
			notification_delete_by_priv_id(pkgname, NOTIFICATION_TYPE_NONE, priv_id);
		}
		return 1;
	}

	return 0;
}

HAPI int quickpanel_emergency_mode_syspopup_launch(void)
{
	int ret;
	bundle *b = NULL;

	DBG("");

	b = bundle_create();
	if (b == NULL) {
		return QP_FAIL;
	}

	bundle_add(b, "_MODE_SYSTEM_POPUP_TYPE_", "POPUP_EMERGENCY_PSMODE");
	ret = syspopup_launch(SETTING_SYSPOPUP, b);
	if (ret < 0) {
		ERR("failed to launch syspopup (%s):%d\n", SETTING_SYSPOPUP, ret);
		bundle_free(b);
		return QP_FAIL;
	}

	DBG("");

	bundle_free(b);
	return QP_OK;
}
