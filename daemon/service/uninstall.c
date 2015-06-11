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


#include <vconf.h>
#include <pkgmgr-info.h>
#include <package-manager.h>
#include <notification.h>
#include <badge.h>
#include "common.h"
#include "uninstall.h"

#define QP_PKGMGR_STR_START "start"
#define QP_PKGMGR_STR_END "end"
#define QP_PKGMGR_STR_OK "ok"
#define QP_PKGMGR_STR_UNINSTALL		"uninstall"

typedef struct _pkg_event {
	char *pkgname;
	int is_done;
} Pkg_Event;

static struct _s_info {
	pkgmgr_client *client;
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

static int _pkgmgr_event_cb(int req_id, const char *pkg_type, const char *pkgid,
 const char *key, const char *val, const void *pmsg, void *priv_data)
{
 	if (pkgid == NULL) {
		return 0;
	}

 	SDBG("pkg:%s key:%s val:%s", pkgid, key, val);

 	if (key != NULL && val != NULL) {
 		if (strcasecmp(key, QP_PKGMGR_STR_START) == 0 &&
 			strcasecmp(val, QP_PKGMGR_STR_UNINSTALL) == 0) {

 			ERR("Pkg:%s is being uninstalled", pkgid);

 			Pkg_Event *event = calloc(1, sizeof(Pkg_Event));
 			if (event != NULL) {
 				event->pkgname = strdup(pkgid);
 				s_info.event_list = eina_list_append(s_info.event_list, event);
 			} else {
 				ERR("failed to create event item");
 			}

 			return 0;
 		} else if (strcasecmp(key, QP_PKGMGR_STR_END) == 0 &&
 			strcasecmp(val, QP_PKGMGR_STR_OK) == 0) {
		 	if (_is_item_exist(pkgid, 1) == 1) {
		 		ERR("Pkg:%s is uninstalled, delete related resource", pkgid);
				notification_delete_all_by_type(pkgid, NOTIFICATION_TYPE_NOTI);
				notification_delete_all_by_type(pkgid, NOTIFICATION_TYPE_ONGOING);
				badge_remove(pkgid);
		 	}
 		}
 	}

 	return 0;
}

HAPI void quickpanel_uninstall_init(void *data)
{
	int ret = -1;

	pkgmgr_client *client = pkgmgr_client_new(PC_LISTENING);
	if (client != NULL) {
		if ((ret = pkgmgr_client_listen_status(client, _pkgmgr_event_cb, data)) != PKGMGR_R_OK) {
			ERR("Failed to listen pkgmgr event:%d", ret);
		}
		s_info.client = client;
	} else {
		ERR("Failed to create package manager client");
	}
}

HAPI void quickpanel_uninstall_fini(void *data)
{
	int ret = -1;
	Pkg_Event *event_item = NULL;

	if (s_info.client != NULL) {
		if ((ret = pkgmgr_client_free(s_info.client)) != PKGMGR_R_OK) {
			ERR("Failed to free pkgmgr client:%d", ret);
		}
		s_info.client = NULL;
	}

	EINA_LIST_FREE(s_info.event_list, event_item) {
		_item_del(event_item);
	}
	s_info.event_list = NULL;
}
