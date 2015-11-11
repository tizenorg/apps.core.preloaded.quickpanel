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

#include <alarm.h>
#include <time.h>
#include <unistd.h>
#include <vconf.h>
#include <system_settings.h>
#include <notification.h>
#include <notification_internal.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <feedback.h>
#include <player.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "common_uic.h"
#include "common.h"
#include "noti_node.h"
#include "media.h"
#include "noti.h"

#define REMINDER_MIN_INTERVAL 2

static struct info {
	int alarm_id;
} s_info = {
	.alarm_id = -1,
};

static void _feedback_sound_play(void)
{
	int ret = 0;
	noti_node_item *node = NULL;
	notification_h noti;
	int priv_id = 0;
	const char *nsound_path = NULL;
	notification_sound_type_e nsound_type = NOTIFICATION_SOUND_TYPE_NONE;
	int is_play_default = 0;

	// check first noti sound
	node = quickpanel_noti_node_get_first_noti();
	if (node) {
		noti = node->noti;
		if (noti) {
			notification_get_id(noti, NULL, &priv_id);
			notification_get_sound(noti, &nsound_type, &nsound_path);
			SDBG("reminded notification sound type[%d] path[%s]", nsound_type, nsound_path);

			switch (nsound_type) {
			case NOTIFICATION_SOUND_TYPE_USER_DATA:
				/*
				 *  if user data file isn't playable, play the default ringtone
				 */
				if (nsound_path != NULL) {
					if (quickpanel_media_playable_check(nsound_path) == EINA_TRUE) {
						ret = quickpanel_media_player_play(SOUND_TYPE_NOTIFICATION, nsound_path);
						if (ret == PLAYER_ERROR_NONE) {
							quickpanel_media_player_id_set(priv_id);
						} else {
							ERR("failed to play notification sound[%d]", ret);
							is_play_default = 1;
						}
					}
				}
				break;
			case NOTIFICATION_SOUND_TYPE_DEFAULT:
				is_play_default = 1;
				break;
			case NOTIFICATION_SOUND_TYPE_MAX:
			case NOTIFICATION_SOUND_TYPE_NONE:
				break;
			}
		}
	}

	if (is_play_default) {
		char *default_msg_tone = NULL;

		ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_SOUND_NOTIFICATION, &default_msg_tone);
		msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "ailed to set key(%s) : %d", SYSTEM_SETTINGS_KEY_SOUND_NOTIFICATION, ret);
		SDBG("Reminded setting sound[%s]", default_msg_tone);

		if (default_msg_tone != NULL) {
			ret = quickpanel_media_player_play(SOUND_TYPE_NOTIFICATION, default_msg_tone);
			free(default_msg_tone);
			quickpanel_media_player_id_set(0);
			if (ret != PLAYER_ERROR_NONE) {
				ERR("failed to play feedback sound");
			}
		}

		if (quickpanel_media_is_vib_enabled() == 1) {
			feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_MESSAGE);
		}
	}
}

static int _reminder_interval_get(void)
{
	int key = 0;
	int min = 0;
	int ret = -1;

	ret = vconf_get_int(VCONFKEY_SETAPPL_NOTI_MSG_ALERT_REP_TYPE_INT, &key);
	retif(ret != 0, 0, "failed to get vconf VCONFKEY_SETAPPL_NOTI_MSG_ALERT_REP_TYPE_INT");

	switch (key) {
		case 1:
			min = 2;
			break;
		case 2:
			min = 5;
			break;
		case 3:
			min = 10;
			break;
	}

	DBG("interval:%d", min);

	return min;
}

static int _alarm_delete_cb(alarm_id_t id, void * user_param)
{
	int ret = ALARMMGR_RESULT_SUCCESS;

	ret = alarmmgr_remove_alarm(id);
	if (ret != ALARMMGR_RESULT_SUCCESS) {
		ERR("alarmmgr_enum_alarm_ids() failed");
	}

	return 0;
}

static void _alarm_unset(void)
{
	int ret = ALARMMGR_RESULT_SUCCESS;

	if (s_info.alarm_id != -1){
		ERR("try to delete alarm_id(%d)", s_info.alarm_id);
		ret = alarmmgr_remove_alarm(s_info.alarm_id);
		if (ret != ALARMMGR_RESULT_SUCCESS) {
			ERR("alarmmgr_remove_alarm(%d) failed", s_info.alarm_id);
			ret = alarmmgr_enum_alarm_ids(_alarm_delete_cb, NULL);
			if (ret != ALARMMGR_RESULT_SUCCESS) {
				ERR("alarmmgr_enum_alarm_ids() failed");
			}
		}
		s_info.alarm_id = -1;
	}
}

static Eina_Bool _alarm_set_from_now(int min, void *data)
{
	int ret = ALARMMGR_RESULT_SUCCESS;
	time_t current_time;
	struct tm current_tm;
	alarm_entry_t *alarm_info = NULL;
	alarm_id_t alarm_id;
	alarm_date_t alarm_time;

	/* delete before registering alarm ids */
	_alarm_unset();

	/* set alarm after sec */
	time(&current_time);

	DBG(" %s, after %d MIN alarm set", ctime(&current_time), min);
	localtime_r(&current_time, &current_tm);

	alarm_info = alarmmgr_create_alarm();
	if (alarm_info == NULL) {
		ERR("alarmmgr_create_alarm() is failed\n");
		return EINA_FALSE;
	}

	alarm_time.year = 0;
	alarm_time.month = 0;
	alarm_time.day = 0;
	alarm_time.hour = current_tm.tm_hour;
	alarm_time.min = current_tm.tm_min + min;
	alarm_time.sec = current_tm.tm_sec;

	alarmmgr_set_repeat_mode(alarm_info, ALARM_REPEAT_MODE_ONCE, 0);
	alarmmgr_set_time(alarm_info, alarm_time);
	alarmmgr_set_type(alarm_info, ALARM_TYPE_VOLATILE);

	ret = alarmmgr_add_alarm_with_localtime(alarm_info, NULL, &alarm_id);
	if (ret != ALARMMGR_RESULT_SUCCESS) {
		ERR("alarmmgr_add_alarm_with_localtime() failed:%d", ret);
		alarmmgr_free_alarm(alarm_info) ;
		return EINA_FALSE;
	}

	DBG("alarm id(%d) is set", alarm_id);
	s_info.alarm_id = alarm_id;
	alarmmgr_free_alarm(alarm_info) ;

	return EINA_TRUE;
}

static int _alarm_cb(alarm_id_t alarm_id, void *data)
{
	DBG("");

	int min = _reminder_interval_get();

	if (min >= REMINDER_MIN_INTERVAL) {
		_alarm_set_from_now(min, data);
	} else {
		_alarm_unset();
	}

	if (!quickpanel_uic_is_opened()) {
		_feedback_sound_play();
	} else {
		ERR("quickpanel is opened, skip remind sound");
	}

	return 1;
}

static void _alarm_setting_changed_cb(keynode_t *key, void* data)
{
	int min = _reminder_interval_get();

	if (quickpanel_noti_get_count() <= 0) {
		_alarm_unset();
	} else {
		if (min >= REMINDER_MIN_INTERVAL) {
			_alarm_set_from_now(min, data);
		} else {
			_alarm_unset();
		}
	}
}

HAPI void quickpanel_reminder_init(void *data)
{
	DBG("");

	int ret = 0;

	ret = alarmmgr_init("org.tizen.quickpanel");
	retif(ret < 0, , "alarmmgr_init() failed (%d)", ret);

	ret = alarmmgr_set_cb(_alarm_cb, NULL);
	retif(ret < 0, , "alarmmgr_init() failed (%d)", ret);

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_NOTI_MSG_ALERT_REP_TYPE_INT,
			_alarm_setting_changed_cb, data);
	if (ret != 0) {
		ERR("failed to register a cb key:%s err:%d",
				"VCONFKEY_SETAPPL_NOTI_MSG_ALERT_REP_TYPE_INT", ret);
	}

	s_info.alarm_id = -1;
}

HAPI void quickpanel_reminder_fini(void *data)
{
	DBG("");

	int ret = 0;

	_alarm_unset();

	alarmmgr_fini();

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_NOTI_MSG_ALERT_REP_TYPE_INT, _alarm_setting_changed_cb);
	if (ret != 0) {
		ERR("failed to unregister a cb key:%s err:%d", "VCONFKEY_SETAPPL_NOTI_MSG_ALERT_REP_TYPE_INT", ret);
	}
}

HAPI void quickpanel_reminder_start(void *data)
{
	DBG("");

	int min = _reminder_interval_get();

	if (min >= REMINDER_MIN_INTERVAL) {
		_alarm_set_from_now(min, data);
	}
}

HAPI void quickpanel_reminder_stop(void *data)
{
	DBG("");

	_alarm_unset();
}
