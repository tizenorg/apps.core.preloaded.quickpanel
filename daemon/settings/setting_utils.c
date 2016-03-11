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

#include <notification.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "settings.h"
#include "common.h"
#include "quickpanel_def.h"
#include "quickpanel-ui.h"
#include "setting_utils.h"
#ifdef QP_SCREENREADER_ENABLE
#include "accessibility.h"
#endif

#define DEFAULT_EDJ		EDJDIR"/"PACKAGE".edj"
#define _EDJ(o) elm_layout_edje_get(o)

#define TEXT_LEN 128
#define QP_SETTING_INITIAL_PAGE_NUM 0
#define DIVIDER_MAGIC 0xCAFECAFE
#define E_DATA_DIVIDER_MAGIC "divider_magic"

static inline void __escaped_text_set(Evas_Object *obj, const char *part, const char *text)
{
	char buf[256] = {0,};
	char *ecaped = NULL;

	if (!obj) {
		return;
	}

	if (!part) {
		return;
	}

	strncpy(buf, text, sizeof(buf) - 1);
	quickpanel_common_util_char_trim(buf);
	ecaped = evas_textblock_text_utf8_to_markup(NULL, buf);

	elm_object_part_text_set(obj, part, ecaped);

	if (ecaped) {
		free(ecaped);
	}
}

HAPI int quickpanel_setting_icon_text_set(Evas_Object *icon, const char *text, int state)
{
	retif(icon == NULL, QP_FAIL, "invalid parameter");
	retif(text == NULL, QP_FAIL, "invalid parameter");

	__escaped_text_set(icon, "icon.text", text);

#ifdef QP_SCREENREADER_ENABLE
	char buf[256] = {0,};
	Evas_Object *ao = NULL;

	ao = quickpanel_accessibility_screen_reader_object_get(icon,
			SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", icon);
	if (ao != NULL) {
		elm_access_info_set(ao, ELM_ACCESS_TYPE, _NOT_LOCALIZED("Button"));
		strncpy(buf, text, sizeof(buf) - 1);
		quickpanel_common_util_char_replace(buf, '\n', ' ');
		elm_access_info_set(ao, ELM_ACCESS_INFO, buf);
	}

	ao = quickpanel_accessibility_screen_reader_object_get(icon,
			SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", icon);
	if (ao != NULL) {
		if (state == ICON_VIEW_STATE_ON) {
			elm_access_info_set(ao, ELM_ACCESS_STATE, _NOT_LOCALIZED("On"));
		} else if (state == ICON_VIEW_STATE_DIM) {
			elm_access_info_set(ao, ELM_ACCESS_STATE, _NOT_LOCALIZED("Turned off"));
		} else if (state == ICON_VIEW_STATE_OFF) {
			elm_access_info_set(ao, ELM_ACCESS_STATE, _NOT_LOCALIZED("Off"));
		}
	}
#endif

	return QP_OK;
}

HAPI void quickpanel_setting_icon_access_text_set(Evas_Object *icon, const char *text)
{
#ifdef QP_SCREENREADER_ENABLE
	char buf[256] = {0,};
	Evas_Object *ao = NULL;
#endif

	retif(icon == NULL, , "invalid parameter");
	retif(text == NULL, , "invalid parameter");

#ifdef QP_SCREENREADER_ENABLE
	ao = quickpanel_accessibility_screen_reader_object_get(icon,
			SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", icon);
	if (ao != NULL) {
		elm_access_info_set(ao, ELM_ACCESS_TYPE, _NOT_LOCALIZED("Button"));
		strncpy(buf, text, sizeof(buf) - 1);
		quickpanel_common_util_char_replace(buf, '\n', ' ');
		elm_access_info_set(ao, ELM_ACCESS_INFO, buf);
	}
#endif
}

HAPI Evas_Object *quickpanel_setting_icon_content_get(Evas_Object *icon)
{
	retif(icon == NULL, NULL, "invalid parameter");

	struct appdata *ad = (struct appdata*)quickpanel_get_app_data();
	retif(ad == NULL, NULL, "application data is NULL");

	return elm_object_part_content_get(icon, "icon.swallow.wvga");
}

HAPI int quickpanel_setting_icon_content_set(Evas_Object *icon, Evas_Object *content)
{
	retif(icon == NULL, QP_FAIL, "invalid parameter");
	retif(content == NULL, QP_FAIL, "invalid parameter");

	struct appdata *ad = (struct appdata*)quickpanel_get_app_data();
	retif(ad == NULL, QP_FAIL, "application data is NULL");

	elm_object_part_content_set(icon, "icon.swallow.wvga", content);

	return QP_OK;
}

HAPI int quickpanel_setting_icon_state_set(Evas_Object *icon, int state)
{
#ifdef QP_SCREENREADER_ENABLE
	Evas_Object *ao = NULL;
#endif
	retif(icon == NULL, -1, "invalid parameter");
	SERR("icon:%p state:%d", icon, state);

#ifdef QP_SCREENREADER_ENABLE
	ao = quickpanel_accessibility_screen_reader_object_get(icon,
			SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", icon);
	if (ao != NULL) {
		if (state == ICON_VIEW_STATE_ON) {
			elm_access_info_set(ao, ELM_ACCESS_STATE, _NOT_LOCALIZED("On"));
		} else if (state == ICON_VIEW_STATE_DIM) {
			elm_access_info_set(ao, ELM_ACCESS_STATE, _NOT_LOCALIZED("Turned off"));
		} else if (state == ICON_VIEW_STATE_OFF) {
			elm_access_info_set(ao, ELM_ACCESS_STATE, _NOT_LOCALIZED("Off"));
		}
	}
#endif

	if (state == ICON_VIEW_STATE_ON) {
		elm_object_signal_emit(icon, "icon.on", "quickpanl.prog");
	} else if (state == ICON_VIEW_STATE_DIM) {
		elm_object_signal_emit(icon, "icon.dim", "quickpanl.prog");
	} else {
		elm_object_signal_emit(icon, "icon.off", "quickpanl.prog");
	}

	edje_object_message_signal_process(_EDJ(icon));

	return 0;
}

HAPI int quickpanel_setting_icon_state_progress_set(Evas_Object *icon)
{
	retif(icon == NULL, -1, "invalid parameter");

	elm_object_signal_emit(icon, "icon.progress", "quickpanl.prog");
	edje_object_message_signal_process(_EDJ(icon));

	return 0;
}

HAPI Evas_Object *quickpanel_setting_icon_new(Evas_Object *parent)
{
	const char *signal = NULL;
	Evas_Object *icon = NULL;
	retif(parent == NULL, NULL, "invalid parameter");

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, EINA_FALSE, "invalid data.");

	icon = elm_layout_add(parent);
	retif(!icon, NULL, "fail to add layout");

	elm_layout_file_set(icon, DEFAULT_EDJ, "quickpanel/setting_icon_wvga");

	evas_object_size_hint_weight_set(icon,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_signal_emit(icon, "icon.off", "quickpanl.prog");

	Evas_Object *focus = quickpanel_accessibility_ui_get_focus_object(icon);
	elm_object_part_content_set(icon, "focus", focus);

	if (ad->angle == 0 || ad->angle == 180) {
		signal = "icon.portrait";
	} else {
		signal = "icon.landscape";
	}

	elm_object_signal_emit(icon, signal, "quickpanl.prog");
	edje_object_message_signal_process(_EDJ(icon));

	return icon;
}

HAPI Evas_Object *quickpanel_setting_icon_image_new(Evas_Object *parent, const char *img_path)
{
	Evas_Object *content = NULL;
	retif(parent == NULL, NULL, "invalid parameter");
	retif(img_path == NULL, NULL, "invalid parameter");

	content = elm_image_add(parent);
	retif(content == NULL, NULL, "failed to create image");

	if (!elm_image_file_set(content, DEFAULT_EDJ, img_path)) {
		ERR("fail to set file[%s]", img_path);
		evas_object_del(content);
		content = NULL;
		return NULL;
	}

	return content;
}

static Evas_Object *quickpanel_setting_container_get(Evas_Object *base)
{
	Evas_Object *container = NULL;
	retif(base == NULL, NULL, "invalid parameter");

	container = elm_object_part_content_get(base, QP_SETTING_BASE_PART);

	return container;
}

HAPI Evas_Object *quickpanel_setting_scroller_get(Evas_Object *base)
{
	Evas_Object *container = NULL;
	Evas_Object *scroller = NULL;
	retif(base == NULL, NULL, "invalid parameter");

	struct appdata *ad = (struct appdata*)quickpanel_get_app_data();
	retif(ad == NULL, NULL, "application data is NULL");

	container = quickpanel_setting_container_get(base);

	scroller = elm_object_part_content_get(container, QP_SETTING_SCROLLER_PART_WVGA);

	retif(scroller == NULL, NULL, "invalid parameter");

	return scroller;
}

HAPI Evas_Object *quickpanel_setting_box_get_from_scroller(Evas_Object *base)
{
	Evas_Object *scroller = NULL;
	Evas_Object *box = NULL;
	retif(base == NULL, NULL, "invalid parameter");

	scroller = quickpanel_setting_scroller_get(base);
	retif(scroller == NULL, NULL, "invalid parameter");

	box = elm_object_content_get(scroller);

	return box;
}

HAPI Evas_Object *quickpanel_setting_box_get(Evas_Object *base)
{
	Evas_Object *container = NULL;
	Evas_Object *box = NULL;
	retif(base == NULL, NULL, "invalid parameter");

	container = quickpanel_setting_container_get(base);
	retif(container == NULL, NULL, "invalid parameter");
	box = elm_object_part_content_get(container, QP_SETTING_SCROLLER_PART_WVGA);

	return box;
}

HAPI int quickpanel_setting_container_rotation_set(Evas_Object *base, int angle)
{
	Evas_Object *container = NULL;
	const char *signal = NULL;

	retif(!base, -1, "base is NULL");
	retif(angle < 0, -1, "angle is %d", angle);

	container = quickpanel_setting_container_get(base);
	retif(!container, -1, "box is NULL");

	if (angle % 180 == 0) {
		signal = "portrait";
	} else {
		signal = "landscape";
	}

	elm_object_signal_emit(container, signal, "background");
	edje_object_message_signal_process(_EDJ(container));

	return 0;
}

HAPI int quickpanel_setting_icons_rotation_set(Evas_Object *base, int angle)
{
	Evas_Object *box = NULL;
	Evas_Object *icon = NULL;
	Eina_List *icons = NULL;
	Eina_List *l = NULL;
	const char *signal = NULL;

	retif(!base, -1, "base is NULL");
	retif(angle < 0, -1, "angle is %d", angle);

	box = quickpanel_setting_box_get(base);
	retif(!box, -1, "box is NULL");

	icons = elm_box_children_get(box);
	retif(!icons, -1, "icons list is NULL");

	if (angle % 180 == 0) {
		signal = "icon.portrait";
	} else {
		signal = "icon.landscape";
	}

	EINA_LIST_FOREACH(icons, l, icon) {
		elm_object_signal_emit(icon, signal, "quickpanl.prog");
		edje_object_message_signal_process(_EDJ(icon));
	}

	eina_list_free(icons);

	return 0;
}

HAPI void quickpanel_setting_icons_emit_sig(Evas_Object *icon, const char *signal)
{
	retif(!icon, , "icon is NULL");
	retif(!signal, , "icon is NULL");

	elm_object_signal_emit(icon, signal, "quickpanl.prog");
	edje_object_message_signal_process(_EDJ(icon));
}

HAPI int quickpanel_setting_icons_dragging_set(Evas_Object *icon, int is_on)
{
	const char *signal = NULL;
	retif(!icon, QP_FAIL, "icon is NULL");

	if (is_on == 1) {
		signal = "dragging.on";
	} else {
		signal = "dragging.off";
	}

	elm_object_signal_emit(icon, signal, "quickpanl.prog");
	edje_object_message_signal_process(_EDJ(icon));

	return QP_OK;
}

HAPI int quickpanel_setting_icons_screen_mode_set(Evas_Object *icon, int screen_mode)
{
	const char *signal = NULL;
	retif(!icon, QP_FAIL, "icon is NULL");

	if (screen_mode == 0) {
		signal = "icon.portrait";
	} else {
		signal = "icon.landscape";
	}

	elm_object_signal_emit(icon, signal, "quickpanl.prog");
	edje_object_message_signal_process(_EDJ(icon));

	return QP_OK;
}

HAPI int quickpanel_setting_icon_pack(Evas_Object *box, Evas_Object *icon, int is_attach_divider)
{
	retif(box == NULL, QP_FAIL, "box is NULL");

	elm_box_pack_end(box, icon);

	return QP_OK;
}

HAPI void quickpanel_setting_icon_unpack_all(Evas_Object *box)
{
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *node = NULL;
	Eina_List *list = NULL;
	retif(box == NULL, , "invalid parameter");

	list = elm_box_children_get(box);
	retif(list == NULL, , "empty list");

	elm_box_unpack_all(box);

	EINA_LIST_FOREACH_SAFE(list, l, l_next, node) {
		if (node != NULL) {
			if (evas_object_data_get(node, E_DATA_DIVIDER_MAGIC) == (void *)DIVIDER_MAGIC) {
				evas_object_del(node);
				node = NULL;
			}
		}
	}

	eina_list_free(list);
}

HAPI int quickpanel_setting_scroll_page_get(void *data)
{
	int page_h = 0x99;
	struct appdata *ad = NULL;
	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	ad = data;
	retif(!ad->ly, QP_FAIL, "layout is NULL!");

	Evas_Object *scroller = quickpanel_setting_scroller_get(ad->ly);
	elm_scroller_current_page_get(scroller, &page_h, NULL);

	return page_h;
}

HAPI int quickpanel_setting_set_scroll_page_width(void *data)
{
	struct appdata *ad = NULL;
	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	ad = data;
	retif(!ad->ly, QP_FAIL, "layout is NULL!");

	Evas_Object *scroller = quickpanel_setting_scroller_get(ad->ly);

	int w, h;
	elm_win_screen_size_get(ad->win, NULL, NULL, &w, &h);
	elm_scroller_page_size_set(scroller, w / QP_SETTING_NUM_PORTRAIT_ICONS, 0);

	return 0;
}

HAPI int quickpanel_setting_start(Evas_Object *base)
{
	Evas_Object *scroller = NULL;
	retif(base == NULL, QP_FAIL, "Invalid parameter!");

	scroller = quickpanel_setting_scroller_get(base);
	retif(scroller == NULL, QP_FAIL, "Invalid parameter!");

	elm_scroller_page_bring_in(scroller, 0, 0);

	return QP_OK;
}

HAPI int quickpanel_setting_stop(Evas_Object *base, int is_bring_in)
{
	int page = QP_SETTING_INITIAL_PAGE_NUM;
	Evas_Object *scroller = NULL;
	retif(base == NULL, QP_FAIL, "Invalid parameter!");

	scroller = quickpanel_setting_scroller_get(base);
	retif(scroller == NULL, QP_FAIL, "Invalid parameter!");

	if (is_bring_in == 1) {
		elm_scroller_page_bring_in(scroller, page, 0);
	} else {
		elm_scroller_page_show(scroller, page, 0);
	}

	return QP_OK;
}

HAPI int quickpanel_setting_layout_set(Evas_Object *base, Evas_Object *setting)
{
	retif(base == NULL, QP_FAIL, "Invalid parameter!");
	retif(setting == NULL, QP_FAIL, "Invalid parameter!");

	elm_object_part_content_set(base, QP_SETTING_BASE_PART, setting);

	return 0;
}

HAPI Evas_Object *quickpanel_setting_layout_get(Evas_Object *base, const char *setting_part)
{
	retif(base == NULL, NULL, "Invalid parameter!");
	retif(setting_part == NULL, NULL, "Invalid parameter!");

	return elm_object_part_content_get(base, setting_part);
}

HAPI int quickpanel_setting_layout_remove(Evas_Object *base)
{
	Evas_Object *container = NULL;
	Evas_Object *scroller = NULL;
	Evas_Object *box = NULL;
	retif(base == NULL, QP_FAIL, "Invalid parameter!");

	container = quickpanel_setting_container_get(base);
	scroller = quickpanel_setting_scroller_get(base);
	box = quickpanel_setting_box_get(base);

	if (box) {
		elm_box_clear(box);
		evas_object_del(box);
		box = NULL;
	}
	if (scroller) {
		evas_object_del(scroller);
		scroller = NULL;
	}
	if (container) {
		evas_object_del(container);
		container = NULL;
	}

	return QP_OK;
}

HAPI void quickpanel_setting_create_confirm_popup(
		Evas_Object *parent,
		char *title,
		char *text,
		Evas_Smart_Cb func)
{
	Evas_Object *popup = elm_popup_add(parent);
	Evas_Object *btn = NULL;

	if (popup == NULL) {
		return;
	}

	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);

	if (title) {
		elm_object_part_text_set(popup, "title,text", title);
	}

	if (text) {
		elm_object_text_set(popup, text);
	}

	btn = elm_button_add(popup);
	elm_object_style_set(btn, "popup");
	elm_object_text_set(btn, _("IDS_ST_SK_OK"));
	elm_object_part_content_set(popup, "button1", btn);
	evas_object_smart_callback_add(btn, "clicked", func, popup);

	evas_object_show(popup);
	quickpanel_common_ui_set_current_popup(popup, func);
}

HAPI void quickpanel_setting_create_2button_confirm_popup(
		Evas_Object *parent,
		char *title,
		char *text,
		char *btn1_text,
		Evas_Smart_Cb btn1_func,
		char *btn2_text,
		Evas_Smart_Cb btn2_func)
{
	Evas_Object *popup = elm_popup_add(parent);
	Evas_Object *btn = NULL;

	if (popup == NULL) {
		return;
	}

	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);

	if (title) {
		elm_object_part_text_set(popup, "title,text", title);
	}

	if (text) {
		elm_object_text_set(popup, text);
	}

	if (btn1_func != NULL && btn1_text != NULL) {
		btn = elm_button_add(popup);
		elm_object_style_set(btn, "popup");
		elm_object_text_set(btn, btn1_text);
		elm_object_part_content_set(popup, "button1", btn);
		evas_object_smart_callback_add(btn, "clicked", btn1_func, popup);
	}
	if (btn2_func != NULL && btn2_text != NULL) {
		btn = elm_button_add(popup);
		elm_object_style_set(btn, "popup");
		elm_object_text_set(btn, btn2_text);
		elm_object_part_content_set(popup, "button2", btn);
		evas_object_smart_callback_add(btn, "clicked", btn2_func, popup);
	}

	evas_object_show(popup);
	quickpanel_common_ui_set_current_popup(popup, btn1_func);
}

HAPI void quickpanel_setting_create_timeout_popup(Evas_Object *parent, char *msg)
{
	retif(msg == NULL, , "invalid parameter");

	notification_status_message_post(msg);
}
