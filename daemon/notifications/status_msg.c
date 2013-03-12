/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <Elementary.h>
#include <Ecore_X.h>
#include <vconf.h>
#include <notification.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "noti_win.h"

#define QP_STATUS_DURATION	3
#define QP_STATUS_DETAIL_DURATION 6

#define STATUS_MSG_LEN				1024
#define DEFAULT_ICON ICONDIR		"/quickpanel_icon_default.png"

#define E_DATA_STATUS_DETAIL "detail"

static Evas_Object *g_status_win;
static Ecore_Timer *g_status_timer;
static int g_noti_height;

static int quickpanel_status_init(void *data);
static int quickpanel_status_fini(void *data);
static void quickpanel_status_reflesh(void *data);

QP_Module ticker_status = {
	.name = "ticker_status",
	.init = quickpanel_status_init,
	.fini = quickpanel_status_fini,
	.hib_enter = NULL,
	.hib_leave = NULL,
	.lang_changed = NULL,
	.refresh = quickpanel_status_reflesh
};

/*****************************************************************************
 *
 * (Static) Util functions
 *
 *****************************************************************************/
static void _quickpanel_status_hide(void *data)
{
	if (g_status_win) {
		evas_object_hide(g_status_win);
		evas_object_del(g_status_win);
		g_status_win = NULL;
	}
}

static void _quickpanel_status_set_text(Evas_Object *detail, const char *message) {
	retif(detail == NULL, , "Invalid parameter!");
	retif(message == NULL, , "Invalid parameter!");

	elm_object_part_text_set(detail, "elm.text", message);
}

static Eina_Bool _quickpanel_status_timeout_cb(void *data)
{
	DBG("");

	g_status_timer = NULL;

	_quickpanel_status_hide(data);

	return ECORE_CALLBACK_CANCEL;
}

static void _quickpanel_status_detail_hide_cb(void *data, Evas *e,
					Evas_Object *obj,
					void *event_info)
{
	if (g_status_timer) {
		ecore_timer_del(g_status_timer);
		g_status_timer = NULL;
	}
}

static void _quickpanel_status_detail_show_cb(void *data, Evas *e,
					Evas_Object *obj,
					void *event_info)
{
	DBG("");
}

static void _quickpanel_status_clicked_cb(void *data, Evas_Object *obj,
					void *event_info)
{
	_quickpanel_status_hide(data);
}

static void _noti_hide_cb(void *data, Evas_Object *obj,
			const char *emission, const char *source)
{
	DBG("");

	if (g_status_timer) {
		ecore_timer_del(g_status_timer);
		g_status_timer = NULL;
	}
}

static Evas_Object *_quickpanel_status_create_status_noti(const char *message, void *data)
{
	Evas_Object *status_noti = NULL;
	Evas_Object *detail = NULL;
	const char *data_win_height = NULL;
	int noti_height = 0;

	retif(message == NULL, NULL, "Invalid parameter!");

	status_noti = noti_win_add(NULL);
	retif(status_noti == NULL, NULL, "Failed to add elm status_noti.");

	detail = elm_layout_add(status_noti);
	if (!detail) {
		ERR("Failed to get detailview.");
		evas_object_del(status_noti);
		return NULL;
	}
	elm_layout_theme_set(detail, "tickernoti", "base", "textonly");
	elm_object_signal_callback_add(detail, "request,hide", "",
				_noti_hide_cb, NULL);

	data_win_height = (char *)elm_layout_data_get(detail, "height");
	if (data_win_height != NULL && elm_config_scale_get() > 0.0)
		noti_height = (int)(elm_config_scale_get()
					* atoi(data_win_height));
	evas_object_size_hint_min_set(detail, 1, noti_height);
	g_noti_height = noti_height;
	DBG("height:%d", g_noti_height);

	noti_win_content_set(status_noti, detail);

	_quickpanel_status_set_text(detail, message);
	/* Use style "default" for detailview mode and
	 * "info" for text only mode
	 */
	elm_object_style_set(status_noti, "textonly");
	evas_object_data_set(status_noti, E_DATA_STATUS_DETAIL, detail);

	return status_noti;
}

static int _quickpanel_status_get_angle(void *data)
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

static void _quickpanel_status_update_geometry_on_rotation(void *data, int *x, int *y, int *w, int *h) {
	int angle = 0;

	if (!data)
		return;

	angle = _quickpanel_status_get_angle(data);
	Evas_Coord root_w, root_h;

	/*
	 * manually calculate win_status_noti_indi window position & size
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
	elm_win_rotation_set(g_status_win, angle);
}

static void _quickpanel_status_win_rotated(void *data) {
	retif(data == NULL, ,"data is NULL");

	struct appdata *ad = data;
	int x = 0, y = 0, w = 0, h = 0;

	_quickpanel_status_update_geometry_on_rotation(ad, &x, &y, &w, &h);

	if (g_status_win != NULL) {
		evas_object_move(g_status_win, x, y);
		evas_object_resize(g_status_win, w, h);
	}
}

static void _quickpanel_status_cb(const char *message, void *data)
{
	DBG("");
	retif(message == NULL, ,"message is NULL");
	retif(data == NULL, ,"data is NULL");

	if (g_status_timer)
		ecore_timer_del(g_status_timer);

	/* Skip if previous status is still shown */
	if (g_status_win != NULL) {
		Evas_Object *detail = evas_object_data_get(g_status_win, E_DATA_STATUS_DETAIL);
		_quickpanel_status_set_text(detail, message);
		elm_win_activate(g_status_win);
	} else {
		g_status_win = _quickpanel_status_create_status_noti(message, data);
		if (g_status_win == NULL) {
			ERR("Fail to create status_noti");
			return;
		}

		_quickpanel_status_win_rotated(data);
		evas_object_show(g_status_win);

		evas_object_event_callback_add(g_status_win, EVAS_CALLBACK_SHOW,
					_quickpanel_status_detail_show_cb,
					g_status_win);
		evas_object_event_callback_add(g_status_win, EVAS_CALLBACK_HIDE,
					_quickpanel_status_detail_hide_cb,
					g_status_win);
		evas_object_smart_callback_add(g_status_win, "clicked",
					_quickpanel_status_clicked_cb,
					g_status_win);
	}

	g_status_timer = ecore_timer_add(QP_STATUS_DURATION,
			_quickpanel_status_timeout_cb, NULL);
}

/*****************************************************************************
 *
 * Util functions
 *
 *****************************************************************************/
static int quickpanel_status_init(void *data)
{
	int ret = QP_OK;

	ret = notification_status_monitor_message_cb_set(_quickpanel_status_cb, data);

	return ret;
}

static int quickpanel_status_fini(void *data)
{
	int ret = 0;
	_quickpanel_status_hide(NULL);

	ret = notification_status_monitor_message_cb_unset();

	return ret;
}

static void quickpanel_status_reflesh(void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	if (g_status_win != NULL) {
		_quickpanel_status_win_rotated(data);
	}
}
