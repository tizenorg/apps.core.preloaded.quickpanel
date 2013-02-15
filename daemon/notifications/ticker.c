/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Elementary.h>
#include <Ecore_X.h>
#include <appcore-common.h>
#include <vconf.h>
#include <appsvc.h>
#include <app_service.h>
#include <notification.h>
#include <time.h>
#include <feedback.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "noti_win.h"

#define QP_TICKER_DURATION	5
#define QP_TICKER_DETAIL_DURATION 6

#define TICKER_MSG_LEN				1024
#define DEFAULT_ICON ICONDIR		"/quickpanel_icon_default.png"

static Evas_Object *g_window;
static Evas_Object *g_ticker;
static Ecore_Timer *g_timer;
static int g_noti_height;

static int quickpanel_ticker_init(void *data);
static int quickpanel_ticker_fini(void *data);
static int quickpanel_ticker_enter_hib(void *data);
static int quickpanel_ticker_leave_hib(void *data);
static void quickpanel_ticker_reflesh(void *data);

QP_Module ticker = {
	.name = "ticker",
	.init = quickpanel_ticker_init,
	.fini = quickpanel_ticker_fini,
	.hib_enter = quickpanel_ticker_enter_hib,
	.hib_leave = quickpanel_ticker_leave_hib,
	.lang_changed = NULL,
	.refresh = quickpanel_ticker_reflesh
};

static int latest_inserted_time;

/*****************************************************************************
 *
 * (Static) Util functions
 *
 *****************************************************************************/

static int _quickpanel_ticker_check_ticker_off(notification_h noti)
{
	char *pkgname = NULL;
	int ret = 0;
	int boolval = 0;

	notification_get_application(noti, &pkgname);

	if (pkgname == NULL)
		notification_get_pkgname(noti, &pkgname);

	if (pkgname == NULL)
		return 1;	/* Ticker is not displaying. */

	if (!strcmp(pkgname, VENDOR".message")) {
		ret = vconf_get_bool(
			VCONFKEY_SETAPPL_STATE_TICKER_NOTI_MESSAGES_BOOL,
			&boolval);
		if (ret == 0 && boolval == 0)
			return 1;
	} else if (!strcmp(pkgname, VENDOR".email")) {
		ret = vconf_get_bool(
			VCONFKEY_SETAPPL_STATE_TICKER_NOTI_EMAIL_BOOL,
			&boolval);
		if (ret == 0 && boolval == 0)
			return 1;
	} else if (!strcmp(pkgname, VENDOR".ims-syspopup")) {
		ret = vconf_get_bool(
				VCONFKEY_SETAPPL_STATE_TICKER_NOTI_IM_BOOL,
			&boolval);
		if (ret == 0 && boolval == 0)
			return 1;
	}

	/* Displaying ticker! */
	return 0;
}

static int _quickpanel_ticker_check_displaying_contents_off(notification_h noti)
{
	char *pkgname = NULL;
	int ret = 0;
	int boolval = 0;

	notification_get_application(noti, &pkgname);

	if (pkgname == NULL)
		notification_get_pkgname(noti, &pkgname);

	if (pkgname == NULL)
		return 0;	/* Ticker is not displaying. */

	if (!strcmp(pkgname, VENDOR".message")) {
		ret = vconf_get_bool(
			VCONFKEY_TICKER_NOTI_DISPLAY_CONTENT_MESSASGES,
			&boolval);
		if (ret == 0 && boolval == 0)
			return 1;
	} else if (!strcmp(pkgname, VENDOR".email")) {
		ret = vconf_get_bool(
			VCONFKEY_TICKER_NOTI_DISPLAY_CONTENT_EMAIL,
			&boolval);
		if (ret == 0 && boolval == 0)
			return 1;
	} else if (!strcmp(pkgname, VENDOR".ims-syspopup")) {
		ret = vconf_get_bool(
			VCONFKEY_TICKER_NOTI_DISPLAY_CONTENT_IM,
			&boolval);
		if (ret == 0 && boolval == 0)
			return 1;
	}

	/* Displaying ticker! */
	return 0;
}

static inline void __ticker_only_noti_del(notification_h noti)
{
	int applist = NOTIFICATION_DISPLAY_APP_ALL;

	retif(noti == NULL, ,"noti is null");

	notification_get_display_applist(noti, &applist);
	if (applist & NOTIFICATION_DISPLAY_APP_TICKER) {
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

static void _quickpanel_ticker_hide(void *data)
{
	if (g_ticker) {
		evas_object_hide(g_ticker);
		evas_object_del(g_ticker);
		g_ticker = NULL;
	}
}

static Eina_Bool _quickpanel_ticker_timeout_cb(void *data)
{
	DBG("");

	g_timer = NULL;

	_quickpanel_ticker_hide(data);

	return ECORE_CALLBACK_CANCEL;
}

static void _quickpanel_ticker_detail_hide_cb(void *data, Evas *e,
					Evas_Object *obj,
					void *event_info)
{
	DBG("");
	notification_h noti = (notification_h) data;

	if (g_timer) {
		ecore_timer_del(g_timer);
		g_timer = NULL;
	}

	retif(noti == NULL, , "Invalid parameter!");

	__ticker_only_noti_del(noti);
	notification_free(noti);
}

static void _quickpanel_ticker_detail_show_cb(void *data, Evas *e,
					Evas_Object *obj,
					void *event_info)
{
	DBG("");
}

static void _quickpanel_ticker_clicked_cb(void *data, Evas_Object *obj,
					void *event_info)
{
	notification_h noti = (notification_h) data;
	char *caller_pkgname = NULL;
	char *pkgname = NULL;
	bundle *args = NULL;
	bundle *single_service_handle = NULL;
	int priv_id = 0;
	int flags = 0;
	int ret = 0;
	int val = 0;
	int flag_launch = 0;
	int flag_delete = 0;
	int type = NOTIFICATION_TYPE_NONE;

	DBG("");

	retif(noti == NULL, , "Invalid parameter!");

	/* Check idle lock state */
	ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val);

	/* If Lock state, there is not any action when clicked. */
	if (ret != 0 || val == VCONFKEY_IDLE_LOCK)
		return;

	notification_get_pkgname(noti, &caller_pkgname);
	notification_get_application(noti, &pkgname);
	if (pkgname == NULL)
		pkgname = caller_pkgname;

	notification_get_id(noti, NULL, &priv_id);
	notification_get_property(noti, &flags);
	notification_get_type(noti, &type);

	if (flags & NOTIFICATION_PROP_DISABLE_APP_LAUNCH)
		flag_launch = 0;
	else
		flag_launch = 1;

	if (flags & NOTIFICATION_PROP_DISABLE_AUTO_DELETE)
		flag_delete = 0;
	else
		flag_delete = 1;

	notification_get_execute_option(noti,
				NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH,
				NULL, &single_service_handle);

	if (flag_launch == 1) {
		if (single_service_handle != NULL)
			appsvc_run_service(single_service_handle, 0, NULL,
					   NULL);
		else {
			notification_get_args(noti, &args, NULL);
			quickpanel_launch_app(pkgname, args);
		}

		/* Hide quickpanel */
		Ecore_X_Window zone;
		zone = ecore_x_e_illume_zone_get(elm_win_xwindow_get(g_window));
		ecore_x_e_illume_quickpanel_state_send(zone,
				ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
	}

	if (flag_delete == 1 && type == NOTIFICATION_TYPE_NOTI) {
		notification_delete_by_priv_id(caller_pkgname,
						NOTIFICATION_TYPE_NOTI,
						priv_id);
	}
}

static void _quickpanel_ticker_button_clicked_cb(void *data, Evas_Object *obj,
					void *event_info)
{
	DBG("");

	if (g_timer) {
		ecore_timer_del(g_timer);
		g_timer = NULL;
	}

	_quickpanel_ticker_hide(data);
}

static Evas_Object *_quickpanel_ticker_create_button(Evas_Object *parent,
						notification_h noti)
{
	Evas_Object *button = NULL;
	int ret = 0;
	int val = 0;

	retif(noti == NULL || parent == NULL, NULL, "Invalid parameter!");

	/* Check idle lock state */
	ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val);
	/* If Lock state, button is diabled */
	if (ret != 0 || val == VCONFKEY_IDLE_LOCK)
		return NULL;

	button = elm_button_add(parent);
	elm_object_style_set(button, "tickernoti");
	elm_object_text_set(button, _S("IDS_COM_BODY_CLOSE"));
	evas_object_smart_callback_add(button, "clicked",
				_quickpanel_ticker_button_clicked_cb, noti);

	return button;
}

static Evas_Object *_quickpanel_ticker_create_icon(Evas_Object *parent,
						notification_h noti)
{
	char *icon_path = NULL;
	Evas_Object *icon = NULL;

	retif(noti == NULL || parent == NULL, NULL, "Invalid parameter!");

	notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, &icon_path);
	icon = elm_image_add(parent);

	if (icon_path == NULL
	    || (elm_image_file_set(icon, icon_path, NULL) == EINA_FALSE)) {
		elm_image_file_set(icon, DEFAULT_ICON, NULL);
		elm_image_resizable_set(icon, EINA_TRUE, EINA_TRUE);
	}

	return icon;
}

static char *_quickpanel_ticker_get_label(notification_h noti)
{
	char buf[TICKER_MSG_LEN] = { 0, };
	int len = 0;
	char *domain = NULL;
	char *dir = NULL;
	char *result_title = NULL;
	char *result_content = NULL;
	char *title_utf8 = NULL;
	char *content_utf8 = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	retif(noti == NULL, NULL, "Invalid parameter!");

	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL)
		bindtextdomain(domain, dir);

	noti_err = notification_get_text(noti, NOTIFICATION_TEXT_TYPE_TITLE,
			&result_title);
	if (noti_err == NOTIFICATION_ERROR_NONE && result_title)
		title_utf8 = elm_entry_utf8_to_markup(result_title);

	if (_quickpanel_ticker_check_displaying_contents_off(noti) == 1) {
		noti_err = notification_get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT_FOR_DISPLAY_OPTION_IS_OFF,
				&result_content);
	}
	else {
		noti_err = notification_get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
				&result_content);
	}

	if (noti_err == NOTIFICATION_ERROR_NONE && result_content)
		content_utf8 = elm_entry_utf8_to_markup(result_content);

	if (title_utf8 && content_utf8) {
		len = snprintf(buf, sizeof(buf),
			"<font_size=26><color=#BABABA>%s</color></font>"
			"<br><font_size=29><color=#F4F4F4>%s</color></font>",
			title_utf8, content_utf8);
	} else if (title_utf8) {
		len = snprintf(buf, sizeof(buf),
			"<font_size=29><color=#BABABA>%s</color></font>",
			title_utf8);
	}

	if (title_utf8)
		free(title_utf8);

	if (content_utf8)
		free(content_utf8);

	if (len > 0)
		return strdup(buf);

	return NULL;
}

static void _noti_hide_cb(void *data, Evas_Object *obj,
			const char *emission, const char *source)
{
	DBG("");

	if (g_timer) {
		ecore_timer_del(g_timer);
		g_timer = NULL;
	}

	_quickpanel_ticker_hide(data);
}

static Evas_Object *_quickpanel_ticker_create_tickernoti(void *data)
{
	notification_h noti = (notification_h) data;
	Evas_Object *tickernoti = NULL;
	Evas_Object *icon = NULL;
	Evas_Object *detail = NULL;
	Evas_Object *button = NULL;
	char *buf = NULL;
	const char *data_win_height = NULL;
	int noti_height = 0;

	retif(noti == NULL, NULL, "Invalid parameter!");

	tickernoti = noti_win_add(NULL);
	retif(tickernoti == NULL, NULL, "Failed to add elm tickernoti.");

	detail = elm_layout_add(tickernoti);
	if (!detail) {
		ERR("Failed to get detailview.");
		evas_object_del(tickernoti);
		return NULL;
	}
	elm_layout_theme_set(detail, "tickernoti", "base", "default");
	elm_object_signal_callback_add(detail, "request,hide", "",
				_noti_hide_cb, noti);

	data_win_height = (char *)elm_layout_data_get(detail, "height");
	if (data_win_height != NULL && elm_config_scale_get() > 0.0)
		noti_height = (int)(elm_config_scale_get()
					* atoi(data_win_height));
	evas_object_size_hint_min_set(detail, 1, noti_height);
	g_noti_height = noti_height;

	noti_win_content_set(tickernoti, detail);

	icon = _quickpanel_ticker_create_icon(detail, noti);
	if (icon != NULL)
		elm_object_part_content_set(detail, "icon", icon);

	button = _quickpanel_ticker_create_button(detail, noti);
	if (button != NULL)
		elm_object_part_content_set(detail, "button", button);

	buf = _quickpanel_ticker_get_label(noti);
	if (buf != NULL) {
		elm_object_part_text_set(detail, "elm.text", buf);
		free(buf);
	}

	/* Use style "default" for detailview mode and
	 * "info" for text only mode
	 */
	elm_object_style_set(tickernoti, "default");

	return tickernoti;
}

static int _quickpanel_ticker_get_angle(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	Ecore_X_Window xwin, root;
	int ret = 0, angle = 0, count = 0;
	unsigned char *prop_data = NULL;

	xwin = elm_win_xwindow_get(ad->win);
	root = ecore_x_window_root_get(xwin);

	ret = ecore_x_window_prop_property_get(root,
				ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE,
				ECORE_X_ATOM_CARDINAL, 32,
				&prop_data, &count);

	if (ret && prop_data) {
		memcpy(&angle, prop_data, sizeof(int));

		if (prop_data)
			free(prop_data);

		return angle;
	} else {
		ERR("Fail to get angle");
		if (prop_data)
			free(prop_data);

		return -1;
	}
}

static void _quickpanel_ticker_update_geometry_on_rotation(void *data, int *x, int *y, int *w, int *h) {
	int angle = 0;

	if (!data)
		return;
	angle = _quickpanel_ticker_get_angle(data);
	Evas_Coord root_w, root_h;

	/*
	 * manually calculate win_tickernoti_indi window position & size
	 *  - win_indi is not full size window
	 */
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &root_w, &root_h);

	// rotate win
	switch(angle)
	{
		case 90:
			*w = g_noti_height;
			*h = root_h;
			break;
		case 270:
			*w = g_noti_height;
			*h = root_h;
			*x = root_w - g_noti_height;
			break;
		case 180:
			*w = root_w;
			*h = g_noti_height;
			*y = root_h - g_noti_height;
			break;
		case 0:
		default:
			*w = root_w;
			*h = g_noti_height;
			break;
	}
	elm_win_rotation_set(g_ticker, angle);
}

static void _quickpanel_ticker_win_rotated(void *data) {
	retif(data == NULL, ,"data is NULL");
	struct appdata *ad = data;
	int x = 0, y = 0, w = 0, h = 0;

	_quickpanel_ticker_update_geometry_on_rotation(ad, &x, &y, &w, &h);

	if (g_ticker != NULL) {
		evas_object_move(g_ticker, x, y);
		evas_object_resize(g_ticker, w, h);
	}
}

static void _quickpanel_ticker_noti_detailed_changed_cb(void *data, notification_type_e type, notification_op *op_list, int num_op)
{
	notification_h noti = NULL;
	int flags = 0;
	int applist = NOTIFICATION_DISPLAY_APP_ALL;
	int ret = 0;
	int op_type = 0;
	int priv_id = 0;

	INFO("_quickpanel_ticker_noti_changed_cb");

	retif(op_list == NULL, ,"op_list is NULL");

	if (num_op == 1) {
		notification_op_get_data(op_list, NOTIFICATION_OP_DATA_TYPE, &op_type);
		notification_op_get_data(op_list, NOTIFICATION_OP_DATA_PRIV_ID, &priv_id);
		DBG("op_type:%d", op_type);
		DBG("op_priv_id:%d", priv_id);

		if (op_type == NOTIFICATION_OP_INSERT) {
			noti = notification_load(NULL, priv_id);
		} else if (op_type == NOTIFICATION_OP_UPDATE) {
			noti = notification_load(NULL, priv_id);
		} else {
			return ;
		}
	} else {
		return ;
	}

	retif(noti == NULL, ,"noti is NULL");

	notification_get_display_applist(noti, &applist);
	if (!(applist & NOTIFICATION_DISPLAY_APP_TICKER)) {
		DBG("No Ticker Msg");
		notification_free(noti);
		return ;
	}

	/* Check setting's event notification */
	ret = _quickpanel_ticker_check_ticker_off(noti);
	if (ret == 1) {
		INFO("Disable tickernoti ret : %d", ret);
		/* delete temporary here only ticker noti display item */
		__ticker_only_noti_del(noti);
		notification_free(noti);

		return;
	}

	/* Play sound */
	notification_sound_type_e nsound_type = NOTIFICATION_SOUND_TYPE_NONE;
	const char *nsound_path = NULL;

	notification_get_sound(noti, &nsound_type, &nsound_path);
	DBG("Sound : %d, %s", nsound_type, nsound_path);
	if (nsound_type > NOTIFICATION_SOUND_TYPE_NONE
	    || nsound_type < NOTIFICATION_SOUND_TYPE_MAX) {

		switch (nsound_type) {
		case NOTIFICATION_SOUND_TYPE_DEFAULT:
			feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_UNLOCK);
			break;
		case NOTIFICATION_SOUND_TYPE_USER_DATA:
			quickpanel_player_play(SOUND_TYPE_NOTIFICATION, nsound_path);
			break;
		default:
			break;
		}
	}
	/* Play Vibration */
	notification_vibration_type_e nvibration_type =
	    NOTIFICATION_VIBRATION_TYPE_NONE;
	const char *nvibration_path = NULL;

	notification_get_vibration(noti, &nvibration_type, &nvibration_path);
	DBG("Vibration : %d, %s", nvibration_type, nvibration_path);
	if (nvibration_type > NOTIFICATION_VIBRATION_TYPE_NONE
	    || nvibration_type < NOTIFICATION_VIBRATION_TYPE_MAX) {

		switch (nvibration_type) {
		case NOTIFICATION_SOUND_TYPE_DEFAULT:
			feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_GENERAL);
			break;
		case NOTIFICATION_SOUND_TYPE_USER_DATA:
			break;
		default:
			break;
		}
	}

	/* Skip if previous ticker is still shown */
	if (g_ticker != NULL) {
		_quickpanel_ticker_hide(NULL);
	}

	/* Check tickernoti flag */
	notification_get_property(noti, &flags);

	if (flags & NOTIFICATION_PROP_DISABLE_TICKERNOTI) {
		INFO("NOTIFICATION_PROP_DISABLE_TICKERNOTI");
		__ticker_only_noti_del(noti);
		notification_free(noti);
	} else if (applist & NOTIFICATION_DISPLAY_APP_TICKER) {
		/* Display ticker */
		if (g_timer)
			ecore_timer_del(g_timer);

		g_ticker = _quickpanel_ticker_create_tickernoti(noti);
		if (g_ticker == NULL) {
			ERR("Fail to create tickernoti");
			__ticker_only_noti_del(noti);
			notification_free(noti);
			return;
		}

		g_timer = ecore_timer_add(QP_TICKER_DURATION,
				_quickpanel_ticker_timeout_cb, noti);

		_quickpanel_ticker_win_rotated(data);
		evas_object_show(g_ticker);

		evas_object_event_callback_add(g_ticker, EVAS_CALLBACK_SHOW,
					_quickpanel_ticker_detail_show_cb,
					g_ticker);
		evas_object_event_callback_add(g_ticker, EVAS_CALLBACK_HIDE,
					_quickpanel_ticker_detail_hide_cb,
					noti);
		evas_object_smart_callback_add(g_ticker, "clicked",
					_quickpanel_ticker_clicked_cb,
					noti);
	}
}

/*****************************************************************************
 *
 * Util functions
 *
 *****************************************************************************/
static int quickpanel_ticker_init(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	latest_inserted_time = time(NULL);
	g_window = ad->win;

	notification_register_detailed_changed_cb(_quickpanel_ticker_noti_detailed_changed_cb,
					data);

	return QP_OK;
}

static int quickpanel_ticker_fini(void *data)
{
	_quickpanel_ticker_hide(NULL);

	return QP_OK;
}

static int quickpanel_ticker_enter_hib(void *data)
{
	return QP_OK;
}

static int quickpanel_ticker_leave_hib(void *data)
{
	return QP_OK;
}

static void quickpanel_ticker_reflesh(void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	if (g_ticker != NULL) {
		_quickpanel_ticker_win_rotated(data);
	}
}
