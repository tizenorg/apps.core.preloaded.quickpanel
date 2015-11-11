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
#include <glib.h>

#include <vconf.h>
#include <notification.h>
#include <notification_internal.h>

#include "common.h"
#include "noti_util.h"
#include "noti_led.h"
#include "noti_node.h"

#define LED_ON 1
#define LED_OFF 0
#define LED_MISSED_NOTI	5

typedef struct _QP_LED {
	int priv_id;
	notification_led_op_e op;
	int argb;
	int timestamp;
	int time_on;
	int time_off;
} QP_LED_T;

static struct _s_led_info {
	Eina_List *list;
	int is_turned_on;
} s_led_info = {
	.list = NULL,
	.is_turned_on = 0,
};

static QP_LED_T * _led_entry_new(int priv_id, notification_led_op_e op, int argb, int time_on, int time_off)
{
	QP_LED_T *led_entry = (QP_LED_T *)calloc(1, sizeof(QP_LED_T));

	retif(led_entry == NULL, NULL, "failed to memory allocation");

	led_entry->priv_id = priv_id;
	led_entry->op = op;
	led_entry->argb = argb;
	led_entry->time_on = (time_on <= 0) ? -1 : time_on ;
	led_entry->time_off = (time_off <= 0) ? -1 : time_off;
	led_entry->timestamp = (int)time(NULL);

	return led_entry;
}

static void _led_entry_del(QP_LED_T *led_entry)
{
	retif(led_entry == NULL, ,"invalid parameter");

	free(led_entry);
}

static int _led_list_sort_cb(const void *data1, const void *data2)
{
	QP_LED_T *entry_1 = (QP_LED_T *)data1;
	QP_LED_T *entry_2 = (QP_LED_T *)data2;

	if (entry_1 == NULL || entry_2 == NULL) {
		return 0;
	}

	return entry_2->timestamp - entry_1->timestamp;
}

static void _led_list_add(QP_LED_T *led_entry)
{
	retif(led_entry == NULL, ,"invalid parameter");

	s_led_info.list = eina_list_sorted_insert(s_led_info.list, _led_list_sort_cb, led_entry);
}

static void _led_list_del(QP_LED_T *led_entry)
{
	retif(led_entry == NULL, ,"invalid parameter");

	s_led_info.list = eina_list_remove(s_led_info.list, led_entry);
}

static void _led_list_sort(void)
{
	retif(s_led_info.list == NULL, ,"invalid parameter");

	s_led_info.list = eina_list_sort(s_led_info.list, 0, _led_list_sort_cb);
}

static QP_LED_T *_led_list_find_by_priv_id(int priv_id)
{
	Eina_List *l;
	Eina_List *n;
	QP_LED_T *led_entry = NULL;

	retif(s_led_info.list == NULL, NULL,"invalid parameter");

	EINA_LIST_FOREACH_SAFE(s_led_info.list, l, n, led_entry) {
		if (led_entry != NULL) {
			if (led_entry->priv_id == priv_id) return led_entry;
		}
	}

	return NULL;
}

static void _led_list_clean_up(void)
{
	Eina_List *l;
	Eina_List *n;
	QP_LED_T *led_entry = NULL;
	Eina_List *list_temp = NULL;

	retif(s_led_info.list == NULL, ,"invalid parameter");

	list_temp = s_led_info.list;
	s_led_info.list = NULL;
	EINA_LIST_FOREACH_SAFE(list_temp, l, n, led_entry) {
		if (led_entry != NULL) {
			_led_entry_del(led_entry);
		}
	}

	eina_list_free(list_temp);

}

static QP_LED_T *_led_list_get_first(void)
{
	return eina_list_nth(s_led_info.list, 0);
}

static inline int _is_led_enabled(void)
{
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

static int _led_set_mode(int mode, bool val, int on, int off, unsigned int color)
{
	// TODO: Kiran device does not support front led.
	// Because H/W is not fixed, if led should work, use dbus method call.
	//
	// bus name : org.tizen.system.deviced
	// object path : /Org/Tizen/System/DeviceD/Led
	// interface name : org.tizen.system.deviced.Led
	// method name : SetMode
	// input argument : "iiiiu" (int32:mode,
	//							int32:on(1)/off(0),
	//							int32:[custom]on duty (default:-1),
	//							int32:[custom]off duty (default:-1),
	//							uint32:[custom]color (default:0))
	// mode LED_MISSED_NOTI = 5, LED_VOICE_RECORDING = 6, LED_REMOTE_CONTROLLER = 7, LED_AIR_WAKEUP = 8
	// custom : only support for MISSED_NOTI and VOICE_RECORDING case
	// output argument : "i" (int32:result)

	return -1;
}

static void _noti_led_on(QP_LED_T *led_entry)
{
	int ret = 0;
	retif(led_entry == NULL, , "invalid data");

	DBG("turn on LED with OP:%d ARGB:%x ON:%d OFF:%d",
			led_entry->op, led_entry->argb, led_entry->time_on, led_entry->time_off);

	if (led_entry->op == NOTIFICATION_LED_OP_ON) {
		if ((ret = _led_set_mode(LED_MISSED_NOTI, LED_ON, led_entry->time_on, led_entry->time_off, 0)) == -1) {
			ERR("failed led_set_mode:%d", ret);
		} else {
			s_led_info.is_turned_on = 1;
		}
	} else if (led_entry->op == NOTIFICATION_LED_OP_ON_CUSTOM_COLOR) {
		if ((ret = _led_set_mode(LED_MISSED_NOTI, LED_ON, led_entry->time_on, led_entry->time_off, led_entry->argb)) == -1) {
			ERR("failed led_set_mode:%d", ret);
		} else {
			s_led_info.is_turned_on = 1;
		}
	} else {
		ERR("NOTIFICATION_LED_OP_OFF");
	}
}

static void _noti_led_off(int is_force)
{
	int ret = 0;

	ERR("try to turn off LED");
	retif(s_led_info.is_turned_on == 0 && is_force == 1, , "LED already turned off");

	if ((ret = _led_set_mode(LED_MISSED_NOTI, LED_OFF, 0, 0, 0)) == -1) {
		ERR("failed led_set_mode:%d", ret);
	} else {
		s_led_info.is_turned_on = 0;
	}
}

HAPI void quickpanel_noti_led_proc(notification_h noti, int op_type)
{
	int priv_id = 0;
	int led_argb = 0;
	int time_on = 0;
	int time_off = 0;
	QP_LED_T *led_entry = NULL;
	notification_led_op_e led_op = -1;
	retif(noti == NULL, , "Invalid parameter!");

	notification_get_id(noti, NULL, &priv_id);
	notification_get_led(noti, &led_op, &led_argb);
	notification_get_led_time_period(noti, &time_on, &time_off);

	DBG("on:%d off:%d", time_on, time_off);

	if (op_type == NOTIFICATION_OP_INSERT || op_type == NOTIFICATION_OP_UPDATE) {
		led_entry = _led_list_find_by_priv_id(priv_id);
		if (led_entry != NULL) {
			if (led_op == NOTIFICATION_LED_OP_OFF) {
				_led_list_del(led_entry);
				_led_entry_del(led_entry);
			} else {
				led_entry->op = led_op;
				led_entry->argb = led_argb;
				led_entry->time_on = (time_on <= 0) ? -1 : time_on ;
				led_entry->time_off = (time_off <= 0) ? -1 : time_off;
				led_entry->timestamp = (int)time(NULL);
				_led_list_sort();
			}
		} else {
			if (led_op >= NOTIFICATION_LED_OP_ON) {
				led_entry = _led_entry_new(priv_id, led_op, led_argb, time_on, time_off);
				_led_list_add(led_entry);
			}
		}
	} else if (op_type == NOTIFICATION_OP_DELETE) {
		led_entry = _led_list_find_by_priv_id(priv_id);
		if (led_entry != NULL) {
			_led_list_del(led_entry);
			_led_entry_del(led_entry);
		}
	}

	//turn on or off LED
	if (_is_led_enabled() == 1) {
		led_entry = _led_list_get_first();
		if (led_entry != NULL) {
			_noti_led_on(led_entry);
		} else {
			_noti_led_off(0);
		}
	} else {
		_noti_led_off(0);
	}
}

static void _led_option_vconf_cb(keynode_t *node, void *data)
{
	QP_LED_T *led_entry = NULL;

	if (_is_led_enabled() == 1) {
		DBG("led notification is enabled");
		led_entry = _led_list_get_first();
		if (led_entry != NULL) {
			DBG("try to turn on LED, op:%d argb:%x", led_entry->op, led_entry->argb);
			_noti_led_on(led_entry);
		} else {
			_noti_led_off(1);
		}
	} else {
		DBG("led notification is disabled");
		_noti_led_off(1);
	}
}

static void _led_init_data_cb(gpointer key, gpointer value, gpointer user_data)
{
	int priv_id = 0;
	int led_argb = 0;
	int time_on = 0;
	int time_off = 0;
	notification_led_op_e led_op = -1;
	noti_node_item *node = (noti_node_item *)value;
	retif(node == NULL, , "Invalid parameter!");
	retif(node->noti == NULL, , "Invalid parameter!");

	notification_get_id(node->noti, NULL, &priv_id);
	notification_get_led(node->noti, &led_op, &led_argb);
	notification_get_led_time_period(node->noti, &time_on, &time_off);
	if (led_op >= NOTIFICATION_LED_OP_ON) {
		QP_LED_T *new_entry = _led_entry_new(priv_id, led_op, led_argb, time_on, time_off);
		_led_list_add(new_entry);
	}
}

static void _led_init_data(noti_node *nodes)
{
	QP_LED_T *led_entry = NULL;
	retif(nodes == NULL, , "Invalid parameter!");
	retif(nodes->table == NULL, , "Invalid parameter!");

	g_hash_table_foreach(nodes->table, _led_init_data_cb, NULL);

	if (_is_led_enabled() == 1) {
		led_entry = _led_list_get_first();
		if (led_entry != NULL) {
			_noti_led_on(led_entry);
		} else {
			_noti_led_off(1);
		}
	} else {
		_noti_led_off(1);
	}
}

HAPI void quickpanel_noti_led_init(void *data, void *nodes)
{
	int ret = 0;
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_LED_INDICATOR_NOTIFICATIONS,_led_option_vconf_cb, ad);

	if (ret != 0) {
		ERR("failed to notify key[%s] : %d",
				VCONFKEY_SETAPPL_AUTO_ROTATE_SCREEN_BOOL, ret);
	}

	if (nodes != NULL) {
		_led_init_data((noti_node *)nodes);
	}
}

HAPI void quickpanel_noti_led_fini(void *data)
{
	int ret = 0;
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_LED_INDICATOR_NOTIFICATIONS,_led_option_vconf_cb);
	if (ret != 0) {
		ERR("failed to ignore key[%s] : %d", VCONFKEY_SETAPPL_LED_INDICATOR_NOTIFICATIONS, ret);
	}

	_led_list_clean_up();
}
