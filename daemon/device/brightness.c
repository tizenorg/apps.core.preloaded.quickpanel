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

#include <glib.h>
#include <string.h>
#include <vconf.h>
#include <device.h>
#include "common.h"
#include "quickpanel-ui.h"
#include "list_util.h"
#include "quickpanel_def.h"

#define BRIGHTNESS_MIN 1
#define BRIGHTNESS_MAX 100

#define QP_BRIGHTNESS_CONTROL_ICON_IMG ICONDIR"/Q02_Notification_brightness.png"

typedef struct _brightness_ctrl_obj {
	int min_level;
	int max_level;
	Evas_Object *viewer;
	void *data;
} brightness_ctrl_obj;

static int quickpanel_brightness_init(void *data);
static int quickpanel_brightness_fini(void *data);
static void quickpanel_brightness_lang_changed(void *data);
static void quickpanel_brightness_qp_opened(void *data);
static void quickpanel_brightness_qp_closed(void *data);
static void _brightness_view_update(void);
static void _brightness_register_event_cb(brightness_ctrl_obj *ctrl_obj);
static void _brightness_deregister_event_cb(brightness_ctrl_obj *ctrl_obj);

QP_Module brightness_ctrl = {
	.name = "brightness_ctrl",
	.init = quickpanel_brightness_init,
	.fini = quickpanel_brightness_fini,
	.suspend = NULL,
	.resume = NULL,
	.hib_enter = NULL,
	.hib_leave = NULL,
	.lang_changed = quickpanel_brightness_lang_changed,
	.refresh = NULL,
	.get_height = NULL,
	.qp_opened = quickpanel_brightness_qp_opened,
	.qp_closed = quickpanel_brightness_qp_closed,
};

static brightness_ctrl_obj *g_ctrl_obj;

static void _set_text_to_part(Evas_Object *obj, const char *part, const char *text) {
	const char *old_text = NULL;

	retif(obj == NULL, , "Invalid parameter!");
	retif(part == NULL, , "Invalid parameter!");
	retif(text == NULL, , "Invalid parameter!");

	old_text = elm_object_part_text_get(obj, part);
	if (old_text != NULL) {
		if (strcmp(old_text, text) == 0) {
			return ;
		}
	}

	elm_object_part_text_set(obj, part, text);
}

static Evas_Object *_check_duplicated_image_loading(Evas_Object *obj, const char *part, const char *file_path) {
	Evas_Object *old_ic = NULL;
	const char *old_ic_path = NULL;

	retif(obj == NULL, NULL, "Invalid parameter!");
	retif(part == NULL, NULL, "Invalid parameter!");
	retif(file_path == NULL, NULL, "Invalid parameter!");

	old_ic = elm_object_part_content_get(obj, part);
	if (old_ic != NULL) {
		elm_image_file_get(old_ic, &old_ic_path, NULL);
		if (old_ic_path != NULL) {
			if (strcmp(old_ic_path, file_path) == 0)
				return old_ic;
		}

		elm_object_part_content_unset(obj, part);
		evas_object_del(old_ic);
	}

	return NULL;
}

static Evas_Object *_check_duplicated_loading(Evas_Object *obj, const char *part) {
	Evas_Object *old_content = NULL;

	retif(obj == NULL, NULL, "Invalid parameter!");
	retif(part == NULL, NULL, "Invalid parameter!");

	old_content = elm_object_part_content_get(obj, part);
	if (old_content != NULL) {
			return old_content;
	}

	return NULL;
}

static int _brightness_get_automate_level(void) {
	int is_on = 0;

	if (vconf_get_int(VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT, &is_on) == 0)
		return is_on;
	else
		return SETTING_BRIGHTNESS_AUTOMATIC_OFF;
}

static void _brightness_vconf_cb(keynode_t *key, void* data) {
	brightness_ctrl_obj *ctrl_obj = NULL;

	retif(data == NULL, , "Data parameter is NULL");
	ctrl_obj = data;

	if (ctrl_obj->viewer != NULL) {
		_brightness_view_update();
	}
}

static void quickpanel_brightness_qp_opened(void *data)
{
	struct appdata *ad = NULL;

	retif(data == NULL, , "Invalid parameter!");
	ad = data;

	retif(g_ctrl_obj == NULL, , "Invalid parameter!");

	if (g_ctrl_obj->viewer != NULL) {
		_brightness_view_update();
	}
}

static void quickpanel_brightness_qp_closed(void *data)
{
	struct appdata *ad = NULL;

	retif(data == NULL, , "Invalid parameter!");
	ad = data;

	retif(g_ctrl_obj == NULL, , "Invalid parameter!");

	if (g_ctrl_obj->viewer != NULL) {
		_brightness_view_update();
	}
}

static int _brightness_set_level(int level) {
	int ret = DEVICE_ERROR_NONE;

	ret = device_set_brightness_to_settings(0, level);
	if (ret != DEVICE_ERROR_NONE) {
		ERR("failed to set brightness");
	}

	return level;
}

static int _brightness_get_level(void) {
	int level = 0;

	if (vconf_get_int(VCONFKEY_SETAPPL_LCD_BRIGHTNESS, &level) == 0)
		return level;
	else
		return SETTING_BRIGHTNESS_LEVEL5;
}

static int _brightness_set_automate_level(int is_on) {
	return vconf_set_int(VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT, is_on);
}

static void
_brightness_ctrl_slider_changed_cb(void *data,
							 Evas_Object *obj,
							 void *event_info)
{
	int value = 0;
	static int old_val = -1;
	brightness_ctrl_obj *ctrl_obj = NULL;

	retif(data == NULL, , "Data parameter is NULL");
	ctrl_obj = data;

	double val = elm_slider_value_get(obj);
	value = (int)(val + 0.5);

	if (value != old_val)
	{
		if (value >= ctrl_obj->min_level && value <= ctrl_obj->max_level) {
			DBG("brightness is changed to %d", value);
			_brightness_set_level(value);
		}
	}
}

static void
_brightness_ctrl_slider_delayed_changed_cb(void *data,
							 Evas_Object *obj,
							 void *event_info)
{
	int value = 0;

	value = _brightness_get_level();
	DBG("brightness is changed to %d", value);
	_brightness_set_level(value);
}

static void
_brightness_ctrl_slider_drag_start_cb(void *data,
							 Evas_Object *obj,
							 void *event_info)
{
	_brightness_deregister_event_cb(data);
}

static void
_brightness_ctrl_slider_drag_stop_cb(void *data,
							 Evas_Object *obj,
							 void *event_info)
{
	_brightness_register_event_cb(data);
}

static void _brightness_ctrl_checker_toggle_cb(void *data,
								Evas_Object * obj,
								void *event_info)
{
	quickpanel_play_feedback();

	retif(obj == NULL, , "obj parameter is NULL");

	int status = elm_check_state_get(obj);
	brightness_ctrl_obj *ctrl_obj = NULL;

	retif(data == NULL, , "Data parameter is NULL");
	ctrl_obj = data;

	_brightness_set_automate_level(status);

	if (ctrl_obj->viewer != NULL) {
		_brightness_view_update();
	}
}

static Evas_Object *_brightness_view_create(Evas_Object *list)
{
	Eina_Bool ret = EINA_TRUE;
	Evas_Object *view = NULL;

	retif(list == NULL, NULL, "list parameter is NULL");

	view = elm_layout_add(list);

	if (view != NULL) {
		ret = elm_layout_file_set(view, DEFAULT_EDJ,
				"quickpanel/brightness_controller/default");

		if (ret == EINA_FALSE) {
			ERR("failed to load brightness layout");
		}
		evas_object_size_hint_weight_set(view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(view, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_show(view);
	}

	return view;
}

static void _brightness_set_text(void)
{
	brightness_ctrl_obj *ctrl_obj = g_ctrl_obj;
	retif(ctrl_obj == NULL, , "Invalid parameter!");
	retif(ctrl_obj->viewer == NULL, , "Invalid parameter!");

	_set_text_to_part(ctrl_obj->viewer, "elm.check.text", _("IDS_QP_BODY_AUTO"));
	_set_text_to_part(ctrl_obj->viewer, "elm.text.label", _S("IDS_COM_OPT_BRIGHTNESS"));
}

static void _brightness_set_image(void)
{
	Evas_Object *ic = NULL;
	Evas_Object *old_ic = NULL;
	brightness_ctrl_obj *ctrl_obj = g_ctrl_obj;
	retif(ctrl_obj == NULL, , "Invalid parameter!");
	retif(ctrl_obj->viewer == NULL, , "Invalid parameter!");

	old_ic = _check_duplicated_image_loading(ctrl_obj->viewer,
			"elm.swallow.thumbnail", QP_BRIGHTNESS_CONTROL_ICON_IMG);

	if (old_ic == NULL) {
		ic = elm_image_add(ctrl_obj->viewer);

		if (ic != NULL) {
			elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
			elm_image_file_set(ic, QP_BRIGHTNESS_CONTROL_ICON_IMG, "elm.swallow.thumbnail");
			elm_object_part_content_set(ctrl_obj->viewer, "elm.swallow.thumbnail", ic);
		}
	}
}

static void _brightness_set_checker(void)
{
	Evas_Object *checker = NULL;
	Evas_Object *old_obj = NULL;
	brightness_ctrl_obj *ctrl_obj = g_ctrl_obj;
	retif(ctrl_obj == NULL, , "Invalid parameter!");
	retif(ctrl_obj->viewer == NULL, , "Invalid parameter!");

	old_obj = _check_duplicated_loading(ctrl_obj->viewer,
			"elm.check.swallow");

	if (old_obj == NULL) {
		checker = elm_check_add(ctrl_obj->viewer);

		if (checker != NULL) {
			elm_object_style_set(checker, "quickpanel");
			evas_object_smart_callback_add(checker,"changed",_brightness_ctrl_checker_toggle_cb , ctrl_obj);
			elm_object_part_content_set(ctrl_obj->viewer, "elm.check.swallow", checker);
		} else {
			ERR("failed to create checker");
			return ;
		}
	} else {
		checker = old_obj;
	}

	elm_check_state_set(checker, _brightness_get_automate_level());
}

static void _brightness_set_slider(void)
{
	int value = 0;
	Evas_Object *slider = NULL;
	Evas_Object *old_obj = NULL;
	brightness_ctrl_obj *ctrl_obj = g_ctrl_obj;
	retif(ctrl_obj == NULL, , "Invalid parameter!");
	retif(ctrl_obj->viewer == NULL, , "Invalid parameter!");

	old_obj = _check_duplicated_loading(ctrl_obj->viewer,
			"elm.swallow.slider");

	if (old_obj == NULL) {
		slider = elm_slider_add(ctrl_obj->viewer);

		if (slider != NULL) {
			elm_object_style_set(slider, "quickpanel");

			evas_object_size_hint_weight_set(slider, EVAS_HINT_EXPAND, 0.0);
			evas_object_size_hint_align_set(slider, EVAS_HINT_FILL, 0.5);
			elm_slider_min_max_set(slider, ctrl_obj->min_level, ctrl_obj->max_level);
			evas_object_smart_callback_add(slider, "changed",
					_brightness_ctrl_slider_changed_cb, ctrl_obj);
			evas_object_smart_callback_add(slider, "delay,changed",
					_brightness_ctrl_slider_delayed_changed_cb, ctrl_obj);
			evas_object_smart_callback_add(slider, "slider,drag,start",
					_brightness_ctrl_slider_drag_start_cb, ctrl_obj);
			evas_object_smart_callback_add(slider, "slider,drag,stop",
					_brightness_ctrl_slider_drag_stop_cb, ctrl_obj);
			elm_object_part_content_set(ctrl_obj->viewer, "elm.swallow.slider", slider);
		} else {
			ERR("failed to create slider");
			return ;
		}
	} else {
		slider = old_obj;
	}

	value = _brightness_get_level();

	elm_slider_value_set(slider, value);

	if (_brightness_get_automate_level()) {
		elm_object_disabled_set(slider, EINA_TRUE);
	} else {
		elm_object_disabled_set(slider, EINA_FALSE);
	}
}

static void _brightness_view_update(void)
{
	_brightness_set_text();
	_brightness_set_image();
	_brightness_set_checker();
	_brightness_set_slider();
}

static void _brightness_add(brightness_ctrl_obj *ctrl_obj, void *data)
{
	qp_item_data *qid = NULL;
	struct appdata *ad;

	ad = data;
	retif(!ad->list, , "list is NULL");

	ctrl_obj->viewer = _brightness_view_create(ad->list);
	ctrl_obj->data = data;

	if (ctrl_obj->viewer != NULL) {
		qid = quickpanel_list_util_item_new(QP_ITEM_TYPE_BRIGHTNESS, ctrl_obj);
		if (qid != NULL) {
			quickpanel_list_util_item_set_tag(ctrl_obj->viewer, qid);
			quickpanel_list_util_sort_insert(ad->list, ctrl_obj->viewer);
		} else {
			ERR("fail to alloc vit");
		}
	} else {
		ERR("failed to create brightness view");
	}
}

static void _brightness_remove(brightness_ctrl_obj *ctrl_obj, void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "list is NULL");
	retif(ad->list == NULL, , "list is NULL");

	if (g_ctrl_obj != NULL) {
		if (g_ctrl_obj->viewer != NULL) {
			quickpanel_list_util_item_unpack_by_object(ad->list
					, g_ctrl_obj->viewer);
			quickpanel_list_util_item_del_tag(g_ctrl_obj->viewer);
			evas_object_del(g_ctrl_obj->viewer);
		}
		INFO("success to remove brightness controller");
		free(g_ctrl_obj);
		g_ctrl_obj = NULL;
	}
}

static void _brightness_register_event_cb(brightness_ctrl_obj *ctrl_obj)
{
	int ret = 0;
	retif(ctrl_obj == NULL, , "Data parameter is NULL");

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_LCD_BRIGHTNESS,
			_brightness_vconf_cb, ctrl_obj);
	if (ret != 0) {
		ERR("failed to register a cb key:%s err:%d",
				"VCONFKEY_SETAPPL_LCD_BRIGHTNESS", ret);
	}
}

static void _brightness_deregister_event_cb(brightness_ctrl_obj *ctrl_obj)
{
	int ret = 0;

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_LCD_BRIGHTNESS, _brightness_vconf_cb);
	if (ret != 0) {
		ERR("failed to register a cb key:%s err:%d", "VCONFKEY_SETAPPL_LCD_BRIGHTNESS", ret);
	}
}

static int quickpanel_brightness_init(void *data)
{
	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	if (g_ctrl_obj == NULL) {
		g_ctrl_obj = (brightness_ctrl_obj *)malloc(sizeof(brightness_ctrl_obj));
	}

	if (g_ctrl_obj != NULL) {
		g_ctrl_obj->min_level = BRIGHTNESS_MIN;
		g_ctrl_obj->max_level = BRIGHTNESS_MAX;

		DBG("brightness range %d~%d\n", g_ctrl_obj->min_level, g_ctrl_obj->max_level);

		_brightness_add(g_ctrl_obj, data);
		_brightness_view_update();
		_brightness_register_event_cb(g_ctrl_obj);
	}

	return QP_OK;
}

static int quickpanel_brightness_fini(void *data)
{
	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	if (g_ctrl_obj != NULL) {
		_brightness_deregister_event_cb(g_ctrl_obj);
		_brightness_remove(g_ctrl_obj, data);
	}

	g_ctrl_obj = NULL;
	return QP_OK;
}

static void quickpanel_brightness_lang_changed(void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	if (g_ctrl_obj != NULL && g_ctrl_obj->viewer != NULL) {
		_brightness_view_update();
	}
}
