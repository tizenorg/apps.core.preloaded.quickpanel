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

#include <string.h>
#include <Ecore_X.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "list_util.h"
#include "quickpanel_def.h"
#include "noti_list_item.h"
#include "noti_node.h"
#include "noti.h"

#define QP_DEFAULT_ICON	ICONDIR"/quickpanel_icon_default.png"

static void _set_image(Evas_Object *noti_list_item, notification_h noti,
		notification_image_type_e image_type, const char *part, int is_stretch) {
	char *image = NULL;

	DBG("");

	retif(noti_list_item == NULL, , "Invalid parameter!");
	retif(noti == NULL, , "Invalid parameter!");
	retif(part == NULL, , "Invalid parameter!");

	notification_get_image(noti, image_type, &image);

	if (image != NULL) {
		Evas_Object *content = NULL;
		content = elm_image_add(noti_list_item);
		elm_image_file_set(content, image, NULL);
		if (is_stretch == 1) {
			elm_image_aspect_fixed_set(content, EINA_FALSE);
			elm_image_resizable_set(content, EINA_TRUE, EINA_TRUE);
		}

		elm_object_part_content_set(noti_list_item, part, content);
		elm_object_signal_emit(noti_list_item, "object.show", part);
	}
}

static int _set_text(Evas_Object *noti_list_item, notification_h noti,
		notification_text_type_e text_type, const char *part, int is_need_effect) {
	char buf[128] = { 0, };
	char *text = NULL;
	time_t time = 0;

	retif(noti_list_item == NULL, 0, "Invalid parameter!");
	retif(noti == NULL, 0, "Invalid parameter!");
	retif(part == NULL, 0, "Invalid parameter!");

	if (notification_get_time_from_text(noti, text_type, &time) == NOTIFICATION_ERROR_NONE) {
		if ((int)time > 0) {
			quickpanel_noti_get_time(time, buf, sizeof(buf));
			text = buf;
		}
	} else {
		notification_get_text(noti, text_type, &text);
	}

	if (text != NULL) {
		if (strlen(text) > 0) {
			elm_object_part_text_set(noti_list_item, part, text);
			if (is_need_effect == 1)
				elm_object_signal_emit(noti_list_item, "object.show.effect", part);
			else
				elm_object_signal_emit(noti_list_item, "object.show", part);
		}

		return strlen(text);
	}

	return 0;
}

static int _check_text_null(notification_h noti,
		notification_text_type_e text_type) {
	char *text = NULL;

	retif(noti == NULL, 0, "Invalid parameter!");

	notification_get_text(noti, text_type, &text);

	if (text == NULL) {
		return 1;
	}

	return 0;
}

static int _check_image_null(notification_h noti,
		notification_image_type_e image_type) {
	char *image = NULL;

	retif(noti == NULL, 0, "Invalid parameter!");

	notification_get_image(noti, image_type, &image);

	if (image == NULL) {
		return 1;
	}

	if (strncasecmp(image, "(null)", strlen(image)) == 0) {
		return 1;
	}

	return 0;
}

static Evas_Object *_check_duplicated_progress_loading(Evas_Object *obj, const char *part, const char *style_name) {
	Evas_Object *old_content = NULL;
	const char *old_style_name = NULL;

	retif(obj == NULL, NULL, "Invalid parameter!");
	retif(part == NULL, NULL, "Invalid parameter!");
	retif(style_name == NULL, NULL, "Invalid parameter!");

	DBG("");

	old_content = elm_object_part_content_get(obj, part);
	if (old_content != NULL) {
		old_style_name = elm_object_style_get(old_content);
		if (old_style_name != NULL) {
			DBG("%s", old_style_name);
			if (strcmp(old_style_name, style_name) == 0)
				return old_content;

			elm_object_part_content_unset(obj, part);
			evas_object_del(old_content);
		}
	}

	return NULL;
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
			DBG("%s:%s", old_ic_path, file_path);
			if (strcmp(old_ic_path, file_path) == 0)
				return old_ic;
		}

		elm_object_part_content_unset(obj, part);
		evas_object_del(old_ic);
	}

	return NULL;
}

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

static char *_noti_get_progress(notification_h noti, char *buf,
					   int buf_len)
{
	double size = 0.0;
	double percentage = 0.0;

	retif(noti == NULL, NULL, "Invalid parameter!");
	retif(buf == NULL, NULL, "Invalid parameter!");

	notification_get_size(noti, &size);
	notification_get_progress(noti, &percentage);

	if (percentage < 1 && percentage > 0) {
		if (snprintf(buf, buf_len, "%d%%", (int)(percentage * 100))
			<= 0)
			return NULL;

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
			if (snprintf(buf, buf_len, "%lfB", size) <= 0)
				return NULL;

			return buf;
		}
	}

	return NULL;
}

static void _noti_list_item_ongoing_set_progressbar(Evas_Object *noti_list_item)
{
	notification_h noti = NULL;
	Evas_Object *ic = NULL;
	Evas_Object *old_ic = NULL;
	double size = 0.0;
	double percentage = 0.0;
	notification_type_e type = NOTIFICATION_TYPE_NONE;
	notification_ly_type_e layout = NOTIFICATION_LY_NONE ;

	retif(noti_list_item == NULL, , "Invalid parameter!");

	noti_list_item_h *noti_list_item_data = evas_object_data_get(noti_list_item, E_DATA_NOTI_LIST_ITEM_H);
	retif(noti_list_item == NULL, , "data is NULL");

	noti_node_item *item = noti_list_item_data->data;
	retif(item == NULL, , "data is NULL");

	noti = item->noti;
	retif(noti == NULL, , "noti is NULL");

	notification_get_type(noti, &type);
	if (type == NOTIFICATION_TYPE_ONGOING) {
		notification_get_size(noti, &size);
		notification_get_progress(noti, &percentage);
		notification_get_layout(noti, &layout);

		if (layout != NOTIFICATION_LY_ONGOING_EVENT) {
			if (percentage > 0 && percentage <= 1) {
				old_ic = _check_duplicated_progress_loading(noti_list_item,
						"elm.swallow.progress", "quickpanel/list_progress");
				if (old_ic == NULL) {
					ic = elm_progressbar_add(noti_list_item);
					if (ic == NULL)
						return ;
					elm_object_style_set(ic, "quickpanel/list_progress");
				} else {
					ic = old_ic;
				}

				elm_progressbar_value_set(ic, percentage);
				elm_progressbar_horizontal_set(ic, EINA_TRUE);
				elm_progressbar_pulse(ic, EINA_FALSE);
			} else if (size >= 0 && percentage == 0) {
				old_ic = _check_duplicated_progress_loading(noti_list_item,
						"elm.swallow.progress", "quickpanel/pending_list");
				if (old_ic == NULL) {
					ic = elm_progressbar_add(noti_list_item);
					if (ic == NULL)
						return ;
					elm_object_style_set(ic, "quickpanel/pending_list");
				} else {
					ic = old_ic;
				}

				elm_progressbar_horizontal_set(ic, EINA_TRUE);
				elm_progressbar_pulse(ic, EINA_TRUE);
			}
		}
	}

	if (ic != NULL) {
		elm_object_part_content_set(noti_list_item, "elm.swallow.progress", ic);
	}
}

static void _noti_list_item_ongoing_set_icon(Evas_Object *noti_list_item)
{
	notification_h noti = NULL;
	Evas_Object *ic = NULL;
	Evas_Object *old_ic = NULL;
	char *icon_path = NULL;
	char *thumbnail_path = NULL;
	char *main_icon_path = NULL;
	char *sub_icon_path = NULL;

	retif(noti_list_item == NULL, , "Invalid parameter!");

	noti_list_item_h *noti_list_item_data = evas_object_data_get(noti_list_item, E_DATA_NOTI_LIST_ITEM_H);
	retif(noti_list_item == NULL, , "data is NULL");

	noti_node_item *item = noti_list_item_data->data;
	retif(item == NULL, , "data is NULL");

	noti = item->noti;
	retif(noti == NULL, , "noti is NULL");

	notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL,
			       &thumbnail_path);
	notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, &icon_path);

	if (thumbnail_path != NULL && icon_path != NULL) {
		main_icon_path = thumbnail_path;
		sub_icon_path = icon_path;
	} else if (icon_path != NULL && thumbnail_path == NULL) {
		main_icon_path = icon_path;
		sub_icon_path = NULL;
	} else if (icon_path == NULL && thumbnail_path != NULL) {
		main_icon_path = thumbnail_path;
		sub_icon_path = NULL;
	} else {
		main_icon_path = NULL;
		sub_icon_path = NULL;
	}

	if (main_icon_path != NULL) {
		old_ic = _check_duplicated_image_loading(noti_list_item,
				"elm.swallow.thumbnail", main_icon_path);

		if (old_ic == NULL) {
			ic = elm_image_add(noti_list_item);
			elm_image_resizable_set(ic, EINA_FALSE, EINA_TRUE);
			elm_image_file_set(ic, main_icon_path, "elm.swallow.thumbnail");
			elm_object_part_content_set(noti_list_item, "elm.swallow.thumbnail", ic);
		}
	}

	if (sub_icon_path != NULL) {
		old_ic = _check_duplicated_image_loading(noti_list_item,
				"elm.swallow.icon", sub_icon_path);

		if (old_ic == NULL) {
			ic = elm_image_add(noti_list_item);
			elm_image_resizable_set(ic, EINA_FALSE, EINA_TRUE);
			elm_image_file_set(ic, sub_icon_path, "elm.swallow.icon");
			elm_object_part_content_set(noti_list_item, "elm.swallow.icon", ic);
		}
	}

	if (main_icon_path == NULL && sub_icon_path == NULL) {
		old_ic = _check_duplicated_image_loading(noti_list_item,
				"elm.swallow.thumbnail", QP_DEFAULT_ICON);

		if (old_ic == NULL) {
			ic = elm_image_add(noti_list_item);
			elm_image_resizable_set(ic, EINA_FALSE, EINA_TRUE);
			elm_image_file_set(ic, QP_DEFAULT_ICON, "elm.swallow.thumbnail");
			elm_object_part_content_set(noti_list_item, "elm.swallow.thumbnail", ic);
		}
	}
}

static void _noti_list_item_ongoing_set_text(Evas_Object *noti_list_item)
{
	notification_h noti = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	char *text = NULL;
	char *domain = NULL;
	char *dir = NULL;
	char *pkgname = NULL;
	char *caller_pkgname = NULL;
	int group_id = 0, priv_id = 0;
	char buf[128] = { 0, };
	notification_type_e type = NOTIFICATION_TYPE_NONE;
	double size = 0.0;
	double percentage = 0.0;
	int isProgressBarEnabled = 1;
	notification_ly_type_e layout = NOTIFICATION_LY_NONE ;

	retif(noti_list_item == NULL, , "Invalid parameter!");

	noti_list_item_h *noti_list_item_data = evas_object_data_get(noti_list_item, E_DATA_NOTI_LIST_ITEM_H);
	retif(noti_list_item == NULL, , "data is NULL");

	noti_node_item *item = noti_list_item_data->data;
	retif(item == NULL, , "data is NULL");

	noti = item->noti;
	retif(noti == NULL, , "noti is NULL");

	/* Set text domain */
	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL)
		bindtextdomain(domain, dir);

	/* Get pkgname & id */
	notification_get_pkgname(noti, &pkgname);
	notification_get_application(noti, &caller_pkgname);
	notification_get_id(noti, &group_id, &priv_id);
	notification_get_type(noti, &type);
	notification_get_size(noti, &size);
	notification_get_progress(noti, &percentage);
	notification_get_layout(noti, &layout);

	DBG("percentage:%f size:%f", percentage, size);

	if (percentage <= 0.0 && size <= 0.0) {
		isProgressBarEnabled = 0;
	}

	noti_err = notification_get_text(noti,
							NOTIFICATION_TEXT_TYPE_TITLE,
							&text);

	if (noti_err == NOTIFICATION_ERROR_NONE && text != NULL) {
		_set_text_to_part(noti_list_item, "elm.text.title", text);
	}

	noti_err = notification_get_text(noti,
			NOTIFICATION_TEXT_TYPE_CONTENT,
							&text);
	if (noti_err == NOTIFICATION_ERROR_NONE && text != NULL) {
		if (layout == NOTIFICATION_LY_ONGOING_EVENT) {
			_set_text_to_part(noti_list_item, "elm.text.content.multiline", text);
			elm_object_signal_emit(noti_list_item, "elm,state,elm.text.content.multiline,active", "elm");
		} else {
			_set_text_to_part(noti_list_item, "elm.text.content", text);
			elm_object_signal_emit(noti_list_item, "elm,state,elm.text.content,active", "elm");
		}
	}

	if (isProgressBarEnabled != 0
			&& layout != NOTIFICATION_LY_ONGOING_EVENT) {
		text = _noti_get_progress(noti, buf,
									  sizeof(buf));
		if (text != NULL) {
			_set_text_to_part(noti_list_item, "elm.text.time", text);
		}
	}
}

static void _noti_list_item_call_item_cb(Evas_Object *noti_list_item, const char *emission) {
	retif(noti_list_item == NULL, , "invalid parameter");
	retif(emission == NULL, , "invalid parameter");

	DBG("%s", emission);

	void (*cb)(void *data, Evas_Object *obj) = NULL;
	noti_list_item_h *data = NULL;

	data = evas_object_data_get(noti_list_item, E_DATA_NOTI_LIST_ITEM_H);

	if (strncmp(emission,"selected", strlen("selected")) == 0) {
		cb = evas_object_data_get(noti_list_item, E_DATA_NOTI_LIST_CB_SELECTED_ITEM);

		if (cb != NULL && data != NULL) {
			cb(data->data, noti_list_item);
		}
	}
	if (strncmp(emission,"button_1", strlen("button_1")) == 0) {
		cb = evas_object_data_get(noti_list_item, E_DATA_NOTI_LIST_CB_BUTTON_1);

		if (cb != NULL && data != NULL) {
			cb(data->data, noti_list_item);
		}
	}
	if (strncmp(emission,"deleted", strlen("deleted")) == 0) {
		cb = evas_object_data_get(noti_list_item, E_DATA_NOTI_LIST_CB_DELETED_ITEM);

		if (cb != NULL && data != NULL) {
			cb(data->data, noti_list_item);
		}
	}
}

static void _signal_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
	retif(data == NULL, , "invalid parameter");
	retif(o == NULL, , "invalid parameter");
	retif(emission == NULL, , "invalid parameter");

	_noti_list_item_call_item_cb(o, emission);
}

Evas_Object *noti_list_item_create(Evas_Object *parent, notification_ly_type_e layout) {
	Evas_Object *box = NULL;

	retif(parent == NULL, NULL, "Invalid parameter!");

	box = elm_layout_add(parent);

	DBG("");
	elm_layout_file_set(box, DEFAULT_EDJ,
			"quickpanel/listitem/default");

	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(box);

	noti_list_item_h *box_h = (noti_list_item_h *) malloc(sizeof(noti_list_item_h));
	box_h->layout = layout;
	box_h->status = STATE_NORMAL;
	box_h->data = NULL;
	evas_object_data_set(box, E_DATA_NOTI_LIST_ITEM_H, box_h);
	DBG("created box:%p", box);

	//add event
	elm_object_signal_callback_add(box,
			"selected",
			"edje",
			_signal_cb,
			parent
	);

	//add event
	elm_object_signal_callback_add(box,
			"button_1",
			"edje",
			_signal_cb,
			parent
	);

	//add event
	elm_object_signal_callback_add(box,
			"deleted",
			"edje",
			_signal_cb,
			parent
	);

	return box;
}

static void _noti_list_item_set_layout_ongoing_noti(Evas_Object *noti_list_item,
		notification_h noti) {
	DBG("");
	retif(noti_list_item == NULL, , "invalid parameter");

	_noti_list_item_ongoing_set_progressbar(noti_list_item);
	_noti_list_item_ongoing_set_icon(noti_list_item);
	_noti_list_item_ongoing_set_text(noti_list_item);
}

static void _noti_list_item_set_layout(Evas_Object *noti_list_item, notification_h noti,
		notification_ly_type_e layout) {

	DBG("layout:%d", layout);

	switch (layout) {
		case NOTIFICATION_LY_NOTI_EVENT_SINGLE:
			break;
		case NOTIFICATION_LY_NOTI_EVENT_MULTIPLE:
			break;
		case NOTIFICATION_LY_NOTI_THUMBNAIL:
			break;
		case NOTIFICATION_LY_NONE:
			break;
		case NOTIFICATION_LY_ONGOING_EVENT:
		case NOTIFICATION_LY_ONGOING_PROGRESS:
			_noti_list_item_set_layout_ongoing_noti(noti_list_item, noti);
			break;
		case NOTIFICATION_LY_MAX:
			DBG("not supported layout type:%d", layout);
			break;
	}
}

void noti_list_item_remove(Evas_Object *noti_list_item) {

	retif(noti_list_item == NULL, , "invalid parameter");

	noti_list_item_h *noti_list_item_h = evas_object_data_get(noti_list_item, E_DATA_NOTI_LIST_ITEM_H);

	if (noti_list_item_h != NULL)
		free(noti_list_item_h);

	evas_object_data_del(noti_list_item, E_DATA_NOTI_LIST_ITEM_H);
	evas_object_data_del(noti_list_item, E_DATA_NOTI_LIST_CB_SELECTED_ITEM);
	evas_object_data_del(noti_list_item, E_DATA_NOTI_LIST_CB_BUTTON_1);
	evas_object_data_del(noti_list_item, E_DATA_NOTI_LIST_CB_DELETED_ITEM);

	evas_object_del(noti_list_item);
}

void noti_list_item_update(Evas_Object *noti_list_item) {
	retif(noti_list_item == NULL, , "invalid parameter");

	_noti_list_item_ongoing_set_progressbar(noti_list_item);
	_noti_list_item_ongoing_set_icon(noti_list_item);
	_noti_list_item_ongoing_set_text(noti_list_item);
}

void noti_list_item_set_status(Evas_Object *noti_list_item, int status) {
	retif(noti_list_item == NULL, , "invalid parameter");

	noti_list_item_h *noti_list_item_h = evas_object_data_get(noti_list_item, E_DATA_NOTI_LIST_ITEM_H);

	if (noti_list_item_h != NULL) {
		noti_list_item_h->status = status;
	}
}

int noti_list_item_get_status(Evas_Object *noti_list_item) {
	retif(noti_list_item == NULL, STATE_NORMAL, "invalid parameter");

	noti_list_item_h *noti_list_item_h = evas_object_data_get(noti_list_item, E_DATA_NOTI_LIST_ITEM_H);

	if (noti_list_item_h != NULL) {
		return noti_list_item_h->status;
	}

	return STATE_DELETING;
}

void noti_list_item_node_set(Evas_Object *noti_list_item, void *data) {
	retif(noti_list_item == NULL, , "invalid parameter");
	retif(data == NULL, , "invalid parameter");

	noti_list_item_h *noti_list_item_data = evas_object_data_get(noti_list_item, E_DATA_NOTI_LIST_ITEM_H);

	if (noti_list_item_data != NULL) {
		noti_list_item_data->data = data;

		if (data != NULL) {
			noti_node_item *item = data;
			_noti_list_item_set_layout(noti_list_item, item->noti, noti_list_item_data->layout);
		}
	}
}

void *noti_list_item_node_get(Evas_Object *noti_list_item) {
	retif(noti_list_item == NULL, NULL, "invalid parameter");

	noti_list_item_h *noti_list_item_data = evas_object_data_get(noti_list_item, E_DATA_NOTI_LIST_ITEM_H);

	if (noti_list_item_data != NULL) {
		return noti_list_item_data->data;
	}

	return NULL;
}

void noti_list_item_set_item_selected_cb(Evas_Object *noti_list_item,
		void(*selected_cb)(void *data, Evas_Object *obj)) {
	retif(noti_list_item == NULL, , "invalid parameter");
	retif(selected_cb == NULL, , "invalid parameter");

	evas_object_data_set(noti_list_item, E_DATA_NOTI_LIST_CB_SELECTED_ITEM, selected_cb);
}

void noti_list_item_set_item_button_1_cb(Evas_Object *noti_list_item,
		void(*button_1_cb)(void *data, Evas_Object *obj)) {
	retif(noti_list_item == NULL, , "invalid parameter");
	retif(button_1_cb == NULL, , "invalid parameter");

	evas_object_data_set(noti_list_item, E_DATA_NOTI_LIST_CB_BUTTON_1, button_1_cb);
}

void noti_list_item_set_item_deleted_cb(Evas_Object *noti_list_item,
		void(*deleted_cb)(void *data, Evas_Object *obj)) {
	retif(noti_list_item == NULL, , "invalid parameter");
	retif(deleted_cb == NULL, , "invalid parameter");

	evas_object_data_set(noti_list_item, E_DATA_NOTI_LIST_CB_DELETED_ITEM, deleted_cb);
}
