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


#ifndef __QUICKPANEL_UI_H__
#define __QUICKPANEL_UI_H__

#if !defined(PACKAGE)
#  define PACKAGE	"quickpanel"
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR	"/usr/apps/org.tizen.quickpanel/res/locale"
#endif

#if !defined(EDJDIR)
#  define EDJDIR	"/usr/apps/org.tizen.quickpanel/res/edje"
#endif

#if !defined(SHARED_DIR)
#  define SHARED_DIR	"/usr/apps/org.tizen.quickpanel/shared/res"
#endif

/* EDJ theme */
#define DEFAULT_EDJ		EDJDIR"/"PACKAGE".edj"
#define DEFAULT_THEME_EDJ	EDJDIR"/"PACKAGE"_theme.edj"
#define SLIDER_THEME_EDJ EDJDIR"/"PACKAGE"_slider_theme.edj"
#define ACTIVENOTI_EDJ		EDJDIR"/"PACKAGE"_activenoti.edj"

#define _EDJ(o) elm_layout_edje_get(o)

#undef _
#define _(str) gettext(str)
#define _NOT_LOCALIZED(str) (str)

#define STR_ATOM_WINDOW_INPUT_REGION    "_E_COMP_WINDOW_INPUT_REGION"
#define STR_ATOM_WINDOW_CONTENTS_REGION "_E_COMP_WINDOW_CONTENTS_REGION"

#define MAX_NAM_LEN 4096
#define MAX_FILE_PATH_LEN 1024

#define INDICATOR_COVER_W 64
#define INDICATOR_COVER_H 60

#define _NEWLINE '\n'
#define _SPACE ' '

#define QP_DBUS_NAME "org.tizen.quickpanel"
#define QP_DBUS_PATH "/Org/Tizen/Quickpanel"

#define QP_DBUS_CLIENT_NAME "org.tizen.quickpanelsetting"
#define QP_DBUS_CLIENT_PATH "/Org/Tizen/Quickpanelsetting"

#if !defined(VENDOR)
#define QP_PKG_QUICKPANEL	"org.tizen.quickpanel"
#define QP_SETTING_PKG_SETTING	"org.tizen.setting"
#define QP_MINIAPPTRAY_PKG "org.tizen.mini-apps"
#else
#define QP_PKG_QUICKPANEL	VENDOR".quickpanel"
#define QP_SETTING_PKG_SETTING	VENDOR".setting"
#define QP_MINIAPPTRAY_PKG VENDOR".mini-apps"
#endif
#define QP_SEARCH_PKG "org.tizen.sfinder"

struct appdata {
	Evas_Object *win;
	tzsh_h tzsh;
	tzsh_quickpanel_service_h quickpanel_service;

	Evas_Object *background;
	Evas_Object *view_root;
	Evas_Object *view_page_zero;
	Evas_Object *ly; //view_base

	Evas *evas;

	Evas_Object *scroller;
	Evas_Object *list;
	Evas_Object *popup;
	int angle;
	double scale;
	char *theme;

	int win_width;
	int win_height;
	int gl_limit_height;
	int gl_distance_from_top;
	int gl_distance_to_bottom;

	int is_emul; /* 0 : target, 1 : emul */
	int is_suspended;
	int is_opened;
	int opening_reason;

	Ecore_Event_Handler *hdl_client_message;
	Ecore_Event_Handler *hdl_hardkey_down;
	Ecore_Event_Handler *hdl_hardkey_up;
	Eina_Bool is_hardkey_cancel;

	E_DBus_Connection *dbus_connection;
};

typedef struct _QP_Module {
	char *name;
	/* func */
	int (*init) (void *);
	void (*init_job_cb) (void *);
	int (*fini) (void *);
	int (*suspend) (void *);
	int (*resume) (void *);
	int (*hib_enter) (void *);
	int (*hib_leave) (void *);
	void (*lang_changed) (void *);
	void (*refresh) (void *);
	unsigned int (*get_height) (void *);
	void (*qp_opened) (void *);
	void (*qp_closed) (void *);
	void (*mw_enabled) (void *);
	void (*mw_disabled) (void *);

	/* do not modify this area */
	/* internal data */
	Eina_Bool state;
} QP_Module;

extern void *quickpanel_get_app_data(void);

#endif				/* __QUICKPANEL_UI_H__ */
