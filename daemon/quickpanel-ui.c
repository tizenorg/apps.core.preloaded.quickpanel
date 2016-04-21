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


#include <stdio.h>
#include <signal.h>
#include <sys/utsname.h>
#include <Elementary.h>
#include <Ecore_Input.h>
#include <unistd.h>
#include <malloc.h>

#include <app.h>
#include <vconf.h>
#include <E_DBus.h>
#include <tapi_common.h>
#include <ITapiSim.h>
#include <tzsh_quickpanel_service.h>
#include <notification.h>
#include <sound_manager.h>
#include <media.h>
#include <system_settings.h>

/* quickpanel basics */
#include "common.h"
#include "common_uic.h"
#include "quickpanel-ui.h"
#include "modules.h"
#include "quickpanel_def.h"
#include "list_util.h"
#include "noti_node.h"
#include "vi_manager.h"
#include "pager.h"
#include "page_base.h"
#include "page_setting_all.h"

#include "sim_controller.h"
#include "noti.h"

/* services */
#include "keyboard.h"
#include "uninstall.h"
#ifdef QP_REMINDER_ENABLE
#include "reminder.h"
#endif
#ifdef QP_EMERGENCY_MODE_ENABLE
#include "emergency_mode.h"
#endif
#include "minictrl.h"
#include "util-time.h"

#define QP_WINDOW_PRIO 300

static struct appdata *g_app_data = NULL;

static void _ui_rotate(void *data, int new_angle);
static void _ui_geometry_info_set(void *data);
static void _ui_handler_info_set(void *data);
static void _ui_efl_cache_flush(void *evas);

HAPI void *quickpanel_get_app_data(void)
{
	return g_app_data;
}

/******************************************************************************
 *
 * UI
 *
 ****************************************************************************/
static void _ui_efl_cache_flush(void *evas)
{
	int file_cache;
	int collection_cache;
	int image_cache;
	int font_cache;

	retif(evas == NULL, , "Evas is NULL\n");
	file_cache = edje_file_cache_get();
	collection_cache = edje_collection_cache_get();
	image_cache = evas_image_cache_get(evas);
	font_cache = evas_font_cache_get(evas);

	edje_file_cache_set(file_cache);
	edje_collection_cache_set(collection_cache);
	evas_image_cache_set(evas, 0);
	evas_font_cache_set(evas, 0);

	evas_image_cache_flush(evas);
	evas_render_idle_flush(evas);
	evas_font_cache_flush(evas);

	edje_file_cache_flush();
	edje_collection_cache_flush();

	edje_file_cache_set(file_cache);
	edje_collection_cache_set(collection_cache);
	evas_image_cache_set(evas, image_cache);
	evas_font_cache_set(evas, font_cache);

	elm_cache_all_flush();
	malloc_trim(0);
}

static void _ui_handler_input_region_set(void *data, int contents_height)
{
	struct appdata *ad = NULL;
	tzsh_region_h region;
	unsigned int window_input_region[4] = {0,};

	retif(data == NULL,  , "Invialid parameter!");
	ad = data;
	
	region = tzsh_region_create(ad->tzsh);

	switch (ad->angle) {
	case 0:
		window_input_region[0] = 0; //X
		window_input_region[1] = contents_height; // Y
		window_input_region[2] = ad->win_width; // Width
		window_input_region[3] = ELM_SCALE_SIZE(QP_HANDLE_H); // height
		break;
	case 90:
		window_input_region[0] = contents_height; //X
		window_input_region[1] = 0; // Y
		window_input_region[2] = ELM_SCALE_SIZE(QP_HANDLE_H); // Width
		window_input_region[3] = ad->win_height; // height
		break;
	case 180:
		window_input_region[0] = 0; //X
		window_input_region[1] = ad->win_height - contents_height - ELM_SCALE_SIZE(QP_HANDLE_H); // Y
		window_input_region[2] = ad->win_width; // Width
		window_input_region[3] = ELM_SCALE_SIZE(QP_HANDLE_H); // height
		break;
	case 270:
		window_input_region[0] = ad->win_width - contents_height - ELM_SCALE_SIZE(QP_HANDLE_H); //X
		window_input_region[1] = 0; // Y
		window_input_region[2] = ELM_SCALE_SIZE(QP_HANDLE_H); // Width
		window_input_region[3] = ad->win_height; // height
		break;
	}

	INFO("win_input_0:%d\nwin_input_1:%d\nwin_input_2:%d\nwin_input_3:%d\n"
			,window_input_region[0]
			,window_input_region[1]
			,window_input_region[2]
			,window_input_region[3]
		);

	tzsh_region_add(region, window_input_region[0], window_input_region[1], window_input_region[2], window_input_region[3]);
	tzsh_quickpanel_service_handler_region_set(ad->quickpanel_service, ad->angle, region);
	tzsh_region_destroy(region);
}

static void _ui_handler_content_region_set(void *data, int contents_height)
{
	struct appdata *ad = NULL;

	tzsh_region_h region;

	unsigned int window_contents_region[4] = {0,};

	retif(data == NULL,  , "Invialid parameter!");
	ad = data;

	region = tzsh_region_create(ad->tzsh);

	switch (ad->angle) {
	case 0:
		window_contents_region[0] = 0; //X
		window_contents_region[1] = 0; // Y
		window_contents_region[2] = ad->win_width; // Width
		window_contents_region[3] = contents_height; // height
		break;
	case 90:
		window_contents_region[0] = 0; //X
		window_contents_region[1] = 0; // Y
		window_contents_region[2] = contents_height; // Width
		window_contents_region[3] = ad->win_height; // height
		break;
	case 180:
		window_contents_region[0] = 0; //X
		window_contents_region[1] = ad->win_height - contents_height; // Y
		window_contents_region[2] = ad->win_width; // Width
		window_contents_region[3] = contents_height; // height
		break;
	case 270:
		window_contents_region[0] = ad->win_width - contents_height ; //X
		window_contents_region[1] = 0; // Y
		window_contents_region[2] = contents_height; // Width
		window_contents_region[3] = ad->win_height; // height
		break;
	}

	DBG("win_contents_0:%d\nwin_contents_1:%d\nwin_contents_2:%d\nwin_contents_3:%d\n"
			,window_contents_region[0]
			,window_contents_region[1]
			,window_contents_region[2]
			,window_contents_region[3]
	   );

	tzsh_region_add(region, window_contents_region[0], window_contents_region[1], window_contents_region[2], window_contents_region[3]);
	tzsh_quickpanel_service_content_region_set(ad->quickpanel_service, ad->angle, region);
	tzsh_region_destroy(region);

}

static void _ui_handler_info_set(void *data)
{
	int contents_height = 0;
	struct appdata *ad = NULL;

	retif(data == NULL, , "data is NULL");
	ad = data;

	contents_height = ad->gl_distance_from_top + ad->gl_limit_height;

	_ui_handler_input_region_set(ad, contents_height);
	_ui_handler_content_region_set(ad, contents_height);
}

static void _ui_geometry_info_set(void *data)
{
	struct appdata *ad = NULL;
	int max_height_window = 0;
	Evas_Coord genlist_y = 0;

	retif(data == NULL, , "data is NULL");
	ad = data;

	if (ad->angle == 90 || ad->angle == 270 ) {
		max_height_window = ad->win_width;
	} else {
		max_height_window = ad->win_height;
	}

	edje_object_part_geometry_get(_EDJ(ad->ly), "qp.base.list.swallow", NULL, &genlist_y, NULL, NULL);

	ad->gl_distance_from_top = genlist_y;
	ad->gl_distance_to_bottom = ELM_SCALE_SIZE(QP_HANDLE_H);
	ad->gl_limit_height = max_height_window - ad->gl_distance_from_top - ad->gl_distance_to_bottom;
}

/*****************************************************************************
 *
 * ui rotation functions
 *
 ****************************************************************************/
static void _ui_rotation_wm_cb(void *data, Evas_Object *obj, void *event)
{
	int angle = 0;
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	angle = elm_win_rotation_get((Evas_Object *)obj);

	DBG("ROTATE:%d", angle);

	quickpanel_minictrl_rotation_report(angle);

	_ui_rotate(ad, angle);
}

static int _ui_rotation_angle_get(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retif(ad == NULL, 0, "Invalid parameter!");
	retif(ad->win == NULL, 0, "Invalid parameter!");

	return elm_win_rotation_get(ad->win);
}

static void _ui_handler_enable_set(Eina_Bool is_enable)
{
	const char *signal = NULL;
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid data.");
	retif(ad->view_root == NULL, , "data is NULL");

	if (is_enable == EINA_TRUE) {
		signal = "mouse,down,1";
	} else {
		signal = "mouse,up,1";
	}

	elm_object_signal_emit(ad->view_root, signal, "qp.handler.bg");
}

static void _ui_rotation_handler(struct appdata *ad, int angle)
{
	const char *signal = NULL;

	retif(ad == NULL, , "data is NULL");
	retif(ad->view_root == NULL, , "data is NULL");


	if (angle == 90 || angle == 270) {
		signal = "quickpanel.landscape";
	} else {
		signal = "quickpanel.portrait";
	}

	elm_object_signal_emit(ad->view_root, signal, "quickpanel.prog");
}

static void _ui_rotate(void *data, int new_angle)
{
	struct appdata *ad = data;
	retif(data == NULL, , "Invalid parameter!");

	DBG("ROTATION: new:%d old:%d", new_angle, ad->angle);

	if (new_angle == 0 || new_angle == 90 || new_angle == 180 || new_angle == 270) {
		if (new_angle != ad->angle) {
			ad->angle = new_angle;
			quickpanel_modules_refresh(ad);
			_ui_geometry_info_set(ad);
			_ui_handler_info_set(ad);
			_ui_rotation_handler(ad, ad->angle);
		}
	}
}
#if 1
static int _tzsh_set(Evas_Object *win)
{
	tzsh_h tzsh = NULL;
	tzsh_quickpanel_service_h quickpanel_service = NULL;
	tzsh_window tz_win;

	retif(!win, QP_FAIL, "Invialid parameter!");

	tzsh = tzsh_create(TZSH_TOOLKIT_TYPE_EFL);
	retif(!tzsh, QP_FAIL, "tzsh_create ERROR!");

	struct appdata *ad = quickpanel_get_app_data();

	ad->tzsh = tzsh;

	tz_win = elm_win_window_id_get(win);
	if (!tz_win) {
		tzsh_destroy(tzsh);
		return QP_FAIL;
	}

	quickpanel_service = tzsh_quickpanel_service_create(tzsh, tz_win);
	if (!quickpanel_service) {
		tzsh_destroy(tzsh);
		return QP_FAIL;
	}
	ad->quickpanel_service = quickpanel_service;

	return QP_OK;
}

static void _tzsh_unset(void)
{
	struct appdata *ad = quickpanel_get_app_data();

	if (ad->quickpanel_service) {
		tzsh_quickpanel_service_destroy(ad->quickpanel_service);
		ad->quickpanel_service = NULL;
	}

	if (ad->tzsh) {
		tzsh_destroy(ad->tzsh);
		ad->tzsh = NULL;
	}
}
#endif

/*****************************************************************************
 *
 * ui creation/deletion functions
 *
 ****************************************************************************/
static Evas_Object *_ui_window_add(const char *name, int prio)
{
	Evas_Object *eo = NULL;

	eo = elm_win_add(NULL, name, ELM_WIN_BASIC);

	if (eo != NULL) {
		elm_win_alpha_set(eo, EINA_TRUE);
		elm_win_indicator_mode_set(eo, ELM_WIN_INDICATOR_HIDE);

		elm_win_title_set(eo, name);
		elm_win_borderless_set(eo, EINA_TRUE);
		elm_win_autodel_set(eo, EINA_TRUE);

		/* set this window as a quickpanel */
		elm_win_quickpanel_set(eo, 1);
		elm_win_quickpanel_priority_major_set(eo, prio);

		if (elm_win_wm_rotation_supported_get(eo)) {
			int rots[4] = { 0, 90, 180, 270 };
			elm_win_wm_rotation_available_rotations_set(eo, rots, 4);
		}

		evas_object_show(eo);

		if( QP_OK != _tzsh_set(eo)) {
			ERR("Failed to set tzsh");
		}
	}

	return eo;
}

static int _ui_gui_create(void *data)
{
	struct appdata *ad = data;
	int w = 0, h = 0;
	int initial_angle = 0;
	Evas_Object *page_base = NULL;

	retif(data == NULL, QP_FAIL, "Invialid parameter!");

	ad->win = _ui_window_add("Quickpanel Window",
			QP_WINDOW_PRIO);
	retif(ad->win == NULL, QP_FAIL, "Failed to create main window");
	//build error
	//elm_win_focus_allow_set(ad->win, EINA_TRUE);

	evas_object_smart_callback_add(ad->win, "wm,rotation,changed",
			_ui_rotation_wm_cb, ad);

	ad->background = elm_bg_add(ad->win);
	if (ad->background  != NULL) {
		evas_object_size_hint_weight_set(ad->background,
				EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_win_resize_object_add(ad->win, ad->background);
		evas_object_show(ad->background);
	} else {
		ERR("failed to create background");
	}

	ad->view_root = quickpanel_uic_load_edj(ad->background,
			DEFAULT_EDJ, "quickpanel/root", 0);
	retif(ad->view_root == NULL, QP_FAIL, "Failed to create main page");

	Evas_Object *pager_scroller = quickpanel_pager_new(ad->view_root, NULL);
	Evas_Object *pager_box = quickpanel_pager_view_get("BOX");

	page_base = quickpanel_page_base_create(pager_box, NULL);
	retif(page_base == NULL, QP_FAIL, "Failed to create main page");
	ad->ly = quickpanel_page_base_view_get("LAYOUT");
	retif(ad->ly == NULL, QP_FAIL, "Failed to create main page");

	elm_box_pack_end(pager_box, page_base);
	elm_win_resize_object_add(ad->win, ad->view_root);
	elm_object_part_content_set(ad->view_root, "qp.root.swallow", pager_scroller);

	/* get noti evas */
	ad->evas = evas_object_evas_get(ad->win);
	ad->list = quickpanel_page_base_view_get("BOX");
	ad->scroller = quickpanel_page_base_view_get("SCROLLER");

	//ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
	elm_win_screen_size_get(ad->win, NULL, NULL, &w, &h);
	evas_object_resize(ad->win, w, h);

	ad->win_width = w;
	ad->win_height = h;

	_ui_geometry_info_set(ad);

	initial_angle = _ui_rotation_angle_get(ad);
	_ui_rotate(ad, initial_angle);

	quickpanel_pager_page_set(PAGE_IDX_MAIN, 1);

	sim_controller_init(ad->ly);

	quickpanel_noti_init_noti_section();

	return 0;
}

static int _ui_gui_destroy(void *data)
{
	struct appdata *ad = data;
	retif(data == NULL, QP_FAIL, "Invialid parameter!");

	if (ad->list != NULL) {
		evas_object_del(ad->list);
		ad->list = NULL;
	}
	if (ad->scroller != NULL) {
		evas_object_del(ad->scroller);
		ad->scroller = NULL;
	}
	if (ad->ly != NULL) {
		evas_object_del(ad->ly);
		ad->ly = NULL;
	}
	if (ad->win != NULL) {
		evas_object_del(ad->win);
		ad->win = NULL;
	}

	_tzsh_unset();

	return QP_OK;
}

static void _ui_setting_visibility_set(struct appdata *ad, int show)
{
	retif(ad == NULL, , "data is NULL");
	retif(ad->ly == NULL, , "data is NULL");

	elm_object_signal_emit(ad->ly, "quickpanel.setting.show",
			"quickpanel.prog");
}

/*****************************************************************************
 *
 * event handler initialization functions
 *
 ****************************************************************************/
static void _vconf_event_powerff_cb(keynode_t *node,
		void *data)
{
	int val;
	if (vconf_get_int(VCONFKEY_SYSMAN_POWER_OFF_STATUS, &val) == 0 &&
			(val == VCONFKEY_SYSMAN_POWER_OFF_DIRECT || val == VCONFKEY_SYSMAN_POWER_OFF_RESTART)) {
		ui_app_exit();
	}
}

static void _vconf_event_lcdoff_cb(keynode_t *node,
		void *data)
{
	int ret = 0;
	int pm_state = VCONFKEY_PM_STATE_NORMAL;

	ret = vconf_get_int(VCONFKEY_PM_STATE, &pm_state);

	if (ret == 0 && pm_state == VCONFKEY_PM_STATE_LCDOFF) {
		quickpanel_uic_close_quickpanel(false, 0);
	}
}


void _event_message_cb(void *data, Evas_Object *obj, void *event_info)
 {
	bool visiblity = (bool)event_info;
	struct appdata *ad = data;

	if(visiblity == 1) { // show
		DBG("quickpanel is opened");

		ad->is_opened = 1;
		quickpanel_util_time_timer_enable_set(1);
		quickpanel_keyboard_openning_init(ad);
		quickpanel_modules_opened(data);
		quickpanel_uic_opened_reason_set(OPENED_NO_REASON);
	} else {
		DBG("quickpanel is closed");

		ad->is_opened = 0;
		quickpanel_util_time_timer_enable_set(0);
		quickpanel_keyboard_closing_fini(ad);
		quickpanel_modules_closed(data);
	}
	quickpanel_media_player_stop();
 }

static void _vconf_init(struct appdata *ad)
{
	int ret = 0;

	ret = vconf_notify_key_changed(VCONFKEY_PM_STATE,
			_vconf_event_lcdoff_cb, ad);
	if (ret != 0) {
		ERR("VCONFKEY_PM_STATE: %d", ret);
	}

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_POWER_OFF_STATUS,
			_vconf_event_powerff_cb, ad);
	if (ret != 0) {
		ERR("VCONFKEY_PM_STATE: %d", ret);
	}
}

static void _vconf_fini(struct appdata *ad)
{
	int ret = 0;

	ret = vconf_ignore_key_changed(VCONFKEY_PM_STATE,
			_vconf_event_lcdoff_cb);
	if (ret != 0) {
		ERR("VCONFKEY_PM_STATE: %d", ret);
	}

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_POWER_OFF_STATUS,
			_vconf_event_powerff_cb);
	if (ret != 0) {
		ERR("VCONFKEY_PM_STATE: %d", ret);
	}
}

static void _edbus_init(struct appdata *ad)
{
	e_dbus_init();
	ad->dbus_connection = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (ad->dbus_connection == NULL) {
		ERR("noti register : failed to get dbus bus");
	}
}

static void _edbus_fini(struct appdata *ad)
{
	if (ad->dbus_connection != NULL) {
		e_dbus_connection_close(ad->dbus_connection);
		ad->dbus_connection = NULL;
		e_dbus_shutdown();
	}
}

static void _ecore_event_init(struct appdata *ad)
{
	DBG("");
	evas_object_smart_callback_add(ad->win, "visibility,changed", _event_message_cb, ad);
}

static void _ecore_event_fini(struct appdata *ad)
{
	if (ad->hdl_client_message != NULL) {
		ecore_event_handler_del(ad->hdl_client_message);
		ad->hdl_client_message = NULL;
	}
}

/*****************************************************************************
 *
 * App efl main interface
 *
 ****************************************************************************/
static void _sigaction_terminate_handler(int signum, siginfo_t *info, void *unused)
{
	ERR("quickpanel going to be terminated");
	ui_app_exit();
}

static void _service_request_process(app_control_h service, void *data)
{
	char *value = NULL;
	retif(service == NULL, , "Invialid parameter!");

	if (!app_control_get_extra_data(service, "HIDE_LAUNCH", &value))
	{
		if (value != NULL) {
			ERR("HIDE_LAUNCH: %s", value);
			if (!strcmp(value, "1")) {
				quickpanel_uic_close_quickpanel(false, 0);
			} else {
				quickpanel_uic_open_quickpanel(OPENED_BY_CMD_HIDE_LAUNCH);
			}

			free(value);
		}
	} else if (!app_control_get_extra_data(service, "SHOW_SETTINGS", &value)) {
		if (value != NULL) {
			ERR("SHOW_SETTINGS: %s", value);
			if (!strcmp(value, "1")) {
				quickpanel_pager_page_set(PAGE_IDX_EDITING, 0);
				quickpanel_uic_open_quickpanel(OPENED_BY_CMD_SHOW_SETTINGS);
			}

			free(value);
		}
	}
#ifdef QP_EMERGENCY_MODE_ENABLE
	else if (!app_control_get_extra_data(service, "EMERGENCY_MODE_LAUNCH", &value)) {
		if (value != NULL) {
			ERR("EMERGENCY_MODE_LAUNCH: %s", value);
			if (!strcmp(value, "1")) {
				if (quickpanel_emergency_mode_syspopup_launch() == QP_FAIL) {
					ERR("failed to launch emergency mode syspopup");
				} else {
					quickpanel_uic_close_quickpanel(true, 0);
				}
			}

			free(value);
		}
	}
#endif
}

static Eina_Bool _appcore_cache_flush_timer_cb(void *data)
{
	if (!quickpanel_uic_is_suspended()) {
		return ECORE_CALLBACK_CANCEL;
	}

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _ui_refresh_idler_cb(void *data)
{
	DBG("");
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	quickpanel_modules_refresh(ad);
	_ui_geometry_info_set(ad);
	_ui_handler_info_set(ad);

	/*	Cache memory is cleared when the application paused (every time, after 5 seconds (in appcore)),
	 * 	but after running in a minimized mode application have status AS_RUNNING.
	 * 	Application have status AS_PAUSED only after change of visibility to hidden condition by user (on hiding window)
	 * 	Cleaning must be performed only once after application loading in hidden condition
	 * 	(and stay in a hidden condition at time of cleaning).
	 */
	ecore_timer_add(10, _appcore_cache_flush_timer_cb, NULL);

	return EINA_FALSE;
}

static void _quickpanel_initialize(void *data)
{
	int ret = 0;
	struct appdata *ad = data;
	retif(ad == NULL, , "Invialid parameter!");

	INFO(">> Creating Quickpanel");
	/* Check emulator */
	ad->is_emul = quickpanel_uic_is_emul();
	INFO("quickpanel run in %s", ad->is_emul ? "Emul" : "Device");

	int w, h;
	elm_win_screen_size_get(ad->win, NULL, NULL, &w, &h);
	ad->scale = elm_config_scale_get();
	if (ad->scale < 0) {
		ad->scale = 1.0;
	}

	INFO("quickpanel scale %f", ad->scale);

	ad->is_suspended = 1;

	/* Get theme */
	elm_theme_extension_add(NULL, DEFAULT_THEME_EDJ);
	/* create quickpanel window */
	ret = _ui_gui_create(ad);
	retif(ret != QP_OK, , "Failed to create window!");

	quickpanel_media_init();

	_ecore_event_init(ad);
	_vconf_init(ad);
	_edbus_init(ad);

	quickpanel_uninstall_init(ad);
#ifdef QP_EMERGENCY_MODE_ENABLE
	quickpanel_emergency_mode_init(ad);
#endif
	quickpanel_keyboard_init(ad);
#ifdef QP_REMINDER_ENABLE
	quickpanel_reminder_init(ad);
#endif

#ifdef QP_SETTING_ENABLE
	_ui_setting_visibility_set(ad, 1);
#else /* QP_SETTING_ENABLE */
	_ui_setting_visibility_set(ad, 0);
#endif /* QP_SETTING_ENABLE */

	/* init quickpanel modules */
	quickpanel_modules_init(ad);
	ecore_idler_add(_ui_refresh_idler_cb, ad);
}

static bool _app_create_cb(void *data)
{
	ERR("");

	elm_app_base_scale_set(1.8);

	pid_t pid;
	int r;
	char err_buf[128] = {0,};

	// signal handler
	struct sigaction act;
	act.sa_sigaction = _sigaction_terminate_handler;
	act.sa_flags = SA_SIGINFO;

	int ret = sigemptyset(&act.sa_mask);
	if (ret < 0) {
		strerror_r(errno, err_buf, sizeof(err_buf));
		ERR("Failed to sigemptyset[%d / %s]", errno, err_buf);
	}
	ret = sigaddset(&act.sa_mask, SIGTERM);
	if (ret < 0) {
		strerror_r(errno, err_buf, sizeof(err_buf));
		ERR("Failed to sigaddset[%d / %s]", errno, err_buf);
	}
	ret = sigaction(SIGTERM, &act, NULL);
	if (ret < 0) {
		strerror_r(errno, err_buf, sizeof(err_buf));
		ERR("Failed to sigaction[%d / %s]", errno, err_buf);
	}

	pid = setsid();
	if (pid < 0) {
		WARN("Failed to set session id!");
	}

	r = nice(2);
	if (r == -1) {
		WARN("Failed to set nice value!");
	}

	return TRUE;
}

static void _app_service_cb(app_control_h service, void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invialid parameter!");

	if (ad->win == NULL && ad->ly == NULL) {
		_quickpanel_initialize(data);
	} else {
		_service_request_process(service, data);
	}
}

static void _app_terminate_cb(void *data)
{
	ERR("");

	struct appdata *ad = data;
	retif(ad == NULL, , "invalid data.");

	quickpanel_media_fini();

	/* fini quickpanel modules */
	quickpanel_modules_fini(ad);
	_edbus_fini(ad);
	_vconf_fini(ad);
	_ecore_event_fini(ad);

	quickpanel_keyboard_fini(ad);
	quickpanel_uninstall_fini(ad);
#ifdef QP_REMINDER_ENABLE
	quickpanel_reminder_fini(ad);
#endif
#ifdef QP_EMERGENCY_MODE_ENABLE
	quickpanel_emergency_mode_fini(ad);
#endif

	/* delete quickpanel window */
	_ui_gui_destroy(ad);

	INFO("Quickpanel is terminated");
}

static void _app_resume_cb(void *data)
{
	DBG("");
	struct appdata *ad = data;
	retif(ad == NULL,, "invalid data.");

	ad->is_suspended = 0;
	_ui_handler_enable_set(EINA_FALSE);

	quickpanel_modules_resume(data);

	sim_controller_resume();
}

static void _app_pause_cb(void *data)
{
	DBG("");
	struct appdata *ad = data;
	retif(ad == NULL,, "invalid data.");


	quickpanel_modules_suspend(ad);

	ad->is_suspended = 1;

	if (ad->evas != NULL) {
		_ui_efl_cache_flush(ad->evas);
		evas_event_feed_mouse_cancel(ad->evas, ecore_time_get(), NULL);
	}
}

static void _app_language_changed_cb(app_event_info_h event_info, void *data)
{
	DBG("");
	quickpanel_modules_lang_change(data);

	sim_controller_on_language_change();
}

static void _app_region_format_changed_cb(app_event_info_h event_info, void *data)
{
	DBG("");
	quickpanel_modules_lang_change(data);
}

int main(int argc, char *argv[])
{
	INFO("BUILD START: %s %s", __DATE__, __TIME__);
	ERR("BUILD START: %s %s", __DATE__, __TIME__);

	int ret = 0;
	struct appdata ad;
	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	ERR("quickpanel is forked");

	event_callback.create = _app_create_cb;
	event_callback.terminate = _app_terminate_cb;
	event_callback.pause = _app_pause_cb;
	event_callback.resume = _app_resume_cb;
	event_callback.app_control = _app_service_cb;

//	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, NULL, NULL);
//	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, NULL, NULL);
//	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, NULL, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, _app_language_changed_cb, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, _app_region_format_changed_cb, &ad);

	memset(&ad, 0x0, sizeof(struct appdata));

	g_app_data = &ad;

	ret = ui_app_main(argc, argv, &event_callback, (void *)&ad);
	if (ret != APP_ERROR_NONE) {
		ERR("ui_app_main() is failed. err = %d", ret);
	}

	return ret;

}
