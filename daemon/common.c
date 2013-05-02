/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common.h"
#include "quickpanel-ui.h"

HAPI void quickpanel_util_char_replace(char *text, char s, char t) {
	retif(text == NULL, , "invalid argument");

	int i = 0, text_len = 0;

	text_len = strlen(text);

	for (i = 0; i < text_len; i++) {
		if (*(text + i) == s) {
			*(text + i) = t;
		}
	}
}

static void _current_popup_deleted_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	retif(obj == NULL, , "obj is NULL");

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid argument");

	if (ad->popup == obj) {
		ad->popup = NULL;
	} else {
		ERR("popup is created over the popup");
	}
}

HAPI void quickpanel_ui_set_current_popup(Evas_Object *popup) {
	retif(popup == NULL, , "invalid argument");

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid argument");

	ad->popup = popup;
	evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL, _current_popup_deleted_cb, NULL);
}

HAPI void quickpanel_ui_del_current_popup(void) {
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid argument");

	if (ad->popup != NULL) {
		evas_object_del(ad->popup);
	}
}
