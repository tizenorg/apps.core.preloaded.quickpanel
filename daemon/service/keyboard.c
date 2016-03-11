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
#include <vconf.h>
#include <Ecore_Input.h>
#include <feedback.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <notification.h>

#include <E_DBus.h>

#include "quickpanel-ui.h" // appdata
#include "common_uic.h"
#include "common.h"
#include "noti_util.h"
#include "keyboard.h"

#define KEY_BACK	"XF86Back"
#define KEY_CANCEL	"Cancel"
#define KEY_MENU	"XF86Menu"
#define KEY_QUICKPANEL	"XF86QuickPanel"
#define KEY_HOME	"XF86Home"

static Eina_Bool _service_hardkey_up_cb(void *data, int type, void *event)
{
	struct appdata *ad = NULL;
	Ecore_Event_Key *key_event = NULL;

	retif(data == NULL || event == NULL, EINA_FALSE, "Invalid parameter!");
	ad = data;
	key_event = event;

	if (!strcmp(key_event->keyname, KEY_HOME)) {
		if (ad->is_hardkey_cancel == EINA_FALSE) {
			quickpanel_uic_close_quickpanel(false, 0);
		} else {
			DBG("Cancel status, do nothing");
		}
	} else if (!strcmp(key_event->keyname, KEY_CANCEL)) {
		ad->is_hardkey_cancel = EINA_FALSE;
	} else if (!strcmp(key_event->keyname, KEY_BACK)) {
		if (ad->popup != NULL) {
			Evas_Smart_Cb back_cb = evas_object_data_get(ad->popup, EDATA_BACKKEY_CB);
			if (back_cb != NULL) {
				back_cb(ad->popup, ad->popup, NULL);
			}
		} else {
			quickpanel_uic_close_quickpanel(false, 0);
		}
	}
	return EINA_FALSE;
}

static Eina_Bool _service_hardkey_down_cb(void *data, int type, void *event)
{
	Ecore_Event_Key *key_event = event;
	struct appdata *ad = data;
	retif(key_event == NULL, EINA_FALSE, "Invalid parameter!");
	retif(ad == NULL, EINA_FALSE, "Invalid parameter!");

	if (!strcmp(key_event->keyname, KEY_CANCEL)) {
		ad->is_hardkey_cancel = EINA_TRUE;
	} else if (!strcmp(key_event->keyname, KEY_QUICKPANEL)) {
		quickpanel_uic_toggle_openning_quickpanel();
	}
	return EINA_FALSE;
}

HAPI void quickpanel_keyboard_init(void *data)
{
	struct appdata *ad = data;
	Ecore_Event_Handler *hdl_key_down = NULL;
	Ecore_Event_Handler *hdl_key_up = NULL;
	retif(ad == NULL, , "Invalid parameter!");

	if (elm_win_keygrab_set(ad->win, KEY_QUICKPANEL, 0, 0, 0, ELM_WIN_KEYGRAB_SHARED) == EINA_FALSE) {
		ERR("failed to grab KEY_QUICKPANEL");
	}

	hdl_key_down = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _service_hardkey_down_cb, ad);
	if (hdl_key_down == NULL) {
		ERR("failed to add handler(ECORE_EVENT_KEY_DOWN)");
	}
	ad->hdl_hardkey_down = hdl_key_down;

	hdl_key_up = ecore_event_handler_add(ECORE_EVENT_KEY_UP, _service_hardkey_up_cb, ad);
	if (hdl_key_up == NULL) {
		ERR("failed to add handler(ECORE_EVENT_KEY_UP)");
	}
	ad->hdl_hardkey_up = hdl_key_up;
	ad->is_hardkey_cancel = EINA_FALSE;
}

HAPI void quickpanel_keyboard_fini(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	if (ad->hdl_hardkey_up != NULL) {
		ecore_event_handler_del(ad->hdl_hardkey_up);
		ad->hdl_hardkey_up = NULL;
	}

	if (ad->hdl_hardkey_down != NULL) {
		ecore_event_handler_del(ad->hdl_hardkey_down);
		ad->hdl_hardkey_down = NULL;
	}

	if (elm_win_keygrab_unset(ad->win, KEY_QUICKPANEL, 0, 0) == EINA_FALSE) {
		ERR("failed to ungrab KEY_QUICKPANEL");
	}
}

HAPI void quickpanel_keyboard_openning_init(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	if (elm_win_keygrab_set(ad->win, KEY_BACK, 0, 0, 0, ELM_WIN_KEYGRAB_EXCLUSIVE)  == EINA_FALSE) {
		ERR("failed to grab KEY_BACK");
	}

	if (elm_win_keygrab_set(ad->win, KEY_MENU, 0, 0, 0, ELM_WIN_KEYGRAB_EXCLUSIVE) == EINA_FALSE) {
		ERR("failed to grab KEY_MENU");
	}

	if (elm_win_keygrab_set(ad->win, KEY_HOME, 0, 0, 0, ELM_WIN_KEYGRAB_SHARED)  == EINA_FALSE) {
		ERR("failed to grab KEY_HOME");
	}

}

HAPI void quickpanel_keyboard_closing_fini(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	if (elm_win_keygrab_unset(ad->win, KEY_BACK, 0, 0) == EINA_FALSE) {
		ERR("failed to ungrab KEY_BACK");
	}

	if (elm_win_keygrab_unset(ad->win, KEY_MENU, 0, 0) == EINA_FALSE) {
		ERR("failed to ungrab KEY_MENU");
	}

	if (elm_win_keygrab_unset(ad->win, KEY_HOME, 0, 0) == EINA_FALSE) {
		ERR("failed to ungrab KEY_HOME");
	}
}


