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
#include "quickpanel_theme_def.h"

#define BRIGHTNESS_MIN 1
#define BRIGHTNESS_MAX 100

#define QP_BRIGHTNESS_CONTROL_ICON_IMG ICONDIR"/Q02_Notification_brightness.png"

static int quickpanel_brightness_init(void *data);
static int quickpanel_brightness_fini(void *data);
static void quickpanel_brightness_lang_changed(void *data);
static unsigned int quickpanel_brightness_get_height(void *data);
static void quickpanel_brightness_qp_opened(void *data);
static void quickpanel_brightness_qp_closed(void *data);

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
	.get_height = quickpanel_brightness_get_height,
	.qp_opened = quickpanel_brightness_qp_opened,
	.qp_closed = quickpanel_brightness_qp_closed,
};

typedef struct _brightness_ctrl_obj {
	int min_level;
	int max_level;
	Evas_Object *checker;
	Evas_Object *slider;
	Elm_Object_Item *it;
	void *data;
} brightness_ctrl_obj;

brightness_ctrl_obj *g_ctrl_obj;

static void _brightness_vconf_cb(keynode_t *key, void* data) {
	brightness_ctrl_obj *ctrl_obj = NULL;

	retif(data == NULL, , "Data parameter is NULL");
	ctrl_obj = data;

	if (ctrl_obj->it != NULL) {
		elm_genlist_item_fields_update(ctrl_obj->it, "elm.check.swallow", ELM_GENLIST_ITEM_FIELD_CONTENT);
		elm_genlist_item_fields_update(g_ctrl_obj->it, "elm.swallow.slider", ELM_GENLIST_ITEM_FIELD_CONTENT);
	}
}

static void quickpanel_brightness_qp_opened(void *data)
{
	struct appdata *ad = NULL;

	retif(data == NULL, , "Invalid parameter!");
	ad = data;

	retif(g_ctrl_obj == NULL, , "Invalid parameter!");

	if (g_ctrl_obj->it != NULL) {
		elm_genlist_item_fields_update(g_ctrl_obj->it, "elm.swallow.slider", ELM_GENLIST_ITEM_FIELD_CONTENT);
		elm_genlist_item_fields_update(g_ctrl_obj->it, "elm.check.swallow", ELM_GENLIST_ITEM_FIELD_CONTENT);
		vconf_ignore_key_changed(VCONFKEY_SETAPPL_LCD_BRIGHTNESS, _brightness_vconf_cb);
	}
}

static void quickpanel_brightness_qp_closed(void *data)
{
	struct appdata *ad = NULL;

	retif(data == NULL, , "Invalid parameter!");
	ad = data;

	retif(g_ctrl_obj == NULL, , "Invalid parameter!");

	if (g_ctrl_obj->it != NULL) {
		vconf_notify_key_changed(VCONFKEY_SETAPPL_LCD_BRIGHTNESS, _brightness_vconf_cb, g_ctrl_obj);
	}
}

int _brightness_is_low_battery(void) {
	int battery_value;

	vconf_get_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, &battery_value);

	if (battery_value < VCONFKEY_SYSMAN_BAT_WARNING_LOW)
		return 1;
	else
		return 0;
}

int _brightness_set_level(int level) {
	int ret = DEVICE_ERROR_NONE;

	ret = device_set_brightness_to_settings(0, level);
	if (ret != DEVICE_ERROR_NONE) {
		ERR("failed to set brightness");
	}

	return level;
}

int _brightness_get_level(void) {
	int level = 0;

	if (vconf_get_int(VCONFKEY_SETAPPL_LCD_BRIGHTNESS, &level) == 0)
		return level;
	else
		return SETTING_BRIGHTNESS_LEVEL5;
}

int _brightness_set_automate_level(int is_on) {
	return vconf_set_int(VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT, is_on);
}

int _brightness_get_automate_level(void) {
	int is_on = 0;

	if (vconf_get_int(VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT, &is_on) == 0)
		return is_on;
	else
		return SETTING_BRIGHTNESS_AUTOMATIC_OFF;
}

static void
_brightness_ctrl_slider_changed_cb(void *data,
							 Evas_Object *obj,
							 void *event_info)
{
	int ret = DEVICE_ERROR_NONE;
	int value = 0;
	static int old_val = -1;
	brightness_ctrl_obj *ctrl_obj = NULL;

	DBG("");

	retif(data == NULL, , "Data parameter is NULL");
	ctrl_obj = data;

	if (ctrl_obj->checker != NULL) {
		value = elm_check_state_get(ctrl_obj->checker);
		if (value == 1) {	/* it is automatic setting mode, */
			/* do nothing */
			return;
		}
	}

	double val = elm_slider_value_get(obj);
	value = (int)(val + 0.5);

	DBG("value:%d old_val:%d", value, old_val);

	if (value != old_val)
	{
		DBG("min_level:%d max_level:%d", ctrl_obj->min_level, ctrl_obj->max_level);
		if (value >= ctrl_obj->min_level && value <= ctrl_obj->max_level) {
			DBG("new brgt:%d", value);
			ret = device_set_brightness_to_settings(0, value);
			old_val = value;
			if (ret != DEVICE_ERROR_NONE) {
				ERR("failed to set brightness");
			}
		}
	}
}

static void
_brightness_ctrl_slider_delayed_changed_cb(void *data,
							 Evas_Object *obj,
							 void *event_info)
{
	int ret = DEVICE_ERROR_NONE;
	int value = 0;

	ret = device_get_brightness(0, &value);

	DBG("ret:%d value:%d", ret, value);
	if (ret == DEVICE_ERROR_NONE) {
		DBG("delayed new brgt:%d", value);
		_brightness_set_level(value);
	}
}

static void _brightness_ctrl_checker_toggle_cb(void *data,
								Evas_Object * obj,
								void *event_info)
{
	retif(obj == NULL, , "obj parameter is NULL");

	int status = elm_check_state_get(obj);
	brightness_ctrl_obj *ctrl_obj = NULL;

	retif(data == NULL, , "Data parameter is NULL");
	ctrl_obj = data;

	_brightness_set_automate_level(status);

	if (ctrl_obj->it != NULL) {
		elm_genlist_item_fields_update(ctrl_obj->it, "elm.swallow.slider", ELM_GENLIST_ITEM_FIELD_CONTENT);
		elm_genlist_item_fields_update(ctrl_obj->it, "elm.check.swallow", ELM_GENLIST_ITEM_FIELD_CONTENT);
	}
}

static char *_quickpanel_noti_gl_get_text(void *data, Evas_Object * obj,
					   const char *part)
{
	if (strcmp(part, "elm.check.text") == 0) {
		return strdup(_("IDS_QP_BODY_AUTO"));
	}
	if (strcmp(part, "elm.text.label") == 0) {
		return strdup(_S("IDS_COM_OPT_BRIGHTNESS"));
	}
	return NULL;
}

static Evas_Object *_brightness_gl_get_content(void *data, Evas_Object * obj,
					const char *part)
{
	qp_item_data *qid = NULL;
	brightness_ctrl_obj *ctrl_obj = NULL;

	retif(data == NULL, NULL, "Invalid parameter!");
	qid = data;

	ctrl_obj = quickpanel_list_util_item_get_data(qid);
	retif(!ctrl_obj, NULL, "item is NULL");

	if (strcmp(part, "elm.swallow.thumbnail") == 0) {
		Evas_Object *image = elm_image_add(obj);

		if (image != NULL) {
			elm_image_file_set(image, QP_BRIGHTNESS_CONTROL_ICON_IMG, NULL);
			elm_image_resizable_set(image, EINA_FALSE, EINA_TRUE);
		}

		return image;
	} else if (strcmp(part, "elm.swallow.slider") == 0) {
		int value = _brightness_get_level();
		Evas_Object *slider = elm_slider_add(obj);

		if (slider != NULL) {
			ctrl_obj->slider = NULL;
			elm_object_style_set(slider, "quickpanel");
			elm_slider_indicator_format_set(slider, "%1.0f");

			evas_object_size_hint_weight_set(slider, EVAS_HINT_EXPAND, 0.0);
			evas_object_size_hint_align_set(slider, EVAS_HINT_FILL, 0.5);
			elm_slider_min_max_set(slider, ctrl_obj->min_level, ctrl_obj->max_level);
			elm_slider_value_set(slider, value);
			evas_object_smart_callback_add(slider, "changed", _brightness_ctrl_slider_changed_cb, ctrl_obj);
			evas_object_smart_callback_add(slider, "delay,changed", _brightness_ctrl_slider_delayed_changed_cb, ctrl_obj);

			if (_brightness_get_automate_level() || _brightness_is_low_battery() == 1) {
				elm_object_disabled_set(slider, EINA_TRUE);
			} else {
				elm_object_disabled_set(slider, EINA_FALSE);
			}

			ctrl_obj->slider = slider;
			return slider;
		}
	} else if (strcmp(part, "elm.check.swallow") == 0) {
		Evas_Object *checker = elm_check_add(obj);

		if (checker != NULL) {
			ctrl_obj->checker = NULL;

			elm_object_style_set(checker, "quickpanel");
			elm_check_state_set(checker, _brightness_get_automate_level());
			evas_object_smart_callback_add(checker,"changed",_brightness_ctrl_checker_toggle_cb , ctrl_obj);

			if (_brightness_is_low_battery() == 1) {
				elm_object_disabled_set(checker, EINA_TRUE);
			} else {
				elm_object_disabled_set(checker, EINA_FALSE);
			}
			ctrl_obj->checker = checker;

			return checker;
		}
	}

	return NULL;
}

static Eina_Bool _brightness_gl_get_state(void *data, Evas_Object *obj,
					const char *part)
{
	return EINA_FALSE;
}

static void _brightness_gl_del(void *data, Evas_Object *obj)
{
	if (data) {
		quickpanel_list_util_del_count(data);
		free(data);
	}

	return;
}

static Elm_Genlist_Item_Class *_brightness_gl_style_get(void)
{
	Elm_Genlist_Item_Class *itc = NULL;

	itc = elm_genlist_item_class_new();
	if (!itc) {
		ERR("fail to elm_genlist_item_class_new()");
		return NULL;
	}

	itc->item_style = "brightness_controller/default";
	itc->func.text_get = _quickpanel_noti_gl_get_text;
	itc->func.content_get = _brightness_gl_get_content;
	itc->func.state_get = _brightness_gl_get_state;
	itc->func.del = _brightness_gl_del;

	return itc;
}

static void _brightness_add(brightness_ctrl_obj *ctrl_obj, void *data)
{
	qp_item_data *qid = NULL;
	Elm_Genlist_Item_Class *itc = NULL;
	qp_item_type_e type = QP_ITEM_TYPE_BRIGHTNESS;
	struct appdata *ad;

	ad = data;
	retif(!ad->list, , "list is NULL");

	itc = _brightness_gl_style_get();
	if (!itc) {
		ERR("fail to _brightness_gl_style_get()");
		return;
	}

	qid = quickpanel_list_util_item_new(type, ctrl_obj);
	if (!qid) {
		ERR("fail to alloc vit");
		elm_genlist_item_class_free(itc);
		free(g_ctrl_obj);
		return;
	}
	ctrl_obj->data = data;
	ctrl_obj->it = quickpanel_list_util_sort_insert(ad->list, itc, qid, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);

	elm_genlist_item_class_free(itc);

	quickpanel_ui_update_height(ad);
}

static void _brightness_remove(brightness_ctrl_obj *ctrl_obj, void *data)
{
	if (g_ctrl_obj != NULL) {
		INFO("success to remove brightness controller");
		free(g_ctrl_obj);
		g_ctrl_obj = NULL;
		retif(data == NULL, , "data is NULL");
		quickpanel_ui_update_height(data);
	}
}

static void _brightness_register_event_cb(brightness_ctrl_obj *ctrl_obj)
{
	retif(ctrl_obj == NULL, , "Invalid parameter!");

	vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, _brightness_vconf_cb, ctrl_obj);
	vconf_notify_key_changed(VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT, _brightness_vconf_cb, ctrl_obj);
}

static void _brightness_deregister_event_cb(brightness_ctrl_obj *ctrl_obj)
{
	retif(ctrl_obj == NULL, , "Invalid parameter!");

	vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, _brightness_vconf_cb);
	vconf_ignore_key_changed(VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT, _brightness_vconf_cb);
}

static int quickpanel_brightness_init(void *data)
{
	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	if (g_ctrl_obj == NULL) {
		DBG("brightness controller alloced");
		g_ctrl_obj = (brightness_ctrl_obj *)malloc(sizeof(brightness_ctrl_obj));
	}

	DBG("");

	if (g_ctrl_obj != NULL) {
		g_ctrl_obj->min_level = BRIGHTNESS_MIN;
		g_ctrl_obj->max_level = BRIGHTNESS_MAX;

		DBG("brightness range %d~%d\n", g_ctrl_obj->min_level, g_ctrl_obj->max_level);

		_brightness_add(g_ctrl_obj, data);
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

	_brightness_remove(g_ctrl_obj, data);

	g_ctrl_obj = NULL;
	return QP_OK;
}

static void quickpanel_brightness_lang_changed(void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	if (g_ctrl_obj != NULL && g_ctrl_obj->it != NULL) {
		elm_genlist_item_fields_update(g_ctrl_obj->it, "elm.check.text", ELM_GENLIST_ITEM_FIELD_TEXT);
		elm_genlist_item_fields_update(g_ctrl_obj->it, "elm.text.label", ELM_GENLIST_ITEM_FIELD_TEXT);
	}
}

static unsigned int quickpanel_brightness_get_height(void *data)
{
	struct appdata *ad = data;

	retif(ad == NULL, 0, "Invalid parameter!");

	if (g_ctrl_obj != NULL)
		return QP_THEME_LIST_ITEM_BRIGHTNESS_HEIGHT * ad->scale;
	else
		return 0;
}
