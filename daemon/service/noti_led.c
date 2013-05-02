/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef QP_SERVICE_NOTI_LED_ENABLE
#include <vconf.h>
#include "common.h"
#include "noti_util.h"
#include "noti_led.h"

#define LED_ON 1
#define LED_OFF 0

static int g_is_led_turned_on = 0;

static inline int _is_led_notification_enabled(void) {
	int ret = -1;
	int status = 1;

	ret = vconf_get_bool(VCONFKEY_SETAPPL_LED_INDICATOR_NOTIFICATIONS, &status);

	if (ret == 0) {
		if (status == 0) {
			ERR("LED notification turned off");
			return 0;
		}
	} else {
		ERR("failed to get value of VCONFKEY_SETAPPL_LED_INDICATOR_NOTIFICATIONS:%d", ret);
	}

	return 1;
}

static inline int __quickpanel_service_get_event_count(const char *pkgname) {
	int count = 0;
	notification_h noti = NULL;
	notification_list_h noti_list = NULL;

	retif(pkgname == NULL, 0, "Invalid parameter!");

	notification_get_detail_list(pkgname, NOTIFICATION_GROUP_ID_NONE, NOTIFICATION_PRIV_ID_NONE, -1, &noti_list);
	if (noti_list != NULL) {
		noti = notification_list_get_data(noti_list);
		if (noti != NULL) {
			count = quickpanel_noti_get_event_count_from_noti(noti);
		}
		notification_free_list(noti_list);
		return count;
	} else {
		return 0;
	}

	return 0;
}

static void _noti_led_on_with_custom_color(int led_argb) {
	int ret = 0;

	if ((ret = led_set_mode_with_color(LED_MISSED_NOTI, LED_ON, led_argb)) == -1) {
		ERR("failed led_set_mode:%d", ret);
	}
	g_is_led_turned_on = 1;
}

static void _noti_led_on(notification_h noti) {
	int ret = 0;
	notification_led_op_e operation = -1;
	int led_argb = 0x0;

	if (noti == NULL) {
		if ((ret = led_set_mode(LED_MISSED_NOTI, LED_ON)) == -1) {
			ERR("failed led_set_mode:%d", ret);
		}
		g_is_led_turned_on = 1;
	} else {
		notification_get_led(noti, &operation, &led_argb);

		if (operation == NOTIFICATION_LED_OP_ON) {
			if ((ret = led_set_mode(LED_MISSED_NOTI, LED_ON)) == -1) {
				ERR("failed led_set_mode:%d", ret);
			}
			g_is_led_turned_on = 1;
		} else if (operation == NOTIFICATION_LED_OP_ON_CUSTOM_COLOR) {
			if ((ret = led_set_mode_with_color(LED_MISSED_NOTI, LED_ON, led_argb)) == -1) {
				ERR("failed led_set_mode:%d", ret);
			}
			g_is_led_turned_on = 1;
		} else {
			ERR("NOTIFICATION_LED_OP_OFF");
		}
	}
}

static void _noti_led_off(void) {
	int ret = 0;

	if ((ret = led_set_mode(LED_MISSED_NOTI, LED_OFF)) == -1) {
		ERR("failed led_set_mode:%d", ret);
	}
	g_is_led_turned_on = 0;
}

static int _is_keep_turn_on_led(int *op, int *argb) {
	notification_h noti = NULL;
	notification_list_h noti_list = NULL;
	notification_list_h noti_list_head = NULL;
	notification_led_op_e operation = -1;
	int led_argb = 0;

	notification_get_list(NOTIFICATION_TYPE_NOTI , -1, &noti_list_head);
	noti_list = noti_list_head;

	while (noti_list != NULL) {
		noti = notification_list_get_data(noti_list);
		if (noti != NULL) {
			notification_get_led(noti, &operation, &led_argb);
			if (operation >= NOTIFICATION_LED_OP_ON) {
				notification_free_list(noti_list_head);
				noti_list_head = NULL;

				if (op != NULL) *op = operation;
				if (argb != NULL) *argb = led_argb;
				return 1;
			}
		}

		noti_list = notification_list_get_next(noti_list);
	}

	if (noti_list_head != NULL) {
		notification_free_list(noti_list_head);
		noti_list_head = NULL;
	}

	return 0;
}

HAPI void quickpanel_service_noti_led_on(notification_h noti) {
	notification_led_op_e operation = -1;
	int led_argb = 0;

	retif(_is_led_notification_enabled() == 0, , "led noti disabled");

	if (noti == NULL) {
		if (_is_keep_turn_on_led(&operation, &led_argb) >= 1) {
			if (operation == NOTIFICATION_LED_OP_ON) {
				_noti_led_on(NULL);
			} else if (operation == NOTIFICATION_LED_OP_ON_CUSTOM_COLOR) {
				_noti_led_on_with_custom_color(led_argb);
			}
		}
	} else {
		_noti_led_on(noti);
	}
}

HAPI void quickpanel_service_noti_led_off(notification_h noti) {
	retif(g_is_led_turned_on == 0, , "led already turned off");

	if (_is_keep_turn_on_led(NULL, NULL) == 0) {
		_noti_led_off();
	}
}


static void quickpanel_service_noti_vconf_cb(keynode_t *node,
						void *data)
{
	int ret = 0;
	int is_on = 0;

	is_on = _is_led_notification_enabled();

	ERR("led notification status:%d", is_on);

	if (is_on == 0) {
		if ((ret = led_set_mode(LED_MISSED_NOTI, LED_OFF)) == -1) {
			ERR("failed led_set_mode:%d", ret);
		}
		g_is_led_turned_on = 0;
	} else {
		quickpanel_service_noti_led_on(NULL);
	}
}

HAPI void quickpanel_service_noti_init(void *data) {
	int ret = 0;
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_LED_INDICATOR_NOTIFICATIONS,
			quickpanel_service_noti_vconf_cb,
				ad);
	if (ret != 0) {
		ERR("failed to notify key[%s] : %d",
			VCONFKEY_SETAPPL_AUTO_ROTATE_SCREEN_BOOL, ret);
	}
}

HAPI void quickpanel_service_noti_fini(void *data) {
	int ret = 0;
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_LED_INDICATOR_NOTIFICATIONS,
			quickpanel_service_noti_vconf_cb);
	if (ret != 0) {
		ERR("failed to ignore key[%s] : %d",
				VCONFKEY_SETAPPL_LED_INDICATOR_NOTIFICATIONS, ret);
	}
}
#endif
