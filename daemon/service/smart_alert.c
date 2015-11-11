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
#include <notification_list.h>

#include "common.h"
#include "noti_util.h"
#include "smart_alert.h"

static inline int __quickpanel_service_update_event_count(const char *pkgname, const char *vconfkey)
{
	int ret = 0, count = 0;
	notification_h noti = NULL;
	notification_list_h noti_list = NULL;

	retif(pkgname == NULL, 0, "Invalid parameter!");
	retif(vconfkey == NULL, 0, "Invalid parameter!");

	notification_get_detail_list(pkgname, NOTIFICATION_GROUP_ID_NONE, NOTIFICATION_PRIV_ID_NONE, -1, &noti_list);
	if (noti_list != NULL) {
		noti = notification_list_get_data(noti_list);
		if (noti != NULL) {
			count = quickpanel_noti_util_get_event_count_from_noti(noti);
			ret = vconf_set_int(vconfkey, count);

			ERR("event set:%s, count:%d", pkgname, count);

			if (ret != 0) {
				ERR("failed to set vconf key[%s] : %d", vconfkey, ret);
			}
		} else {
			ERR("no data found:%s", pkgname);
		}
		notification_free_list(noti_list);
		return count;
	} else {
		ret = vconf_set_int(vconfkey, 0);

		ERR("event unset:%s", pkgname);

		if (ret != 0) {
			ERR("failed to set vconf key[%s] : %d", vconfkey, ret);
		}
	}

	return 0;
}

HAPI void quickpanel_smart_alert_update_info(notification_h noti)
{
	char *pkgname = NULL;
	int event_count_call = 0;
	int event_count_vtcall = 0;

	if (noti == NULL) {
		event_count_call = quickpanel_noti_util_get_event_count_by_pkgname(SMART_ALARM_CALL_PKGNAME);
		event_count_vtcall = quickpanel_noti_util_get_event_count_by_pkgname(SMART_ALARM_VTCALL_PKGNAME);
		ERR("call event set, count:%d, %d", event_count_call, event_count_vtcall);
	} else {
		notification_get_pkgname(noti, &pkgname);
		retif(pkgname == NULL, , "Invalid parameter!");

		if (strncmp(pkgname, SMART_ALARM_CALL_PKGNAME, strlen(pkgname)) == 0 || strncmp(pkgname, SMART_ALARM_VTCALL_PKGNAME, strlen(pkgname)) == 0) {
			event_count_call = quickpanel_noti_util_get_event_count_by_pkgname(SMART_ALARM_CALL_PKGNAME);
			event_count_vtcall = quickpanel_noti_util_get_event_count_by_pkgname(SMART_ALARM_VTCALL_PKGNAME);

			ERR("call event set, count:%d, %d", event_count_call, event_count_vtcall);
		}
	}
}
