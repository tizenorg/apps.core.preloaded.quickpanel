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

#include <vconf.h>
#include <notification.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <system_settings.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "quickpanel_def.h"
#include "common_uic.h"
#include "common.h"
#include "noti_node.h"
#include "noti.h"
#include "list_util.h"
#include "noti_section.h"

#ifdef QP_SCREENREADER_ENABLE
#include "accessibility.h"
#endif

#define NOTI_CLEAR_ALL_SECTION "quickpanel/notisection/clear_all"
#define NOTI_DEFAULT_SECTION "quickpanel/notisection/default"

static void _noti_section_set_text(Evas_Object *noti_section, int count)
{
	char text[128] = { 0, };
	char *format;
	const char *old_text;

	if (!noti_section) {
		ERR("Invalid parameter");
		return;
	}

	DBG("count is : %d ", count);
	format = _("IDS_QP_HEADER_NOTIFICATIONS_HPD_ABB");
	snprintf(text, sizeof(text) - 1, format, count);

	old_text = elm_object_part_text_get(noti_section, "elm.text.notifications_number");
	if (old_text != NULL) {
		if (strcmp(old_text, text) == 0) {
			return;
		}
	}

#ifdef QP_SCREENREADER_ENABLE
	Evas_Object *ao;

	ao = quickpanel_accessibility_screen_reader_object_get(noti_section, SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus.label", noti_section);
	if (ao != NULL) {
		elm_access_info_set(ao, ELM_ACCESS_TYPE, "");
		elm_access_info_set(ao, ELM_ACCESS_INFO, text);
	}
#endif

	DBG("Trying to set text :%s ", text);
	elm_object_part_text_set(noti_section, "elm.text.notifications_number", text);
	elm_object_part_text_set(noti_section, "text.button.clear_all", _("IDS_QP_HEADER_CLEAR_ALL_ABB"));
}

HAPI Evas_Object *quickpanel_noti_section_create(Evas_Object *parent, qp_item_type_e type)
{
	Evas_Object *section;
	Evas_Object *focus;
	struct appdata *ad;
	qp_item_data *qid;
	Eina_Bool ret;

	ad = quickpanel_get_app_data();
	if (!ad || !parent) {
		ERR("Invalid parameter");
		return NULL;
	}

	section = elm_layout_add(parent);
	ret = elm_layout_file_set(section, DEFAULT_EDJ,	NOTI_CLEAR_ALL_SECTION);
	if (ret == EINA_FALSE) {
		ERR("Failed to set a file");
		evas_object_del(section);
		return NULL;
	}

	evas_object_size_hint_weight_set(section, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(section, EVAS_HINT_FILL, EVAS_HINT_FILL);
	quickpanel_uic_initial_resize(section, QP_THEME_LIST_ITEM_NOTI_SECTION_HEIGHT);
	evas_object_show(section);

	qid = quickpanel_list_util_item_new(type, NULL);
	if (!qid) {
		ERR("Unable to create a qid");
		evas_object_del(section);
		return NULL;
	}
	quickpanel_list_util_item_set_tag(section, qid);
	quickpanel_list_util_sort_insert(ad->list, section);

	focus = quickpanel_accessibility_ui_get_focus_object(section);
	if (!focus) {
		ERR("Unable to get the focus object");
		quickpanel_list_util_item_del(qid);
		evas_object_del(section);
		return NULL;
	}
	elm_object_part_content_set(section, "focus", focus);
	evas_object_smart_callback_add(focus, "clicked", quickpanel_noti_on_clear_all_clicked, NULL);

	return section;
}

static void _focus_pair_set(Evas_Object *view)
{
	Evas_Object *label = NULL;
	Evas_Object *button = NULL;
	retif(view == NULL, , "Invalid parameter!");

	label = elm_object_part_content_get(view, "focus.label");
	button = elm_object_part_content_get(view, "elm.swallow.icon");

	if (label != NULL && button != NULL) {
		/* label */
		elm_object_focus_next_object_set(label, button, ELM_FOCUS_RIGHT);
		elm_object_focus_next_object_set(label, button, ELM_FOCUS_DOWN);

		/* button */
		elm_object_focus_next_object_set(button, label, ELM_FOCUS_LEFT);
		elm_object_focus_next_object_set(button, label, ELM_FOCUS_UP);
	}
}

HAPI void quickpanel_noti_section_update(Evas_Object *noti_section, int noti_count)
{
	retif(noti_section == NULL, , "invalid parameter");

	_noti_section_set_text(noti_section, noti_count);
	_focus_pair_set(noti_section);

	quickpanel_noti_set_clear_all_status();
}

HAPI void quickpanel_noti_section_set_deleted_cb(Evas_Object *noti_section,
		Evas_Object_Event_Cb func, void *data)
{
	retif(noti_section == NULL, , "invalid parameter");
	retif(func == NULL, , "invalid parameter");

	evas_object_event_callback_add(noti_section, EVAS_CALLBACK_DEL, func, data);
}

HAPI void quickpanel_noti_section_remove(Evas_Object *noti_section)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid parameter");
	retif(noti_section == NULL, , "invalid parameter");

	quickpanel_list_util_item_del_tag(noti_section);
	quickpanel_list_util_item_unpack_by_object(ad->list, noti_section, 0, 0);
}
