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


#include <glib.h>
#include "common.h"
#include "quickpanel-ui.h"

#define QP_DBUS_ACTIVENOTI_NAME QP_DBUS_NAME".activenoti"
#define QP_DBUS_ACTIVENOTI_PATH QP_DBUS_PATH"/activenoti"
#define QP_DBUS_ACTIVENOTI_MEMBER_SHOW "show"
#define QP_DBUS_ACTIVENOTI_MEMBER_HIDE "hide"

HAPI void quickpanel_dbus_activenoti_visibility_send(int is_visible)
{
	DBusMessage *signal = NULL;
	const char *member = NULL;
	struct appdata *ad  = quickpanel_get_app_data();

	retif(ad == NULL, , "invalid parameter");
	retif(ad->dbus_connection == NULL, , "failed to get dbus system bus");

	if (is_visible == 1) {
		member = QP_DBUS_ACTIVENOTI_MEMBER_SHOW;
	} else {
		member = QP_DBUS_ACTIVENOTI_MEMBER_HIDE;
	}
	signal =
	    dbus_message_new_signal(QP_DBUS_ACTIVENOTI_PATH
	    		, QP_DBUS_ACTIVENOTI_NAME
	    		, member);
	if (signal == NULL) {
		ERR("Fail to dbus_message_new_signal");
		return;
	}

	DBG("status:%s", member);

	e_dbus_message_send(ad->dbus_connection,
			signal,
			NULL,
			0,
			NULL);
	dbus_message_unref(signal);
}
