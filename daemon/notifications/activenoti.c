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
#include <appcore-common.h>
#include <vconf.h>
#include <app_control.h>
#include <notification.h>
#include <feedback.h>
#include <system_settings.h>
#ifdef HAVE_X
#include <utilX.h>
#endif
#include "quickpanel-ui.h"
#include "common.h"
#include "noti.h"
#include "noti_win.h"
#include "noti_util.h"
#ifdef QP_EMERGENCY_MODE_ENABLE
#include "emergency_mode.h"
#endif
#include "animated_icon.h"

#define QP_ACTIVENOTI_DURATION	3
#define QP_ACTIVENOTI_DETAIL_DURATION 6

#define ACTIVENOTI_MSG_LEN 100
#define DEFAULT_ICON RESDIR"/quickpanel_icon_default.png"

#define E_DATA_ICON "E_DATA_ICON"
#define E_DATA_BADGE "E_DATA_BADGE"
#define E_DATA_NOTI "E_DATA_NOTI"
#define E_DATA_BTN_IMAGE "E_DATA_BTN_IMAGE"
#define E_DATA_BG_IMAGE "E_DATA_BG_IMAGE"
#define DELAY_TIMER_VALUE	0.480
#define DEL_TIMER_VALUE	8.0

static struct info {
	Evas_Object *activenoti;
	Evas_Object *layout;
	Evas_Object *btnbox;
	Evas_Object *gesture;
	Ecore_Timer *delay_timer;
	Ecore_Timer *close_timer;
} s_info = {
	.activenoti = NULL,
	.layout = NULL,
	.btnbox = NULL,
	.gesture = NULL,
	.delay_timer = NULL,
	.close_timer = NULL,
};


static int _activenoti_init(void *data);
static int _activenoti_fini(void *data);
static int _activenoti_enter_hib(void *data);
static int _activenoti_leave_hib(void *data);
static void _activenoti_reflesh(void *data);
static void _activenoti_create_activenoti(void *data);
static void _activenoti_win_rotated(void *data, int need_hide);
static void _activenoti_hide(void *data, int delay);
static void _activenoti_destroy_activenoti();


QP_Module activenoti = {
	.name = "activenoti",
	.init = _activenoti_init,
	.fini = _activenoti_fini,
	.hib_enter = _activenoti_enter_hib,
	.hib_leave = _activenoti_leave_hib,
	.lang_changed = NULL,
	.refresh = _activenoti_reflesh
};

static inline int _is_text_exist(const char *text)
{
	if (text != NULL) {
		if (strlen(text) > 0) {
			return 1;
		}
	}

	return 0;
}

static int _is_sound_playable(void) {
	int status = 0, ret = 0;

	ret = vconf_get_int(VCONFKEY_CAMERA_STATE, &status);
	if (ret == 0 && status == VCONFKEY_CAMERA_STATE_RECORDING) {
		ERR("camcorder is working, don't play notification sound %d %d", ret, status);
		return 0;
	}
	return 1;
}

static int _is_security_lockscreen_launched(void)
{
	int ret = 0;
	int is_idle_lock = 0;
	int lock_type = 0;

	ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &is_idle_lock);
	retif(ret != 0, 0,"failed to get VCONFKEY_IDLE_LOCK_STATE %d %d", ret, is_idle_lock);

	if (is_idle_lock  == VCONFKEY_IDLE_LOCK ) {
		DBG("Lock screen is launched");
		return 1; //don't show on lock screen
		/*
		ret = vconf_get_int(VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT, &lock_type);
		retif(ret != 0, 0,"failed to get VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT %d %d", ret, lock_type);
		if (lock_type != SETTING_SCREEN_LOCK_TYPE_NONE && lock_type != SETTING_SCREEN_LOCK_TYPE_SWIPE) {
			return 1;
		}
		*/
	}

	return 0;
}

static Evas_Event_Flags __flick_end_cb(void *data, void *event_info)
{
	DBG("");
	Elm_Gesture_Line_Info *line_info = (Elm_Gesture_Line_Info *) event_info;

	DBG("line_info->momentum.my : %d", line_info->momentum.my);

	/* Flick Up */
	if (line_info->momentum.my < 0)
	{
		_activenoti_hide(NULL,0);
	}

	return EVAS_EVENT_FLAG_ON_HOLD;
}


static Evas_Object *_gesture_create(Evas_Object *layout)
{
	Evas_Object *gesture_layer = NULL;

	INFO("gesture create");

	gesture_layer = elm_gesture_layer_add(layout);
	retif(!gesture_layer, NULL,);
	elm_gesture_layer_attach(gesture_layer, layout);
	evas_object_show(gesture_layer);

	elm_gesture_layer_cb_set(gesture_layer, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_END, __flick_end_cb, NULL);

	return gesture_layer;
}

static int _check_sound_off(notification_h noti)
{
	char *pkgname = NULL;

	notification_get_pkgname(noti, &pkgname);

	//to do

	return 0;
}


static void _gesture_destroy()
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
	int applist = NOTIFICATION_DISPLAY_APP_ALL;

	retif(noti == NULL, ,"noti is null");

	notification_get_display_applist(noti, &applist);
#ifdef HAVE_X
	if (applist & NOTIFICATION_DISPLAY_APP_HEADS_UP)
#endif		
	{
		if (!(applist & NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY)) {
			char *pkgname = NULL;
			int priv_id = 0;

			notification_get_pkgname(noti, &pkgname);
			notification_get_id(noti, NULL, &priv_id);
			notification_delete_by_priv_id(pkgname,
						NOTIFICATION_TYPE_NONE,
						priv_id);
		}
	}
}


static Eina_Bool _activenoti_hide_timer_cb(void *data)
{
	_activenoti_hide(NULL,0);
	s_info.close_timer = NULL;

	if (s_info.delay_timer != NULL) {
		ecore_timer_del(s_info.delay_timer);
		s_info.delay_timer = NULL;
	}

	return ECORE_CALLBACK_CANCEL;
}



static void _activenoti_hide(void *data, int delay)
{
	if (delay == 1) {
		if (s_info.delay_timer == NULL) {
			s_info.delay_timer = ecore_timer_add(DELAY_TIMER_VALUE, _activenoti_hide_timer_cb, NULL);
		}
	} else {
		if (s_info.delay_timer != NULL) {
			ecore_timer_del(s_info.delay_timer);
			s_info.delay_timer = NULL;
		}

		if (s_info.close_timer != NULL) {
			ecore_timer_del(s_info.close_timer);
			s_info.close_timer = NULL;
		}

		if (s_info.activenoti) {
			evas_object_hide(s_info.activenoti);
		}
	}
}



static void _activenoti_detail_show_cb(void *data, Evas *e,
					Evas_Object *obj,
					void *event_info)
{
	DBG("");
}

static Evas_Object *_activenoti_create_badge(Evas_Object *parent,
						notification_h noti)
{
	char *pkgname = NULL;
	char *icon_path = NULL;
	char *icon_default = NULL;
	Evas_Object *icon = NULL;
	int ret = NOTIFICATION_ERROR_NONE;

	retif(noti == NULL || parent == NULL, NULL, "Invalid parameter!");

	ret = notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB, &icon_path);

	if (ret == NOTIFICATION_ERROR_NONE && icon_path != NULL) {
		DBG("icon_path :  %s", icon_path);
		icon = elm_image_add(parent);
		elm_image_resizable_set(icon, EINA_TRUE, EINA_TRUE);
		if ( elm_image_file_set(icon, icon_path, NULL) == EINA_FALSE) {
			ERR("fail to set file[%s]", icon_path);
			evas_object_del(icon);
			icon = NULL;
			free(icon_path);
			return NULL;
		}
	} else {
		/*
		notification_get_pkgname(noti, &pkgname);
		if (pkgname != NULL) {
			INFO("pkgname : %s", pkgname);
			icon_default = quickpanel_common_ui_get_pkginfo_icon(pkgname);
			if (icon_default != NULL) {
				elm_image_file_set(icon, icon_default, NULL);
				free(icon_default);
			} else {
				return NULL;
			}
		} else {
			return NULL;
		}
		*/
		return NULL;
	}

	free(icon_path);
	return icon;
}

static void _image_press_cb(void *data, Evas_Object *obj,
			const char *emission, const char *source)
{
	DBG("");

	app_control_h app_control = data;
	int ret = APP_CONTROL_ERROR_NONE;

	DBG("");
	if (app_control) {
		char *app_id = NULL;
		ret = app_control_get_app_id(app_control, &app_id);

		DBG("app_id : %s",app_id);
		if (ret == APP_CONTROL_ERROR_NONE && app_id != NULL) {
			ret = app_control_send_launch_request(app_control, NULL, NULL);
			DBG("ret [%s]", ret);
			free(app_id);
		}

	} else {
		ERR("app_control is NULL");
	}

	_activenoti_hide(NULL, 1);
}


static Evas_Object *_activenoti_create_icon(Evas_Object *parent, notification_h noti)
{
	char *pkgname = NULL;
	char *icon_path = NULL;
	char *thumb_path = NULL;
	char *icon_default = NULL;
	Evas_Object *icon = NULL;
	retif(noti == NULL || parent == NULL, NULL, "Invalid parameter!");

	notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, &icon_path);
	notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL, &thumb_path);

	icon = elm_image_add(parent);
	elm_image_resizable_set(icon, EINA_TRUE, EINA_TRUE);

	if (icon_path == NULL
		|| (icon_path != NULL && elm_image_file_set(icon, icon_path, NULL) == EINA_FALSE)) {
		DBG("icon_path :  %s", icon_path);

		if (thumb_path == NULL
			|| (thumb_path != NULL && elm_image_file_set(icon, thumb_path, NULL) == EINA_FALSE)) {
			DBG("thumb_path :  %s", thumb_path);

			int ret = notification_get_pkgname(noti, &pkgname);
			if (ret == NOTIFICATION_ERROR_NONE && pkgname != NULL) {
				DBG("pkgname :	%s", icon_default);

				icon_default = quickpanel_common_ui_get_pkginfo_icon(pkgname);
				DBG("icon_default :  %s", icon_default);

				if (icon_default == NULL
					|| ( icon_default != NULL && elm_image_file_set(icon, icon_default, NULL) == EINA_FALSE)) {
					DBG("DEFAULT_ICON :  %s", DEFAULT_ICON);

					if( elm_image_file_set(icon, DEFAULT_ICON, NULL) == EINA_FALSE) {
						evas_object_del(icon);
						icon = NULL;
					}
				}
			} else {
				if ( elm_image_file_set(icon, DEFAULT_ICON, NULL) == EINA_FALSE) {
					evas_object_del(icon);
					icon = NULL;
				}
			}
		}
	}

	if (icon != NULL) {
		elm_object_signal_callback_add(parent, "image_press" , "", _image_press_cb, noti);
	}

	free(icon_path);
	free(thumb_path);
	free(pkgname);
	free(icon_default);

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

static char *_activenoti_get_label_layout_default(notification_h noti, int is_screenreader, char **subtitle, char **title, char **content)
{
	int len = 0;
	char *domain = NULL;
	char *dir = NULL;
	const char *tmp = NULL;
	char buf[ACTIVENOTI_MSG_LEN] = { 0, };

	retif(noti == NULL, NULL, "Invalid parameter!");

	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL) {
		bindtextdomain(domain, dir);
	}

	if ( title != NULL ) {
		*title = _get_text(noti, NOTIFICATION_TEXT_TYPE_TITLE);
	}

	if ( content != NULL ) {
		*content = _get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT);
	}

	if ( subtitle != NULL ) {
		*subtitle = _get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_1);
	}

	if (is_screenreader == 1) {
		if (title == NULL && content == NULL) {
			len = 0;
		} else {
			Eina_Strbuf *strbuf = eina_strbuf_new();
			if (strbuf != NULL) {
				eina_strbuf_append(strbuf, _("IDS_QP_BUTTON_NOTIFICATION"));
				eina_strbuf_append_char(strbuf, '\n');
				_check_and_add_to_buffer(strbuf, title, 1);
				_check_and_add_to_buffer(strbuf, content, 1);

				if (eina_strbuf_length_get(strbuf) > 0) {
					len = snprintf(buf, sizeof(buf) - 1, "%s", eina_strbuf_string_get(strbuf));
				}
				eina_strbuf_free(strbuf);
			}
		}

		if (len > 0) {
			return strdup(buf);
		}
	}

	return NULL;
}


static char *_activenoti_get_text(notification_h noti, int is_screenreader, char **subtitle, char **title, char **content)
{
	char *result = NULL;
	result = _activenoti_get_label_layout_default(noti, is_screenreader, subtitle, title, content);

	return result;
}

static void _noti_hide_cb(void *data, Evas_Object *obj,
			const char *emission, const char *source)
{
	_activenoti_hide(data, 0);
}

static void _noti_press_cb(void *data, Evas_Object *obj,
			const char *emission, const char *source)
{
	DBG("");

	notification_h noti = (notification_h) data;
	int ret = APP_CONTROL_ERROR_NONE;
	char *caller_pkgname = NULL;
	bundle *responding_service_handle = NULL;
	bundle *single_service_handle = NULL;
	bundle *multi_service_handle = NULL;
	int flags = 0, group_id = 0, priv_id = 0, count = 0, flag_launch = 0;
	notification_type_e type = NOTIFICATION_TYPE_NONE;

	retif(noti == NULL, , "Invalid parameter!");

	notification_get_pkgname(noti, &caller_pkgname);
	notification_get_id(noti, &group_id, &priv_id);
	notification_get_property(noti, &flags);
	notification_get_type(noti, &type);

	if (flags & NOTIFICATION_PROP_DISABLE_APP_LAUNCH) {
		flag_launch = 0;
	} else {
		flag_launch = 1;
	}

	notification_get_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_RESPONDING,	NULL, &responding_service_handle);
	notification_get_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH, NULL, &single_service_handle);
	notification_get_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_MULTI_LAUNCH, NULL, &multi_service_handle);

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
			DBG("");
			ret = quickpanel_uic_launch_app(NULL, single_service_handle);
		} else if (single_service_handle == NULL && multi_service_handle != NULL) {
			DBG("");
			ret = quickpanel_uic_launch_app(NULL, multi_service_handle);
		} else if (single_service_handle != NULL && multi_service_handle != NULL) {
			DBG("");
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


static void _button_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("");
	app_control_h app_control = data;
	Evas_Object *btn_img;

	msgif(app_control == NULL, , "Invalid parameter!");

	btn_img = evas_object_data_get(obj, E_DATA_BTN_IMAGE);

	if (btn_img) {
		evas_object_del(btn_img);
		btn_img = NULL;
	}

	if (app_control) {
		app_control_destroy(app_control);
	}
}


static void _button_press_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	app_control_h app_control = data;
	int ret = APP_CONTROL_ERROR_NONE;

	if (app_control) {
		char *app_id = NULL;
		ret = app_control_get_app_id(app_control, &app_id);

		DBG("app_id : %s",app_id);
		if (ret == APP_CONTROL_ERROR_NONE && app_id != NULL) {
			ret = app_control_send_launch_request(app_control, NULL, NULL);
			DBG("ret [%s]", ret);
			free(app_id);
		}

	} else {
		ERR("app_control is NULL");
	}

	_activenoti_hide(NULL, 1);
}


static Evas_Object *_get_btn_img(Evas_Object *parent, notification_h noti, int btn_num)
{
	char *btn_path = NULL;
	Evas_Object *btn_img = NULL;
	retif(noti == NULL || parent == NULL, NULL, "Invalid parameter!");

#ifdef HAVE_X
	notification_get_image(noti, btn_num + NOTIFICATION_IMAGE_TYPE_BUTTON_1, &btn_path);
#endif

	if (btn_path == NULL ){
		return NULL;
	}

	btn_img = elm_image_add(parent);
	evas_object_size_hint_weight_set(btn_img, EVAS_HINT_EXPAND,	EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(btn_img, EVAS_HINT_FILL, EVAS_HINT_FILL);

	if (btn_img) {
		elm_image_resizable_set(btn_img, EINA_TRUE, EINA_TRUE);
		if(	elm_image_file_set(btn_img, btn_path, NULL) == EINA_FALSE) {
			evas_object_del(btn_img);
			btn_img = NULL;
			return NULL;
		}
	} else {
		return NULL;
	}

	free(btn_path);

	return btn_img;
}


static Evas_Object *_get_bg_img(Evas_Object *parent, notification_h noti)
{
	char *bg_path = NULL;
	Evas_Object *bg_img = NULL;
	retif(noti == NULL || parent == NULL, NULL, "Invalid parameter!");

	notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND, &bg_path);

	if (bg_path == NULL ){
		return NULL;
	}

	bg_img = elm_image_add(parent);
	evas_object_size_hint_weight_set(bg_img, EVAS_HINT_EXPAND,	EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(bg_img, EVAS_HINT_FILL, EVAS_HINT_FILL);

	if (bg_img) {
		elm_image_resizable_set(bg_img, EINA_TRUE, EINA_TRUE);
		if(	elm_image_file_set(bg_img, bg_path, NULL) == EINA_FALSE) {
			evas_object_del(bg_img);
			bg_img = NULL;
			return NULL;
		}
	} else {
		return NULL;
	}

	free(bg_path);

	return bg_img;
}


static int _activenoti_create_button(Evas_Object *obj, notification_h noti)
{
	int btn_cnt= 0;
	int ret = APP_CONTROL_ERROR_NONE;
	app_control_h app_control = NULL;

	retif(obj == NULL, 0, "obj is NULL!");
	retif(noti == NULL, 0, "noti is NULL!");

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

	for( btn_cnt = 0 ; btn_cnt <= 2 ; btn_cnt++) {
		retif(ret != APP_CONTROL_ERROR_NONE, 0, "noapp_control_createti is failed!");
#ifdef HAVE_X
		ret = notification_get_event_handler(noti, btn_cnt + NOTIFICATION_EVENT_TYPE_CLICK_ON_BUTTON_1, &app_control);
#endif
		if(ret != NOTIFICATION_ERROR_NONE || app_control == NULL) {
			INFO("no more button, button count is %d", btn_cnt);
			INFO("ret is %d", ret);
			if (btn_cnt == 0) { // noti doesn't have button
				if (app_control) {
					app_control_destroy(app_control);
				}
				evas_object_del(s_info.btnbox);
				s_info.btnbox = NULL;
				return 0;
			}
			break;
		} else {
			Evas_Object *bt_layout;
			char *btn_text;
			Evas_Object *image;

			bt_layout = elm_layout_add(s_info.btnbox);
			if(bt_layout == NULL) {
				ERR("bt_layout is null");
				if (app_control) {
					app_control_destroy(app_control);
				}
				evas_object_del(s_info.btnbox);
				s_info.btnbox = NULL;
				return 0;
			}

			elm_layout_file_set(bt_layout, ACTIVENOTI_EDJ, "button_layout");
			evas_object_size_hint_weight_set (bt_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(bt_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

			image = _get_btn_img(bt_layout , noti, btn_cnt);
			if (image != NULL) {
				elm_object_part_content_set(bt_layout, "content.button.image", image);
				evas_object_data_set(bt_layout, E_DATA_BTN_IMAGE, image);
			}
			
#ifdef HAVE_X
			btn_text = _get_text(noti, btn_cnt + NOTIFICATION_TEXT_TYPE_BUTTON_1);
#endif
			if (btn_text != NULL) {
				DBG("btn_text :%s", btn_text);
				elm_object_part_text_set(bt_layout, "content.button.text", btn_text);
				free(btn_text);
			}

			elm_object_signal_callback_add(bt_layout, "button_clicked" , "", _button_press_cb, app_control);
			evas_object_event_callback_add(bt_layout, EVAS_CALLBACK_DEL, _button_del_cb, app_control);
			evas_object_show(bt_layout);
			elm_box_pack_end(s_info.btnbox,bt_layout);
		}
	}
	elm_object_part_content_set(obj, "button.swallow", s_info.btnbox);
	return btn_cnt;
}

static void _activenoti_update_activenoti(void *data)
{
	DBG("");
	Eina_Bool ret = EINA_FALSE;
	notification_h noti = (notification_h) data;
	Evas_Object *icon = NULL;
	Evas_Object *badge = NULL;

	Evas_Object *textblock = NULL;
	char *title = NULL;
	char *subtitle = NULL;
	char *content = NULL;
	int btn_cnt = 0;

	retif(noti == NULL, , "Invalid parameter!");

	if (s_info.activenoti == NULL) {
		ERR("Active notification doesn't exist");
		return;
	}

	btn_cnt = _activenoti_create_button(s_info.layout, noti);

	if (btn_cnt == 0) { //no button
		elm_object_signal_emit(s_info.layout, "btn_hide", "button.space");
	} else {
		elm_object_signal_emit(s_info.layout, "btn_show", "button.space");
	}

	icon = elm_object_part_content_get(s_info.layout, "icon_big");
	if(icon != NULL) {
		evas_object_del(icon);
		icon = NULL;
	}

	icon = _activenoti_create_icon(s_info.layout, noti);
	if (icon != NULL) {
		elm_object_part_content_set(s_info.layout, "icon_big", icon);

		badge = elm_object_part_content_get(s_info.layout, "icon_badge");
		if(badge != NULL) {
			evas_object_del(badge);
			badge = NULL;
		}

		badge = _activenoti_create_badge(s_info.layout, noti);
		if (badge != NULL) {
			elm_object_part_content_set(s_info.layout, "icon_badge", badge);
		} else {
			INFO("badge is NULL");
		}
	} else {
		INFO("icon is NULL");
	}
	_activenoti_get_text(noti, 0, &subtitle, &title, &content);

	if (title != NULL) {
		Eina_Strbuf *bufferT = eina_strbuf_new();
		eina_strbuf_append(bufferT, title);
		eina_strbuf_append(bufferT, "<b/>");
		elm_object_part_text_set(s_info.layout, "title_text", eina_strbuf_string_get(bufferT));
		free(title);
		eina_strbuf_free(bufferT);
	}

	if (subtitle != NULL) {
		Eina_Strbuf *bufferST = eina_strbuf_new();
		eina_strbuf_append(bufferST, subtitle);
		elm_object_part_text_set(s_info.layout, "subtitle_text", eina_strbuf_string_get(bufferST));
		free(subtitle);
		eina_strbuf_free(bufferST);
		elm_object_signal_emit(s_info.layout, "sub_show", "subtitle_text");
	} else {
		elm_object_signal_emit(s_info.layout, "sub_hide", "subtitle_text");
	}

	if (content != NULL) {
		Eina_Strbuf *bufferC = eina_strbuf_new();
		eina_strbuf_append(bufferC, content);
		elm_object_part_text_set(s_info.layout, "content_text", eina_strbuf_string_get(bufferC));
		free(content);
		eina_strbuf_free(bufferC);
	}


	if (s_info.close_timer != NULL) {
		ecore_timer_del(s_info.close_timer);
		s_info.close_timer = NULL;
	}

	s_info.close_timer = ecore_timer_add(DEL_TIMER_VALUE, _activenoti_hide_timer_cb, NULL);
	evas_object_show(s_info.activenoti);

	SERR("activenoti noti is updated");
}

static void _activenoti_create_activenoti(void *data)
{
	DBG("");
	Eina_Bool ret = EINA_FALSE;
	notification_h noti = (notification_h) data;
	Evas_Object *icon = NULL;
	Evas_Object *badge = NULL;
	Evas_Object *bg_img = NULL;


	Evas_Object *textblock = NULL;
	char *title = NULL;
	char *subtitle = NULL;
	char *content = NULL;
	int *is_activenoti_executed = NULL;
	int btn_cnt = 0;

	retif(noti == NULL, , "Invalid parameter!");

	if (s_info.activenoti != NULL) {
		ERR("Instant notification exists");
		return;
	}

	s_info.activenoti = quickpanel_noti_win_add(NULL);
	retif(s_info.activenoti == NULL, , "Failed to add elm activenoti.");
	evas_object_data_set(s_info.activenoti, E_DATA_NOTI, noti);

	s_info.layout = elm_layout_add(s_info.activenoti);
	if (!s_info.layout) {
		ERR("Failed to get detailview.");
		_activenoti_hide(s_info.activenoti, 0);
		return;
	}

	ret = elm_layout_file_set(s_info.layout, ACTIVENOTI_EDJ, "headsup/base");
	retif(ret == EINA_FALSE, , "failed to load layout");
	evas_object_show(s_info.layout);

	elm_object_signal_callback_add(s_info.layout, "noti_press" , "", _noti_press_cb, noti);
	elm_object_signal_callback_add(s_info.layout, "del" , "", _noti_hide_cb, noti);

	bg_img = _get_bg_img(s_info.layout , noti);
	if (bg_img != NULL) {
		elm_object_part_content_set(s_info.layout, "bg_img", bg_img);
		evas_object_data_set(s_info.activenoti, E_DATA_BG_IMAGE, bg_img);
	}

	btn_cnt = _activenoti_create_button(s_info.layout, noti);

	if (btn_cnt == 0) { //no button
		elm_object_signal_emit(s_info.layout, "btn_hide", "button.space");
	}

	quickpanel_noti_win_content_set(s_info.activenoti, s_info.layout, btn_cnt);

	icon = _activenoti_create_icon(s_info.layout, noti);
	if (icon != NULL) {
		elm_object_part_content_set(s_info.layout, "icon_big", icon);
		evas_object_data_set(s_info.activenoti, E_DATA_ICON, icon);

		badge = _activenoti_create_badge(s_info.layout, noti);
		if (badge != NULL) {
			elm_object_part_content_set(s_info.layout, "icon_badge", badge);
			evas_object_data_set(s_info.activenoti, E_DATA_BADGE, icon);
		} else {
			INFO("badge is NULL");
		}
	} else {
		INFO("icon is NULL");
	}
	_activenoti_get_text(noti, 0, &subtitle, &title, &content);

	if (title != NULL) {
		Eina_Strbuf *bufferT = eina_strbuf_new();
		eina_strbuf_append(bufferT, title);
		elm_object_part_text_set(s_info.layout, "title_text", eina_strbuf_string_get(bufferT));
		free(title);
		eina_strbuf_free(bufferT);
	}


	if (subtitle != NULL) {
		Eina_Strbuf *bufferST = eina_strbuf_new();
		eina_strbuf_append(bufferST, subtitle);
		elm_object_part_text_set(s_info.layout, "subtitle_text", eina_strbuf_string_get(bufferST));
		free(subtitle);
		eina_strbuf_free(bufferST);
		elm_object_signal_emit(s_info.layout, "subtext_show", "subtitle_text");
	}

	if (content != NULL) {
		Eina_Strbuf *bufferC = eina_strbuf_new();
		eina_strbuf_append(bufferC, content);
		elm_object_part_text_set(s_info.layout, "content_text", eina_strbuf_string_get(bufferC));
		free(content);
		eina_strbuf_free(bufferC);
	}

	SERR("activenoti noti is created");

	s_info.gesture = _gesture_create(s_info.layout);
	_activenoti_win_rotated(data, 0);

	if (s_info.close_timer != NULL) {
		ecore_timer_del(s_info.close_timer);
		s_info.close_timer = NULL;
	}

	s_info.close_timer = ecore_timer_add(DEL_TIMER_VALUE, _activenoti_hide_timer_cb, NULL);

	evas_object_show(s_info.activenoti);
	SERR("show activenoti noti");
}

static void _activenoti_destroy_activenoti()
{
	Evas_Object *bg;
	Evas_Object *icon;
	Evas_Object *badge;
	notification_h noti;

	retif(!s_info.activenoti,,"s_info->activenoti is null");

	_gesture_destroy();

	DBG("_activenoti_destroy_activenoti");

	if (s_info.delay_timer != NULL) {
		ecore_timer_del(s_info.delay_timer);
		s_info.delay_timer = NULL;
	}

	if (s_info.close_timer != NULL) {
		ecore_timer_del(s_info.close_timer);
		s_info.close_timer = NULL;
	}

	bg = evas_object_data_del(s_info.activenoti, E_DATA_BG_IMAGE);
	if (bg) {
		evas_object_del(bg);
	}

	icon = evas_object_data_del(s_info.activenoti, E_DATA_ICON);
	if (icon) {
		evas_object_del(icon);
	}

	badge = evas_object_data_del(s_info.activenoti, E_DATA_BADGE);
	if (badge) {
		evas_object_del(badge);
	}

	if (s_info.btnbox) {
		evas_object_del(s_info.btnbox);
		s_info.btnbox = NULL;
	}

	if (s_info.layout) {
		evas_object_del(s_info.layout);
		s_info.layout = NULL;
	}

	noti = evas_object_data_del(s_info.activenoti, E_DATA_NOTI);
	if (noti) {
		_activenoti_only_noti_del(noti);
		notification_free(noti);
	}
	if (s_info.activenoti) {
		evas_object_hide(s_info.activenoti);
		s_info.activenoti = NULL;
	}
}

static void _activenoti_win_rotated(void *data, int need_hide)
{
	int angle = 0;
	retif(data == NULL, ,"data is NULL");
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
	int ret = 0, priv_id = 0;
	const char *nsound_path = NULL;
	notification_sound_type_e nsound_type = NOTIFICATION_SOUND_TYPE_NONE;
	char *default_msg_tone = NULL;
	retif(noti == NULL, ,"op_list is NULL");

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
#ifdef HAVE_X
						ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_SOUND_NOTIFICATION, &default_msg_tone);
						msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "ailed to get key(%s) : %d", SYSTEM_SETTINGS_KEY_SOUND_NOTIFICATION, ret);
#endif


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
#ifdef HAVE_X
			ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_SOUND_NOTIFICATION, &default_msg_tone);
			msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "ailed to get key(%s) : %d", SYSTEM_SETTINGS_KEY_SOUND_NOTIFICATION, ret);
#endif
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
	retif(noti == NULL, ,"op_list is NULL");

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
	notification_h noti = NULL;
	notification_h noti_from_master = NULL;
	int flags = 0;
	int applist = NOTIFICATION_DISPLAY_APP_ALL;
	int ret = 0;
	int op_type = 0;
	int priv_id = 0;

	DBG("_quickpanel_activenoti_noti_changed_cb");

	retif(op_list == NULL, ,"op_list is NULL");

	if (num_op == 1) {
		notification_op_get_data(op_list, NOTIFICATION_OP_DATA_TYPE, &op_type);
		notification_op_get_data(op_list, NOTIFICATION_OP_DATA_PRIV_ID, &priv_id);
		notification_op_get_data(op_list, NOTIFICATION_OP_DATA_NOTI, &noti_from_master);
		DBG("op_type:%d", op_type);
		DBG("op_priv_id:%d", priv_id);
		DBG("noti:%p", noti_from_master);

		if (op_type != NOTIFICATION_OP_INSERT &&
				op_type != NOTIFICATION_OP_UPDATE) {
			return;
		}
		if (noti_from_master == NULL) {
			ERR("failed to get a notification from master");
			return;
		}
		if (notification_clone(noti_from_master, &noti) != NOTIFICATION_ERROR_NONE) {
			ERR("failed to create a cloned notification");
			return;
		}
#ifdef QP_EMERGENCY_MODE_ENABLE
		if (quickpanel_emergency_mode_is_on()) {
			if (quickpanel_emergency_mode_notification_filter(noti, 1)) {
				DBG("notification filtered");
				notification_free(noti);
				return;
			}
		}
#endif
	}

	retif(noti == NULL, ,"noti is NULL");

	if (op_type == NOTIFICATION_OP_INSERT || op_type == NOTIFICATION_OP_UPDATE) {
		if (_is_sound_playable() == 1) {
			if (_check_sound_off(noti) == 0) {
				DBG("try to play notification sound");
				_media_feedback_sound(noti);
			}
			if (quickpanel_media_is_vib_enabled() == 1
					|| quickpanel_media_is_sound_enabled() == 1) {
				_media_feedback_vibration(noti);
			}
		}
	}

	notification_get_display_applist(noti, &applist);

	/* Check activenoti flag */
	notification_get_property(noti, &flags);

	DBG("applist : %x" ,applist);

#ifdef HAVE_X
	if (applist & NOTIFICATION_DISPLAY_APP_HEADS_UP)
#endif
	{
		if (_is_security_lockscreen_launched()) {
			notification_free(noti);
			INFO("lock screen is launched");
			return;
		}

		if (quickpanel_uic_is_opened()) {
			ERR("quickpanel is opened, activenoti will be not displayed");
			notification_free(noti);
			INFO("quickpanel has opened");
			return;
		}

		/* wait if s_info.activenoti is not NULL */
		if (s_info.activenoti == NULL) {
			_activenoti_create_activenoti(noti);
		} else {
			INFO("s_info.activenoti is not NULL");
			_activenoti_update_activenoti(noti);
		}

		if (s_info.activenoti == NULL) {
			ERR("Fail to create activenoti");
			_activenoti_only_noti_del(noti);
			notification_free(noti);
			return;
		}
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
	struct appdata *ad = (struct appdata *)data;

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
