/*
* Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All Rights Reserved.
*
* This file is part of quickpanel
*
* Written by Rajeev Ranjan <rajeev.r@samsung.com>
*
* PROPRIETARY/CONFIDENTIAL
*
* This software is the confidential and proprietary information of
* SAMSUNG ELECTRONICS ("Confidential Information").
* You shall not disclose such Confidential Information and shall use it only
* in accordance with the terms of the license agreement you entered into
* with SAMSUNG ELECTRONICS.
* SAMSUNG make no representations or warranties about the suitability of
* the software, either express or implied, including but not limited to
* the implied warranties of merchantability, fitness for a particular purpose,
* or non-infringement. SAMSUNG shall not be liable for any damages suffered by
* licensee as a result of using, modifying or distributing this software or
* its derivatives.
*
*/

#include <E_Notify.h>
#include <E_Notification_Daemon.h>
#include <Elementary.h>
#include "noti_win.h"
#include "quickpanel-ui.h"
#include "noti_display_app.h"

#define INFO(str, args...) fprintf(stdout, str"\n", ##args)
#define ERR(str, args...) fprintf(stderr, str"\n", ##args)

static E_Notification_Daemon *g_notification_daemon;
static const char *data_key = "_noti_data";
static Evas_Object *app_win;

struct Daemon_Data {
	E_Notification_Daemon *daemon;
	Eina_List *notes;
	double default_timeout;
	int next_id;
};

struct Timer_Data {
	struct Daemon_Data *daemon_data;
	E_Notification *n;
	Ecore_Timer *timer;
	enum Noti_Orient orient;
};

struct Noti_Data {
	E_Notification *n;
	const char *time;
};

/* Windows to display different orientations of notification */
static Evas_Object *noti_win[NOTI_ORIENT_LAST];

Eina_List *notification_daemon_note_list_get()
{
	struct Daemon_Data *daemon_data;

	if (!g_notification_daemon)
		return NULL;
	daemon_data = e_notification_daemon_data_get(g_notification_daemon);
	if (!daemon_data)
		return NULL;
	return daemon_data->notes;
}

void notification_daemon_note_list_clear()
{
	struct Daemon_Data *daemon_data;
	Eina_List *l;
	E_Notification *n;
	struct Noti_Data *noti_data;

	if (!g_notification_daemon)
		return;
	daemon_data = e_notification_daemon_data_get(g_notification_daemon);
	if (!daemon_data)
		return;
	EINA_LIST_FOREACH(daemon_data->notes, l, noti_data) {
		daemon_data->notes = eina_list_remove(daemon_data->notes,
			noti_data);
		n = noti_data->n;
		eina_stringshare_replace(&noti_data->time, NULL);
		e_notification_unref(n);
		free(noti_data);
	}
}

static struct Noti_Data *_notification_daemon_noti_data_find(
	struct Daemon_Data *daemon_data, E_Notification *n)
{
	Eina_List *l;
	struct Noti_Data *noti_data;

	EINA_LIST_FOREACH(daemon_data->notes, l, noti_data)
	if (noti_data->n == n)
		return noti_data;

	return NULL;
}

static void _notification_daemon_note_close(struct Daemon_Data *daemon_data,
	E_Notification *n, int reason)
{
	struct Noti_Data *noti_data;
	struct Noti_Data *noti_data_temp;
	Eina_List *l;

	noti_data = _notification_daemon_noti_data_find(daemon_data, n);
	if (!noti_data)
		return;
	daemon_data->notes = eina_list_remove(daemon_data->notes, noti_data);
	n = noti_data->n;
	eina_stringshare_replace(&noti_data->time, NULL);
	e_notification_unref(n);
	free(noti_data);
	e_notification_closed_set(n, 1);
	e_notification_daemon_signal_notification_closed(daemon_data->daemon,
		e_notification_id_get(n), reason);
}

static void _note_destroy(struct Timer_Data *timer_data,
	enum E_Notification_Closed_Reason reason)
{
	enum Noti_Orient orient;

	if (!timer_data)
		return;
	orient = timer_data->orient;
	if (!e_notification_closed_get(timer_data->n))
		_notification_daemon_note_close(timer_data->daemon_data,
			timer_data->n,
			reason);
	e_notification_unref(timer_data->n);
	ecore_timer_del(timer_data->timer);
	free(timer_data);
	if (noti_win[orient]) {
		evas_object_data_set(noti_win[orient], data_key, NULL);
		evas_object_del(noti_win[orient]);
		noti_win[orient] = NULL;
	}
}

Eina_Bool _note_close_timer_cb(void *data)
{
	_note_destroy(data, E_NOTIFICATION_CLOSED_EXPIRED);
	return ECORE_CALLBACK_CANCEL;
}

static E_Notification *_notification_daemon_note_open_find(
		struct Daemon_Data *daemon_data, unsigned int id)
{
	Eina_List *l;
	struct Noti_Data *noti_data;

	EINA_LIST_FOREACH(daemon_data->notes, l, noti_data)
	if (e_notification_id_get(noti_data->n) == id)
		return noti_data->n;

	return NULL;
}

static void _notification_daemon_note_close_cb(E_Notification_Daemon *daemon,
	unsigned int notification_id)
{
	struct Daemon_Data *daemon_data;
	E_Notification *n;

	daemon_data = e_notification_daemon_data_get(daemon);
	n = _notification_daemon_note_open_find(daemon_data, notification_id);
	if (n)
		_notification_daemon_note_close(daemon_data, n,
			E_NOTIFICATION_CLOSED_REQUESTED);
	/* else send error */
}


static void _noti_hide_cb(void *data, Evas_Object *obj,
	const char *emission, const char *source)
{
	_note_destroy(data, E_NOTIFICATION_CLOSED_DISMISSED);
}

static void _noti_button_clicked_cb(void *data, Evas_Object *obj,
	void *event_info)
{
	_note_destroy(data, E_NOTIFICATION_CLOSED_REQUESTED);
}

static void _noti_show(E_Notification *n, enum Noti_Orient orient,
	struct Timer_Data *timer_data)
{
	const char *summary;
	Evas_Object *layout;
	const char *path_icon;
	Evas_Object *icon;
	const char *category;
	const char *data_win_height = NULL;
	int noti_height = 50;
	struct Timer_Data *old_timer_data = NULL;
	Evas_Object *button = NULL;

	if (noti_win[orient]) {
		old_timer_data = evas_object_data_get(noti_win[orient],
			data_key);
		_note_destroy(old_timer_data, E_NOTIFICATION_CLOSED_REQUESTED);
	}
	noti_win[orient] = noti_win_add(NULL);
	evas_object_data_set(noti_win[orient], data_key, timer_data);
	layout = elm_layout_add(noti_win[orient]);
	/* Only for sample code the theme implementation for layout has been
	used. Apps should implement this layout/edje object which can have
	at least one TEXT/TEXT BLOCK for showing body of notification and one
	swallow part for showing icon of the notification.
	Optionally the summary can be shown as well.
	Applications need to set the minimum height of the layout or edje object
	using evas_object_size_hint_min_set which can be used by the
	notification window to resize itself.
	*/
	if (orient == NOTI_ORIENT_BOTTOM)
		elm_layout_theme_set(layout, "tickernoti", "base", "info");
	else {
			elm_layout_theme_set(layout, "tickernoti", "base",
				"default");
			button = elm_button_add(layout);
			elm_object_style_set(button, "tickernoti");
			elm_object_text_set(button, _S("IDS_COM_BODY_CLOSE"));
			elm_object_part_content_set(layout, "button", button);
			evas_object_smart_callback_add(button, "clicked",
				_noti_button_clicked_cb, timer_data);
	}
	elm_object_signal_callback_add(layout, "request,hide", "",
		_noti_hide_cb, timer_data);
	/* Getting the height of the layout from theme just for sample code.
	App is free to use any other method.
	*/
	data_win_height = (char *)elm_layout_data_get(layout, "height");
	if (data_win_height != NULL && elm_config_scale_get() > 0.0)
		noti_height = (int)(elm_config_scale_get()
			* atoi(data_win_height));
	evas_object_size_hint_min_set(layout, 1, noti_height);
	noti_win_content_set(noti_win[orient], layout);
	summary = e_notification_summary_get(n);
	if (summary)
		elm_object_part_text_set(layout, "elm.text", summary);
	else
		elm_object_part_text_set(layout, "elm.text",
			"new notification");
	path_icon = e_notification_app_icon_get(n);
	if (path_icon) {
		INFO("%s", path_icon);
		icon = elm_icon_add(layout);
		if (elm_icon_file_set(icon, path_icon, NULL)) {
			elm_icon_resizable_set(icon, EINA_TRUE, EINA_TRUE);
			elm_object_part_content_set(layout, "icon", icon);
		}
	}
	noti_win_orient_set(noti_win[orient], orient);
	if (app_win)
		elm_win_rotation_with_resize_set(noti_win[orient],
			elm_win_rotation_get(app_win));
	evas_object_show(noti_win[orient]);
}

void _notification_daemon_note_show(struct Daemon_Data *daemon_data,
	E_Notification *n)
{
	int timeout;
	const char *category;
	enum Noti_Orient orient = NOTI_ORIENT_TOP;
	struct Noti_Data *noti_data;
	time_t current;
	char buf[256];
	struct tm time_st;
	struct Timer_Data *timer_data;

	category = e_notification_hint_category_get(n);
	if (category) {
		if (!strcmp("device", category))
			orient = NOTI_ORIENT_BOTTOM;
	}

	noti_data = calloc(1, sizeof(struct Noti_Data));
	if (!noti_data)
		return;
	e_notification_ref(n);
	noti_data->n = n;
	current = time(NULL);
	localtime_r(&current, &time_st);
	strftime(buf, sizeof(buf), "%I:%M %p", &time_st);
	eina_stringshare_replace(&noti_data->time, buf);
	daemon_data->notes = eina_list_append(daemon_data->notes, noti_data);
	timeout = e_notification_timeout_get(n);
	timer_data = calloc(1, sizeof(struct Timer_Data));
	_noti_show(n, orient, timer_data);
	timer_data->daemon_data = daemon_data;
	timer_data->orient = orient;
	e_notification_ref(n);
	e_notification_closed_set(n, 0);
	timer_data->n = n;
	timer_data->timer = ecore_timer_add(timeout == -1 ?
		daemon_data->default_timeout : (float)timeout / 1000,
		_note_close_timer_cb, timer_data);

	INFO("Received notification from %s:%s %s",
	e_notification_app_name_get(n),
	e_notification_summary_get(n), e_notification_body_get(n));
}

static int _notification_cb(E_Notification_Daemon *daemon, E_Notification *n)
{
	struct Daemon_Data *daemon_data;
	unsigned int replaces_id;
	unsigned int new_id;

	daemon_data = e_notification_daemon_data_get(daemon);
	replaces_id = e_notification_replaces_id_get(n);
	if (replaces_id)
		new_id = replaces_id;
	else
		new_id = daemon_data->next_id++;
	e_notification_id_set(n, new_id);
	_notification_daemon_note_show(daemon_data, n);
	return new_id;
}

static Eina_Bool _init()
{
	Eina_Bool ret = EINA_FALSE;
	struct Daemon_Data *daemon_data;

	if (!g_notification_daemon) {
		e_notification_daemon_init();
		g_notification_daemon = e_notification_daemon_add("notification"
			, "Enlightenment");
		if (!g_notification_daemon) {
			ERR("Unable to add a notification daemon");
			return EINA_FALSE;
		}
		daemon_data = calloc(1, sizeof(struct Daemon_Data));
		if (!daemon_data) {
			notification_daemon_shutdown();
			return EINA_FALSE;
		}
		daemon_data->default_timeout = 1.0;
		daemon_data->daemon = g_notification_daemon;
		e_notification_daemon_data_set(g_notification_daemon,
			daemon_data);
		e_notification_daemon_callback_notify_set(g_notification_daemon,
			_notification_cb);
		e_notification_daemon_callback_close_notification_set(
			g_notification_daemon,
			_notification_daemon_note_close_cb);
		INFO("Initializing Notification Daemon");
	}
	ret = !!g_notification_daemon;
	return ret;
}

static Eina_Bool _shutdown()
{
	struct Daemon_Data *daemon_data;
	Eina_List *l;
	E_Notification *n;
	int i;
	struct Noti_Data *noti_data;
	Eina_Bool ret = EINA_FALSE;

	for (i = 0; i < NOTI_ORIENT_LAST; i++)
		if (noti_win[i]) {
			evas_object_del(noti_win[i]);
			noti_win[i] = NULL;
		}
	if (!g_notification_daemon)
		return ret;
	daemon_data = e_notification_daemon_data_get(
			g_notification_daemon);
	if (!daemon_data)
		return ret;
	EINA_LIST_FOREACH(daemon_data->notes, l, noti_data) {
		daemon_data->notes = eina_list_remove(daemon_data->notes,
			noti_data);
		n = noti_data->n;
		eina_stringshare_replace(&noti_data->time, NULL);
		e_notification_unref(n);
		free(noti_data);
	}
	free(daemon_data);
	e_notification_daemon_free(g_notification_daemon);
	e_notification_daemon_shutdown();
	g_notification_daemon = NULL;
	ret = EINA_TRUE;
	return ret;
}

void notification_daemon_init()
{
	int i;

	if (!_init()) {
		ERR("Error in intializing the notification daemon");
		return;
	}
	for (i = 0; i < NOTI_ORIENT_LAST; i++)
		if (noti_win[i]) {
			evas_object_del(noti_win[i]);
			noti_win[i] = NULL;
		}
}

void notification_daemon_win_set(Evas_Object *win)
{
	/* This window is used to detect the orientation of the receiver app
	window to support rotation of the notification window.
	*/
	app_win = win;
}

void notification_daemon_shutdown()
{
	if (!_shutdown()) {
		ERR("Error in shutting down the notification daemon");
		return;
	}
	app_win = NULL;
	INFO("Terminating Notification Daemon");
}
