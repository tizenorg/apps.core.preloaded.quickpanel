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

#include <glib.h>
#include <string.h>
#include <Elementary.h>

#include <app.h>
#include <vconf.h>
#include <notification.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <sound_manager.h>
#include <E_DBus.h>

#include "common.h"
#include "common_uic.h"
#include "quickpanel-ui.h"
#include "list_util.h"
#include "quickpanel_def.h"
#include "modules.h"
#include "util-time.h"
#include "media.h"

#ifdef QP_SCREENREADER_ENABLE
#include "accessibility.h"
#endif

#ifdef QP_EMERGENCY_MODE_ENABLE
#include "emergency_mode.h"
#endif

static int _init(void *data);
static int _fini(void *data);

#define PKG_SETTING_EDIT "quickpanel-setting-efl"
#define QP_TIMEDATE_SETTING_UG "setting-time-efl"
#define E_DATA_EDITING_VISIBILITT "editing_visible"
#define E_DATA_TIME_N_DATE_EVENT	"time_n_date_event"

QP_Module qp_datetime_view = {
	.name = "qp_datetime_view",
	.init = _init,
	.fini = _fini,
	.suspend = NULL,
	.resume = NULL,
	.lang_changed = NULL,
	.refresh = NULL,
};

static Evas_Object *_datetime_view_get(void);

static void _flag_set(Evas_Object *container, const char *key, int value)
{
	retif(container == NULL, , "invalid parameter");
	retif(key == NULL, , "invalid parameter");

	evas_object_data_set(container, key, (void *)(long)(value));
}

static int _flag_get(Evas_Object *container, const char *key)
{
	retif(container == NULL, 0, "invalid parameter");
	retif(key == NULL, 0, "invalid parameter");

	return (int)(long)evas_object_data_get(container, key);
}

static void _set_text_to_part(Evas_Object *obj, const char *part, const char *text)
{
	const char *old_text = NULL;

	retif(obj == NULL, , "Invalid parameter!");
	retif(part == NULL, , "Invalid parameter!");
	retif(text == NULL, , "Invalid parameter!");

	old_text = elm_object_part_text_get(obj, part);
	if (old_text != NULL) {
		if (strcmp(old_text, text) == 0) {
			return;
		}
	}

	elm_object_part_text_set(obj, part, text);
}

static void _text_time_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *view = _datetime_view_get();
	int ret;

	if (view) {
		if (_flag_get(view, E_DATA_TIME_N_DATE_EVENT) == 0) {
			DBG("Time & date area click is event disabled");
			return;
		}
	}

	quickpanel_media_play_feedback();

	ret = quickpanel_uic_launch_ug_by_appcontrol(QP_TIMEDATE_SETTING_UG, NULL);
	quickpanel_uic_launch_app_inform_result(QP_TIMEDATE_SETTING_UG, ret);

	quickpanel_uic_close_quickpanel(true, 1);
}

static void _button_setting_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	quickpanel_media_play_feedback();

#ifdef QP_EMERGENCY_MODE_ENABLE
	if (quickpanel_emergency_mode_is_on()) {
		quickpanel_uic_launch_app(PACKAGE_EMERGENCY_MODE_SETTING, NULL);
	} else {
		quickpanel_uic_launch_app(QP_SETTING_PKG_SETTING, NULL);
	}
#else
	quickpanel_uic_launch_app(QP_SETTING_PKG_SETTING, NULL);
#endif
	quickpanel_uic_close_quickpanel(true, 1);
}

static Evas_Object *_datetime_view_create(Evas_Object *parent)
{
	Evas_Object *focus = NULL;
	Eina_Bool ret = EINA_TRUE;
	Evas_Object *view = NULL;

	retif(parent == NULL, NULL, "Invalid parameter!");

	view = elm_layout_add(parent);

	if (view != NULL) {
		ret = elm_layout_file_set(view, DEFAULT_EDJ,
				"quickpanel/datetime");
		if (ret == EINA_FALSE) {
			ERR("failed to load quickpanel/datetime layout");
		}
		evas_object_size_hint_weight_set(view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(view, EVAS_HINT_FILL, EVAS_HINT_FILL);
		quickpanel_uic_initial_resize(view, QP_DATE_H);

		focus = quickpanel_accessibility_ui_get_focus_object(view);
		elm_object_part_content_set(view, "focus.datetime", focus);
		evas_object_smart_callback_add(focus, "clicked", _text_time_clicked_cb, view);

		focus = quickpanel_accessibility_ui_get_focus_object(view);
		elm_object_part_content_set(view, "focus.setting", focus);
		evas_object_smart_callback_add(focus, "clicked", _button_setting_clicked_cb, view);

		_flag_set(view, E_DATA_EDITING_VISIBILITT, 0);

#ifdef QP_EMERGENCY_MODE_ENABLE
		if (quickpanel_emergency_mode_is_on()) {
			_flag_set(view, E_DATA_TIME_N_DATE_EVENT, 0);
			elm_object_signal_emit(view, "timendate.click.disable", "prog");
		} else {
#endif
			_flag_set(view, E_DATA_TIME_N_DATE_EVENT, 1);
			elm_object_signal_emit(view, "timendate.click.enable", "prog");
#ifdef QP_EMERGENCY_MODE_ENABLE
		}
#endif

		evas_object_show(view);
	}

	return view;
}

static Evas_Object *_datetime_view_get(void) {
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, NULL, "invalid argument");
	retif(ad->view_root == NULL, NULL, "invalid argument");

	return elm_object_part_content_get(ad->view_root
			, "qp.base.datetime.swallow");
}

static void _datetime_view_attach(void *data)
{
	Evas_Object *view = NULL;
	struct appdata *ad = data;
	retif(ad == NULL, ,"invalid parameter");
	retif(ad->view_root == NULL, ,"invalid parameter");

	view = _datetime_view_create(ad->view_root);
	if (view != NULL) {
		elm_object_part_content_set(ad->view_root, "qp.base.datetime.swallow", view);
	}
}

static void _datetime_view_deattach(void *data)
{
	Evas_Object *view = NULL;
	struct appdata *ad = data;
	retif(ad == NULL, ,"invalid parameter");
	retif(ad->view_root == NULL, ,"invalid parameter");

	view = elm_object_part_content_unset(ad->view_root, "qp.base.datetime.swallow");
	if (view != NULL) {
		evas_object_del(view);
		view = NULL;
	}
}

static int _init(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL,"invalid parameter");

	_datetime_view_attach(ad);

	return QP_OK;
}

static int _fini(void *data)
{
	_datetime_view_deattach(data);

	return QP_OK;
}

HAPI void quickpanel_datetime_datentime_event_set(int is_clickable)
{
	Evas_Object *view = _datetime_view_get();

	DBG("date n time clickable set[%d]", is_clickable);

	if (view != NULL) {
		if (is_clickable == 1) {
			if (_flag_get(view, E_DATA_TIME_N_DATE_EVENT) == 0) {
				_flag_set(view, E_DATA_TIME_N_DATE_EVENT, 1);
				elm_object_signal_emit(view, "timendate.click.enable", "prog");
			}
		} else {
			if (_flag_get(view, E_DATA_TIME_N_DATE_EVENT) == 1) {
				_flag_set(view, E_DATA_TIME_N_DATE_EVENT, 0);
				elm_object_signal_emit(view, "timendate.click.disable", "prog");
			}
		}
	}
}

HAPI void quickpanel_datetime_editing_icon_visibility_set(int is_visible)
{
	Evas_Object *view = _datetime_view_get();

	DBG("visibility set:%d", is_visible);

	if (view != NULL) {
		if (is_visible == 1) {
			if (_flag_get(view, E_DATA_EDITING_VISIBILITT) == 0) {
				_flag_set(view, E_DATA_EDITING_VISIBILITT, 1);
				elm_object_signal_emit(view, "button,editing,show", "prog");
			}
		} else {
			if (_flag_get(view, E_DATA_EDITING_VISIBILITT) == 1) {
				_flag_set(view, E_DATA_EDITING_VISIBILITT, 0);
				elm_object_signal_emit(view, "button,editing,hide", "prog");
			}
		}
	}
}

HAPI void quickpanel_datetime_view_update(char *date, char *time, char *meridiem, int meridiem_type)
{
	Evas_Object *view = NULL;

	Eina_Strbuf *strbuf_date = NULL;
	Eina_Strbuf *strbuf_time = NULL;
	Eina_Strbuf *strbuf_access = NULL;

	view = _datetime_view_get();

	if (!view) {
		ERR("view == NULL");
		return;
	}

	strbuf_date = eina_strbuf_new();
	if(!strbuf_date) {
		ERR("strbuf_date == NULL");
		return;
	}

	strbuf_time = eina_strbuf_new();
	if(!strbuf_time) {
		ERR("strbuf_time == NULL");
		eina_strbuf_free(strbuf_date);
		return;
	}

	strbuf_access = eina_strbuf_new();
	if(!strbuf_access) {
		ERR("strbuf_access == NULL");
		eina_strbuf_free(strbuf_date);
		eina_strbuf_free(strbuf_time);
		return;
	}


	DBG("update time: %s %s %s", date, time, meridiem);

	if (date != NULL) {
		eina_strbuf_append_printf(strbuf_date, "%s", date);
		eina_strbuf_append_printf(strbuf_access, "%s ", date);
	}

	eina_strbuf_ltrim(strbuf_date);

	// -------------------------------------------------------------------------------------

	if (meridiem_type == UTIL_TIME_MERIDIEM_TYPE_PRE && meridiem != NULL && strlen(meridiem) != 0) {
		eina_strbuf_append_printf(strbuf_time, "<ampm>%s</> ", meridiem);
		eina_strbuf_append_printf(strbuf_access, "%s ", meridiem);
	}

	if (time != NULL) {
		eina_strbuf_append_printf(strbuf_time, "<time>%s</>", time);
		eina_strbuf_append_printf(strbuf_access, "%s ", time);
	}

	if (meridiem_type == UTIL_TIME_MERIDIEM_TYPE_POST && meridiem != NULL && strlen(meridiem) != 0) {
		eina_strbuf_append_printf(strbuf_time, " <ampm>%s</>", meridiem);
		eina_strbuf_append_printf(strbuf_access, "%s ", meridiem);
	}

	eina_strbuf_ltrim(strbuf_time);

	// -------------------------------------------------------------------------------------

	LOGI("DATE STR SET: %s", eina_strbuf_string_get(strbuf_time));

	_set_text_to_part(view, "text.date", eina_strbuf_string_get(strbuf_date));
	_set_text_to_part(view, "text.time", eina_strbuf_string_get(strbuf_time));

	quickpanel_accessibility_screen_reader_data_set(view, "focus.datetime", "", (char *)eina_strbuf_string_get(strbuf_access));

	eina_strbuf_free(strbuf_date);
	eina_strbuf_free(strbuf_time);
	eina_strbuf_free(strbuf_access);

	quickpanel_accessibility_screen_reader_data_set(view
			, "focus.setting", "", _NOT_LOCALIZED("Settings"));

}
