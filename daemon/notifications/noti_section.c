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

#include <notification.h>
#include "quickpanel-ui.h"
#include "quickpanel_def.h"
#include "common.h"
#include "noti.h"
#include "noti_section.h"
#include "list_util.h"

static void _noti_section_button_clicked_cb(void *data, Evas_Object * obj,
					void *event_info)
{
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	noti_err = notifiation_clear(NOTIFICATION_TYPE_NOTI);

	DBG("Clear Clicked : noti_err(%d)", noti_err);

	quickpanel_play_feedback();
}

static void _noti_section_set_button(Evas_Object *noti_section)
{
	Evas_Object *eo = NULL;
	Evas_Object *old_eo = NULL;

	DBG("");
	retif(noti_section == NULL, , "invalid parameter");

	old_eo = elm_object_part_content_get(noti_section, "elm.swallow.icon");

	if (old_eo == NULL) {
		eo = elm_button_add(noti_section);
		retif(eo == NULL, , "Failed to create clear button!");
		elm_object_style_set(eo, "quickpanel_standard");
		evas_object_smart_callback_add(eo, "clicked",
				_noti_section_button_clicked_cb, NULL);
		elm_object_part_content_set(noti_section, "elm.swallow.icon", eo);
	} else {
		eo = old_eo;
	}

	elm_object_text_set(eo, _S("IDS_COM_BODY_CLEAR_ALL"));
}

static void _noti_section_set_text(Evas_Object *noti_section, int count)
{
	char text[128] = {0,};
	char format[256] = {0,};
	const char *old_text = NULL;

	DBG("");
	retif(noti_section == NULL, , "invalid parameter");

	snprintf(format, sizeof(format), "%s %%d", _("IDS_QP_BODY_NOTIFICATIONS_ABB2"));
	snprintf(text, sizeof(text), format, count);

	old_text = elm_object_part_text_get(noti_section, "elm.text.text");
	if (old_text != NULL) {
		if (strcmp(old_text, text) == 0) {
			return ;
		}
	}

	elm_object_part_text_set(noti_section, "elm.text.text", text);
}

HAPI Evas_Object *noti_section_create(Evas_Object *parent) {
	Eina_Bool ret = EINA_FALSE;
	Evas_Object *section = NULL;

	DBG("");
	retif(parent == NULL, NULL, "invalid parameter");

	section = elm_layout_add(parent);
	ret = elm_layout_file_set(section, DEFAULT_EDJ,
			"quickpanel/notisection/default");
	retif(ret == EINA_FALSE, NULL, "failed to load layout");

	evas_object_size_hint_weight_set(section, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(section, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(section);

	qp_item_data *qid
		= quickpanel_list_util_item_new(QP_ITEM_TYPE_NOTI_GROUP, NULL);
	quickpanel_list_util_item_set_tag(section, qid);

	return section;
}

HAPI void noti_section_update(Evas_Object *noti_section, int noti_count) {
	retif(noti_section == NULL, , "invalid parameter");

	_noti_section_set_button(noti_section);
	_noti_section_set_text(noti_section, noti_count);
}

HAPI void noti_section_remove(Evas_Object *noti_section) {
	retif(noti_section == NULL, , "invalid parameter");

	quickpanel_list_util_item_del_tag(noti_section);
	evas_object_del(noti_section);
}
