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
#include "noti_box.h"
#include "noti_node.h"
#include "noti.h"

static void _noti_box_call_item_cb(Evas_Object *noti_box, const char *emission) {
	retif(noti_box == NULL, , "invalid parameter");
	retif(emission == NULL, , "invalid parameter");

	DBG("%s", emission);

	void (*cb)(void *data, Evas_Object *obj) = NULL;
	noti_box_h *data = NULL;

	data = evas_object_data_get(noti_box, E_DATA_NOTI_BOX_H);

	if (strncmp(emission,"selected", strlen("selected")) == 0) {
		cb = evas_object_data_get(noti_box, E_DATA_CB_SELECTED_ITEM);

		if (cb != NULL && data != NULL) {
			cb(data->data, noti_box);
		}
	}
	if (strncmp(emission,"button_1", strlen("button_1")) == 0) {
		cb = evas_object_data_get(noti_box, E_DATA_CB_BUTTON_1);

		if (cb != NULL && data != NULL) {
			cb(data->data, noti_box);
		}
	}
	if (strncmp(emission,"deleted", strlen("deleted")) == 0) {
		cb = evas_object_data_get(noti_box, E_DATA_CB_DELETED_ITEM);

		if (cb != NULL && data != NULL) {
			cb(data->data, noti_box);
		}
	}
}

static void _signal_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
	retif(data == NULL, , "invalid parameter");
	retif(o == NULL, , "invalid parameter");
	retif(emission == NULL, , "invalid parameter");

	_noti_box_call_item_cb(o, emission);
}

Evas_Object *noti_box_create(Evas_Object *parent, notification_ly_type_e layout) {
	Evas_Object *box = NULL;

	box = elm_layout_add(parent);

	DBG("");
	if (layout == NOTIFICATION_LY_NOTI_EVENT_SINGLE
			|| layout == NOTIFICATION_LY_NOTI_EVENT_MULTIPLE) {
		elm_layout_file_set(box, DEFAULT_EDJ,
				"quickpanel/notibox/single_multi");
	} else if (layout == NOTIFICATION_LY_NOTI_THUMBNAIL) {
		elm_layout_file_set(box, DEFAULT_EDJ, "quickpanel/notibox/thumbnail");
	} else {
		elm_layout_file_set(box, DEFAULT_EDJ,
				"quickpanel/notibox/single_multi");
	}

	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(box);

	noti_box_h *box_h = (noti_box_h *) malloc(sizeof(noti_box_h));
	box_h->layout = layout;
	box_h->status = STATE_NORMAL;
	box_h->data = NULL;
	evas_object_data_set(box, E_DATA_NOTI_BOX_H, box_h);
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

static void _set_image(Evas_Object *noti_box, notification_h noti,
		notification_image_type_e image_type, const char *part, int is_stretch) {

	DBG("");

	char *image = NULL;

	notification_get_image(noti, image_type, &image);

	if (image != NULL) {
		Evas_Object *content = NULL;
		content = elm_image_add(noti_box);
		elm_image_file_set(content, image, NULL);
		if (is_stretch == 1) {
			elm_image_aspect_fixed_set(content, EINA_FALSE);
			elm_image_resizable_set(content, EINA_TRUE, EINA_TRUE);
		}

		elm_object_part_content_set(noti_box, part, content);
		elm_object_signal_emit(noti_box, "object.show", part);
	}
}

static int _set_text(Evas_Object *noti_box, notification_h noti,
		notification_text_type_e text_type, const char *part, int is_need_effect) {
	char buf[128] = { 0, };

	char *text = NULL;
	time_t time = 0;

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
			elm_object_part_text_set(noti_box, part, text);
			if (is_need_effect == 1)
				elm_object_signal_emit(noti_box, "object.show.effect", part);
			else
				elm_object_signal_emit(noti_box, "object.show", part);
		}

		return strlen(text);
	}

	return 0;
}

static int _check_text_null(notification_h noti,
		notification_text_type_e text_type) {
	DBG("");

	char *text = NULL;

	notification_get_text(noti, text_type, &text);

	if (text == NULL) {
		return 1;
	}

	return 0;
}

static int _check_image_null(notification_h noti,
		notification_image_type_e image_type) {
	DBG("");

	char *image = NULL;

	notification_get_image(noti, image_type, &image);

	if (image == NULL) {
		return 1;
	}

	if (strncasecmp(image, "(null)", strlen(image)) == 0) {
		return 1;
	}

	return 0;
}

static void _noti_box_set_layout_single(Evas_Object *noti_box,
		notification_h noti) {
	DBG("");

	char *dir = NULL;
	char *domain = NULL;
	int is_need_effect = 0;
	int is_contents_only = 0;
	int is_sub_info_1_only = 0;

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 0) {
		is_need_effect = 1;
	}

	if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_1) == 1
		&& _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1) == 1
		&& _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_2) == 1
		&& _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2) == 1) {
		is_contents_only = 1;
	}

	if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_1) != 1
		&& _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1) == 1
		&& _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_2) == 1
		&& _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2) == 1) {
		is_sub_info_1_only = 1;
	}

	DBG("is_contents_only:%d is_sub_info_1_only:%d", is_contents_only, is_sub_info_1_only);

	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL)
		bindtextdomain(domain, dir);

	_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_TITLE,
			"object.text.title", is_need_effect);

	if (is_contents_only == 1) {
		_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT,
				"object.text.contents.multiline", is_need_effect);
	} else {
		_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT,
				"object.text.contents", is_need_effect);

		if (is_sub_info_1_only == 1) {
			_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
										"object.text.info.1.multiline", is_need_effect);
		} else {
			if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_1) == 0) {
				if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1) == 1) {
					_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
							"object.text.info.1", is_need_effect);
				} else {
					_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
							"object.text.info.1.short", is_need_effect);
					_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1,
							"object.text.info.sub.1", is_need_effect);
				}
			}
			_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_2,
					"object.text.info.2", is_need_effect);
		}
	}

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 0) {
		_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_ICON,
				"object.icon.sub", 1);
		_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL,
				"object.icon", 1);
	} else {
		_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_ICON,
				"object.icon", 1);
		_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB,
				"object.icon.sub", 1);
	}
	_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND,
			"object.icon.background", 1);

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 0) {
		elm_object_signal_emit(noti_box, "box.show.dim", "box.prog");
	}
	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON) == 1
			&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 1) {
		elm_object_signal_emit(noti_box, "box.hide.icon.bg", "box.prog");
	}
	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB) == 0
			|| _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 0) {
		elm_object_signal_emit(noti_box, "box.show.sub.bg", "box.prog");
	}
}

static void _noti_box_set_layout_multi(Evas_Object *noti_box,
		notification_h noti) {
	DBG("");

	int length = 0;
	char *dir = NULL;
	char *domain = NULL;
	char buf[128] = {0,};
	int is_need_effect = 0;
	int is_sub_info_1_only = 0;

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 0) {
		is_need_effect = 1;
	}

	if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_1) != 1
		&& _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1) == 1
		&& _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_2) == 1
		&& _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2) == 1) {
		is_sub_info_1_only = 1;
	}

	DBG("is_sub_info_1_only:%d", is_sub_info_1_only);

	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL)
		bindtextdomain(domain, dir);

	_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_TITLE,
			"object.text.title", is_need_effect);
	if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT) == 0) {
		_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT,
				"object.text.contents.short", is_need_effect);
		length = _set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT,
				"object.text.count", is_need_effect);
		length = (length >= 5) ? 5 : length;
		snprintf(buf, sizeof(buf), "box.count.%d", length);
		elm_object_signal_emit(noti_box, buf, "box.prog");
	} else {
		_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT,
				"object.text.contents", is_need_effect);
	}

	if (is_sub_info_1_only == 1) {
		_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
									"object.text.info.1.multiline", is_need_effect);
	} else {
		if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_1) == 0) {
			if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1) == 1) {
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
						"object.text.info.1", is_need_effect);
			} else {
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
						"object.text.info.1.short", is_need_effect);
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1,
						"object.text.info.sub.1", is_need_effect);
			}
		}
		if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_2) == 0) {
			if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2) == 1) {
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_2,
						"object.text.info.2", is_need_effect);
			} else {
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_2,
						"object.text.info.2.short", is_need_effect);
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2,
						"object.text.info.sub.2", is_need_effect);
			}
		}
	}

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 0) {
		_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_ICON,
				"object.icon.sub", 1);
		_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL,
				"object.icon", 1);
	} else {
		_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_ICON,
				"object.icon", 1);
		_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB,
				"object.icon.sub", 1);
	}
	_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND,
			"object.icon.background", 1);
	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 0) {
		elm_object_signal_emit(noti_box, "box.show.dim", "box.prog");
	}
	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON) == 1
			&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 1) {
		elm_object_signal_emit(noti_box, "box.hide.icon.bg", "box.prog");
	}
	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB) == 0
			|| _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 0) {
		elm_object_signal_emit(noti_box, "box.show.sub.bg", "box.prog");
	}
}

static void _noti_box_set_layout_thumbnail(Evas_Object *noti_box,
		notification_h noti) {
	DBG("");

	char *dir = NULL;
	char *domain = NULL;
	int is_need_effect = 0;

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 0)
		is_need_effect = 1;
	else
		is_need_effect = 0;

	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL)
		bindtextdomain(domain, dir);

	_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_TITLE,
			"object.text.title", is_need_effect);
	_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT,
			"object.text.contents", is_need_effect);

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 0) {
		_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_ICON,
				"object.icon.sub", 0);
		_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL,
				"object.icon", 0);
	} else {
		_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_ICON,
				"object.icon", 0);
		_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB,
				"object.icon.sub", 0);
	}
	_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND,
			"object.icon.background", 1);

	_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_LIST_1,
			"object.thumbnail.list.1", 1);
	_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_LIST_2,
			"object.thumbnail.list.2", 1);
	_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_LIST_3,
			"object.thumbnail.list.3", 1);
	_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_LIST_4,
			"object.thumbnail.list.4", 1);
	_set_image(noti_box, noti, NOTIFICATION_IMAGE_TYPE_LIST_5,
			"object.thumbnail.list.5", 1);

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 0) {
		elm_object_signal_emit(noti_box, "box.show.dim", "box.prog");
	}
	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON) == 1
			&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 1) {
		elm_object_signal_emit(noti_box, "box.hide.icon.bg", "box.prog");
	}
	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB) == 0
			|| _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 0) {
		elm_object_signal_emit(noti_box, "box.show.sub.bg", "box.prog");
	}
}

static void _noti_box_set_layout(Evas_Object *noti_box, notification_h noti,
		notification_ly_type_e layout) {

	DBG("layout:%d", layout);

	switch (layout) {
		case NOTIFICATION_LY_NOTI_EVENT_SINGLE:
			_noti_box_set_layout_single(noti_box, noti);
			break;
		case NOTIFICATION_LY_NOTI_EVENT_MULTIPLE:
			_noti_box_set_layout_multi(noti_box, noti);
			break;
		case NOTIFICATION_LY_NOTI_THUMBNAIL:
			_noti_box_set_layout_thumbnail(noti_box, noti);
			break;
		case NOTIFICATION_LY_NONE:
		case NOTIFICATION_LY_ONGOING_EVENT:
		case NOTIFICATION_LY_ONGOING_PROGRESS:
		case NOTIFICATION_LY_MAX:
			DBG("not supported layout type:%d", layout);
			break;
	}
}

void noti_box_remove(Evas_Object *noti_box) {

	retif(noti_box == NULL, , "invalid parameter");

	noti_box_h *noti_box_h = evas_object_data_get(noti_box, E_DATA_NOTI_BOX_H);

	if (noti_box_h != NULL)
		free(noti_box_h);

	evas_object_data_del(noti_box, E_DATA_NOTI_BOX_H);
	evas_object_data_del(noti_box, E_DATA_CB_SELECTED_ITEM);
	evas_object_data_del(noti_box, E_DATA_CB_DELETED_ITEM);

	evas_object_del(noti_box);
}

void noti_box_set_status(Evas_Object *noti_box, int status) {
	retif(noti_box == NULL, , "invalid parameter");

	noti_box_h *noti_box_h = evas_object_data_get(noti_box, E_DATA_NOTI_BOX_H);

	if (noti_box_h != NULL) {
		noti_box_h->status = status;
	}
}

int noti_box_get_status(Evas_Object *noti_box) {
	retif(noti_box == NULL, STATE_NORMAL, "invalid parameter");

	noti_box_h *noti_box_h = evas_object_data_get(noti_box, E_DATA_NOTI_BOX_H);

	if (noti_box_h != NULL) {
		return noti_box_h->status;
	}

	return STATE_DELETING;
}

void noti_box_node_set(Evas_Object *noti_box, void *data) {
	retif(noti_box == NULL, , "invalid parameter");
	retif(data == NULL, , "invalid parameter");

	noti_box_h *noti_box_data = evas_object_data_get(noti_box, E_DATA_NOTI_BOX_H);

	if (noti_box_data != NULL) {
		noti_box_data->data = data;

		if (data != NULL) {
			noti_node_item *item = data;
			_noti_box_set_layout(noti_box, item->noti, noti_box_data->layout);
		}
	}
}

void *noti_box_node_get(Evas_Object *noti_box) {
	retif(noti_box == NULL, NULL, "invalid parameter");

	noti_box_h *noti_box_data = evas_object_data_get(noti_box, E_DATA_NOTI_BOX_H);

	if (noti_box_data != NULL) {
		return noti_box_data->data;
	}

	return NULL;
}

void noti_box_set_item_selected_cb(Evas_Object *noti_box,
		void(*selected_cb)(void *data, Evas_Object *obj)) {
	retif(noti_box == NULL, , "invalid parameter");
	retif(selected_cb == NULL, , "invalid parameter");

	evas_object_data_set(noti_box, E_DATA_CB_SELECTED_ITEM, selected_cb);
}

void noti_box_set_item_button_1_cb(Evas_Object *noti_box,
		void(*button_1_cb)(void *data, Evas_Object *obj)) {
	retif(noti_box == NULL, , "invalid parameter");
	retif(button_1_cb == NULL, , "invalid parameter");

	evas_object_data_set(noti_box, E_DATA_CB_BUTTON_1, button_1_cb);
}

void noti_box_set_item_deleted_cb(Evas_Object *noti_box,
		void(*deleted_cb)(void *data, Evas_Object *obj)) {
	retif(noti_box == NULL, , "invalid parameter");
	retif(deleted_cb == NULL, , "invalid parameter");

	evas_object_data_set(noti_box, E_DATA_CB_DELETED_ITEM, deleted_cb);
}
