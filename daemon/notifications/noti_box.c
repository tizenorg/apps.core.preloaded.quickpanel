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


#include <string.h>
#include <notification.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "list_util.h"
#include "quickpanel_def.h"
#include "noti_box.h"
#include "noti_node.h"
#include "noti.h"
#include "noti_util.h"
#ifdef QP_SCREENREADER_ENABLE
#include "accessibility.h"
#endif
#ifdef QP_ANIMATED_IMAGE_ENABLE
#include "animated_image.h"
#endif
#include "animated_icon.h"

#define IMAGE_NO_RESIZE 0
#define IMAGE_RESIZE 1

#define IMAGE_BY_FILE 0
#define IMAGE_BY_BUFFER 1

#define TEXT_NO_CR 0
#define TEXT_CR 1

#define THRESHOLD_DRAGGING_TIME_LIMIT 1.0
#define LIMIT_ZOOM_RATIO 0.55
#define LIMIT_FADEOUT_RATIO 0.1
#define THRESHOLD_DELETE_START 80
#define THRESHOLD_DELETE_START_Y_LIMIT 60
#define THRESHOLD_DISTANCE ((BOX_WIDTH_P >> 1))

static struct _info {
	int box_debug_step;
} s_info = {
	.box_debug_step = 0,
};

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

static void _noti_box_call_item_cb(Evas_Object *noti_box, const char *emission)
{
	retif(noti_box == NULL, , "invalid parameter");
	retif(emission == NULL, , "invalid parameter");

	double time_current = 0.0;
	static double time_called = 0.0;

	if (time_called == 0) {
		time_called = ecore_loop_time_get();
	} else {
		time_current = ecore_loop_time_get();
		if ((time_current - time_called) < 0.4) {
			DBG("click rejected");
			return;
		}
	}

	void (*cb)(void *data, Evas_Object *obj) = NULL;
	noti_box_h *data = NULL;
	data = evas_object_data_get(noti_box, E_DATA_NOTI_BOX_H);
	retif(data == NULL, , "invalid parameter");

	if (strncmp(emission,"selected", strlen("selected")) == 0) {
		if (data->need_to_cancel_press > 0) {
			data->need_to_cancel_press = 0;
			return;
		}

		cb = evas_object_data_get(noti_box, E_DATA_CB_SELECTED_ITEM);

		if (cb != NULL) {
			cb(data->noti_node, noti_box);
		}
		time_called = time_current;
	} else if (strncmp(emission,"button_1", strlen("button_1")) == 0) {
		if (data->need_to_cancel_press > 0) {
			data->need_to_cancel_press = 0;
			return;
		}

		cb = evas_object_data_get(noti_box, E_DATA_CB_BUTTON_1);

		if (cb != NULL) {
			cb(data->noti_node, noti_box);
		}
		time_called = time_current;
	} else if (strncmp(emission,"deleted", strlen("deleted")) == 0) {
		data->need_to_cancel_press = 0;
		cb = evas_object_data_get(noti_box, E_DATA_CB_DELETED_ITEM);

		if (cb != NULL && data != NULL) {
			cb(data->noti_node, noti_box);
		}
	}
}

noti_box_h *_box_handler_get(Evas_Object *obj)
{

	return evas_object_data_get(obj, E_DATA_NOTI_BOX_H);
}

static void _attach_memfile(Evas_Object *noti_box, notification_image_type_e image_type, void *memfile)
{
	char buf[32] = {0,};

	snprintf(buf, sizeof(buf), "%s_%d", E_DATA_NOTI_BOX_MB_BG, image_type);

	void *memfile_attached = evas_object_data_get(noti_box, buf);
	if (memfile_attached != NULL) {
		free(memfile_attached);
	}
	evas_object_data_set(noti_box, buf, memfile);
}

static void _deattach_memfile_all(Evas_Object *noti_box)
{
	char buf[32] = {0,};
	void *memfile = NULL;
	int i = NOTIFICATION_TEXT_TYPE_NONE + 1;

	for ( ; i < NOTIFICATION_TEXT_TYPE_MAX; i++) {
		snprintf(buf, sizeof(buf), "%s_%d", E_DATA_NOTI_BOX_MB_BG, i);

		memfile = evas_object_data_get(noti_box, buf);
		if (memfile != NULL) {
			free(memfile);
		}
		evas_object_data_set(noti_box, buf, NULL);
		evas_object_data_del(noti_box, buf);
	}
}

static void _text_clean_all(Evas_Object *noti_box)
{
	int i = 0;
	const char *text_parts[] = {
		"object.text.title",
		"object.text.contents",
		"object.text.contents.multiline.short",
		"object.text.contents.multiline",
		"object.text.count",
		"object.text.info.1",
		"object.text.info.1.short",
		"object.text.info.1.multiline",
		"object.text.info.sub.1",
		"object.text.info.2",
		"object.text.info.2.short",
		"object.text.info.sub.2",
		NULL
	};

	for (i = 0; text_parts[i] != NULL; i++) {
		elm_object_part_text_set(noti_box, text_parts[i], "");
		elm_object_part_text_set(noti_box, text_parts[i], NULL);
	}
}

static void _signal_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
	retif(data == NULL, , "invalid parameter");
	retif(o == NULL, , "invalid parameter");
	retif(emission == NULL, , "invalid parameter");

	_noti_box_call_item_cb(o, emission);
}

#ifdef QP_SCREENREADER_ENABLE
static inline void _check_and_add_to_buffer(notification_h noti,
		notification_text_type_e text_type, Eina_Strbuf *str_buf)
{
	char buf[256] = { 0, };
	char buf_number[QP_UTIL_PHONE_NUMBER_MAX_LEN * 2] = { 0, };

	char *text = NULL;
	time_t time = 0;

	if (notification_get_time_from_text(noti, text_type, &time) == NOTIFICATION_ERROR_NONE) {
		if ((int)time > 0) {
			quickpanel_noti_util_get_time(time, buf, sizeof(buf));
			text = buf;
		}
	} else {
		notification_get_text(noti, text_type, &text);
	}

	if (text != NULL) {
		if (strlen(text) > 0) {
			if (quickpanel_common_util_is_phone_number(text)) {
				quickpanel_common_util_phone_number_tts_make(buf_number, text,
						(QP_UTIL_PHONE_NUMBER_MAX_LEN * 2) - 1);
				DBG("[%s]", buf_number);
				eina_strbuf_append(str_buf, buf_number);
			} else {
				eina_strbuf_append(str_buf, text);
			}
			eina_strbuf_append_char(str_buf, '\n');
		}
	}
}

static void _noti_box_set_rs_layout_single(Evas_Object *noti_box,
		notification_h noti)
{
	Evas_Object *ao = NULL;
	Eina_Strbuf *str_buf = NULL;
	char *dir = NULL;
	char *domain = NULL;

	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL) {
		bindtextdomain(domain, dir);
	}

	str_buf = eina_strbuf_new();
	retif(str_buf == NULL, , "invalid parameter");

	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_TITLE, str_buf);
	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_CONTENT, str_buf);
	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_INFO_1, str_buf);
	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1, str_buf);
	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_INFO_2, str_buf);
	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2, str_buf);

	if (str_buf != NULL) {
		ao = quickpanel_accessibility_screen_reader_object_get(noti_box,
				SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", noti_box);
		if (ao != NULL) {
			elm_access_info_set(ao, ELM_ACCESS_TYPE, _("IDS_QP_BUTTON_NOTIFICATION"));
			elm_access_info_set(ao, ELM_ACCESS_INFO, eina_strbuf_string_get(str_buf));
		}

		eina_strbuf_free(str_buf);
	}
}

static void _noti_box_set_rs_layout_multi(Evas_Object *noti_box,
		notification_h noti)
{
	DBG("");

	Evas_Object *ao = NULL;
	Eina_Strbuf *str_buf = NULL;
	char *dir = NULL;
	char *domain = NULL;

	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL) {
		bindtextdomain(domain, dir);
	}

	str_buf = eina_strbuf_new();
	retif(str_buf == NULL, , "invalid parameter");

	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_TITLE, str_buf);
	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_CONTENT, str_buf);
	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_INFO_1, str_buf);
	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1, str_buf);
	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_INFO_2, str_buf);
	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2, str_buf);

	if (str_buf != NULL) {
		ao = quickpanel_accessibility_screen_reader_object_get(noti_box,
				SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", noti_box);
		if (ao != NULL) {
			elm_access_info_set(ao, ELM_ACCESS_TYPE, _("IDS_QP_BUTTON_NOTIFICATION"));
			elm_access_info_set(ao, ELM_ACCESS_INFO, eina_strbuf_string_get(str_buf));
		}

		eina_strbuf_free(str_buf);
	}
}

static void _noti_box_set_rs_layout_thumbnail(Evas_Object *noti_box,
		notification_h noti)
{
	DBG("");

	Evas_Object *ao = NULL;
	Eina_Strbuf *str_buf = NULL;
	char *dir = NULL;
	char *domain = NULL;

	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL) {
		bindtextdomain(domain, dir);
	}

	str_buf = eina_strbuf_new();
	retif(str_buf == NULL, , "invalid parameter");

	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_TITLE, str_buf);
	_check_and_add_to_buffer(noti, NOTIFICATION_TEXT_TYPE_CONTENT, str_buf);

	if (str_buf != NULL) {
		DBG("access:%s", eina_strbuf_string_get(str_buf));

		ao = quickpanel_accessibility_screen_reader_object_get(noti_box,
				SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", noti_box);
		if (ao != NULL) {
			elm_access_info_set(ao, ELM_ACCESS_TYPE, _("IDS_QP_BUTTON_NOTIFICATION"));
			elm_access_info_set(ao, ELM_ACCESS_INFO, eina_strbuf_string_get(str_buf));
		}

		eina_strbuf_free(str_buf);
	}
}
#endif

static Eina_Bool _drag_cancel_cb(void *data)
{
	QP_VI *vi = data;
	noti_box_h *box_h = NULL;
	retif(vi == NULL, EINA_FALSE, "invalid parameter");

	if (vi->target != NULL) {
		DBG("Canceling dragging");

		box_h = _box_handler_get(vi->target);
		retif(box_h == NULL, EINA_FALSE, "box_h is NULL");

		box_h->state = NOTIBOX_STATE_GETSTURE_CANCELED;
		evas_object_map_enable_set(vi->target, EINA_FALSE);

		box_h->vi = NULL;
	}

	return EINA_TRUE;
}

static void _mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	int w = 0, h = 0;
	noti_box_h *box_h = NULL;
	Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)event_info;
	retif(ev == NULL, , "event_info is NULL");

	box_h = _box_handler_get(obj);
	retif(box_h == NULL, , "box_h is NULL");

	evas_object_geometry_get(obj, NULL, NULL, &w, &h);

	box_h->press_x = ev->canvas.x;
	box_h->press_y = ev->canvas.y;
	box_h->obj_w = w;
	box_h->obj_h = h;
	box_h->state = NOTIBOX_STATE_NORMAL;

	s_info.box_debug_step = 1;
	SDBG("mouse down:%d %d %d", box_h->obj_w, box_h->obj_h, box_h->state);

	if (box_h->vi != NULL) {
		quickpanel_vi_user_event_del(box_h->vi);
		box_h->vi = NULL;
	}

	box_h->need_to_cancel_press = 0;
}

static void _mouse_move_cb(void* data, Evas* e, Evas_Object* obj, void* event_info)
{
	int delta_x = 0;
	int delta_y = 0;
	static int delta_prev = -1;
	int x = 0, y = 0;
	int w = 0, h = 0;
	noti_box_h *box_h = NULL;
	double zoom_ratio = 0.0;
	double color_ratio = 0.0;
	Evas_Map *map = NULL;
	Evas_Event_Mouse_Move* ev = event_info;
	QP_VI *vi = NULL;
	retif(ev == NULL, , "event_info is NULL");

	box_h = _box_handler_get(obj);
	retif(box_h == NULL, , "box_h is NULL");

	if (box_h->state == NOTIBOX_STATE_GETSTURE_CANCELED) {
		DBG("deletion has been canceled");
		return;
	}

	evas_object_geometry_get(obj, &x, &y, &w, &h);
	delta_x = abs(box_h->press_x - ev->cur.output.x);
	delta_y = abs(box_h->press_y - ev->cur.output.y);

	if (s_info.box_debug_step == 1) {
		SDBG("mouse move:%d %d %d", delta_x, delta_y, box_h->state);
		s_info.box_debug_step = 2;
	}

	if (box_h->state == NOTIBOX_STATE_NORMAL) {
		if (delta_x >= THRESHOLD_DELETE_START
				&& delta_y <= THRESHOLD_DELETE_START_Y_LIMIT) {
			DBG("start a deletion");
			box_h->state = NOTIBOX_STATE_GETSTURE_WAIT;

			vi = quickpanel_vi_new_with_data(
					VI_OP_DELETE,
					QP_ITEM_TYPE_NOTI,
					NULL,
					obj,
					NULL,
					NULL,
					NULL,
					_drag_cancel_cb,
					vi,
					NULL,
					0,
					0);
			box_h->vi = vi;
			box_h->need_to_cancel_press = 1;
			quickpanel_vi_user_event_add(vi);
		}
	} else if (box_h->state == NOTIBOX_STATE_GETSTURE_WAIT) {
		if (delta_prev != delta_x) {
			delta_x = (delta_x > THRESHOLD_DISTANCE) ? THRESHOLD_DISTANCE : delta_x;
			delta_x = (delta_x <= 0) ? 1 : delta_x;
			zoom_ratio = (1.0 - LIMIT_ZOOM_RATIO) * (1.0 - (double)delta_x / (double)THRESHOLD_DISTANCE);
			color_ratio = LIMIT_FADEOUT_RATIO +
					((1.0 - LIMIT_FADEOUT_RATIO) * (1.0 - ((double)delta_x / (double)(THRESHOLD_DISTANCE))));

			map = evas_map_new(4);
			if (map != NULL) {
				evas_map_util_points_populate_from_object(map, obj);
				evas_map_util_zoom(map
						, LIMIT_ZOOM_RATIO + zoom_ratio
						, LIMIT_ZOOM_RATIO + zoom_ratio
						, x + (w >> 1)
						, y + (h >> 1));
				evas_map_util_points_color_set(map
						, 255 * color_ratio
						, 255 * color_ratio
						, 255 * color_ratio
						, 255 * color_ratio);
				evas_object_map_enable_set(obj, EINA_TRUE);
				evas_object_map_set(obj, map);
				evas_map_free(map);
				quickpanel_list_util_scroll_freeze_set(EINA_TRUE);
			}
			delta_prev = delta_x;
		}
	}

	box_h->distance = delta_x;
}

static void _mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	noti_box_h *box_h = NULL;
	box_h = _box_handler_get(obj);
	retif(box_h == NULL, , "box_h is NULL");

	quickpanel_list_util_scroll_freeze_set(EINA_FALSE);

	if (s_info.box_debug_step == 2) {
		SDBG("mouse up:%d", box_h->state);
		s_info.box_debug_step = 3;
	}

	if (box_h->state == NOTIBOX_STATE_GETSTURE_WAIT) {
		if (box_h->distance >= (THRESHOLD_DISTANCE - 10)) {
			_noti_box_call_item_cb(obj, "deleted");
		} else {
			evas_object_map_enable_set(obj, EINA_FALSE);
		}

		if (box_h->vi != NULL) {
			quickpanel_vi_user_event_del(box_h->vi);
			box_h->vi = NULL;
		}
	}

	box_h->state = NOTIBOX_STATE_NORMAL;
}

HAPI Evas_Object *quickpanel_noti_box_create(Evas_Object *parent, notification_ly_type_e layout)
{
	Evas_Object *box = NULL;

	box = elm_layout_add(parent);

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
	retif(box_h == NULL, NULL, "failed to allocate a memory");

	box_h->state = NOTIBOX_STATE_NORMAL;
	box_h->layout = layout;
	box_h->noti_node = NULL;
	evas_object_data_set(box, E_DATA_NOTI_BOX_H, box_h);

	Evas_Object *focus = quickpanel_accessibility_ui_get_focus_object(box);
	elm_object_part_content_set(box, "focus", focus);

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

	evas_object_event_callback_add(box, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, NULL);
	evas_object_event_callback_add(box, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move_cb, NULL);
	evas_object_event_callback_add(box, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, NULL);

	return box;
}

static Evas_Object *_set_image(Evas_Object *noti_box, notification_h noti, char *image_path,
		notification_image_type_e image_type, const char *part, int is_stretch, int is_use_buffer)
{
	Evas_Object *content = NULL;
	char *image = NULL;
	char ext[32] = {0,};
	void *memfile = NULL;
	size_t memfile_size = 0;
	retif(part == NULL, NULL,"invalid parameter");

	notification_get_image(noti, image_type, &image);
	if (image == NULL && image_path != NULL) {
		image = image_path;
	}

	if (image != NULL) {
		if (is_use_buffer == IMAGE_BY_BUFFER) {
			content = quickpanel_animated_icon_get(noti_box, image);
			if (content == NULL) {
				content = elm_image_add(noti_box);

				memfile = quickpanel_common_ui_get_buffer_from_image(image, &memfile_size, ext, sizeof(ext));
				if (memfile != NULL && memfile_size > 0) {
					_attach_memfile(noti_box, image_type, memfile);
					if (elm_image_memfile_set(content, memfile, memfile_size, ext,
							quickpanel_animated_image_get_groupname(image)) == EINA_FALSE) {
						ERR("failed to set memfile set");
						elm_image_file_set(content, image,
								quickpanel_animated_image_get_groupname(image));
					}
				} else {
					if (memfile) {
						free(memfile);	// due to prevent
					}
					elm_image_file_set(content, image,
							quickpanel_animated_image_get_groupname(image));
				}
			}
		} else {
			content = _check_duplicated_image_loading(noti_box, part, image);
			if (content == NULL) {
				content = quickpanel_animated_icon_get(noti_box, image);
				if (content == NULL) {
					content = elm_image_add(noti_box);

					elm_image_file_set(content, image,
							quickpanel_animated_image_get_groupname(image));
				}
			} else {
				return content;
			}
		}
		if (is_stretch == IMAGE_RESIZE) {
			elm_image_aspect_fixed_set(content, EINA_FALSE);
			elm_image_resizable_set(content, EINA_TRUE, EINA_TRUE);
		} else {
			if (strcmp(part, BOX_PART_ICON) == 0 || strcmp(part, BOX_PART_ICON_SUB) == 0) {
				elm_image_resizable_set(content, EINA_FALSE, EINA_TRUE);
			} else {
				elm_image_aspect_fixed_set(content, EINA_TRUE);
				elm_image_fill_outside_set(content, EINA_TRUE);
			}
		}

		elm_object_part_content_set(noti_box, part, content);
		elm_object_signal_emit(noti_box, "object.show", part);
	}

	return content;
}

static int _set_text(Evas_Object *noti_box, notification_h noti,
		notification_text_type_e text_type, const char *part, char *str, int is_need_effect, int is_support_cr)
{
	char buf[128] = { 0, };

	char *text = NULL;
	char *text_utf8 = NULL;
	time_t time = 0;

	if (str != NULL) {
		text = str;
	} else if (notification_get_time_from_text(noti, text_type, &time) == NOTIFICATION_ERROR_NONE) {
		if ((int)time > 0) {
			quickpanel_noti_util_get_time(time, buf, sizeof(buf));
			text = buf;
		}
	} else {
		notification_get_text(noti, text_type, &text);
	}

	if (text != NULL) {
		if (strlen(text) > 0) {

			if (is_support_cr == TEXT_CR) {
				text_utf8 = elm_entry_utf8_to_markup(text);
				if (text_utf8 != NULL) {
					elm_object_part_text_set(noti_box, part, text_utf8);
					free(text_utf8);
				} else {
					elm_object_part_text_set(noti_box, part, text);
				}
			} else {
				quickpanel_common_util_char_replace(text, _NEWLINE, _SPACE);
				elm_object_part_text_set(noti_box, part, text);
			}

			if (is_need_effect == 1) {
				elm_object_signal_emit(noti_box, "object.show.effect", part);
			} else {
				elm_object_signal_emit(noti_box, "object.show", part);
			}
		}

		return strlen(text);
	}

	return 0;
}

static int _check_text_null(notification_h noti,
		notification_text_type_e text_type)
{
	char *text = NULL;

	notification_get_text(noti, text_type, &text);

	if (text == NULL) {
		return 1;
	}

	return 0;
}

static int _check_image_null(notification_h noti,
		notification_image_type_e image_type)
{
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
		notification_h noti)
{
	char *dir = NULL;
	char *domain = NULL;
	char *pkgname = NULL;
	char *icon_path = NULL;
	int is_need_effect = 0;
	int is_contents_only = 0;
	int is_sub_info_1_only = 0;
	int is_contents_and_sub_info_2 = 0;
	Evas_Object *icon = NULL;

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

	if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_1) == 1
		&& _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1) == 1
		&& (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_2) != 1
		|| _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2) != 1)) {
		is_contents_and_sub_info_2 = 1;
	}

	DBG("is_contents_only:%d is_sub_info_1_only:%d", is_contents_only, is_sub_info_1_only);

	notification_get_pkgname(noti, &pkgname);
	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL) {
		bindtextdomain(domain, dir);
	}

	_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_TITLE,
			"object.text.title", NULL, is_need_effect, TEXT_CR);

	if (is_contents_only == 1) {
		_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT,
				"object.text.contents.multiline", NULL, is_need_effect, TEXT_CR);
	} else {
		if (is_contents_and_sub_info_2 == 1) {
			_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT,
					"object.text.contents.multiline.short", NULL, is_need_effect, TEXT_NO_CR);
		} else {
			_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT,
					"object.text.contents", NULL, is_need_effect, TEXT_NO_CR);
		}

		if (is_sub_info_1_only == 1) {
			_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
										"object.text.info.1.multiline", NULL, is_need_effect, TEXT_CR);
		} else {
			if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_1) == 0) {
				if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1) == 1) {
					_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
							"object.text.info.1", NULL, is_need_effect, TEXT_NO_CR);
				} else {
					_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
							"object.text.info.1.short", NULL, is_need_effect, TEXT_NO_CR);
					_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1,
							"object.text.info.sub.1", NULL, is_need_effect, TEXT_NO_CR);
				}
			}
			_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_2,
					"object.text.info.2", NULL, is_need_effect, TEXT_NO_CR);
		}
	}

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 0) {
		_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_ICON,
				"object.icon.sub", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
		icon = _set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_THUMBNAIL,
				"object.icon", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
#ifdef QP_ANIMATED_IMAGE_ENABLE
		quickpanel_animated_image_add(icon);
#endif
	} else {
		icon = _set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_ICON,
				"object.icon", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
#ifdef QP_ANIMATED_IMAGE_ENABLE
		quickpanel_animated_image_add(icon);
#endif
		_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_ICON_SUB,
				"object.icon.sub", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
	}
	_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_BACKGROUND,
			"object.icon.background", IMAGE_NO_RESIZE, IMAGE_BY_BUFFER);

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 1
			&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON) == 1
			&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB) == 1
			&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 1) {

		icon_path = quickpanel_common_ui_get_pkginfo_icon(pkgname);
		if (icon_path != NULL) {
			_set_image(noti_box, NULL,
					icon_path, NOTIFICATION_IMAGE_TYPE_ICON,
					"object.icon", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
			free(icon_path);
		}
	} else {
		if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 0) {
			elm_object_signal_emit(noti_box, "box.show.dim", "box.prog");
		}
		if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON) == 1
				&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 1) {
			elm_object_signal_emit(noti_box, "box.hide.icon.bg", "box.prog");
			elm_object_signal_emit(noti_box, "box.title.without.icon", "box.prog");
		}
		if (((_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON) == 0
				||  _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 0)
				&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB) == 0)) {
			elm_object_signal_emit(noti_box, "box.show.sub.bg", "box.prog");
		}
	}

#ifdef QP_SCREENREADER_ENABLE
	_noti_box_set_rs_layout_single(noti_box, noti);
#endif
}

static void _noti_box_set_layout_multi(Evas_Object *noti_box,
		notification_h noti)
{
	char *pkgname = NULL;
	char *icon_path = NULL;
	char *dir = NULL;
	char *domain = NULL;
	int is_need_effect = 0;
	int is_contents_only = 0;
	int is_sub_info_1_only = 0;
	int is_contents_and_sub_info_2 = 0;
	Evas_Object *icon = NULL;

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 0) {
		is_need_effect = 1;
	}

	if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT) == 1
	    && _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_1) == 1
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

	if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT) == 1
		&& _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_1) == 1
		&& _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1) == 1
		&& (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_2) != 1
		|| _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2) != 1)) {
		is_contents_and_sub_info_2 = 1;
	}

	DBG("is_sub_info_1_only:%d", is_sub_info_1_only);

	notification_get_pkgname(noti, &pkgname);
	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL) {
		bindtextdomain(domain, dir);
	}

	_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_TITLE,
			"object.text.title", NULL, is_need_effect, TEXT_CR);
	if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT) == 0) {
		_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT, "object.text.count", NULL,
						is_need_effect, TEXT_NO_CR);
		_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT, "object.text.contents", NULL,
						is_need_effect, TEXT_NO_CR);
	} else {
		if (is_contents_only == 1) {
			_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT,
					"object.text.contents.multiline", NULL, is_need_effect, TEXT_CR);
		} else if (is_contents_and_sub_info_2 == 1) {
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT,
						"object.text.contents.multiline.short", NULL, is_need_effect, TEXT_NO_CR);
		} else {
			_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT,
					"object.text.contents", NULL, is_need_effect, TEXT_NO_CR);
		}
	}

	if (is_sub_info_1_only == 1) {
		_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
									"object.text.info.1.multiline", NULL, is_need_effect, TEXT_CR);
	} else {
		if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_1) == 0) {
			if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1) == 1) {
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
						"object.text.info.1", NULL, is_need_effect, TEXT_NO_CR);
			} else {
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
						"object.text.info.1.short", NULL, is_need_effect, TEXT_NO_CR);
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1,
						"object.text.info.sub.1", NULL, is_need_effect, TEXT_NO_CR);
			}
		}
		if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_2) == 0) {
			if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2) == 1) {
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_2,
						"object.text.info.2", NULL, is_need_effect, TEXT_NO_CR);
			} else {
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_2,
						"object.text.info.2.short", NULL, is_need_effect, TEXT_NO_CR);
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2,
						"object.text.info.sub.2", NULL, is_need_effect, TEXT_NO_CR);
			}
		}
	}

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 0) {
		_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_ICON,
				"object.icon.sub", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
		icon = _set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_THUMBNAIL,
				"object.icon", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
#ifdef QP_ANIMATED_IMAGE_ENABLE
		quickpanel_animated_image_add(icon);
#endif
	} else {
		icon = _set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_ICON,
				"object.icon", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
#ifdef QP_ANIMATED_IMAGE_ENABLE
		quickpanel_animated_image_add(icon);
#endif
		_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_ICON_SUB,
				"object.icon.sub", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
	}
	_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_BACKGROUND,
			"object.icon.background", IMAGE_NO_RESIZE, IMAGE_BY_BUFFER);

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 1
			&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON) == 1
			&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB) == 1
			&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 1) {

		icon_path = quickpanel_common_ui_get_pkginfo_icon(pkgname);
		if (icon_path != NULL) {
			_set_image(noti_box, NULL,
					icon_path, NOTIFICATION_IMAGE_TYPE_ICON,
					"object.icon", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
			free(icon_path);
		}
	} else {
		if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 0) {
			elm_object_signal_emit(noti_box, "box.show.dim", "box.prog");
		}
		if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON) == 1
				&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 1) {
			elm_object_signal_emit(noti_box, "box.hide.icon.bg", "box.prog");
			elm_object_signal_emit(noti_box, "box.title.without.icon", "box.prog");
		}
		if (((_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON) == 0
				||  _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 0)
				&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB) == 0)) {
			elm_object_signal_emit(noti_box, "box.show.sub.bg", "box.prog");
		}
	}

#ifdef QP_SCREENREADER_ENABLE
	_noti_box_set_rs_layout_multi(noti_box, noti);
#endif
}

static void _noti_box_set_layout_thumbnail(Evas_Object *noti_box,
		notification_h noti)
{
	char *pkgname = NULL;
	char *icon_path = NULL;
	char *dir = NULL;
	char *domain = NULL;
	int is_need_effect = 0;
	int is_sub_info_1_only = 0;
	int is_show_info = 0;
	Evas_Object *icon = NULL;

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 0) {
		is_need_effect = 1;
	} else {
		is_need_effect = 0;
	}

	if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_1) != 1
		&& _check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_2) == 1) {
		is_sub_info_1_only = 1;
	}

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_LIST_1)!= 1
		&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_LIST_2) == 1
		&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_LIST_3) == 1
		&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_LIST_4) == 1
		&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_LIST_5) == 1) {
		is_show_info = 1;
	}

	notification_get_pkgname(noti, &pkgname);
	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL) {
		bindtextdomain(domain, dir);
	}

	_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_TITLE,
			"object.text.title", NULL, is_need_effect, TEXT_CR);
	if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT) == 0) {
		_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT, "object.text.count", NULL,
						is_need_effect, TEXT_NO_CR);
		_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT, "object.text.contents", NULL,
						is_need_effect, TEXT_NO_CR);
	} else {
		_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_CONTENT,
				"object.text.contents", NULL, is_need_effect, TEXT_NO_CR);
	}

	if (is_show_info == 1) {
		if (is_sub_info_1_only == 1) {
			_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
										"object.text.info.1.multiline", NULL, is_need_effect, TEXT_CR);
		} else {
			if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_1) == 0) {
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_1,
										"object.text.info.1", NULL, is_need_effect, TEXT_NO_CR);
			}
			if (_check_text_null(noti, NOTIFICATION_TEXT_TYPE_INFO_2) == 0) {
				_set_text(noti_box, noti, NOTIFICATION_TEXT_TYPE_INFO_2,
										"object.text.info.2", NULL, is_need_effect, TEXT_NO_CR);
			}
		}
	}

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 0) {
		icon = _set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_THUMBNAIL,
				"object.icon", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
#ifdef QP_ANIMATED_IMAGE_ENABLE
		quickpanel_animated_image_add(icon);
#endif
		if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 0) {
			_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_ICON,
					"object.icon.sub", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
		}
	} else {
		icon = _set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_ICON,
				"object.icon", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
#ifdef QP_ANIMATED_IMAGE_ENABLE
		quickpanel_animated_image_add(icon);
#endif
		if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON) == 0) {
			_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_ICON_SUB,
					"object.icon.sub", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
		}
	}
	_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_BACKGROUND,
			"object.icon.background", IMAGE_NO_RESIZE, IMAGE_BY_BUFFER);

	_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_LIST_1,
			"object.thumbnail.list.1", IMAGE_RESIZE, IMAGE_BY_BUFFER);
	_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_LIST_2,
			"object.thumbnail.list.2", IMAGE_RESIZE, IMAGE_BY_BUFFER);
	_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_LIST_3,
			"object.thumbnail.list.3", IMAGE_RESIZE, IMAGE_BY_BUFFER);
	_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_LIST_4,
			"object.thumbnail.list.4", IMAGE_RESIZE, IMAGE_BY_BUFFER);
	_set_image(noti_box, noti, NULL, NOTIFICATION_IMAGE_TYPE_LIST_5,
			"object.thumbnail.list.5", IMAGE_RESIZE, IMAGE_BY_BUFFER);

	if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 1
			&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON) == 1
			&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB) == 1
			&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 1) {

		icon_path = quickpanel_common_ui_get_pkginfo_icon(pkgname);
		if (icon_path != NULL) {
			_set_image(noti_box, NULL,
					icon_path, NOTIFICATION_IMAGE_TYPE_ICON,
					"object.icon", IMAGE_NO_RESIZE, IMAGE_BY_FILE);
			free(icon_path);
		}
	} else {
		if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND) == 0) {
			elm_object_signal_emit(noti_box, "box.show.dim", "box.prog");
		}
		if (_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON) == 1
				&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 1) {
			elm_object_signal_emit(noti_box, "box.hide.icon.bg", "box.prog");
			elm_object_signal_emit(noti_box, "box.title.without.icon", "box.prog");
		}
		if (((_check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON) == 0
				||  _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL) == 0)
				&& _check_image_null(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB) == 0)) {
			elm_object_signal_emit(noti_box, "box.show.sub.bg", "box.prog");
		}
	}

#ifdef QP_SCREENREADER_ENABLE
	_noti_box_set_rs_layout_thumbnail(noti_box, noti);
#endif
}

static void _noti_box_set_layout(Evas_Object *noti_box, notification_h noti,
		notification_ly_type_e layout)
{

	DBG("notification box layout:%d", layout);

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
		ERR("not supported layout type:%d", layout);
		break;
	}
}

HAPI void quickpanel_noti_box_remove(Evas_Object *noti_box)
{

	retif(noti_box == NULL, , "invalid parameter");

	noti_box_h *noti_box_h = evas_object_data_get(noti_box, E_DATA_NOTI_BOX_H);

	evas_object_data_del(noti_box, E_DATA_NOTI_BOX_H);
	if (noti_box_h != NULL) {
		free(noti_box_h);
	}

	evas_object_data_del(noti_box, E_DATA_CB_SELECTED_ITEM);
	evas_object_data_del(noti_box, E_DATA_CB_DELETED_ITEM);

	_deattach_memfile_all(noti_box);

	evas_object_del(noti_box);
	noti_box = NULL;
}

HAPI void quickpanel_noti_box_node_set(Evas_Object *noti_box, void *noti_node)
{
	int priv_id = -1;
	noti_node_item *item = noti_node;
	retif(noti_box == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");
	retif(item->noti == NULL, , "invalid parameter");

	noti_box_h *noti_box_data = evas_object_data_get(noti_box, E_DATA_NOTI_BOX_H);

	notification_get_id(item->noti, NULL, &priv_id);

	if (noti_box_data != NULL) {
		noti_box_data->noti_node = item;
		noti_box_data->priv_id = priv_id;

		_noti_box_set_layout(noti_box, item->noti, noti_box_data->layout);
	}
}

HAPI void *quickpanel_noti_box_node_get(Evas_Object *noti_box)
{
	retif(noti_box == NULL, NULL, "invalid parameter");

	noti_box_h *noti_box_data = evas_object_data_get(noti_box, E_DATA_NOTI_BOX_H);

	if (noti_box_data != NULL) {
		return noti_box_data->noti_node;
	}

	return NULL;
}

#ifdef QP_SCREENREADER_ENABLE
static void
_noti_box_focus_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *noti_box = data;
	retif(noti_box == NULL, , "invalid parameter");

	_noti_box_call_item_cb(noti_box, "selected");

}
#endif

HAPI void quickpanel_noti_box_set_item_selected_cb(Evas_Object *noti_box,
		void(*selected_cb)(void *data, Evas_Object *obj))
{
	retif(noti_box == NULL, , "invalid parameter");
	retif(selected_cb == NULL, , "invalid parameter");

	evas_object_data_set(noti_box, E_DATA_CB_SELECTED_ITEM, selected_cb);

#ifdef QP_SCREENREADER_ENABLE
	Evas_Object *ao = NULL;
	ao = quickpanel_accessibility_screen_reader_object_get(noti_box,
			SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", noti_box);
	if (ao != NULL) {
		evas_object_smart_callback_add(ao, "clicked", _noti_box_focus_selected_cb, noti_box);
	}
#endif
}

HAPI void quickpanel_noti_box_set_item_button_1_cb(Evas_Object *noti_box,
		void(*button_1_cb)(void *data, Evas_Object *obj))
{
	retif(noti_box == NULL, , "invalid parameter");
	retif(button_1_cb == NULL, , "invalid parameter");

	evas_object_data_set(noti_box, E_DATA_CB_BUTTON_1, button_1_cb);
}

HAPI void quickpanel_noti_box_set_item_deleted_cb(Evas_Object *noti_box,
		void(*deleted_cb)(void *data, Evas_Object *obj))
{
	retif(noti_box == NULL, , "invalid parameter");
	retif(deleted_cb == NULL, , "invalid parameter");

	evas_object_data_set(noti_box, E_DATA_CB_DELETED_ITEM, deleted_cb);
}

HAPI void quickpanel_noti_box_item_dragging_cancel(Evas_Object *noti_box)
{
	retif(noti_box == NULL, , "invalid parameter");

	evas_object_map_enable_set(noti_box, EINA_FALSE);
}

HAPI void quickpanel_noti_box_item_update(Evas_Object *noti_box)
{
	retif(noti_box == NULL, , "invalid parameter");

	noti_node_item *item = NULL;
	noti_box_h *noti_box_data = evas_object_data_get(noti_box, E_DATA_NOTI_BOX_H);

	if (noti_box_data != NULL) {
		if (noti_box_data->noti_node != NULL) {

			item = quickpanel_noti_node_get_by_priv_id(noti_box_data->priv_id);

			if (item != NULL) {
				_deattach_memfile_all(noti_box);
				_text_clean_all(noti_box);

				_noti_box_set_layout(noti_box, item->noti, noti_box_data->layout);
			}
		}
	}
}
