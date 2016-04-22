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
#include <app_control.h>
#include <notification.h>
#include <feedback.h>
#include <system_settings.h>
#include <notification_internal.h>
#include <notification_setting_internal.h>
#include <notification_text_domain.h>
#include <player.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "common_uic.h"
#include "common.h"
#include "media.h"
#include "noti_node.h"
#include "noti.h"
#include "noti_win.h"
#include "noti_util.h"
#include "animated_icon.h"

#ifdef QP_EMERGENCY_MODE_ENABLE
#include "emergency_mode.h"
#endif

#define QP_ACTIVENOTI_DURATION	3
#define QP_ACTIVENOTI_DETAIL_DURATION 6

#define ACTIVENOTI_MSG_LEN 100
#define DEFAULT_ICON RESDIR"/quickpanel_icon_default.png"

#define DELAY_TIMER_VALUE 0.480
#define DEL_TIMER_VALUE	8.0
#define SHOW_MIN_TIMER_VALUE	1.0
#define RELEASE_TIME 8.0

#define NOTI_START_Y ELM_SCALE_SIZE(36)

static struct info {
	Evas_Object *activenoti;
	Evas_Object *layout;
	Evas_Object *btnbox;
	Evas_Object *gesture;
	Ecore_Timer *delay_timer;
	Ecore_Timer *close_timer;
	Ecore_Timer *show_min_timer;

	Eina_List *auto_remove_list;
	Eina_List *non_auto_remove_list;
	notification_h current_noti;

} s_info = {
	.activenoti = NULL,
	.layout = NULL,
	.btnbox = NULL,
	.gesture = NULL,
	.delay_timer = NULL,
	.close_timer = NULL,
	.show_min_timer = NULL,
	.auto_remove_list = NULL,
	.non_auto_remove_list = NULL,
	.current_noti = NULL,
};

struct noti_info {
	notification_h noti;
	Ecore_Timer *timer;
};

static inline char *_get_text(notification_h noti, notification_text_type_e text_type);

static int _activenoti_init(void *data);
static int _activenoti_fini(void *data);
static int _activenoti_enter_hib(void *data);
static int _activenoti_leave_hib(void *data);
static void _activenoti_reflesh(void *data);
static void _activenoti_qp_opened(void *data);

static void _activenoti_update_activenoti();
static void _activenoti_create_activenoti();
static void _activenoti_win_rotated(void *data, int need_hide);

static void _activenoti_destroy_activenoti();
static void _media_feedback_sound(notification_h noti);
static notification_h _activenoti_get_in_list();
static bool _activenoti_remove_in_list(notification_h noti);
static void _activenoti_delete_current_noti(void);
static bool _activenoti_has_pending_noti(void);
static void _activenoti_hide(void *data, int delay);

QP_Module activenoti = {
	.name = "activenoti",
	.init = _activenoti_init,
	.fini = _activenoti_fini,
	.hib_enter = _activenoti_enter_hib,
	.hib_leave = _activenoti_leave_hib,
	.lang_changed = NULL,
	.qp_opened = _activenoti_qp_opened,
	.qp_closed = NULL,
	.refresh = _activenoti_reflesh
};

static void _app_control_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	if (data) {
		DBG("_app_control_del_cb %p", data);
		app_control_destroy(data);
	}
}

static inline int _is_text_exist(const char *text)
{
	if (text != NULL) {
		if (strlen(text) > 0) {
			return 1;
		}
	}
	return 0;
}

static int _is_sound_playable(void)
{
	int status = 0, ret = 0;

	ret = vconf_get_int(VCONFKEY_CAMERA_STATE, &status);
	if (ret == VCONF_OK && status == VCONFKEY_CAMERA_STATE_RECORDING) {
		ERR("camcorder is working, don't play notification sound %d %d", ret, status);
		return 0;
	}

	return 1;
}

static int _is_security_lockscreen_launched(void)
{
	int ret = 0;
	int is_idle_lock = 0;

	ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &is_idle_lock);
	retif(ret != 0, 0,"failed to get VCONFKEY_IDLE_LOCK_STATE %d %d", ret, is_idle_lock);

	if (is_idle_lock  == VCONFKEY_IDLE_LOCK ) {
		DBG("Lock screen is launched");
		return 1; //don't show on lock screen
	}

	return 0;
}

static Eina_Bool _activenoti_hide_timer_cb(void *data)
{
	DBG("");

	s_info.delay_timer = NULL;

	_activenoti_hide(data, 0);
	return ECORE_CALLBACK_CANCEL;
}

static void _activenoti_hide(void *data, int delay)
{
	DBG("delay : %d", delay);
	if (delay == 1) {
		if (s_info.delay_timer == NULL) {
			s_info.delay_timer = ecore_timer_add(DELAY_TIMER_VALUE, _activenoti_hide_timer_cb, NULL);
			if (!s_info.delay_timer) {
				ERR("Failed to create a new timer for hide activenoti");
			}
		}
	} else {
		_activenoti_delete_current_noti();

		if (s_info.delay_timer != NULL) {
			ecore_timer_del(s_info.delay_timer);
			s_info.delay_timer = NULL;
		}

		if (s_info.close_timer != NULL) {
			ecore_timer_del(s_info.close_timer);
			s_info.close_timer = NULL;
		}

		if (s_info.activenoti) {
			if (_activenoti_has_pending_noti()) {
				_activenoti_update_activenoti();
			} else {
				DBG("pending noti is null. called evas_object_hide");
				evas_object_hide(s_info.activenoti);
			}
		}
	}
	DBG("");
}

static Evas_Event_Flags __flick_end_cb(void *data, void *event_info)
{
	Elm_Gesture_Line_Info *line_info = (Elm_Gesture_Line_Info *)event_info;

	DBG("line_info->momentum.my : %d", line_info->momentum.my);

	/* Flick Up */
	if (line_info->momentum.my < 0) {
		DBG("HIDE ACTIVE NOTI");
		if (_activenoti_has_pending_noti() == false) {
			_activenoti_hide(NULL, 0);
		} else {
			_activenoti_delete_current_noti();
			_activenoti_update_activenoti();
		}
	} else {
		DBG("HOLD ACTIVE NOTI");
	}
	return EVAS_EVENT_FLAG_ON_HOLD;
}

static Evas_Object *_gesture_create(Evas_Object *layout)
{
	Evas_Object *gesture_layer = NULL;

	gesture_layer = elm_gesture_layer_add(layout);
	retif(!gesture_layer, NULL,);

	elm_gesture_layer_attach(gesture_layer, layout);
	evas_object_show(gesture_layer);

	elm_gesture_layer_cb_set(gesture_layer, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_END, __flick_end_cb, NULL);

	return gesture_layer;
}

static int _check_sound_off(notification_h noti)
{
	notification_system_setting_h system_setting = NULL;
	notification_setting_h setting = NULL;
	char *pkgname = NULL;
	bool do_not_disturb = false;
	bool do_not_disturb_except = false;
	int err = NOTIFICATION_ERROR_NONE;
	int ret = 0;

	retif(noti == NULL, 0, "Invalid parameter!");

	err = notification_system_setting_load_system_setting(&system_setting);
	if (err != NOTIFICATION_ERROR_NONE || system_setting == NULL) {
		DBG("notification_system_setting_load_system_setting failed [%d]\n", err);
		goto out;
	}

	err = notification_system_setting_get_do_not_disturb(system_setting, &do_not_disturb);
	if (err != NOTIFICATION_ERROR_NONE) {
		DBG("notification_system_setting_get_do_not_disturb failed [%d]", err);
		goto out;
	}

	DBG("do_not_disturb [%d]\n", do_not_disturb);

	if (do_not_disturb) {
		err = notification_get_pkgname(noti, &pkgname);
		if (err != NOTIFICATION_ERROR_NONE || pkgname == NULL) {
			DBG("notification_get_pkgname failed [%d]", err);
			goto out;
		}

		err = notification_setting_get_setting_by_package_name(pkgname, &setting);
		if (err != NOTIFICATION_ERROR_NONE || setting == NULL) {
			DBG("notification_setting_get_setting_by_package_name failed [%d]", err);
			goto out;
		}

		notification_setting_get_do_not_disturb_except(setting, &do_not_disturb_except);
		if (err != NOTIFICATION_ERROR_NONE) {
			DBG("notification_setting_get_do_not_disturb_except failed [%d]", err);
			goto out;
		}

		if (do_not_disturb_except) {
			ret = 0;
		} else {
			ret = 1;
		}
	}

out:
	if (system_setting) {
		notification_system_setting_free_system_setting(system_setting);
	}

	if (setting) {
		notification_setting_free_notification(setting);
	}

	return ret;
}

static void _gesture_destroy(void)
{
	if (s_info.gesture) {
		evas_object_del(s_info.gesture);
		s_info.gesture = NULL;
	} else {
		ERR("s_info.gesture is NULL");
	}
}

static inline void _activenoti_only_noti_del(notification_h noti)
{
	retif(noti == NULL, ,"Invalid parameter!");
	int applist = NOTIFICATION_DISPLAY_APP_ALL;

	notification_get_display_applist(noti, &applist);
	if (applist & NOTIFICATION_DISPLAY_APP_ACTIVE) {
		if (!(applist & NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY)) {
			char *pkgname = NULL;
			int priv_id = 0;

			notification_get_pkgname(noti, &pkgname);
			notification_get_id(noti, NULL, &priv_id);
			notification_delete_by_priv_id(pkgname, NOTIFICATION_TYPE_NONE, priv_id);
		}
	}
}

static Eina_Bool _activenoti_close_timer_cb(void *data)
{
	DBG("%d", _activenoti_has_pending_noti());

	s_info.close_timer = NULL;

	_activenoti_hide(data, 0);

	return ECORE_CALLBACK_CANCEL;
}

static void _activenoti_delete_current_noti(void)
{
	if (!s_info.current_noti) {
		DBG("There is no displaying notification");
		return;
	}

	DBG("");
	notification_free(s_info.current_noti);
	s_info.current_noti = NULL;
}

static Evas_Object *_activenoti_create_badge(Evas_Object *parent, notification_h noti)
{
	DBG("");
	retif(noti == NULL || parent == NULL, NULL, "Invalid parameter!");

	char *icon_path = NULL;
	Evas_Object *icon = NULL;
	int ret = NOTIFICATION_ERROR_NONE;

	ret = notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB, &icon_path);
	if (ret != NOTIFICATION_ERROR_NONE || icon_path == NULL) {
		DBG("notification_get_image failed [%d]", ret);;
		return NULL;
	}

	DBG("NOTIFICATION_IMAGE_TYPE_ICON_SUB :  %s", icon_path);

	icon = elm_image_add(parent);
	if (icon == NULL) {
		//		free(icon_path);
		DBG("icon error");
		return NULL;
	}

	elm_image_resizable_set(icon, EINA_FALSE, EINA_TRUE);

	ret = elm_image_file_set(icon, icon_path, NULL);
	//free(icon_path);

	if (ret == EINA_FALSE) {
		evas_object_del(icon);
		return NULL;
	}

	return icon;
}

static void _image_press_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	DBG("");
	app_control_h app_control = data;

	if (app_control) {
		int ret = APP_CONTROL_ERROR_NONE;
		ret = app_control_send_launch_request(app_control, NULL, NULL);
		DBG("app_control_send_launch_request return [%d]", ret);
	}

	_activenoti_hide(NULL, 1);
}

static Eina_Bool _delete_timer_cb(void *data)
{
	DBG("");
	struct noti_info *noti = data;

	s_info.auto_remove_list = eina_list_remove(s_info.auto_remove_list, noti);
	notification_free(noti->noti);
	free(noti);

	return ECORE_CALLBACK_CANCEL;
}

static int _compare_noti_time(notification_h noti1, notification_h noti2)
{
	time_t noti_time1 = 0;
	time_t noti_time2 = 0;
	int ret;

	ret = notification_get_time(noti1, &noti_time1);
	if (ret != NOTIFICATION_ERROR_NONE || noti_time1 == 0)  {
		notification_get_insert_time(noti1, &noti_time1);
	}

	ret = notification_get_time(noti2, &noti_time2);
	if (ret != NOTIFICATION_ERROR_NONE || noti_time2 == 0)  {
		notification_get_insert_time(noti2, &noti_time2);
	}

	DBG("not_time1 : %d noti_time2 : %d", noti_time1, noti_time2);

	return (int)(noti_time1 - noti_time2);
}

static int _compare_time_cb(const void *data1, const void *data2)
{
	struct noti_info *info1 = (struct noti_info *)data1;
	struct noti_info *info2 = (struct noti_info *)data2;

	return _compare_noti_time(info1->noti, info2->noti);
}

static void _activenoti_remove_list(void)
{
	struct noti_info *info;
	char *tmp;

	EINA_LIST_FREE(s_info.auto_remove_list, info) {
		/**
		 * For debugging
		 */
		tmp = _get_text(info->noti, NOTIFICATION_TEXT_TYPE_TITLE);
		DBG("auto remove %s", tmp);
		free(tmp);

		free(info->noti);
		free(info->timer);
	}

	EINA_LIST_FREE(s_info.non_auto_remove_list, info) {
		/**
		 * For debugging
		 */
		tmp = _get_text(info->noti, NOTIFICATION_TEXT_TYPE_TITLE);
		DBG("nont auto remove %s", tmp);
		free(tmp);

		free(info->noti);
	}
}

static bool _activenoti_add_in_list(notification_h noti)
{
	DBG("");
	bool auto_remove;
	struct noti_info *info;
	int	ret;
	time_t noti_time = 0;

	ret = notification_get_auto_remove( noti, &auto_remove);
	if (ret != NOTIFICATION_ERROR_NONE) {
		DBG("notification_get_auto_remove return [%d]", ret);
		return false;
	}

	info = malloc(sizeof(*info));
	if (!info) {
		DBG("malloc error, can't make noti_info");
		return false;
	}

	ret = notification_clone(noti, &info->noti);
	if (ret != NOTIFICATION_ERROR_NONE || !info->noti) {
		free(info);
		ERR("failed to create a cloned notification");
		return false;
	}

	if (auto_remove == true) {
		ret = notification_get_time(info->noti, &noti_time);
		if (ret!=NOTIFICATION_ERROR_NONE || noti_time == 0) {
			ret = notification_get_insert_time(info->noti, &noti_time);
			if (ret != NOTIFICATION_ERROR_NONE || noti_time == 0) {
				DBG("notification_get_insert_time failed. time is 0 ret %d", ret);
			}
		}

		noti_time -= time(NULL);

		if (noti_time > 8.0f) {
			DBG("remove noti");
			info->timer = NULL;
			_delete_timer_cb(info);
		} else {
			info->timer = ecore_timer_add(8.0f - noti_time, _delete_timer_cb, info);
			DBG("timer add");
			if(!info->timer) {
				DBG("ecore_timer_add failed");
			}
			/**
			 * @todo
			 * Revise this.
			 * Do we need to replace this with eina_list_prepend?
			 */
			s_info.auto_remove_list = eina_list_sorted_insert(s_info.auto_remove_list, _compare_time_cb, info);
		}
	} else {
		info->timer = NULL;
		s_info.non_auto_remove_list = eina_list_sorted_insert(s_info.non_auto_remove_list, _compare_time_cb, info);
	}

	return 1;
}

static bool _activenoti_has_pending_noti(void)
{

	return s_info.non_auto_remove_list || s_info.auto_remove_list;
}

static bool _activenoti_remove_in_list(notification_h noti)
{
	DBG("");

	bool auto_remove;
	int ret;
	Eina_List *l;
	Eina_List *n;
	struct noti_info *info;

	ret = notification_get_auto_remove(noti, &auto_remove);
	if (ret != NOTIFICATION_ERROR_NONE) {
		DBG("notification_get_auto_remove return [%d]", ret);
		return false;
	}

	if (auto_remove) {
		DBG("remove in auto remove list");
		EINA_LIST_FOREACH_SAFE(s_info.auto_remove_list, l, n, info) {
			if (info->noti != noti) {
				int priv_id_from_list;
				int priv_id_from_arg;

				notification_get_id(info->noti, NULL, &priv_id_from_list);
				notification_get_id(noti, NULL, &priv_id_from_arg);

				if (priv_id_from_list != priv_id_from_arg) {
					continue;
				}
			}
			s_info.auto_remove_list = eina_list_remove(s_info.auto_remove_list, info);
			ecore_timer_del(info->timer);
			notification_free(info->noti);
			free(info);
			break;
		}
	} else {
		DBG("remove in non auto remove list");
		EINA_LIST_FOREACH_SAFE(s_info.non_auto_remove_list, l, n, info) {
			if (info->noti != noti) {
				int priv_id_from_list;
				int priv_id_from_arg;

				notification_get_id(info->noti, NULL, &priv_id_from_list);
				notification_get_id(noti, NULL, &priv_id_from_arg);

				if (priv_id_from_list != priv_id_from_arg) {
					continue;
				}
			}
			s_info.non_auto_remove_list = eina_list_remove(s_info.non_auto_remove_list, info);
			notification_free(info->noti);
			free(info);
			break;
		}
	}
	return 1;
}

static notification_h _activenoti_get_in_list(notification_h cur_noti)
{
	DBG("");
	notification_h noti = NULL;
	struct noti_info *info;
	bool auto_remove = true;
	int ret;

	if (cur_noti != NULL) {
		ret = notification_get_auto_remove(cur_noti, &auto_remove);
		if (ret != NOTIFICATION_ERROR_NONE) {
			DBG("notification_get_auto_remove return [%d]", ret);
			return NULL;
		}
	}

	info = eina_list_nth(s_info.non_auto_remove_list, 0);
	if (info) {
		if (!auto_remove) {
			ret = _compare_noti_time(cur_noti, info->noti);

			if (ret > 0) {
				return NULL;
			}
		}
		s_info.non_auto_remove_list = eina_list_remove(s_info.non_auto_remove_list, info);
		// .....
		noti = info->noti;
		free(info);
		return noti;
	}

	if (auto_remove == false) {
		return noti;
	}

	info = eina_list_nth(s_info.auto_remove_list, 0);
	if (info) {
		s_info.auto_remove_list = eina_list_remove(s_info.auto_remove_list, info);
		// .....
		noti = info->noti;
		ecore_timer_del(info->timer);
		free(info);
		return noti;
	}

	return noti;
}

static Evas_Object *_activenoti_create_icon(Evas_Object *parent, notification_h noti)
{
	DBG("");
	retif(noti == NULL || parent == NULL , NULL, "Invalid parameter!");

	char *icon_path;
	char *tmp;
	Evas_Object *icon;
	int ret;
	app_control_h app_control;

	retif(noti == NULL || parent == NULL, NULL, "Invalid parameter!");

	tmp = NULL;
	ret = notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, &tmp);
	if (ret == NOTIFICATION_ERROR_NONE && tmp != NULL) {
		icon_path = strdup(tmp);
		if (!icon_path) {
			ERR("strdup: %s", tmp);
		}

		app_control = NULL;
		ret = notification_get_event_handler(noti, NOTIFICATION_EVENT_TYPE_CLICK_ON_ICON, &app_control);
		if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Failed to get handler");
		}

		DBG("icon_path :  %s app_control = %p", icon_path, app_control);
	} else {
		tmp = NULL;
		ret = notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL, &tmp);
		if (ret == NOTIFICATION_ERROR_NONE && tmp != NULL) {
			icon_path = strdup(tmp);
			if (!icon_path) {
				ERR("strdup: %s", tmp);
			}

			app_control = NULL;
			ret = notification_get_event_handler(noti, NOTIFICATION_EVENT_TYPE_CLICK_ON_THUMBNAIL, &app_control);
			if (ret != NOTIFICATION_ERROR_NONE) {
				ERR("Failed to get handler");
			}

			DBG("thumb_path :  %s app_control = %p", icon_path, app_control);
		} else  {
			char *pkgname;

			icon_path = NULL;
			pkgname = NULL;
			ret = notification_get_pkgname(noti, &pkgname);
			if (ret == NOTIFICATION_ERROR_NONE && pkgname != NULL) {
				icon_path = quickpanel_common_ui_get_pkginfo_icon(pkgname);
				DBG("default_pkgicon_path :  %s", icon_path);
			}

			if (!icon_path) {
				icon_path = strdup(DEFAULT_ICON);
				DBG("default_path :  %s", icon_path);
			}

			app_control = NULL;
		}
	}

	icon = elm_image_add(parent);
	if (!icon) {
		ERR("Failed to create an image object");
		free(icon_path);
		if (app_control) {
			app_control_destroy(app_control);
		}
		return NULL;
	}

	ret = elm_image_file_set(icon, icon_path, NULL);
	free(icon_path);
	if (ret == EINA_FALSE) {
		evas_object_del(icon);
		if (app_control) {
			app_control_destroy(app_control);
		}
		return NULL;
	}
	elm_image_resizable_set(icon, EINA_FALSE, EINA_TRUE);

	elm_object_signal_callback_add(icon, "image_press" , "", _image_press_cb, app_control);
	evas_object_event_callback_add(icon, EVAS_CALLBACK_DEL, _app_control_del_cb, app_control);

	return icon;
}

static inline char *_get_text(notification_h noti, notification_text_type_e text_type)
{
	time_t time = 0;
	char *text = NULL;
	char buf[ACTIVENOTI_MSG_LEN] = {0,};

	if (notification_get_time_from_text(noti, text_type, &time) == NOTIFICATION_ERROR_NONE) {
		if ((int)time > 0) {
			quickpanel_noti_util_get_time(time, buf, sizeof(buf));
			text = buf;
		}
	} else {
		notification_get_text(noti, text_type, &text);
	}

	DBG("text : %s", text);

	if (text != NULL) {
		return elm_entry_utf8_to_markup(text);
	}

	return NULL;
}

static inline void _strbuf_add(Eina_Strbuf *str_buf, char *text, const char *delimiter)
{
	if (text != NULL) {
		if (strlen(text) > 0) {
			if (delimiter != NULL) {
				eina_strbuf_append(str_buf, delimiter);
			}
			eina_strbuf_append(str_buf, text);
		}
	}
}

static inline void _check_and_add_to_buffer(Eina_Strbuf *str_buf, char *text, int is_check_phonenumber)
{
	char buf_number[QP_UTIL_PHONE_NUMBER_MAX_LEN * 2] = { 0, };

	if (text != NULL) {
		if (strlen(text) > 0) {
			if (quickpanel_common_util_is_phone_number(text) && is_check_phonenumber) {
				quickpanel_common_util_phone_number_tts_make(buf_number, text,
						(QP_UTIL_PHONE_NUMBER_MAX_LEN * 2) - 1);
				eina_strbuf_append(str_buf, buf_number);
			} else {
				eina_strbuf_append(str_buf, text);
			}
			eina_strbuf_append_char(str_buf, '\n');
		}
	}
}

static void _activenoti_set_text(notification_h noti, int is_screenreader)
{
	char *domain = NULL;
	char *dir = NULL;
	char *tmp;
	int ret;

	if (!noti) {
		ERR("Invalid parameter");
		return;
	}

	ret = notification_get_text_domain(noti, &domain, &dir);
	if (ret == NOTIFICATION_ERROR_NONE && domain != NULL && dir != NULL) {
		bindtextdomain(domain, dir);
	}

	tmp = _get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_1);
	if (tmp != NULL) {
		elm_object_part_text_set(s_info.layout, "subtitle_text", tmp);
		free(tmp);
		elm_object_signal_emit(s_info.layout, "sub_show", "subtitle_text");
	} else {
		elm_object_signal_emit(s_info.layout, "sub_hide", "subtitle_text");
	}

	tmp = _get_text(noti, NOTIFICATION_TEXT_TYPE_TITLE);
	if (tmp != NULL) {
		elm_object_part_text_set(s_info.layout, "title_text", tmp);
		free(tmp);
	} else {
		elm_object_part_text_set(s_info.layout, "title_text", "");
	}

	tmp = _get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT);
	if (tmp != NULL) {
		elm_object_part_text_set(s_info.layout, "content_text", tmp);
		free(tmp);
	} else {
		elm_object_part_text_set(s_info.layout, "content_text", "");
	}
}

static void _handle_press_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	DBG("");
	_activenoti_hide(data, 0);
}

static void _noti_press_cb(void *data, Evas_Object *obj,	const char *emission, const char *source)
{
	DBG("");
	int ret = APP_CONTROL_ERROR_NONE;
	char *caller_pkgname = NULL;
	bundle *responding_service_handle = NULL;
	bundle *single_service_handle = NULL;
	bundle *multi_service_handle = NULL;
	int flags = 0, group_id = 0, priv_id = 0, count = 0, flag_launch = 0;
	notification_type_e type = NOTIFICATION_TYPE_NONE;
	notification_h noti = NULL;

	retif(s_info.activenoti == NULL, , "Invalid parameter!");
	retif(s_info.current_noti == NULL, , "Invalid parameter!");

	noti = s_info.current_noti;
	notification_get_pkgname(noti, &caller_pkgname);
	notification_get_id(noti, &group_id, &priv_id);
	notification_get_property(noti, &flags);
	notification_get_type(noti, &type);

	if (flags & NOTIFICATION_PROP_DISABLE_APP_LAUNCH) {
		flag_launch = 0;
	} else {
		flag_launch = 1;
	}

	ret = notification_get_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_RESPONDING,	NULL, &responding_service_handle);
	if (ret != NOTIFICATION_ERROR_NONE || responding_service_handle == NULL) {
		DBG("NOTIFICATION_EXECUTE_TYPE_RESPONDING failed [%d]", ret);
	}

	ret = notification_get_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH, NULL, &single_service_handle);
	if (ret != NOTIFICATION_ERROR_NONE || single_service_handle == NULL) {
		DBG("NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH failed [%d]", ret);
	}

	ret = notification_get_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_MULTI_LAUNCH, NULL, &multi_service_handle);
	if (ret != NOTIFICATION_ERROR_NONE || multi_service_handle == NULL) {
		DBG("NOTIFICATION_EXECUTE_TYPE_MULTI_LAUNCH failed [%d]", ret);
	}

	if (responding_service_handle != NULL) {
		DBG("responding_service_handle : %s", responding_service_handle);
		ret = quickpanel_uic_launch_app(NULL, responding_service_handle);
	} else if (flag_launch == 1) {
		char *text_count = NULL;
		notification_get_text(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT, &text_count);

		if (text_count != NULL) {
			count = atoi(text_count);
		} else {
			count = 1;
		}

		if (single_service_handle != NULL && multi_service_handle == NULL) {
			ret = quickpanel_uic_launch_app(NULL, single_service_handle);
		} else if (single_service_handle == NULL && multi_service_handle != NULL) {
			ret = quickpanel_uic_launch_app(NULL, multi_service_handle);
		} else if (single_service_handle != NULL && multi_service_handle != NULL) {
			if (count <= 1) {
				ret = quickpanel_uic_launch_app(NULL, single_service_handle);
			} else {
				ret = quickpanel_uic_launch_app(NULL, multi_service_handle);
			}
		} else { //single_service_handle == NULL && multi_service_handle == NULL
			DBG("there is no execution option in notification");
		}
		quickpanel_uic_launch_app_inform_result(caller_pkgname, ret);
	}

	_activenoti_hide(data , 1);

}

static void _button_press_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	app_control_h app_control = data;
	int ret = APP_CONTROL_ERROR_NONE;

	ret = app_control_send_launch_request(app_control, NULL, NULL);
	DBG("app_control_send_launch_request return [%s]", ret);

	_activenoti_hide(NULL, 1);
}

static Evas_Object *_get_btn_img(Evas_Object *parent, notification_h noti, int btn_num)
{
	retif(noti == NULL || parent == NULL, NULL, "Invalid parameter!");

	char *btn_path = NULL;
	Evas_Object *btn_img = NULL;
	int ret;

	ret = notification_get_image(noti, btn_num + NOTIFICATION_IMAGE_TYPE_BUTTON_1, &btn_path);
	if (ret != NOTIFICATION_ERROR_NONE || btn_path == NULL) {
		DBG("notification_get_image return [%d]", ret);
		return NULL;
	}

	btn_img = elm_image_add(parent);
	if (!btn_img) {
		//free(btn_path);
		return NULL;
	}

	evas_object_size_hint_weight_set(btn_img, EVAS_HINT_EXPAND,	EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(btn_img, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_image_resizable_set(btn_img, EINA_TRUE, EINA_TRUE);

	ret = elm_image_file_set(btn_img, btn_path, NULL);
	//free(btn_path);
	if (ret == EINA_FALSE) {
		evas_object_del(btn_img);
		return NULL;
	}

	return btn_img;
}

static Evas_Object *_get_bg_img(Evas_Object *parent, notification_h noti)
{
	char *bg_path = NULL;
	Evas_Object *bg_img = NULL;
	int ret;

	if (!parent || !noti) {
		ERR("Invalid parameters %p %p", parent, noti);
		return NULL;
	}

	ret = notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND, &bg_path);
	if (ret != NOTIFICATION_ERROR_NONE || bg_path == NULL) {
		DBG("bg_path is null ret = %d", ret);
		return NULL;
	}

	bg_img = elm_image_add(parent);
	if (!bg_img) {
		return NULL;
	}

	evas_object_size_hint_weight_set(bg_img, EVAS_HINT_EXPAND,	EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(bg_img, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_image_resizable_set(bg_img, EINA_TRUE, EINA_TRUE);

	ret = elm_image_file_set(bg_img, bg_path, NULL);
	if (ret == EINA_FALSE) {
		evas_object_del(bg_img);
		return NULL;
	}

	return bg_img;
}

static int _activenoti_create_button(Evas_Object *obj, notification_h noti)
{
	int btn_cnt;
	int ret;
	app_control_h app_control;

	if (!obj || !noti) {
		ERR("Invalid parameters");
		return 0;
	}

	if (s_info.btnbox) { //if exist, delete and create
		evas_object_del(s_info.btnbox);
		s_info.btnbox = NULL;
	}

	Evas_Object *box;
	box = elm_box_add(obj);

	if(box == NULL) {
		ERR("box is null");
		return 0;
	}

	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(box, EINA_TRUE);
	evas_object_show(box);
	s_info.btnbox = box;

	for (btn_cnt = 0; btn_cnt < 3; btn_cnt++) {
		app_control = NULL;
		ret = notification_get_event_handler(noti, btn_cnt + NOTIFICATION_EVENT_TYPE_CLICK_ON_BUTTON_1, &app_control);
		DBG("appcontrol %p", app_control);
		if(ret != NOTIFICATION_ERROR_NONE || app_control == NULL) {
			INFO("no more button, button count is %d", btn_cnt);
			INFO("ret is %d", ret);

			/**
			 * @note
			 * In this case,
			 * The app_control is not used and no one cares it.
			 * So we have to release it from here if it is allocated.
			 */
			if (app_control) {
				app_control_destroy(app_control);
			}

			if (btn_cnt == 0) { // noti doesn't have button
				evas_object_del(s_info.btnbox);
				s_info.btnbox = NULL;
				return 0;
			}
		} else {
			Evas_Object *bt_layout;
			char *btn_text;
			Evas_Object *image;

			bt_layout = elm_layout_add(s_info.btnbox);
			if(bt_layout == NULL) {
				ERR("bt_layout is null");
				evas_object_del(s_info.btnbox);
				app_control_destroy(app_control);
				s_info.btnbox = NULL;
				return 0;
			}

			elm_layout_file_set(bt_layout, ACTIVENOTI_EDJ, "button_layout");
			evas_object_size_hint_weight_set (bt_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(bt_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

			image = _get_btn_img(bt_layout, noti, btn_cnt);
			if (image != NULL) {
				elm_object_part_content_set(bt_layout, "content.button.image", image);
			}

			btn_text = _get_text(noti, btn_cnt + NOTIFICATION_TEXT_TYPE_BUTTON_1);
			if (btn_text != NULL) {
				elm_object_part_text_set(bt_layout, "content.button.text", btn_text);
				free(btn_text);
			}
			elm_object_signal_callback_add(bt_layout, "button_clicked" , "", _button_press_cb, app_control);
			evas_object_event_callback_add(bt_layout, EVAS_CALLBACK_DEL, _app_control_del_cb, app_control);

			evas_object_show(bt_layout);
			elm_box_pack_end(s_info.btnbox,bt_layout);
		}
	}

	elm_object_part_content_set(obj, "button.swallow", s_info.btnbox);
	return btn_cnt;
}

static void _activenoti_create_activenoti(void)
{
	DBG("");
	Eina_Bool ret = EINA_FALSE;
	Evas_Object *base = NULL;
	int w, h;

	if (s_info.activenoti != NULL) {
		ERR("Instant notification exists");
		return;
	}

	s_info.activenoti = quickpanel_noti_win_add(NULL);
	retif(s_info.activenoti == NULL, , "Failed to add elm activenoti.");

	s_info.layout = elm_layout_add(s_info.activenoti);
	if (!s_info.layout) {
		ERR("Failed to get detailview.");
		_activenoti_destroy_activenoti();
		return;
	}

	ret = elm_layout_file_set(s_info.layout, ACTIVENOTI_EDJ, "headsup/base");
	retif(ret == EINA_FALSE, , "failed to load layout");

	elm_object_signal_callback_add(s_info.layout, "noti_press" , "", _noti_press_cb, NULL);
	elm_object_signal_callback_add(s_info.layout, "del" , "", _handle_press_cb, NULL);

	evas_object_geometry_get(s_info.activenoti, NULL, NULL, &w, &h);
	DBG("evas_object_geometry_get x %d y %d", w, h);

	elm_win_resize_object_add(s_info.activenoti, s_info.layout);
	evas_object_show(s_info.layout);

	/* create base rectangle */
	base = evas_object_rectangle_add(evas_object_evas_get(s_info.layout));
	if (!base) {
		ERR("Failed to get detailview.");
		_activenoti_destroy_activenoti();
		return;
	}

	evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_color_set(base, 0, 165, 198, 255);

	elm_object_part_content_set(s_info.layout, "background", base);

	quickpanel_noti_win_content_set(s_info.activenoti, s_info.layout);

	s_info.gesture = _gesture_create(s_info.layout);

	_activenoti_win_rotated(quickpanel_get_app_data(), 0);
}

static void _activenoti_update_activenoti(void)
{
	ERR("");
	Eina_Bool ret = EINA_FALSE;
	notification_h noti;
	Evas_Object *icon = NULL;
	Evas_Object *badge = NULL;
	Evas_Object *bg_img = NULL;
	int btn_cnt = 0;
	bool auto_remove = true;
	bool current_auto_remove = true;

	if (s_info.activenoti == NULL) {
		ERR("Active notification doesn't exist");
		return;
	}

	if (s_info.layout == NULL) {
		ERR("Active notification doesn't exist");
		return;
	}

	if (s_info.delay_timer != NULL) {
		ERR("s_info.delay_timer");
		ecore_timer_del(s_info.delay_timer);
		s_info.delay_timer = NULL;
	}

	if (s_info.close_timer != NULL) {
		ERR("s_info.close_timer");
		ecore_timer_del(s_info.close_timer);
		s_info.close_timer = NULL;
	}

	noti = _activenoti_get_in_list(s_info.current_noti);
	if (noti == NULL) {
		DBG("noti is null");
		return;
	}

	if (s_info.current_noti) {
		ret = notification_get_auto_remove(s_info.current_noti, &current_auto_remove);
		if (ret != NOTIFICATION_ERROR_NONE) {
			DBG("notification_get_auto_remove return [%s] from current_noti", ret);
			return;
		}
		if (!current_auto_remove) {
			DBG("!auto_remove");
			ret = notification_get_auto_remove(noti, &auto_remove);
			if (ret != NOTIFICATION_ERROR_NONE) {
				DBG("notification_get_auto_remove return [%s] from new noti", ret);
				return;
			}

			if (auto_remove) {
				DBG("auto_remove");
				_activenoti_add_in_list(noti); // timer
				return; // check!!!
			} else {
				DBG("!auto_remove");
				_activenoti_add_in_list(s_info.current_noti);
			}
		}
	} else {
		ret = notification_get_auto_remove(noti, &current_auto_remove);
		if (ret != NOTIFICATION_ERROR_NONE) {
			DBG("notification_get_auto_remove return [%s] from current_noti", ret);
			return;
		}
	}

	s_info.current_noti = noti;

	if (current_auto_remove == true) {
		time_t noti_time = 0;
		ret = notification_get_time(s_info.current_noti, &noti_time);
		if (ret != NOTIFICATION_ERROR_NONE || noti_time == 0)	{
			notification_get_insert_time(s_info.current_noti, &noti_time);
		}
		noti_time = time(NULL) - noti_time;
		s_info.close_timer = ecore_timer_add(DEL_TIMER_VALUE - noti_time, _activenoti_close_timer_cb, NULL);
	}

	bg_img = elm_object_part_content_unset(s_info.layout, "bg_img");
	DBG("bg_img %p", bg_img);
	if(bg_img != NULL) {
		evas_object_del(bg_img);
		bg_img = NULL;
	}

	bg_img = _get_bg_img(s_info.layout , s_info.current_noti);
	if (bg_img != NULL) {
		elm_object_part_content_set(s_info.layout, "bg_img", bg_img);
	}

	btn_cnt = _activenoti_create_button(s_info.layout, s_info.current_noti);
	if (btn_cnt == 0) { //no button
		elm_object_signal_emit(s_info.layout, "btn_hide", "button.space");
	} else {
		elm_object_signal_emit(s_info.layout, "btn_show", "button.space");
	}

	icon = elm_object_part_content_unset(s_info.layout, "icon_big");
	DBG("icon %p", icon);
	if(icon != NULL) {
		evas_object_del(icon);
		icon = NULL;
	}

	icon = _activenoti_create_icon(s_info.layout, s_info.current_noti);
	if (icon != NULL) {
		elm_object_part_content_set(s_info.layout, "icon_big", icon);

		badge = elm_object_part_content_unset(s_info.layout, "icon_badge");
		DBG("badget %p", badge);
		if(badge != NULL) {
			evas_object_del(badge);
		}

		badge = _activenoti_create_badge(s_info.layout, s_info.current_noti);
		if (badge != NULL) {
			elm_object_part_content_set(s_info.layout, "icon_badge", badge);
		} else {
			INFO("badge is NULL");
		}
		DBG("");
	} else {
		INFO("icon is NULL");
	}

	_activenoti_set_text(noti, 0);

	evas_object_show(s_info.activenoti);

	SERR("activenoti noti is updated");
}

static void _activenoti_destroy_activenoti(void)
{
	retif(!s_info.activenoti,,"s_info->activenoti is null");

	_gesture_destroy();

	if (s_info.delay_timer != NULL) {
		ecore_timer_del(s_info.delay_timer);
		s_info.delay_timer = NULL;
	}

	if (s_info.close_timer != NULL) {
		ecore_timer_del(s_info.close_timer);
		s_info.close_timer = NULL;
	}

	if (s_info.btnbox) {
		evas_object_del(s_info.btnbox);
		s_info.btnbox = NULL;
	}

	if (s_info.layout) {
		evas_object_del(s_info.layout);
		s_info.layout = NULL;
	}

	if (s_info.activenoti) {
		evas_object_del(s_info.activenoti);
		s_info.activenoti = NULL;
	}
}

static void _activenoti_win_rotated(void *data, int need_hide)
{
	retif(data == NULL, ,"data is NULL");
	int angle = 0;
	struct appdata *ad = data;

	if (s_info.activenoti != NULL) {
		angle = elm_win_rotation_get(s_info.activenoti);

		if (((angle == 0 || angle == 180) && (ad->angle == 90 || ad->angle == 270))
				|| ((angle == 90 || angle == 270) && (ad->angle == 0 || ad->angle == 180))) {
			if (need_hide == 1) {
				evas_object_hide(s_info.activenoti);
			}
		}
	}
}

static void _media_feedback_sound(notification_h noti)
{
	retif(noti == NULL, ,"op_list is NULL");
	int ret = 0, priv_id = 0;
	const char *nsound_path = NULL;
	notification_sound_type_e nsound_type = NOTIFICATION_SOUND_TYPE_NONE;
	char *default_msg_tone = NULL;

	notification_get_id(noti, NULL, &priv_id);
	notification_get_sound(noti, &nsound_type, &nsound_path);
	SDBG("notification sound: %d, %s", nsound_type, nsound_path);

	switch (nsound_type) {
	case NOTIFICATION_SOUND_TYPE_USER_DATA:
		/*
		 *  if user data file isn't playable, play the default ringtone
		 */
		if (nsound_path != NULL) {
			if (quickpanel_media_playable_check(nsound_path) == EINA_TRUE) {
				ret = quickpanel_media_player_play(SOUND_TYPE_NOTIFICATION, nsound_path);
				if (quickpanel_media_player_is_drm_error(ret) == 1) {
					ERR("failed to play notification sound due to DRM problem");
					ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_SOUND_NOTIFICATION, &default_msg_tone);
					msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "ailed to get key(%s) : %d", SYSTEM_SETTINGS_KEY_SOUND_NOTIFICATION, ret);


					if (default_msg_tone != NULL) {
						SDBG("setting sound[%s]", default_msg_tone);
						ret = quickpanel_media_player_play(SOUND_TYPE_NOTIFICATION, default_msg_tone);
						free(default_msg_tone);
					}
				}
				if (ret == PLAYER_ERROR_NONE) {
					quickpanel_media_player_id_set(priv_id);
				} else {
					ERR("failed to play notification sound");
				}
				break;
			} else {
				ERR("playable false, So unlock tone");
				feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_UNLOCK);
			}
		} else {
			ERR("sound path null");
		}

		break;
	case NOTIFICATION_SOUND_TYPE_DEFAULT:
		ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_SOUND_NOTIFICATION, &default_msg_tone);
		msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "ailed to get key(%s) : %d", SYSTEM_SETTINGS_KEY_SOUND_NOTIFICATION, ret);

		if (default_msg_tone != NULL) {
			SDBG("Reminded setting sound[%s]", default_msg_tone);
			ret = quickpanel_media_player_play(SOUND_TYPE_NOTIFICATION, default_msg_tone);
			free(default_msg_tone);
			if (ret == PLAYER_ERROR_NONE) {
				quickpanel_media_player_id_set(priv_id);
			} else {
				ERR("failed to play notification sound(default)");
			}
		}
		break;
	case NOTIFICATION_SOUND_TYPE_MAX:
	case NOTIFICATION_SOUND_TYPE_NONE:
		ERR("None type: No sound");
		break;

	default:
		ERR("UnKnown type[%d]", (int)nsound_type);
		break;
	}
}

static void _media_feedback_vibration(notification_h noti)
{
	retif(noti == NULL, , "Invalid parameter!");

	/* Play Vibration */
	notification_vibration_type_e nvibration_type = NOTIFICATION_VIBRATION_TYPE_NONE;
	const char *nvibration_path = NULL;

	notification_get_vibration(noti, &nvibration_type, &nvibration_path);
	DBG("notification vibration: %d, %s", nvibration_type, nvibration_path);
	switch (nvibration_type) {
	case NOTIFICATION_VIBRATION_TYPE_USER_DATA:
	case NOTIFICATION_VIBRATION_TYPE_DEFAULT:
		feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_MESSAGE);
		break;
	case NOTIFICATION_VIBRATION_TYPE_MAX:
	case NOTIFICATION_VIBRATION_TYPE_NONE:
		break;
	}
}

static void _activenoti_noti_detailed_changed_cb(void *data, notification_type_e type, notification_op *op_list, int num_op)
{
	DBG("");
	retif(op_list == NULL, ,"op_list is NULL");

	notification_h noti = NULL;
	int flags = 0;
	int applist = NOTIFICATION_DISPLAY_APP_ALL;
	int op_type = 0;
	int priv_id = 0;

	notification_op_get_data(op_list, NOTIFICATION_OP_DATA_TYPE, &op_type);
	notification_op_get_data(op_list, NOTIFICATION_OP_DATA_PRIV_ID, &priv_id);
	notification_op_get_data(op_list, NOTIFICATION_OP_DATA_NOTI, &noti);

	DBG("op_type:%d", op_type);
	DBG("op_priv_id:%d", priv_id);
	DBG("noti:%p", noti);

	if( op_type == NOTIFICATION_OP_DELETE) {
		DBG("NOTIFICATION_OP_DELETE");
		int priv_id_current = 0;

		if (s_info.current_noti) {
			notification_get_id(s_info.current_noti, NULL, &priv_id_current);
			if (s_info.current_noti == noti || priv_id_current == priv_id) {
				_activenoti_hide(NULL, 0);
				return;
			}
		}

		_activenoti_remove_in_list(noti);

		return;
	} else if (op_type == NOTIFICATION_OP_DELETE_ALL) {
		if(s_info.current_noti) {
			_activenoti_hide(NULL, 0);
		}
		_activenoti_remove_list();
	}

	retif(noti == NULL, ,"noti is NULL");

	if (op_type == NOTIFICATION_OP_INSERT || op_type == NOTIFICATION_OP_UPDATE) {
		if (_is_sound_playable() == 1) {
			if (_check_sound_off(noti) == 0) {
				DBG("try to play notification sound %x", pthread_self());
				_media_feedback_sound(noti);
				if (quickpanel_media_is_vib_enabled() == 1
						|| quickpanel_media_is_sound_enabled() == 1) {
					_media_feedback_vibration(noti);
				}
			}

		}
	}

	notification_get_display_applist(noti, &applist);
	DBG("applist : %x" ,applist);

	/* Check activenoti flag */
	notification_get_property(noti, &flags);

	if (applist & NOTIFICATION_DISPLAY_APP_ACTIVE) {
		if (_is_security_lockscreen_launched() || _check_sound_off(noti) == 1 ) {
			INFO("lock screen is launched");
			return;
		}

		if (quickpanel_uic_is_opened() && (applist & NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) ) {
			ERR("quickpanel is opened, activenoti will be not displayed");
			return;
		}

		/* wait if s_info.activenoti is not NULL */
		_activenoti_add_in_list(noti);

		_activenoti_create_activenoti();
		if (s_info.activenoti == NULL) {
			ERR("Fail to create activenoti");
			_activenoti_only_noti_del(noti);
			return;
		}

		_activenoti_update_activenoti();
	}
}

/*****************************************************************************
 *
 * Util functions
 *
 *****************************************************************************/
static Eina_Bool _activenoti_callback_register_idler_cb(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, EINA_FALSE, "Invalid parameter!");

	notification_register_detailed_changed_cb(_activenoti_noti_detailed_changed_cb, ad);

	return EINA_FALSE;
}

static int _activenoti_init(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	/* Register notification changed cb */
	ecore_idler_add(_activenoti_callback_register_idler_cb, ad);
	return QP_OK;
}

static int _activenoti_fini(void *data)
{
	// struct appdata *ad = (struct appdata *)data;

	_activenoti_destroy_activenoti();

	return QP_OK;
}

static int _activenoti_enter_hib(void *data)
{
	return QP_OK;
}

static int _activenoti_leave_hib(void *data)
{
	return QP_OK;
}

static void _activenoti_reflesh(void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	if (s_info.activenoti != NULL) {
		_activenoti_win_rotated(data, 1);
	}
}

static void _activenoti_qp_opened(void *data)
{
	DBG("");
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	if (_activenoti_has_pending_noti()) {
		_activenoti_remove_list();
	}
	_activenoti_hide(NULL, 0);
}

