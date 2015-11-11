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
#include <string.h>

#include <notification.h>
#include <vconf.h>
#include <device/display.h>
#include <app_control.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "common.h"
#include "quickpanel-ui.h"
#include "list_util.h"
#include "quickpanel_def.h"
#include "settings_view_featured.h"
#include "preference.h"
#include "setting_utils.h"
#include "page_setting_all.h"

#ifdef QP_SCREENREADER_ENABLE
#include "accessibility.h"
#endif

#ifdef QP_EMERGENCY_MODE_ENABLE
#include "emergency_mode.h"
#endif


#define BRIGHTNESS_MIN 1
#define BRIGHTNESS_MAX 100
#define BRIGHTNESS_SIG_ACTIVITY "BRIGHTNESS"
#define PREF_BRIGHTNESS_ON "ON"
#define PREF_BRIGHTNESS_OFF "OFF"

typedef struct _brightness_ctrl_obj {
	int min_level;
	int max_level;
	int is_event_registered;
	int last_requested_level;
	Evas_Object *viewer;
	void *data;
	int level_before;
	int pos_x;

	Evas_Object *brighntess_slider;
} brightness_ctrl_obj;


int slider_drag_start = -1;
Eina_Bool is_sliding = EINA_FALSE;


static int _init(void *data);
static int _fini(void *data);
static void _lang_changed(void *data);
static void _qp_opened(void *data);
static void _qp_closed(void *data);
static void _brightness_view_update(void);
static void _brightness_register_event_cb(brightness_ctrl_obj *ctrl_obj);
static void _brightness_deregister_event_cb(brightness_ctrl_obj *ctrl_obj);

static void _brightness_set_image(int level);
static void _refresh(void *data);

QP_Module brightness_ctrl = {
	.name = "brightness_ctrl",
	.init = _init,
	.fini = _fini,
	.suspend = NULL,
	.resume = NULL,
	.hib_enter = NULL,
	.hib_leave = NULL,
	.lang_changed = _lang_changed,
	.refresh = _refresh,
	.get_height = NULL,
	.qp_opened = _qp_opened,
	.qp_closed = _qp_closed,
};

static brightness_ctrl_obj *g_ctrl_obj;
E_DBus_Signal_Handler *g_hdl_brightness;

static Evas_Object *_controller_view_get(void)
{
	Evas_Object *view = NULL;

	if (g_ctrl_obj != NULL) {
		if (g_ctrl_obj->viewer != NULL) {
			view = elm_object_part_content_get(g_ctrl_obj->viewer, "elm.swallow.controller");
			if (view == NULL) {
				view = evas_object_data_get(g_ctrl_obj->viewer, "view.controller");
			}
		}
	}

	return view;
}

static void _controller_view_set(Evas_Object *wrapper, Evas_Object *view)
{
	retif(wrapper == NULL, , "invalid data");
	retif(view == NULL, , "invalid data");

	elm_object_part_content_set(wrapper, "elm.swallow.controller", view);
	evas_object_data_set(wrapper, "view.controller", view);
}

static char *_brightness_access_state_cb(void *data, Evas_Object *obj)
{
	char buf[512] = {0,};
	brightness_ctrl_obj *ctrl_obj = data;
	retif(NULL == ctrl_obj, NULL, "invalid data");

	snprintf(buf, sizeof(buf) - 1, _NOT_LOCALIZED("Position %1$d of %2$d"),
			ctrl_obj->last_requested_level, ctrl_obj->max_level);

	return strdup(buf);
}

#ifdef QP_SCREENREADER_ENABLE
static void _set_slider_accessiblity_state(Evas_Object *obj)
{
	Evas_Object *ao = NULL;
	brightness_ctrl_obj *ctrl_obj = g_ctrl_obj;
	retif(ctrl_obj == NULL, , "Invalid parameter!");
	retif(ctrl_obj->viewer == NULL, , "Invalid parameter!");

	ao = quickpanel_accessibility_screen_reader_object_get(obj,
			SCREEN_READER_OBJ_TYPE_ELM_OBJECT, NULL, NULL);
	if (ao != NULL) {
		elm_access_info_set(ao, ELM_ACCESS_INFO, _NOT_LOCALIZED("Brightness"));
		elm_access_info_set(ao, ELM_ACCESS_TYPE, _NOT_LOCALIZED("Slider"));
		elm_access_info_cb_set(ao, ELM_ACCESS_STATE, _brightness_access_state_cb, ctrl_obj);
	}
}
#endif

static Evas_Object *_check_duplicated_loading(Evas_Object *obj, const char *part)
{
	Evas_Object *old_content = NULL;
	retif(obj == NULL, NULL, "Invalid parameter!");
	retif(part == NULL, NULL, "Invalid parameter!");

	old_content = elm_object_part_content_get(obj, part);
	if (old_content != NULL) {
		return old_content;
	}

	return NULL;
}

static void _brightness_vconf_cb(keynode_t *key, void* data)
{
	brightness_ctrl_obj *ctrl_obj = NULL;

	retif(data == NULL, , "Data parameter is NULL");
	ctrl_obj = data;

	if (ctrl_obj->viewer != NULL) {
		_brightness_view_update();
	}
}

static int _brightness_set_level(int level)
{
	int ret = DEVICE_ERROR_NONE;

	ret = device_display_set_brightness(0, level);
	if (ret != DEVICE_ERROR_NONE) {
		ERR("failed to set brightness");
	}

	return level;
}

static int _brightness_get_level(void) {

	int level = 0;

	if (vconf_get_int(VCONFKEY_SETAPPL_LCD_BRIGHTNESS, &level) == 0) {
		return level;
	} else {
		return SETTING_BRIGHTNESS_LEVEL5;
	}
}

Evas_Object *_slider_get(Evas_Object *view, brightness_ctrl_obj *ctrl_obj) {

	retif(view == NULL, NULL, "Data parameter is NULL");

	Evas_Object *obj = elm_object_part_content_get(view, "elm.swallow.slider");

	if (obj) {
		return obj;
	} else {
		return ctrl_obj->brighntess_slider;
	}
}

static void _slider_changed_job_cb(void *data)
{
	int value = 0;
	double val = 0.0;
	Evas_Object *obj = NULL;
	brightness_ctrl_obj *ctrl_obj = data;
	double time_current = 0.0;
	static double time_before = 0.0;

	retif(ctrl_obj == NULL, , "Data parameter is NULL");
	obj = _slider_get(_controller_view_get(), ctrl_obj);
	retif(obj == NULL, , "obj is NULL");

	val = elm_slider_value_get(obj);
	value = (int)(val + 0.5);

	time_current = ecore_loop_time_get();

	if (value != ctrl_obj->last_requested_level) {
		if (value >= ctrl_obj->min_level && value <= ctrl_obj->max_level) {
			ctrl_obj->last_requested_level = value;
			if (time_current - time_before >= 0.045) {
				_brightness_set_level(value);
				time_before = time_current;
			}
			_brightness_set_image(value);
		}
	}


}

static void _brightness_ctrl_slider_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	int pos_new = (long)event_info;
	LOGI("SLIDER_NEW_POS: %d", pos_new);

	_slider_changed_job_cb(data);
}

static void _brightness_ctrl_overheat_check(Evas_Object *slider, void *data, int is_display_popup)
{
	int value = 0;
	int max_brightness = BRIGHTNESS_MAX;
	brightness_ctrl_obj *ctrl_obj = data;
	retif(slider == NULL, , "slider is NULL");
	retif(ctrl_obj == NULL, , "Data parameter is NULL");

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid data.");

	value = ctrl_obj->last_requested_level;

	device_display_get_max_brightness(0, &max_brightness);
	if (value > max_brightness && max_brightness != BRIGHTNESS_MAX) {
		DBG("max brightness is limited");
		if (is_display_popup == 1) {
			if (ad->popup == NULL) {
				quickpanel_setting_create_timeout_popup(ad->win,
						_("IDS_ST_POP_UNABLE_TO_INCREASE_BRIGHTNESS_FURTHER_BECAUSE_OF_PHONE_OVERHEATING"));
			}
		}
		elm_slider_value_set(slider, (double)max_brightness);
		ctrl_obj->last_requested_level = max_brightness;
		_brightness_set_level(max_brightness);
		_brightness_set_image(max_brightness);
		return;
	}
}

static void _slider_delayed_changed_job_cb(void *data)
{
	int value = 0;
	brightness_ctrl_obj *ctrl_obj = data;
	Evas_Object *obj = NULL;
	retif(ctrl_obj == NULL, , "Data parameter is NULL");
	obj = _slider_get(_controller_view_get(), ctrl_obj);
	retif(obj == NULL, , "obj is NULL");

	value = ctrl_obj->last_requested_level;

	if (value >= ctrl_obj->min_level && value <= ctrl_obj->max_level) {
		_brightness_set_level(value);
		_brightness_set_image(value);
	}
}

static void _brightness_ctrl_slider_delayed_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	LOGI("");
	ecore_job_add(_slider_delayed_changed_job_cb, data);
}

static void _brightness_slider_drag_start_cb(void *data, Evas_Object *obj, void *event_info)
{
	is_sliding = EINA_TRUE;
	slider_drag_start = _brightness_get_level();
}

static void _brightness_slider_drag_stop_cb(void *data, Evas_Object *obj, void *event_info)
{
	is_sliding = EINA_FALSE;
}

/*!
 * workaround to avoid focus jump to other pages
 */
static void _frame_focused(void *data, Evas_Object * obj, void *event_info)
{
	quickpanel_page_setting_all_focus_allow_set(EINA_FALSE);
}

static void _frame_unfocused(void *data, Evas_Object * obj, void *event_info)
{
	quickpanel_page_setting_all_focus_allow_set(EINA_TRUE);
}

static void _brightness_view_pos_set()
{
	struct appdata *ad = quickpanel_get_app_data();

	Evas_Coord base_y;
	//	Evas_Coord settings_y;
	Evas_Coord brightness_y;

	Eina_Bool ret = EINA_FALSE;

	edje_object_part_geometry_get(_EDJ(ad->view_root), "qp.root.swallow", NULL, &base_y, NULL, NULL);
	//	edje_object_part_geometry_get(ad->ly, QP_SETTING_BASE_PART, NULL, &settings_y, NULL, NULL);

	Evas_Object *settings_swallow = quickpanel_setting_layout_get(ad->ly, QP_SETTING_BASE_PART);
	ret = edje_object_part_geometry_get(_EDJ(settings_swallow), QP_SETTING_BRIGHTNESS_PART_WVGA, NULL, &brightness_y, NULL, NULL);
	msgif(!ret, "ret is EINA_FALSE");

	evas_object_move(g_ctrl_obj->viewer, 0, base_y + /*settings_y */+ brightness_y);
}

static Evas_Object *_brightness_view_create(Evas_Object *list)
{
	Eina_Bool ret = EINA_TRUE;
	Evas_Object *view_wrapper = NULL;
	Evas_Object *view = NULL;

	retif(list == NULL, NULL, "list parameter is NULL");

	view_wrapper = elm_layout_add(list);
	if (view_wrapper != NULL) {
		ret = elm_layout_file_set(view_wrapper, DEFAULT_EDJ,
				"quickpanel/brightness_controller/wrapper");
		if (ret == EINA_FALSE) {
			ERR("failed to load brightness wapper layout");
		}
		evas_object_size_hint_weight_set(view_wrapper, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(view_wrapper, EVAS_HINT_FILL, EVAS_HINT_FILL);

		view = elm_layout_add(view_wrapper);
		if (view != NULL) {
			ret = elm_layout_file_set(view, DEFAULT_EDJ,
					"quickpanel/brightness_controller/default");

			if (ret == EINA_FALSE) {
				ERR("failed to load brightness layout");
			}
			evas_object_size_hint_weight_set(view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(view, EVAS_HINT_FILL, EVAS_HINT_FILL);

			Evas_Object *focus = quickpanel_accessibility_ui_get_focus_object(view);
			elm_access_info_cb_set(focus, ELM_ACCESS_TYPE, quickpanel_accessibility_info_cb_s, _NOT_LOCALIZED("Brightness"));
			elm_object_part_content_set(view, "focus", focus);

			evas_object_smart_callback_add(focus, "focused", _frame_focused, NULL);
			evas_object_smart_callback_add(focus, "unfocused", _frame_unfocused, NULL);
			evas_object_show(view);
			_brightness_view_pos_set();

			g_ctrl_obj->brighntess_slider = view;
			_controller_view_set(view_wrapper, view);
			evas_object_show(view_wrapper);
		}
	}

	return view_wrapper;
}

static void _brightness_set_image(int level)
{
	int mapped_level;

	if (!g_ctrl_obj) {
		ERR("Ctrl Obj is not defined");
		return;
	}

	if (level <= 1) {
		mapped_level = 0;
	} else if (level >= 100) {
		mapped_level = 11;
	} else if (level > 1 && level <= 9){
		mapped_level = 1;
	} else {
		mapped_level = (level / 10);
	}

	if (g_ctrl_obj->level_before != mapped_level ) {
		char buf[128] = {0,};
		Evas_Object *view;

		view = _controller_view_get();
		snprintf(buf, sizeof(buf) - 1, "icon.state.%d", mapped_level);
		elm_object_signal_emit(view, buf, "prog");
		g_ctrl_obj->level_before = mapped_level;
	}
}

static void _brightness_set_slider(void)
{
	int value = 0;
	Evas_Object *slider = NULL;
	Evas_Object *old_obj = NULL;
	brightness_ctrl_obj *ctrl_obj = g_ctrl_obj;
	Evas_Object *view = _controller_view_get();
	retif(ctrl_obj == NULL, , "Invalid parameter!");
	retif(view == NULL, , "Invalid parameter!");

	old_obj = _check_duplicated_loading(view, "elm.swallow.slider");

	if (old_obj == NULL) {
		slider = elm_slider_add(view);

		if (slider != NULL) {
			evas_object_size_hint_weight_set(slider, EVAS_HINT_EXPAND, 0.0);
			evas_object_size_hint_align_set(slider, EVAS_HINT_FILL, 0.5);
			elm_slider_min_max_set(slider, ctrl_obj->min_level, ctrl_obj->max_level);
			evas_object_smart_callback_add(slider, "changed", _brightness_ctrl_slider_changed_cb, ctrl_obj);
			evas_object_smart_callback_add(slider, "delay,changed", _brightness_ctrl_slider_delayed_changed_cb, ctrl_obj);
			evas_object_smart_callback_add(slider, "slider,drag,start", _brightness_slider_drag_start_cb, ctrl_obj);
			evas_object_smart_callback_add(slider, "slider,drag,stop", _brightness_slider_drag_stop_cb, ctrl_obj);
			elm_object_part_content_set(view, "elm.swallow.slider", slider);
		} else {
			ERR("failed to create slider");
			return;
		}
	} else {
		slider = old_obj;
	}

	elm_object_style_set(slider, "quickpanel_style");

	elm_slider_indicator_format_set(slider, NULL);
	elm_slider_indicator_format_function_set(slider, NULL, NULL);
	elm_slider_indicator_show_set(slider, EINA_FALSE);

#ifdef QP_SCREENREADER_ENABLE
	_set_slider_accessiblity_state(slider);
#endif

	value = _brightness_get_level();
	elm_slider_value_set(slider, value);
	_brightness_set_image(value);
}

static void _focus_pair_set()
{
	brightness_ctrl_obj *ctrl_obj = g_ctrl_obj;
	Evas_Object *focus = NULL;
	Evas_Object *slider = NULL;
	Evas_Object *view = _controller_view_get();
	retif(ctrl_obj == NULL, , "Invalid parameter!");
	retif(view == NULL, , "Invalid parameter!");

	focus = elm_object_part_content_get(view, "focus");
	slider = elm_object_part_content_get(view, "elm.swallow.slider");

	if (focus != NULL && slider != NULL) {
		/* focus */
		elm_object_focus_next_object_set(focus, slider, ELM_FOCUS_RIGHT);
		elm_object_focus_next_object_set(focus, slider, ELM_FOCUS_DOWN);

		/* slider */
		elm_object_focus_next_object_set(slider, focus, ELM_FOCUS_LEFT);
		elm_object_focus_next_object_set(slider, focus, ELM_FOCUS_UP);
	}
}

static void _brightness_view_update(void)
{
	_brightness_set_slider();
	_focus_pair_set();
}

static void _brightness_add(brightness_ctrl_obj *ctrl_obj, void *data)
{
	struct appdata *ad = data;
	retif(!ad, , "list is NULL");
	retif(!ad->list, , "list is NULL");
	retif(ctrl_obj == NULL, , "ctrl_obj is null");
	retif(ctrl_obj->viewer != NULL, , "viewer is already created");

	ctrl_obj->viewer = _brightness_view_create(ad->list);
	ctrl_obj->data = data;

	_brightness_set_image(BRIGHTNESS_MIN);
	_refresh(ad);
}

static void _brightness_remove(brightness_ctrl_obj *ctrl_obj, void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "list is NULL");
	retif(ad->list == NULL, , "list is NULL");

	if (g_ctrl_obj != NULL) {
		if (g_ctrl_obj->viewer != NULL) {
			quickpanel_list_util_item_unpack_by_object(ad->list
					, g_ctrl_obj->viewer, 0, 0);
			quickpanel_list_util_item_del_tag(g_ctrl_obj->viewer);
			evas_object_del(g_ctrl_obj->viewer);
			g_ctrl_obj->viewer = NULL;
		}
		DBG("brightness controller is removed");
		free(g_ctrl_obj);
		g_ctrl_obj = NULL;
	}
}

static void _brightness_register_event_cb(brightness_ctrl_obj *ctrl_obj)
{
	int ret = 0;
	retif(ctrl_obj == NULL, , "Data parameter is NULL");

	if (ctrl_obj->is_event_registered == 0) {
		ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_LCD_BRIGHTNESS,
				_brightness_vconf_cb, ctrl_obj);
		if (ret != 0) {
			ERR("failed to register a cb key:%s err:%d",
					"VCONFKEY_SETAPPL_LCD_BRIGHTNESS", ret);
		}
		ctrl_obj->is_event_registered = 1;
	}
}

static void _brightness_deregister_event_cb(brightness_ctrl_obj *ctrl_obj)
{
	int ret = 0;
	retif(ctrl_obj == NULL, , "Data parameter is NULL");

	if (ctrl_obj->is_event_registered == 1) {
		ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_LCD_BRIGHTNESS, _brightness_vconf_cb);
		if (ret != 0) {
			ERR("failed to register a cb key:%s err:%d", "VCONFKEY_SETAPPL_LCD_BRIGHTNESS", ret);
		}
		ctrl_obj->is_event_registered = 0;
	}
}

static void _brightness_create(void *data)
{
	if (g_ctrl_obj == NULL) {
		g_ctrl_obj = (brightness_ctrl_obj *)calloc(1, sizeof(brightness_ctrl_obj));
		if (g_ctrl_obj != NULL) {
			g_ctrl_obj->min_level = BRIGHTNESS_MIN;
			g_ctrl_obj->max_level = BRIGHTNESS_MAX;
			g_ctrl_obj->last_requested_level = _brightness_get_level();

			_brightness_add(g_ctrl_obj, data);
			_brightness_view_update();
			_brightness_register_event_cb(g_ctrl_obj);

			g_ctrl_obj->brighntess_slider = NULL;

			DBG("brightness controller is created");
		}
	}
}

static void _brightness_destroy(void *data)
{
	if (g_ctrl_obj != NULL) {
		_brightness_deregister_event_cb(g_ctrl_obj);
		_brightness_remove(g_ctrl_obj, data);

		DBG("brightness controller is removed");
	}

	g_ctrl_obj = NULL;
}

static void _handler_brightness(void *data, DBusMessage *msg)
{
	int ret = 0;
	DBusError err;
	char *key = NULL;
	char *value = NULL;
	retif(data == NULL || msg == NULL, , "Invalid parameter!");

	dbus_error_init(&err);
	ret = dbus_message_get_args(msg, &err,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_STRING, &value,
			DBUS_TYPE_INVALID);
	retif(ret == 0, , "dbus_message_get_args error");
	retif(key == NULL, , "Failed to get key");
	retif(value == NULL, , "Failed to get value");

	if (dbus_error_is_set(&err)) {
		ERR("dbus err: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	if (strcmp(key, "visibility") == 0) {
		if (strcmp(value, PREF_BRIGHTNESS_ON) == 0) {
			_brightness_create(data);
			quickpanel_preference_set(PREF_BRIGHTNESS, PREF_BRIGHTNESS_ON);
		} else if (strcmp(value, PREF_BRIGHTNESS_OFF) == 0) {
			_brightness_destroy(data);
			quickpanel_preference_set(PREF_BRIGHTNESS, PREF_BRIGHTNESS_OFF);
		}
	}
}

static void _ipc_init(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");
	retif(ad->dbus_connection == NULL, , "Invalid parameter!");

	g_hdl_brightness =
		e_dbus_signal_handler_add(ad->dbus_connection, NULL,
				QP_DBUS_PATH,
				QP_DBUS_NAME,
				BRIGHTNESS_SIG_ACTIVITY,
				_handler_brightness, data);
	msgif(g_hdl_brightness == NULL, "fail to add size signal");
}

static void _ipc_fini(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");
	retif(ad->dbus_connection == NULL, , "Invalid parameter!");

	if (g_hdl_brightness != NULL) {
		e_dbus_signal_handler_del(ad->dbus_connection, g_hdl_brightness);
		g_hdl_brightness = NULL;
	}
}

static int _init(void *data)
{
	char buf[PREF_LEN_VALUE_MAX] = {0,};
	retif(data == NULL, QP_FAIL, "Invalid parameter!");

#ifdef QP_EMERGENCY_MODE_ENABLE
	if (quickpanel_emergency_mode_is_on()) {
		return QP_OK;
	}
#endif

	quickpanel_preference_get(PREF_BRIGHTNESS, buf);
	_brightness_create(data);

	_ipc_init(data);

	return QP_OK;
}

static int _fini(void *data)
{
	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	_ipc_fini(data);
	_brightness_destroy(data);

	return QP_OK;
}

static void _lang_changed(void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	if (g_ctrl_obj != NULL && g_ctrl_obj->viewer != NULL) {
		_brightness_view_update();
	}
}

static void _qp_opened(void *data)
{
	Evas_Object *slider = NULL;
	Evas_Object *view = _controller_view_get();
	retif(g_ctrl_obj == NULL, , "Invalid parameter!");
	retif(view == NULL, , "Invalid parameter!");

	if (view != NULL) {
		_brightness_view_update();
		slider = elm_object_part_content_get(view, "elm.swallow.slider");
		if (slider != NULL) {
			DBG("quickpanel opened");
			_brightness_ctrl_overheat_check(slider, g_ctrl_obj, 0);
		}
	}
}

static void _qp_closed(void *data)
{
	retif(g_ctrl_obj == NULL, , "Invalid parameter!");

	if (g_ctrl_obj->viewer != NULL) {
		_brightness_view_update();
	}
}

static void _refresh(void *data)
{
	int h = 0;
	struct appdata *ad = data;
	Evas_Object *view = _controller_view_get();
	retif(ad == NULL, , "Invalid parameter!");
	retif(g_ctrl_obj == NULL, , "Invalid parameter!");
	retif(g_ctrl_obj->viewer == NULL, , "Invalid parameter!");
	retif(view == NULL, , "Invalid parameter!");

	evas_object_geometry_get(g_ctrl_obj->viewer, NULL, NULL, NULL, &h);

	if (ad->angle == 90 || ad->angle == 270) {
		evas_object_resize(g_ctrl_obj->viewer, ad->win_height, h);
		evas_object_resize(view, ad->win_height, h);
		_brightness_view_pos_set();

	} else {
		evas_object_resize(g_ctrl_obj->viewer, ad->win_width, h);
		evas_object_resize(view, ad->win_width, h);
		_brightness_view_pos_set();
	}
}
