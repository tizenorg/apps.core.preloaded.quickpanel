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
#include <Eina.h>

#include <vconf.h>
#include <package_manager.h>
#include <notification.h>
#include <notification_internal.h>
#include <badge.h>

#include "common.h"
#include "uninstall.h"

typedef struct _pkg_event {
	char *pkgname;
	int is_done;
} Pkg_Event;

static struct _s_info {
	package_manager_h client;
	Eina_List *event_list;
} s_info = {
	.client = NULL,
	.event_list = NULL,
};

static void _item_del(Pkg_Event *item_event)
{
	if (item_event != NULL) {
		free(item_event->pkgname);
	}

	free(item_event);
}

static int _is_item_exist(const char *pkgid, int remove_if_exist)
{
	int ret = 0;
	Eina_List *l = NULL;
	Pkg_Event *event_item = NULL;
	retif(pkgid == NULL, 0, "invalid parameter");

	EINA_LIST_FOREACH(s_info.event_list, l, event_item) {
		if (event_item != NULL) {
			if (strcmp(event_item->pkgname, pkgid) == 0) {
				ret = 1;
				break;
			}
		}
	}

	if (ret == 1 && remove_if_exist == 1) {
		s_info.event_list = eina_list_remove(s_info.event_list, event_item);
		_item_del(event_item);
	}

	return ret;
}

static void _pkgmgr_event_cb(const char *type, const char *package,
	package_manager_event_type_e event_type,
	package_manager_event_state_e event_state, int progress,
	package_manager_error_e error, void *user_data)
{
	if (error != PACKAGE_MANAGER_ERROR_NONE) {
		ERR("_pkgmgr_event_cb error in cb");
		return;
	}

	if (event_type != PACKAGE_MANAGER_EVENT_TYPE_UNINSTALL) {
		return;
	}

	SDBG("type : %s event_type:%d event_state:%d [%s]", type, event_type, event_state, package);

	if (event_state == PACKAGE_MANAGER_EVENT_STATE_STARTED) {

		DBG("Pkg:%s is being uninstalled", package);

		Pkg_Event *event = calloc(1, sizeof(Pkg_Event));
		if (event != NULL) {
			event->pkgname = strdup(package);
			s_info.event_list = eina_list_append(s_info.event_list, event);
		} else {
			ERR("failed to create event item");
		}
	} else if (event_state == PACKAGE_MANAGER_EVENT_STATE_COMPLETED) {

		if (_is_item_exist(package, 1) == 1) {
			DBG("Pkg:%s is uninstalled, delete related resource", package);

			notification_delete_all_by_type(package, NOTIFICATION_TYPE_NOTI);
			notification_delete_all_by_type(package, NOTIFICATION_TYPE_ONGOING);
			badge_remove(package);
		}
	}
}


HAPI void quickpanel_uninstall_init(void *data)
{
	int ret = -1;

	ret = package_manager_create(&s_info.client);
	if (ret == PACKAGE_MANAGER_ERROR_NONE) {
		if (package_manager_set_event_cb(s_info.client, (void*)_pkgmgr_event_cb, data) != PACKAGE_MANAGER_ERROR_NONE) {
			ERR("Failed to set package manager event:%d", ret);
		}
	} else {
		ERR("Failed to create package manager : %d ", ret);
	}

}

HAPI void quickpanel_uninstall_fini(void *data)
{
	int ret = -1;

	Pkg_Event *event_item = NULL;

	ret = package_manager_destroy(s_info.client);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		ERR("Failed to destory package manager:%d", ret);
	}

	EINA_LIST_FREE(s_info.event_list, event_item) {
		_item_del(event_item);
	}
	s_info.event_list = NULL;
}
