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
#include <string.h>
#include <glib.h>

#include <notification.h>
#include <notification_text_domain.h>
#include <system_settings.h>
#include <vconf.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "common_uic.h"
#include "common.h"
#include "list_util.h"
#include "quickpanel_def.h"
#include "vi_manager.h"
#include "noti_node.h"
#include "noti_list_item.h"
#include "noti.h"
#include "noti_util.h"
#include "animated_icon.h"

#ifdef QP_SCREENREADER_ENABLE
#include "accessibility.h"
#endif

#ifdef QP_ANIMATED_IMAGE_ENABLE
#include "animated_image.h"
#endif

#define LEN_UNIT_TEXTBLOCK 560
#define QP_DEFAULT_ICON	RESDIR"/quickpanel_icon_default.png"

#ifdef QP_SCREENREADER_ENABLE
static inline void _check_and_add_to_buffer(Eina_Strbuf *str_buf, const char *text)
{
	char buf_number[QP_UTIL_PHONE_NUMBER_MAX_LEN * 2] = { 0, };

	retif(str_buf == NULL, , "Invalid parameter!");

	if (text != NULL) {
		if (strlen(text) > 0) {
			if (quickpanel_common_util_is_phone_number(text)) {
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
#endif

static Evas_Object *_check_duplicated_image_loading(Evas_Object *obj, const char *part, const char *file_path)
{
	Evas_Object *old_ic = NULL;
	const char *old_ic_path = NULL;

	retif(obj == NULL, NULL, "Invalid parameter!");
	retif(part == NULL, NULL, "Invalid parameter!");
	retif(file_path == NULL, NULL, "Invalid parameter!");

	old_ic = elm_object_part_content_get(obj, part);

	if (quickpanel_animated_icon_is_same_icon(old_ic, file_path) == 1) {
		return old_ic;
	}

	if (old_ic != NULL) {
		elm_image_file_get(old_ic, &old_ic_path, NULL);
		if (old_ic_path != NULL) {
			if (strcmp(old_ic_path, file_path) == 0)
				return old_ic;
		}

		elm_object_part_content_unset(obj, part);
		evas_object_del(old_ic);
		old_ic = NULL;
	}

	return NULL;
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

static void _set_icon(Evas_Object *item, notification_h noti)
{
	Evas_Object *ic = NULL;
	Evas_Object *old_ic = NULL;
	char *icon_path = NULL;
	char *icon_sub_path = NULL;
	char *thumbnail_path = NULL;
	char *main_icon_path = NULL;
	char *sub_icon_path = NULL;
	char *icon_default = NULL;
	char *pkgname = NULL;

	retif(item == NULL, , "Invalid parameter!");
	retif(noti == NULL, , "noti is NULL");

	notification_get_pkgname(noti, &pkgname);
	notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL,
			&thumbnail_path);
	notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, &icon_path);
	notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB, &icon_sub_path);

	if (thumbnail_path != NULL && icon_path != NULL) {
		main_icon_path = thumbnail_path;
		sub_icon_path = icon_path;
	} else if (icon_path != NULL && thumbnail_path == NULL) {
		main_icon_path = icon_path;
		sub_icon_path = icon_sub_path;
	} else if (icon_path == NULL && thumbnail_path != NULL) {
		main_icon_path = thumbnail_path;
		sub_icon_path = icon_sub_path;
	} else {
		icon_default = quickpanel_common_ui_get_pkginfo_icon(pkgname);
		main_icon_path = icon_default;
		sub_icon_path = NULL;
	}

	if (main_icon_path != NULL) {
		old_ic = _check_duplicated_image_loading(item,
				"elm.swallow.thumbnail", main_icon_path);

		if (old_ic == NULL) {
			ic = quickpanel_animated_icon_get(item, main_icon_path);
			if (ic == NULL) {
				ic = elm_image_add(item);
				elm_image_resizable_set(ic, EINA_FALSE, EINA_TRUE);
				elm_image_file_set(ic, main_icon_path, quickpanel_animated_image_get_groupname(main_icon_path));
#ifdef QP_ANIMATED_IMAGE_ENABLE
				quickpanel_animated_image_add(ic);
#endif

				if (!strncmp(main_icon_path, QP_PRELOAD_NOTI_ICON_PATH, strlen(QP_PRELOAD_NOTI_ICON_PATH))) 	{
					DBG("Apply color theme [%s]", main_icon_path);
					evas_object_color_set(ic, 155, 216, 226, 255);
				} else {
					elm_image_aspect_fixed_set(ic, EINA_TRUE);
				}
			}
			elm_object_part_content_set(item, "elm.swallow.thumbnail", ic);
		}
	}

	if (sub_icon_path != NULL) {
		old_ic = _check_duplicated_image_loading(item,
				"elm.swallow.icon", sub_icon_path);

		if (old_ic == NULL) {
			ic = elm_image_add(item);
			elm_image_resizable_set(ic, EINA_FALSE, EINA_TRUE);
			elm_image_file_set(ic, sub_icon_path, quickpanel_animated_image_get_groupname(sub_icon_path));
			elm_object_part_content_set(item, "elm.swallow.icon", ic);
			elm_object_signal_emit(item, "elm.icon.bg.show", "elm");
		}
	}

	if (main_icon_path == NULL && sub_icon_path == NULL) {
		old_ic = _check_duplicated_image_loading(item,
				"elm.swallow.thumbnail", QP_DEFAULT_ICON);

		if (old_ic == NULL) {
			ic = elm_image_add(item);
			elm_image_resizable_set(ic, EINA_FALSE, EINA_TRUE);
			elm_image_file_set(ic, QP_DEFAULT_ICON, quickpanel_animated_image_get_groupname(QP_DEFAULT_ICON));
			elm_object_part_content_set(item, "elm.swallow.thumbnail", ic);
#ifdef QP_ANIMATED_IMAGE_ENABLE
			quickpanel_animated_image_add(ic);
#endif
		}
	}

	if (icon_default != NULL) {
		free(icon_default);
	}
}

static void _set_text(Evas_Object *item, notification_h noti)
{
	int noti_err = NOTIFICATION_ERROR_NONE;
	char *text = NULL;
	char *domain = NULL;
	char *dir = NULL;
	time_t noti_time;
	char buf[512] = {0,};
#ifdef QP_SCREENREADER_ENABLE
	Evas_Object *ao = NULL;
	Eina_Strbuf *str_buf = NULL;
#endif
	struct appdata *ad = quickpanel_get_app_data();

	retif(ad == NULL, , "Invalid parameter!");
	retif(item == NULL, , "Invalid parameter!");
	retif(noti == NULL, , "noti is NULL");

	/* Set text domain */
	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL) {
		bindtextdomain(domain, dir);
	}

#ifdef QP_SCREENREADER_ENABLE
	ao = quickpanel_accessibility_screen_reader_object_get(item,
			SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", item);
	if (ao != NULL) {
		str_buf = eina_strbuf_new();
		elm_access_info_set(ao, ELM_ACCESS_TYPE, _("IDS_QP_BUTTON_NOTIFICATION"));
	}
#endif

	/* Get pkgname & id */
	noti_err = notification_get_text(noti,
			NOTIFICATION_TEXT_TYPE_TITLE,
			&text);

	if (noti_err == NOTIFICATION_ERROR_NONE && text != NULL) {
		quickpanel_common_util_char_replace(text, _NEWLINE, _SPACE);
		_set_text_to_part(item, "elm.text.title", text);
#ifdef QP_SCREENREADER_ENABLE
		_check_and_add_to_buffer(str_buf, text);
#endif
	}

	noti_err = notification_get_text(noti,
			NOTIFICATION_TEXT_TYPE_EVENT_COUNT,
			&text);

	if (noti_err == NOTIFICATION_ERROR_NONE && text != NULL) {
		quickpanel_common_util_char_replace(text, _NEWLINE, _SPACE);
		int count = atoi(text);
		if (count > 999) {
			_set_text_to_part(item, "elm.text.count", "999+");
		} else {
			_set_text_to_part(item, "elm.text.count", text);
		}
#ifdef QP_SCREENREADER_ENABLE
		_check_and_add_to_buffer(str_buf, text);
#endif
	}

	noti_err = notification_get_text(noti,
			NOTIFICATION_TEXT_TYPE_CONTENT,
			&text);
	if (noti_err == NOTIFICATION_ERROR_NONE && text != NULL) {
		quickpanel_common_util_char_replace(text, _NEWLINE, _SPACE);
		_set_text_to_part(item, "elm.text.content", text);
#ifdef QP_SCREENREADER_ENABLE
		_check_and_add_to_buffer(str_buf, text);
#endif
	}

	noti_err = notification_get_time(noti, &noti_time);
	if (noti_time == 0.0) {
		noti_err = notification_get_insert_time(noti, &noti_time);
	}
	if (noti_err == NOTIFICATION_ERROR_NONE) {
		quickpanel_noti_util_get_time(noti_time, buf, 512);
		_set_text_to_part(item, "elm.text.time", buf);
#ifdef QP_SCREENREADER_ENABLE
		_check_and_add_to_buffer(str_buf, buf);
#endif
	}

	if (elm_object_part_text_get(item, "elm.text.count") != NULL) {
		elm_object_signal_emit(item, "content.short", "prog");
		elm_object_signal_emit(item, "count.show", "prog");
	}
	if (elm_object_part_text_get(item, "elm.text.time") != NULL) {
		elm_object_signal_emit(item, "title.short", "prog");
	}

	const char *get_content = elm_object_part_text_get(item, "elm.text.content");
	if (get_content == NULL || strlen(get_content) == 0) {
		// if there is no content, move title to vertical center.
		elm_object_signal_emit(item, "title.move.center", "prog");
		if (elm_object_part_text_get(item, "elm.text.time") != NULL) {
			elm_object_signal_emit(item, "title.short.center", "prog");
			elm_object_signal_emit(item, "time.move.center", "prog");
		}
	} else {
		elm_object_signal_emit(item, "title.move.default", "prog");
		if (elm_object_part_text_get(item, "elm.text.time") != NULL) {
			elm_object_signal_emit(item, "time.move.default", "prog");
			elm_object_signal_emit(item, "title.short", "prog");
		} else {
			elm_object_signal_emit(item, "title.text.default", "prog");
		}
	}
#ifdef QP_SCREENREADER_ENABLE
	if (ao != NULL && str_buf != NULL) {
		elm_access_info_set(ao, ELM_ACCESS_INFO, eina_strbuf_string_get(str_buf));
		eina_strbuf_free(str_buf);
	}
#endif
}

static Evas_Object *_create(notification_h noti, Evas_Object *parent)
{
	int view_height = 0;
	Evas_Object *view = NULL;
	const char *view_layout_group = NULL;
	retif(parent == NULL, NULL, "Invalid parameter!");
	retif(noti == NULL, NULL, "Invalid parameter!");

	notification_ly_type_e layout = NOTIFICATION_LY_NOTI_EVENT_SINGLE;
	notification_get_layout(noti, &layout);

	view_layout_group = "quickpanel/listitem/notification";
	view_height = QP_THEME_LIST_ITEM_NOTIFICATION_LISTTYPE_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT;

	view = elm_layout_add(parent);
	if (view != NULL) {
		elm_layout_file_set(view, DEFAULT_EDJ, view_layout_group);
		evas_object_size_hint_weight_set(view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(view, EVAS_HINT_FILL, EVAS_HINT_FILL);
		quickpanel_uic_initial_resize(view, view_height);
		evas_object_show(view);
	} else {
		ERR("failed to create ongoing notification view");
	}

	return view;
}

static void _update(noti_node_item *noti_node, notification_ly_type_e layout, Evas_Object *item)
{
	retif(item == NULL, , "Invalid parameter!");
	retif(noti_node == NULL, , "Invalid parameter!");

	_set_icon(item, noti_node->noti);
	_set_text(item, noti_node->noti);
}

Noti_View_H noti_view_listtype_h = {
	.name 			= "noti_view_listtype",
	.create			= _create,
	.update			= _update,
	.remove			= NULL,
};
