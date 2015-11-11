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
#include <string.h>
#include <glib.h>

#include <Elementary.h>

#include <vconf.h>
#include <notification.h>
#include <notification_internal.h>
#include <notification_text_domain.h>
#include <system_settings.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "common_uic.h"
#include "common.h"
#include "list_util.h"
#include "quickpanel_def.h"
#include <vi_manager.h>
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

#define LEN_UNIT_TEXTBLOCK 555
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

static Evas_Object *_check_duplicated_progress_loading(Evas_Object *obj, const char *part, const char *style_name)
{
	Evas_Object *old_content = NULL;
	const char *old_style_name = NULL;

	retif(obj == NULL, NULL, "Invalid parameter!");
	retif(part == NULL, NULL, "Invalid parameter!");
	retif(style_name == NULL, NULL, "Invalid parameter!");

	old_content = elm_object_part_content_get(obj, part);
	if (old_content != NULL) {
		old_style_name = elm_object_style_get(old_content);
		if (old_style_name != NULL) {
			if (strcmp(old_style_name, style_name) == 0) {
				return old_content;
			}

			elm_object_part_content_unset(obj, part);
			evas_object_del(old_content);
			old_content = NULL;
		}
	}

	return NULL;
}

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
	const char *old_text;

	if (!obj || !part || !text) {
		ERR("Invalid parameters");
		return;
	}

	old_text = elm_object_part_text_get(obj, part);
	if (old_text != NULL) {
		if (strcmp(old_text, text) == 0) {
			return;
		}
	}

	elm_object_part_text_set(obj, part, text);
}

static char *_noti_get_progress(notification_h noti, char *buf, int buf_len)
{
	double size = 0.0;
	double percentage = 0.0;

	retif(noti == NULL, NULL, "Invalid parameter!");
	retif(buf == NULL, NULL, "Invalid parameter!");

	notification_get_size(noti, &size);
	notification_get_progress(noti, &percentage);

	if (percentage > 0) {
		if (percentage < 1.0 ) {
			if (snprintf(buf, buf_len, "%d%%", (int)(percentage * 100.0 + 0.5)) <= 0) {
				return NULL;
			}
		}
		if (percentage >= 1.0) {
			snprintf(buf, buf_len, "%d%%", 100);
		}

		return buf;
	} else if (size > 0 && percentage == 0) {
		if (size > (1 << 30)) {
			if (snprintf(buf, buf_len, "%.1lfGB",
						size / 1000000000.0) <= 0)
				return NULL;

			return buf;
		} else if (size > (1 << 20)) {
			if (snprintf(buf, buf_len, "%.1lfMB",
						size / 1000000.0) <= 0)
				return NULL;

			return buf;
		} else if (size > (1 << 10)) {
			if (snprintf(buf, buf_len, "%.1lfKB",
						size / 1000.0) <= 0)
				return NULL;

			return buf;
		} else {
			if (snprintf(buf, buf_len, "%.0lfB", size) <= 0)
				return NULL;

			return buf;
		}
	}

	return NULL;
}

static void _set_progressbar(Evas_Object *item, notification_h noti)
{
	Evas_Object *ic = NULL;
	Evas_Object *old_ic = NULL;
	double size = 0.0;
	double percentage = 0.0;
	notification_type_e type = NOTIFICATION_TYPE_NONE;
	notification_ly_type_e layout = NOTIFICATION_LY_NONE ;

	retif(item == NULL, , "Invalid parameter!");
	retif(noti == NULL, , "noti is NULL");

	notification_get_type(noti, &type);
	if (type == NOTIFICATION_TYPE_ONGOING) {
		notification_get_size(noti, &size);
		notification_get_progress(noti, &percentage);
		notification_get_layout(noti, &layout);

		if (layout != NOTIFICATION_LY_ONGOING_EVENT) {
			if (percentage > 0.0 && percentage <= 1.0) {
				old_ic = _check_duplicated_progress_loading(item, "elm.swallow.progress", "list_progress");
				if (old_ic == NULL) {
					ic = elm_progressbar_add(item);
					elm_progressbar_unit_format_set(ic, "%0.0f%%");
					if (ic == NULL)
						return;
					elm_object_style_set(ic, "list_progress");
				} else {
					ic = old_ic;
				}

				elm_progressbar_value_set(ic, percentage);
				elm_progressbar_horizontal_set(ic, EINA_TRUE);
				elm_progressbar_pulse(ic, EINA_FALSE);
			} else if ((size >= 0.0 && percentage == 0.0) || ((size < 0.0 && percentage == 0.0)|| (size == 0.0 && percentage < 0.0))) {
				old_ic = _check_duplicated_progress_loading(item, "elm.swallow.progress", "pending");
				if (old_ic == NULL) {
					ic = elm_progressbar_add(item);
					elm_progressbar_unit_format_set(ic, "%0.0f%%");
					if (ic == NULL)
						return;
					elm_object_style_set(ic, "pending");
				} else {
					ic = old_ic;
				}

				elm_progressbar_horizontal_set(ic, EINA_TRUE);
				elm_progressbar_pulse(ic, EINA_TRUE);
			}
		}
	}

	if (ic != NULL) {
		elm_object_part_content_set(item, "elm.swallow.progress", ic);
	}
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
	char *text = NULL;
	char *text_utf8 = NULL;
	char *domain = NULL;
	char *dir = NULL;
	char *pkgname = NULL;
	//	char *caller_pkgname = NULL;
	int group_id = 0, priv_id = 0;
	char buf[128] = { 0, };
	notification_type_e type = NOTIFICATION_TYPE_NONE;
	double size = 0.0;
	double percentage = 0.0;
	notification_ly_type_e layout = NOTIFICATION_LY_NONE ;
#ifdef QP_SCREENREADER_ENABLE
	Evas_Object *ao = NULL;
	Eina_Strbuf *str_buf = NULL;
#endif
	Evas_Object *textblock = NULL;
	int len_w = 0, num_line = 1, view_height = 0;
	struct appdata *ad = quickpanel_get_app_data();
	char *text_count = NULL;
	int ret;

	if (!ad || !item || !noti) {
		ERR("Invalid parameters: %p %p %p", ad, item, noti);
		return;
	}

	/* Set text domain */
	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL) {
		bindtextdomain(domain, dir);
	}

#ifdef QP_SCREENREADER_ENABLE
	ao = quickpanel_accessibility_screen_reader_object_get(item, SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", item);
	if (ao != NULL) {
		str_buf = eina_strbuf_new();
		elm_access_info_set(ao, ELM_ACCESS_TYPE, _("IDS_QP_BUTTON_NOTIFICATION"));
	}
#endif

	/* Get pkgname & id */
	ret = notification_get_pkgname(noti, &pkgname);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Unable to get the pkgname");
	}
	//	notification_get_application(noti, &caller_pkgname);
	ret = notification_get_id(noti, &group_id, &priv_id);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Unable to get id");
	}
	ret = notification_get_type(noti, &type);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Unable to get type");
	}
	ret = notification_get_size(noti, &size);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Unable to get size");
	}
	ret = notification_get_progress(noti, &percentage);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Unable to get progress");
	}
	ret = notification_get_layout(noti, &layout);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Unable to get layout");
	}
	ret = notification_get_text(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT, &text_count);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Unable to get event_count");
	} 

	SDBG("percentage:%f size:%f", percentage, size);

	ret = notification_get_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, &text);
	if (ret == NOTIFICATION_ERROR_NONE && text != NULL) {
		quickpanel_common_util_char_replace(text, _NEWLINE, _SPACE);
		_set_text_to_part(item, "elm.text.title", text);
#ifdef QP_SCREENREADER_ENABLE
		_check_and_add_to_buffer(str_buf, text);
#endif
	}

	ret = notification_get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, &text);
	if (ret == NOTIFICATION_ERROR_NONE && text != NULL) {
		if (layout == NOTIFICATION_LY_ONGOING_EVENT) {
			text_utf8 = elm_entry_utf8_to_markup(text);
			if (text_utf8 != NULL) {
				_set_text_to_part(item, "elm.text.content", text_utf8);
				free(text_utf8);
			} else {
				_set_text_to_part(item, "elm.text.content", text);
			}
			textblock = (Evas_Object *)edje_object_part_object_get(_EDJ(item), "elm.text.content");
			evas_object_textblock_size_native_get(textblock, &len_w, NULL);
			num_line = len_w / (LEN_UNIT_TEXTBLOCK * ad->scale);
			num_line = (len_w - (num_line * (LEN_UNIT_TEXTBLOCK * ad->scale))) > 0 ? num_line + 1 : num_line;
			if (num_line <= 1) {
				elm_object_signal_emit(item, "line1.set", "prog");
				view_height = QP_THEME_LIST_ITEM_ONGOING_EVENT_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT;
			} else /*if (num_line >= 2 && num_line < 3)*/ {
				elm_object_signal_emit(item, "line2.set", "prog");
				view_height = QP_THEME_LIST_ITEM_ONGOING_EVENT_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT;
			} /*else {
				elm_object_signal_emit(item, "line3.set", "prog");
				view_height = QP_THEME_LIST_ITEM_ONGOING_EVENT_LINE3_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT;
				}*/
			quickpanel_uic_initial_resize(item, view_height);
#ifdef QP_SCREENREADER_ENABLE
			_check_and_add_to_buffer(str_buf, text);
#endif
		} else {
			quickpanel_common_util_char_replace(text, _NEWLINE, _SPACE);
			_set_text_to_part(item, "elm.text.content", text);
#ifdef QP_SCREENREADER_ENABLE
			_check_and_add_to_buffer(str_buf, text);
#endif
		}
	}

	if (layout != NOTIFICATION_LY_ONGOING_EVENT) {
		text = _noti_get_progress(noti, buf,  sizeof(buf));
		if (text != NULL) {
			quickpanel_common_util_char_replace(text, _NEWLINE, _SPACE);
#ifdef QP_SCREENREADER_ENABLE
			_check_and_add_to_buffer(str_buf, text);
#endif
		} else {
			_set_text_to_part(item, "elm.text.time", "");
		}
	} else {
		const char *get_content;

		get_content = elm_object_part_text_get(item, "elm.text.content");
		if (get_content == NULL || strlen(get_content) == 0) {
			// if there is no content, move title to vertical center.
			elm_object_signal_emit(item, "title.move.center", "prog");
		}
	}

	if (layout == NOTIFICATION_LY_ONGOING_PROGRESS && text_count != NULL) {
		_set_text_to_part(item, "elm.text.count", text_count);
		if (elm_object_part_text_get(item, "elm.text.count") != NULL) {
			elm_object_signal_emit(item, "content.short", "prog");
			elm_object_signal_emit(item, "count.show", "prog");
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

	if (layout == NOTIFICATION_LY_ONGOING_EVENT) {
		view_layout_group = "quickpanel/listitem/event";
		view_height = QP_THEME_LIST_ITEM_ONGOING_EVENT_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT;
	} else if (layout == NOTIFICATION_LY_ONGOING_PROGRESS) {
		view_layout_group = "quickpanel/listitem/progress";
		view_height = QP_THEME_LIST_ITEM_ONGOING_PROGRESS_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT;
	}

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

	_set_progressbar(item, noti_node->noti);
	_set_icon(item, noti_node->noti);
	_set_text(item, noti_node->noti);
}

Noti_View_H ongoing_noti_view_h = {
	.name 			= "ongoing_noti_view",
	.create			= _create,
	.update			= _update,
	.remove			= NULL,
};
