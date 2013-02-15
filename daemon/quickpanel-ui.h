/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __QUICKPANEL_UI_H__
#define __QUICKPANEL_UI_H__

#include <Elementary.h>
#include <player.h>

#if !defined(VENDOR)
#  define VENDOR "org.tizen"
#endif
#if !defined(PACKAGE)
#  define PACKAGE			"quickpanel"
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR			"/usr/apps/"VENDOR"."PACKAGE"/res/locale"
#endif

#if !defined(EDJDIR)
#  define EDJDIR			"/usr/apps/"VENDOR"."PACKAGE"/res/edje"
#endif

/* EDJ theme */
#define DEFAULT_EDJ		EDJDIR"/"PACKAGE".edj"
#define DEFAULT_THEME_EDJ	EDJDIR"/"PACKAGE"_theme.edj"

#define _EDJ(o) elm_layout_edje_get(o)
#define _S(str)	dgettext("sys_string", str)
#undef _
#define _(str) gettext(str)
#define _NOT_LOCALIZED(str) (str)

#define STR_ATOM_WINDOW_INPUT_REGION    "_E_COMP_WINDOW_INPUT_REGION"
#define STR_ATOM_WINDOW_CONTENTS_REGION "_E_COMP_WINDOW_CONTENTS_REGION"

#define MAX_NAM_LEN 4096

#define INDICATOR_COVER_W 82
#define INDICATOR_COVER_H 60

#define QP_SETTING_PKG_SETTING	VENDOR".setting"
#define QP_SETTING_PKG_SETTING_EMUL	"kto5jikgul.Settings"

struct appdata {
	Evas_Object *win;
#ifdef QP_INDICATOR_WIDGET_ENABLE
	Evas_Object *comformant;
#endif
	Evas_Object *ly;
	Evas *evas;

	Evas_Object *list;
	int angle;
	double scale;
	char *theme;

	int win_width;
	int win_height;
	int gl_limit_height;
	int gl_distance_from_top;
	int gl_distance_to_bottom;

	int is_emul; /* 0 : target, 1 : emul */
	int show_setting;

	Ecore_Event_Handler *hdl_client_message;
	Ecore_Event_Handler *hdl_hardkey;

	E_DBus_Connection *dbus_connection;
	E_DBus_Signal_Handler *dbus_handler_size;
	E_DBus_Signal_Handler *dbus_handler_progress;
	E_DBus_Signal_Handler *dbus_handler_content;

	Evas_Object *cover_indicator_left;
	Evas_Object *cover_indicator_right;
};

typedef struct _QP_Module {
	char *name;
	/* func */
	int (*init) (void *);
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

	/* do not modify this area */
	/* internal data */
	Eina_Bool state;
} QP_Module;

void quickpanel_player_play(sound_type_e sound_type, const char *sound_file);
int quickpanel_launch_app(char *app_id, void *data);
int quickpanel_ui_check_emul(void);
void quickpanel_init_size_genlist(void *data);
void quickpanel_ui_update_height(void *data);
void *quickpanel_get_app_data(void);
Evas_Object *quickpanel_ui_load_edj(Evas_Object * parent, const char *file,
					    const char *group, int is_just_load);
void quickpanel_ui_set_indicator_cover(void *data);

#endif				/* __QUICKPANEL_UI_H__ */
