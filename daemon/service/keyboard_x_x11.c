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


#include <vconf.h>
#include <utilX.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>

#include <Ecore_Input.h>
#include <feedback.h>
#include "common.h"
#include "noti_util.h"
#include "keyboard_x.h"

#define TAB             23
#define SHIFT           50
#define RETURN          36
#define ARROW_KP_UP     80
#define ARROW_KP_DOWN   88
#define ARROW_KP_LEFT   83
#define ARROW_KP_RIGHT  85
#define ARROW_UP        111
#define ARROW_DOWN      116
#define ARROW_LEFT      113
#define ARROW_RIGHT     114

typedef struct _key_info {
	int keycode;
	const char *keyname;
	const char *key;
	const char *string;
	const char *compose;
} key_info;

key_info key_infos[] = {
	{TAB, "Tab", "Tab", "\t", "\t"},
	{RETURN, "Return", "Return", "\n", "\n"},
	{ARROW_UP, "Up", "Up", NULL, NULL},
	{ARROW_KP_UP, "Up", "Up", NULL, NULL},
	{ARROW_DOWN, "Down", "Down", NULL, NULL},
	{ARROW_KP_DOWN, "Down", "Down", NULL, NULL},
	{ARROW_LEFT, "Left", "Left", NULL, NULL},
	{ARROW_KP_LEFT, "Left", "Left", NULL, NULL},
	{ARROW_RIGHT, "Right", "Right", NULL, NULL},
	{ARROW_KP_RIGHT, "Right", "Right", NULL, NULL},
	{SHIFT, "Shift", "Shift", NULL, NULL},
};

static int _cb_event_generic(void *data, int ev_type, void *event);
static Eina_Bool _xinput_init(void);
static void _key_event_select(void);
static void _focus_ui_process_press(XIRawEvent *raw_event);
static void _focus_ui_process_release(XIRawEvent *raw_event);

static struct _s_info {
	int xi2_opcode;
	int is_shift_pressed;
	Ecore_Event_Handler *hdl_key_event;
} s_info = {
	.xi2_opcode = -1,
	.is_shift_pressed = 0,
	.hdl_key_event = NULL,
};

static int _key_event_validation_check(int keycode)
{
	int i = 0, len = 0;

	len = sizeof(key_infos) / sizeof(key_infos[0]);

	for (i = 0; i < len; i++ ) {
		if (key_infos[i].keycode == keycode) {
			return 1;
		}
	}

	return 0;
}

static void _key_event_select(void)
{
	int rc;
	XIEventMask mask;
	Ecore_X_Display *d = NULL;

    d = ecore_x_display_get();
    if (d == NULL) {
    	ERR("failed to get ecore-display");
    	return;
    }

	mask.mask_len = XIMaskLen(XI_LASTEVENT);
	mask.deviceid = XIAllDevices;
    mask.mask = calloc(mask.mask_len, sizeof(char));
    if (mask.mask == NULL) {
    	ERR("failed to get ecore-display");
    	return;
    }
    memset(mask.mask, 0, mask.mask_len);

    XISetMask(mask.mask, XI_RawKeyPress);
    XISetMask(mask.mask, XI_RawKeyRelease);

    rc = XISelectEvents(d, ecore_x_window_root_first_get(), &mask, 1);
    if (Success != rc) {
    	ERR("Failed to select XInput extension events");
    }
    if (mask.mask) {
		free( mask.mask);
	}
    ecore_x_sync();
}

static Eina_Bool _xinput_init(void)
{
   int event, error;

   if (!XQueryExtension(ecore_x_display_get(), "XInputExtension",
                               &s_info.xi2_opcode, &event, &error)) {
      s_info.xi2_opcode = -1;

      SERR("failed to initialize key event receiver");
      return EINA_FALSE;
   }

   _key_event_select();

   return EINA_TRUE;
}

static int _cb_event_generic(void *data, int ev_type, void *event)
{
	Ecore_X_Event_Generic *e = (Ecore_X_Event_Generic *)event;
	XIDeviceEvent *evData = (XIDeviceEvent *)(e->data);

	if ( e->extension != s_info.xi2_opcode ) {
		return ECORE_CALLBACK_PASS_ON;
	}
	if ( !evData || evData->send_event ) {
		return ECORE_CALLBACK_PASS_ON;
	}

	switch( e->evtype ) {
	case XI_RawKeyPress:
		if (evData->deviceid == 3) {
			break;
		}
		_focus_ui_process_press((XIRawEvent *)evData);
		break;
	case XI_RawKeyRelease:
		if (evData->deviceid == 3) {
			break;
		}
		_focus_ui_process_release((XIRawEvent *)evData);
		break;
	default:
		break;
	}

	return ECORE_CALLBACK_PASS_ON;
}

static void _focus_ui_process_press(XIRawEvent *raw_event)
{
	XEvent xev;

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid data.");
	retif(raw_event == NULL, , "invalid data.");

	if (raw_event->detail == SHIFT) {
		s_info.is_shift_pressed = 1;
		return;
	}
	if (_key_event_validation_check(raw_event->detail) == 0) {
		return;
	}

	Ecore_X_Display *d = ecore_x_display_get();
    if (d == NULL) {
    	ERR("failed to get ecore-display");
    	return;
    }

	memset(&xev, 0, sizeof(XEvent));
	xev.xany.display = ecore_x_display_get();
	xev.xkey.keycode = raw_event->detail;
	xev.xkey.time = raw_event->time;
	if (s_info.is_shift_pressed == 1) {
		xev.xkey.state = 0x1;
	} else {
		xev.xkey.state = 0;
	}
	xev.xkey.root = ecore_x_window_root_first_get();
	xev.xkey.send_event = 1;
	xev.xkey.subwindow = None;
	xev.xkey.type = KeyPress;
	xev.xkey.window = elm_win_xwindow_get(ad->win);
	XSendEvent(d, elm_win_xwindow_get(ad->win)
			, False, NoEventMask, &xev);
	DBG("keypressed:%d", raw_event->detail);
}

static void _focus_ui_process_release(XIRawEvent *raw_event)
{
	XEvent xev;

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid data.");
	retif(raw_event == NULL, , "invalid data.");

	if (raw_event->detail == SHIFT) {
		s_info.is_shift_pressed = 0;
		return;
	}
	if (_key_event_validation_check(raw_event->detail) == 0) {
		return;
	}

	Ecore_X_Display *d = ecore_x_display_get();
    if (d == NULL) {
    	ERR("failed to get ecore-display");
    	return;
    }

	memset(&xev, 0, sizeof(XEvent));
	xev.xany.display = d;
	xev.xkey.keycode = raw_event->detail;
	xev.xkey.time = raw_event->time;
	if (s_info.is_shift_pressed == 1) {
		xev.xkey.state = 0x1;
	} else {
		xev.xkey.state = 0;
	}
	xev.xkey.root = ecore_x_window_root_first_get();
	xev.xkey.send_event = 1;
	xev.xkey.subwindow = None;
	xev.xkey.type = KeyRelease;
	xev.xkey.window = elm_win_xwindow_get(ad->win);
	XSendEvent(d, elm_win_xwindow_get(ad->win)
			, False, NoEventMask, &xev);
	DBG("keyrelease:%d", raw_event->detail);
}

static void _focus_cleanup(void *data)
{
	struct appdata *ad = data;
	Evas_Object *focused_obj = NULL;
	retif(ad == NULL, , "invalid data.");
	retif(ad->win == NULL, , "invalid data.");

	focused_obj = elm_object_focused_object_get(ad->win);
	if (focused_obj != NULL) {
		elm_object_focus_set(focused_obj, EINA_FALSE);
	}
	elm_win_focus_highlight_enabled_set(ad->win, EINA_FALSE);
}

HAPI void quickpanel_keyboard_x_init(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	_xinput_init();
}

HAPI void quickpanel_keyboard_x_fini(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	if (s_info.hdl_key_event != NULL) {
		ecore_event_handler_del(s_info.hdl_key_event);
		s_info.hdl_key_event = NULL;
	}

	_focus_cleanup(ad);
}

HAPI void quickpanel_keyboard_x_openning_init(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	if (s_info.hdl_key_event != NULL) {
		ecore_event_handler_del(s_info.hdl_key_event);
		s_info.hdl_key_event = NULL;
	}
	s_info.hdl_key_event =
			ecore_event_handler_add(ECORE_X_EVENT_GENERIC, (Ecore_Event_Handler_Cb)_cb_event_generic, NULL);
}

HAPI void quickpanel_keyboard_x_closing_fini(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	if (s_info.hdl_key_event != NULL) {
		ecore_event_handler_del(s_info.hdl_key_event);
		s_info.hdl_key_event = NULL;
	}

	if (ad->win != NULL) {
		elm_win_focus_highlight_enabled_set(ad->win, EINA_FALSE);
	}
}
