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

#include <stdio.h>
#include <signal.h>
#include <app.h>
#include <system_info.h>
#include <sys/utsname.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <utilX.h>
#include <Ecore_X.h>
#include <Ecore_Input.h>
#include <heynoti.h>
#include <vconf.h>
#include <unistd.h>
#include <privilege-control.h>
#include <bundle.h>
#include <feedback.h>

#include "common.h"
#include "quickpanel-ui.h"
#include "modules.h"
#include "notifications/noti_display_app.h"
#include "quickpanel_def.h"

#define HIBERNATION_ENTER_NOTI	"HIBERNATION_ENTER"
#define HIBERNATION_LEAVE_NOTI	"HIBERNATION_LEAVE"

#define QP_WINDOW_PRIO 300
#define QP_PLAY_DURATION_LIMIT 15

/* heynoti handle */
static int g_hdl_heynoti;
static player_h g_sound_player;
static Ecore_Timer *g_sound_player_timer;
struct appdata *g_app_data = NULL;

/* binary information */
#define QP_EMUL_STR		"Emulator"

static int common_cache_flush(void *evas);

/*****************************************************************************
  *
  * HIBERNATION
  *
  ****************************************************************************/
static void _hibernation_enter_cb(void *data)
{
	struct appdata *ad = data;

	INFO(" >>>>>>>>>>>>>>> ENTER HIBERNATION!! <<<<<<<<<<<<<<<< ");
	hib_enter_modules(data);
	if (ad)
		common_cache_flush(ad->evas);
}

static void _hibernation_leave_cb(void *data)
{
	hib_leave_modules(data);
	INFO(" >>>>>>>>>>>>>>> LEAVE HIBERNATION!! <<<<<<<<<<<<<<<< ");
}

/******************************************************************************
  *
  * UI
  *
  ****************************************************************************/
static Eina_Bool quickpanel_ui_refresh_cb(void *data)
{
	struct appdata *ad = NULL;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");
	ad = data;

	INFO(" >>>>>>>>>>>>>>> Refresh QP modules!! <<<<<<<<<<<<<<<< ");
	refresh_modules(data);

	if (ad->list) {
		elm_genlist_realized_items_update(ad->list);
	}

	quickpanel_init_size_genlist(ad);
	quickpanel_ui_update_height(ad);

	return EINA_FALSE;
}

static int common_cache_flush(void *evas)
{
	int file_cache;
	int collection_cache;
	int image_cache;
	int font_cache;

	retif(evas == NULL, QP_FAIL, "Evas is NULL\n");

	file_cache = edje_file_cache_get();
	collection_cache = edje_collection_cache_get();
	image_cache = evas_image_cache_get(evas);
	font_cache = evas_font_cache_get(evas);

	edje_file_cache_set(file_cache);
	edje_collection_cache_set(collection_cache);
	evas_image_cache_set(evas, 0);
	evas_font_cache_set(evas, 0);

	evas_image_cache_flush(evas);
	evas_render_idle_flush(evas);
	evas_font_cache_flush(evas);

	edje_file_cache_flush();
	edje_collection_cache_flush();

	edje_file_cache_set(file_cache);
	edje_collection_cache_set(collection_cache);
	evas_image_cache_set(evas, image_cache);
	evas_font_cache_set(evas, font_cache);

	return QP_OK;
}

static int _resize_noti_win(void *data, int new_angle)
{
	struct appdata *ad = (struct appdata *)data;
	int w = 0, h = 0;
	int tot_h = 0;
	int diff = 0;

	diff = (ad->angle > new_angle) ?
	    (ad->angle - new_angle) : (new_angle - ad->angle);

#if 0
	int tot_h = QP_HANDLE_H * ad->scale;

	/* get indicator height */
	ecore_x_e_illume_indicator_geometry_get(ecore_x_window_root_first_get(),
						NULL, NULL, NULL, &h);
	if (h <= 0)
		h = (int)(QP_INDICATOR_H * ad->scale);

	tot_h += h;
	INFO("tot_h[%d], scale[%lf],indi[%d]", tot_h, ad->scale, h);
#else
	tot_h = 0;
	INFO("tot_h[%d], scale[%lf]", tot_h, ad->scale);
#endif

	ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
	if (diff % 180 != 0) {
		int width = 0;
		int height = 0;
		if (ad->angle % 180 == 0) {
			width = w - tot_h;
			height = h;
		} else {
			width = h - tot_h;
			height = w;
		}
		INFO("win[%dx%d], Resize[%dx%d] diff[%d], angle[%d]",
			w, h, width, height, diff, ad->angle);
		evas_object_resize(ad->win, (int)width-1, (int)height-1); //workaround
		evas_object_resize(ad->win, (int)width, (int)height);
	}
	return 0;
}

static Eina_Bool quickpanel_hardkey_cb(void *data, int type, void *event)
{
	struct appdata *ad = NULL;
	Ecore_Event_Key *key_event = NULL;
	Ecore_X_Window xwin;

	retif(data == NULL || event == NULL,
		EINA_FALSE, "Invalid parameter!");
	ad = data;
	key_event = event;

	if (!strcmp(key_event->keyname, KEY_SELECT)) {
		xwin = elm_win_xwindow_get(ad->win);
		if (xwin != 0)
			ecore_x_e_illume_quickpanel_state_send(ecore_x_e_illume_zone_get(xwin),ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
	}
	return EINA_FALSE;
}

static Eina_Bool quickpanel_ui_client_message_cb(void *data, int type,
						 void *event)
{
	struct appdata *ad = data;
	Ecore_X_Event_Client_Message *ev = event;
	int new_angle;

	retif(data == NULL || event == NULL,
		ECORE_CALLBACK_RENEW, "Invalid parameter!");

	if (ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE) {
		new_angle = ev->data.l[0];

		if (new_angle == 0 || new_angle == 90 || new_angle == 180 || new_angle == 270) {
			if (new_angle != ad->angle) {
				INFO("ROTATION: new:%d old:%d", new_angle, ad->angle);
				_resize_noti_win(ad, new_angle);

				elm_win_rotation_with_resize_set(ad->win,
								 new_angle);
				ad->angle = new_angle;

				quickpanel_ui_set_indicator_cover(ad);
			}
		}
		ecore_idler_add(quickpanel_ui_refresh_cb, ad);
	} else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE) {
		if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_ON) {
			qp_opened_modules(data);
		} else {
			qp_closed_modules(data);
		}
	}
	return ECORE_CALLBACK_RENEW;
}

static Evas_Object *_quickpanel_ui_window_add(const char *name, int prio)
{
	Evas_Object *eo = NULL;
	Ecore_X_Window xwin;

	eo = elm_win_add(NULL, name, ELM_WIN_BASIC);

	if (eo != NULL) {
		elm_win_alpha_set(eo, EINA_TRUE);
		elm_win_indicator_mode_set(eo, ELM_WIN_INDICATOR_SHOW);
		elm_win_title_set(eo, name);
		elm_win_borderless_set(eo, EINA_TRUE);
		elm_win_autodel_set(eo, EINA_TRUE);

		/* set this window as a quickpanel */
		elm_win_quickpanel_set(eo, 1);
		elm_win_quickpanel_priority_major_set(eo, prio);

		/* icccm name class set */
		xwin = elm_win_xwindow_get(eo);
		ecore_x_icccm_name_class_set(xwin, "QUICKPANEL", "QUICKPANEL");
		evas_object_show(eo);
	}

	return eo;
}

Evas_Object *quickpanel_ui_load_edj(Evas_Object * parent, const char *file,
					    const char *group, int is_just_load)
{
	Eina_Bool r;
	Evas_Object *eo = NULL;

	retif(parent == NULL, NULL, "Invalid parameter!");

	eo = elm_layout_add(parent);
	retif(eo == NULL, NULL, "Failed to add layout object!");

	r = elm_layout_file_set(eo, file, group);
	retif(r != EINA_TRUE, NULL,
		"Failed to set edje object file[%s-%s]!", file, group);

	evas_object_size_hint_weight_set(eo,
					 EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	if (is_just_load == 1) {
		elm_win_resize_object_add(parent, eo);
	}
	evas_object_show(eo);

	return eo;
}

static void _quickpanel_ui_close_quickpanel(void *data, Evas_Object *o,
		const char *emission, const char *source) {

	Ecore_X_Window xwin = 0;
	struct appdata *ad = NULL;

	retif(data == NULL, , "data is NULL");
	ad = data;

	DBG("close quick panel");

	xwin = elm_win_xwindow_get(ad->win);

	if (xwin != 0)
		ecore_x_e_illume_quickpanel_state_send(ecore_x_e_illume_zone_get(xwin),ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
}

static int _quickpanel_ui_create_win(void *data)
{
	struct appdata *ad = data;
	int w = 0;
	int h = 0;

	retif(data == NULL, QP_FAIL, "Invialid parameter!");

	ad->win = _quickpanel_ui_window_add("Quickpanel Window",
					QP_WINDOW_PRIO);
	if (ad->win == NULL) {
		ERR("ui create : failed to create window.");
		return -1;
	}
#ifdef QP_INDICATOR_WIDGET_ENABLE
	ad->comformant = elm_conformant_add(ad->win);
	elm_object_style_set(ad->comformant, "nokeypad");

	ad->ly = quickpanel_ui_load_edj(ad->comformant,
				DEFAULT_EDJ, "quickpanel/gl_base", 0);
	if (ad->ly == NULL)
		return -1;

	evas_object_size_hint_weight_set(ad->comformant,
					 EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->comformant);
	evas_object_show(ad->comformant);
	elm_object_content_set(ad->comformant, ad->ly);

#else
	ad->ly = quickpanel_ui_load_edj(ad->win,
				DEFAULT_EDJ, "quickpanel/gl_base", 0);
	if (ad->ly == NULL)
		return -1;
#endif

	/* get noti evas */
	ad->evas = evas_object_evas_get(ad->win);

	ad->list = elm_genlist_add(ad->ly);
	if (!ad->list) {
		ERR("failed to elm_genlist_add");
		evas_object_del(ad->ly);
		evas_object_del(ad->win);
		ad->ly = NULL;
		ad->win = NULL;
		ad->evas = NULL;
		return -1;
	}
	elm_genlist_homogeneous_set(ad->list, EINA_FALSE);
	elm_scroller_policy_set(ad->list, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	elm_object_part_content_set(ad->ly, "qp.gl_base.gl.swallow", ad->list);

	ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
	evas_object_resize(ad->win, w, h);

	ad->win_width = w;
	ad->win_height = h;

	edje_object_signal_callback_add(_EDJ(ad->ly),
			"close.quickpanel", "*", _quickpanel_ui_close_quickpanel,
			ad);

	quickpanel_init_size_genlist(ad);

	quickpanel_ui_set_indicator_cover(ad);

	/* key grab */
	utilx_grab_key(ecore_x_display_get(), elm_win_xwindow_get(ad->win), KEY_SELECT, SHARED_GRAB);

	return 0;
}


void quickpanel_ui_set_indicator_cover(void *data)
{
	retif(data == NULL, , "data is NULL");
	struct appdata *ad = data;

	int x_1 = 0, y_1 = 0;
	int x_2 = 0, y_2 = 0;
	int angle = ad->angle;

	int width = INDICATOR_COVER_W * ad->scale;
	int height = INDICATOR_COVER_H * ad->scale;

	switch (angle) {
		case 0:
			x_1 = 0;
			y_1 = 0;
			x_2 = ad->win_width - width;
			y_2 = 0;
			break;
		case 90:
			x_1 = 0;
			y_1 = 0;
			x_2 = ad->win_height - width;
			y_2 = 0;
			break;
		case 180:
			x_1 = 0;
			y_1 = 0;
			x_2 = ad->win_width - width;
			y_2 = 0;
			break;
		case 270:
			x_1 = 0;
			y_1 = 0;
			x_2 = ad->win_height - width;
			y_2 = 0;
			break;
	}

	if (ad->cover_indicator_left == NULL) {
		Evas_Object *bg = evas_object_rectangle_add(ad->evas);
		evas_object_color_set(bg, 52, 52, 50, 255); // opaque white background
		evas_object_repeat_events_set(bg, EINA_FALSE);
		evas_object_resize(bg, width, height); // covers full canvas
		evas_object_move(bg, x_1, y_1);
		evas_object_show(bg);
		ad->cover_indicator_left = bg;
	} else {
		evas_object_move(ad->cover_indicator_left, x_1, y_1);
	}
	if (ad->cover_indicator_right == NULL) {
		Evas_Object *bg = evas_object_rectangle_add(ad->evas);
		evas_object_color_set(bg, 52, 52, 50, 255); // opaque white background
		evas_object_repeat_events_set(bg, EINA_FALSE);
		evas_object_resize(bg, width, height); // covers full canvas
		evas_object_move(bg, x_2, y_2);
		evas_object_show(bg);
		ad->cover_indicator_right = bg;
	} else {
		evas_object_move(ad->cover_indicator_right, x_2, y_2);
	}
}

void quickpanel_ui_window_set_input_region(void *data, int contents_height)
{
	struct appdata *ad = NULL;
	Ecore_X_Window xwin;
	Ecore_X_Atom atom_window_input_region = 0;
	unsigned int window_input_region[4] = {0,};

	retif(data == NULL,  , "Invialid parameter!");
	ad = data;

	xwin = elm_win_xwindow_get(ad->win);

	DBG("angle:%d", ad->angle);
	switch (ad->angle) {
		case 0:
			window_input_region[0] = 0; //X
			window_input_region[1] = contents_height; // Y
			window_input_region[2] = ad->win_width; // Width
			window_input_region[3] = ad->scale * QP_HANDLE_H; // height
			break;
		case 90:
			window_input_region[0] = contents_height; //X
			window_input_region[1] = 0; // Y
			window_input_region[2] = ad->scale * QP_HANDLE_H; // Width
			window_input_region[3] = ad->win_height; // height
			break;
		case 180:
			window_input_region[0] = 0; //X
			window_input_region[1] = ad->win_height - contents_height - ad->scale * QP_HANDLE_H; // Y
			window_input_region[2] = ad->win_width; // Width
			window_input_region[3] = ad->scale * QP_HANDLE_H; // height
			break;
		case 270:
			window_input_region[0] = ad->win_width - contents_height - ad->scale * QP_HANDLE_H ; //X
			window_input_region[1] = 0; // Y
			window_input_region[2] = ad->scale * QP_HANDLE_H; // Width
			window_input_region[3] = ad->win_height; // height
			break;
	}

	DBG("win_input_0:%d\nwin_input_1:%d\nwin_input_2:%d\nwin_input_3:%d\n"
			,window_input_region[0]
			,window_input_region[1]
			,window_input_region[2]
		    ,window_input_region[3]
			);

	atom_window_input_region = ecore_x_atom_get(STR_ATOM_WINDOW_INPUT_REGION);
	ecore_x_window_prop_card32_set(xwin, atom_window_input_region, window_input_region, 4);
}

void quickpanel_ui_window_set_content_region(void *data, int contents_height)
{
	struct appdata *ad = NULL;
	Ecore_X_Window xwin;
	Ecore_X_Atom atom_window_contents_region = 0;
	unsigned int window_contents_region[4] = {0,};

	retif(data == NULL,  , "Invialid parameter!");
	ad = data;

	xwin = elm_win_xwindow_get(ad->win);

	DBG("angle:%d", ad->angle);
	switch (ad->angle) {
		case 0:
			window_contents_region[0] = 0; //X
			window_contents_region[1] = 0; // Y
			window_contents_region[2] = ad->win_width; // Width
			window_contents_region[3] = contents_height; // height
			break;
		case 90:
			window_contents_region[0] = 0; //X
			window_contents_region[1] = 0; // Y
			window_contents_region[2] = contents_height; // Width
			window_contents_region[3] = ad->win_height; // height
			break;
		case 180:
			window_contents_region[0] = 0; //X
			window_contents_region[1] = ad->win_height - contents_height; // Y
			window_contents_region[2] = ad->win_width; // Width
			window_contents_region[3] = contents_height; // height
			break;
		case 270:
			window_contents_region[0] = ad->win_width - contents_height ; //X
			window_contents_region[1] = 0; // Y
			window_contents_region[2] = contents_height; // Width
			window_contents_region[3] = ad->win_height; // height
			break;
	}

	DBG("win_contents_0:%d\nwin_contents_1:%d\nwin_contents_2:%d\nwin_contents_3:%d\n"
			,window_contents_region[0]
			,window_contents_region[1]
			,window_contents_region[2]
		    ,window_contents_region[3]
			);

    atom_window_contents_region = ecore_x_atom_get(STR_ATOM_WINDOW_CONTENTS_REGION);
    ecore_x_window_prop_card32_set(xwin, atom_window_contents_region, window_contents_region, 4);
}

static void _quickpanel_move_data_to_service(const char *key, const char *val, void *data)
{
	retif(data == NULL || key == NULL || val == NULL, , "Invialid parameter!");

	service_h service = data;
	service_add_extra_data(service, key, val);
}

int quickpanel_launch_app(char *app_id, void *data)
{
	int ret = SERVICE_ERROR_NONE;
	service_h service = NULL;

	retif(app_id == NULL, SERVICE_ERROR_INVALID_PARAMETER, "Invialid parameter!");

	ret = service_create(&service);
	if (ret != SERVICE_ERROR_NONE) {
		DBG("service_create() return error : %d", ret);
		return ret;
	}
	retif(service == NULL, SERVICE_ERROR_INVALID_PARAMETER, "fail to create service handle!");

	service_set_operation(service, SERVICE_OPERATION_DEFAULT);
	service_set_app_id(service, app_id);

	if (data != NULL) {
		bundle_iterate((bundle *)data, _quickpanel_move_data_to_service, service);
	}

	ret = service_send_launch_request(service, NULL, NULL);
	if (ret != SERVICE_ERROR_NONE) {
		DBG("service_send_launch_request() is failed : %d", ret);
		service_destroy(service);
		return ret;
	}
	service_destroy(service);
	return ret;
}

static void _quickpanel_player_free(player_h *sound_player)
{
	retif(sound_player == NULL, , "invalid parameter");

	player_state_e state = PLAYER_STATE_NONE;

	if (*sound_player != NULL) {
		if (player_get_state(*sound_player, &state) == PLAYER_ERROR_NONE) {

			DBG("state of player %d", state);

			if (state == PLAYER_STATE_PLAYING) {
				player_stop(*sound_player);
				player_unprepare(*sound_player);
			}
			if (state == PLAYER_STATE_READY) {
				player_unprepare(*sound_player);
			}
		}
		player_destroy(*sound_player);
		*sound_player = NULL;
	}
}

static void
_quickpanel_player_del_timeout_timer(void)
{
	if (g_sound_player_timer) {
		ecore_timer_del(g_sound_player_timer);
		g_sound_player_timer = NULL;
	}
}

static Eina_Bool _quickpanel_player_timeout_cb(void *data)
{
	g_sound_player_timer = NULL;

	retif(data == NULL, ECORE_CALLBACK_CANCEL, "invalid parameter");
	player_h *sound_player = data;

	_quickpanel_player_free(sound_player);

	return ECORE_CALLBACK_CANCEL;
}

static void
_quickpanel_player_completed_cb(void *user_data)
{
	retif(user_data == NULL, , "invalid parameter");
	player_h *sound_player = user_data;

	_quickpanel_player_del_timeout_timer();
	_quickpanel_player_free(sound_player);
}

static void
_quickpanel_player_interrupted_cb(player_interrupted_code_e code, void *user_data)
{
	retif(user_data == NULL, , "invalid parameter");
	player_h *sound_player = user_data;

	_quickpanel_player_del_timeout_timer();
	_quickpanel_player_free(sound_player);
}

static void
_quickpanel_player_error_cb(int error_code, void *user_data)
{
	retif(user_data == NULL, , "invalid parameter");
	player_h *sound_player = user_data;

	_quickpanel_player_del_timeout_timer();
	_quickpanel_player_free(sound_player);
}

void quickpanel_player_play(sound_type_e sound_type, const char *sound_file)
{
	player_h *sound_player = &g_sound_player;

	int ret = PLAYER_ERROR_NONE;
	player_state_e state = PLAYER_STATE_NONE;

	_quickpanel_player_del_timeout_timer();

	if (*sound_player != NULL) {
		_quickpanel_player_free(sound_player);
	}

	ret = player_create(sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		ERR("creating the player handle failed[%d]", ret);
		player_destroy(*sound_player);
	}

	ret = player_set_sound_type(*sound_player, SOUND_TYPE_MEDIA);
	if (ret != PLAYER_ERROR_NONE) {
		ERR("player_set_sound_type() ERR: %x!!!!", ret);
		_quickpanel_player_free(sound_player);
		return ;
	}

	player_get_state(*sound_player, &state);
	if (state > PLAYER_STATE_READY) {
		_quickpanel_player_free(sound_player);
		return;
	}

	ret = player_set_uri(*sound_player, sound_file);
	if (ret != PLAYER_ERROR_NONE) {
		DBG("set attribute---profile_uri[%d]", ret);
		_quickpanel_player_free(sound_player);
		return;
	}

	ret = player_prepare(*sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		DBG("realizing the player handle failed[%d]", ret);
		_quickpanel_player_free(sound_player);
		return;
	}

	player_get_state(*sound_player, &state);
	if (state != PLAYER_STATE_READY) {
		DBG("state of player is invalid %d", state);
		_quickpanel_player_free(sound_player);
		return;
	}

	/* register callback */
	ret = player_set_completed_cb(*sound_player, _quickpanel_player_completed_cb, sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		DBG("player_set_completed_cb() ERR: %x!!!!", ret);
		_quickpanel_player_free(sound_player);
		return;
	}

	ret = player_set_interrupted_cb(*sound_player, _quickpanel_player_interrupted_cb, sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		_quickpanel_player_free(sound_player);
		return;
	}

	ret = player_set_error_cb(*sound_player, _quickpanel_player_error_cb, sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		_quickpanel_player_free(sound_player);
		return;
	}

	ret = player_start(*sound_player);
	if (ret != PLAYER_ERROR_NONE) {	/* if directly return retor.. */
		DBG("player_start [%d]", ret);
		_quickpanel_player_free(sound_player);
		return;
	}

	g_sound_player_timer = ecore_timer_add(QP_PLAY_DURATION_LIMIT,
			_quickpanel_player_timeout_cb, sound_player);
}

static int _quickpanel_ui_delete_win(void *data)
{
	struct appdata *ad = data;
	retif(data == NULL, QP_FAIL, "Invialid parameter!");

	if (ad->ly != NULL) {
		evas_object_del(ad->ly);
		ad->ly = NULL;
	}
	if (ad->win != NULL) {
		evas_object_del(ad->win);
		ad->win = NULL;
	}

	return QP_OK;
}

static void _quickpanel_ui_init_heynoti(struct appdata *ad)
{
	int ret = 0;

	/* init heynoti */
	g_hdl_heynoti = heynoti_init();
	if (g_hdl_heynoti == -1) {
		ERR("ui init heynoti : fail to heynoti_init.");
		g_hdl_heynoti = 0;
		return;
	}

	/* subscribe hibernation */
	heynoti_subscribe(g_hdl_heynoti, HIBERNATION_ENTER_NOTI,
			  _hibernation_enter_cb, (void *)ad);
	heynoti_subscribe(g_hdl_heynoti, HIBERNATION_LEAVE_NOTI,
			  _hibernation_leave_cb, (void *)ad);

	ret = heynoti_attach_handler(g_hdl_heynoti);
	if (ret == -1) {
		ERR("ui init heynoti : fail to heynoti_attach_handler.");
		return;
	}
}

static void _quickpanel_ui_fini_heynoti(void)
{
	if (g_hdl_heynoti != 0) {
		/* unsubscribe hibernation */
		heynoti_unsubscribe(g_hdl_heynoti, HIBERNATION_ENTER_NOTI,
				    _hibernation_enter_cb);
		heynoti_unsubscribe(g_hdl_heynoti, HIBERNATION_LEAVE_NOTI,
				    _hibernation_leave_cb);

		/* close heynoti */
		heynoti_close(g_hdl_heynoti);
		g_hdl_heynoti = 0;
	}
}

static void _quickpanel_ui_init_ecore_event(struct appdata *ad)
{
	Ecore_Event_Handler *hdl = NULL;
	Ecore_Event_Handler *hdl_key = NULL;

	/* Register window rotate event */
	hdl = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
				quickpanel_ui_client_message_cb, ad);
	if (hdl == NULL)
		ERR("failed to add handler(ECORE_X_EVENT_CLIENT_MESSAGE)");

	ad->hdl_client_message = hdl;

	hdl_key = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, quickpanel_hardkey_cb, ad);
	if (hdl_key == NULL)
		ERR("failed to add handler(ECORE_EVENT_KEY_UP)");

	ad->hdl_hardkey = hdl_key;
}

static void _quickpanel_ui_fini_ecore_event(struct appdata *ad)
{
	if (ad->hdl_client_message != NULL) {
		ecore_event_handler_del(ad->hdl_client_message);
		ad->hdl_client_message = NULL;
	}
	if (ad->hdl_hardkey != NULL) {
		ecore_event_handler_del(ad->hdl_hardkey);
		ad->hdl_hardkey = NULL;
	}
}
int quickpanel_ui_check_emul(void)
{
	int is_emul = 0;
	char *info = NULL;

	if (system_info_get_value_string(SYSTEM_INFO_KEY_MODEL, &info) == 0) {
		if (info == NULL) return 0;
		if (!strncmp(QP_EMUL_STR, info, strlen(info))) {
			is_emul = 1;
		}
	}

	if (info != NULL) free(info);

	return is_emul;
}

static void _quickpanel_ui_setting_show(struct appdata *ad, int show)
{
	if (!ad)
		return;

	if (!ad->ly)
		return;

	ad->show_setting = 1;

}

#ifdef QP_BRIGHTNESS_ENABLE
/* toggle */
extern QP_Module brightness_ctrl;
#endif /* QP_BRIGHTNESS_ENABLE */
#ifdef QP_MINICTRL_ENABLE
extern QP_Module minictrl;
#endif /* QP_MINICTRL_ENABLE */
extern QP_Module noti;

static void _quickpanel_ui_update_height(void *data)
{
	int contents_height = 0;
	int height_genlist = 0;

	struct appdata *ad = NULL;

	retif(data == NULL, , "data is NULL");
	ad = data;

	DBG("current item count:%d", elm_genlist_items_count(ad->list));

	height_genlist += noti.get_height(data);
#ifdef QP_BRIGHTNESS_ENABLE
	height_genlist += brightness_ctrl.get_height(data);
#endif
#ifdef QP_MINICTRL_ENABLE
	height_genlist += minictrl.get_height(data);
#endif

	height_genlist = ad->gl_limit_height;

	contents_height = ad->gl_distance_from_top + height_genlist + ad->gl_distance_to_bottom - ad->scale * QP_HANDLE_H;

	DBG("height_genlist:%d\n gl_distance_from_top:%d\n gl_distance_to_bottom:%d\n gl_limit_height:%d\nnew_height:%d"
			,height_genlist
			,ad->gl_distance_from_top
			,ad->gl_distance_to_bottom
			,ad->gl_limit_height
			,contents_height
			);

	quickpanel_ui_window_set_input_region(ad, contents_height);
	quickpanel_ui_window_set_content_region(ad, contents_height);
}

void quickpanel_ui_update_height(void *data)
{
	_quickpanel_ui_update_height(data);
}

void quickpanel_init_size_genlist(void *data)
{
	struct appdata *ad = NULL;
	int max_height_window = 0;
	Evas_Coord genlist_y = 0;
	Evas_Coord spn_height = 0;

	retif(data == NULL, , "data is NULL");
	ad = data;

	if (ad->angle == 90 || ad->angle == 270 )
		max_height_window = ad->win_width;
	else
		max_height_window = ad->win_height;

	edje_object_part_geometry_get(_EDJ(ad->ly), "qp.gl_base.gl.swallow", NULL, &genlist_y, NULL, NULL);
	DBG("quickpanel, qp.gl_base.gl.swallow y: %d",genlist_y);

	edje_object_part_geometry_get(_EDJ(ad->ly), "qp.base.spn.swallow", NULL, NULL, NULL, &spn_height);
	DBG("quickpanel, to spn_height: %d",spn_height);

	ad->gl_distance_from_top = genlist_y;
	ad->gl_distance_to_bottom = spn_height + (1 * ad->scale) + (ad->scale*QP_HANDLE_H) ;
	ad->gl_limit_height = max_height_window - ad->gl_distance_from_top - ad->gl_distance_to_bottom;
}

void *quickpanel_get_app_data(void)
{
	return g_app_data;
}

/*****************************************************************************
  *
  * App efl main interface
  *
  ****************************************************************************/

static void _signal_handler(int signum, siginfo_t *info, void *unused)
{
	DBG("Terminated...");
	app_efl_exit();
}

static void _heynoti_event_power_off(void *data)
{
	DBG("Terminated...");
	app_efl_exit();
}

static bool quickpanel_app_create(void *data)
{
	DBG("");

	pid_t pid;
	int r;

	// signal handler
	struct sigaction act;
	act.sa_sigaction = _signal_handler;
	act.sa_flags = SA_SIGINFO;

	int ret = sigemptyset(&act.sa_mask);
	if (ret < 0) {
		ERR("Failed to sigemptyset[%s]", strerror(errno));
	}
	ret = sigaddset(&act.sa_mask, SIGTERM);
	if (ret < 0) {
		ERR("Failed to sigaddset[%s]", strerror(errno));
	}
	ret = sigaction(SIGTERM, &act, NULL);
	if (ret < 0) {
		ERR("Failed to sigaction[%s]", strerror(errno));
	}

	pid = setsid();
	if (pid < 0)
		WARN("Failed to set session id!");

	r = nice(2);
	if (r == -1)
		WARN("Failed to set nice value!");

	return TRUE;
}

static void quickpanel_app_terminate(void *data)
{
	DBG("");

	struct appdata *ad = data;
	retif(ad == NULL, , "invalid data.");

	/* fini quickpanel modules */
	fini_modules(ad);

	common_cache_flush(ad->evas);

	feedback_deinitialize();

	/* unregister system event callback */
	_quickpanel_ui_fini_heynoti();

	notification_daemon_shutdown();

	_quickpanel_ui_fini_ecore_event(ad);

	if (ad->cover_indicator_left != NULL) {
		evas_object_del(ad->cover_indicator_left);
		ad->cover_indicator_left = NULL;
	}
	if (ad->cover_indicator_right != NULL) {
		evas_object_del(ad->cover_indicator_right);
		ad->cover_indicator_right = NULL;
	}
	/* delete quickpanel window */
	_quickpanel_ui_delete_win(ad);

	INFO(" >>>>>>>>>>>>>>> QUICKPANEL IS TERMINATED!! <<<<<<<<<<<<<<<< ");
}

static void quickpanel_app_pause(void *data)
{
	DBG("");

	struct appdata *ad = data;
	retif(ad == NULL,, "invalid data.");

	suspend_modules(ad);

	common_cache_flush(ad->evas);
}

static void quickpanel_app_resume(void *data)
{
	DBG("");

	struct appdata *ad = data;
	retif(ad == NULL,, "invalid data.");

	resume_modules(data);
}

static void quickpanel_app_service(service_h service, void *data)
{
	struct appdata *ad = data;
	int ret = 0;

	retif(ad == NULL, , "Invialid parameter!");

	INFO(" >>>>>>>>>>>>>>> QUICKPANEL IS STARTED!! <<<<<<<<<<<<<<<< ");

	/* Check emulator */
	ad->is_emul = quickpanel_ui_check_emul();
	INFO("quickpanel run in %s", ad->is_emul ? "Emul" : "Device");

	ad->scale = elm_config_scale_get();
	if (ad->scale < 0)
		ad->scale = 1.0;

	/* Get theme */
	elm_theme_extension_add(NULL, DEFAULT_THEME_EDJ);

	/* create quickpanel window */
	ret = _quickpanel_ui_create_win(ad);
	retif(ret != QP_OK, , "Failed to create window!");

	/* init internationalization */
	notification_daemon_win_set(ad->win);

	_quickpanel_ui_init_ecore_event(ad);

	_quickpanel_ui_init_heynoti(ad);

	feedback_initialize();

#ifdef QP_SETTING_ENABLE
	_quickpanel_ui_setting_show(ad, 1);
#else /* QP_SETTING_ENABLE */
	_quickpanel_ui_setting_show(ad, 0);
#endif /* QP_SETTING_ENABLE */

	/* init quickpanel modules */
	init_modules(ad);

	ecore_idler_add(quickpanel_ui_refresh_cb, ad);
}

static void quickpanel_app_language_changed_cb(void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	INFO(" >>>>>>>>>>>>>>> LANGUAGE CHANGED!! <<<<<<<<<<<<<<<< ");
	lang_change_modules(data);
}

static void quickpanel_app_region_format_changed_cb(void *data)
{
	INFO(" >>>>>>>>>>>>>>> region_format CHANGED!! <<<<<<<<<<<<<<<< ");
}

int main(int argc, char *argv[])
{
	int r = 0;
	struct appdata ad;
	app_event_callback_s app_callback = {0,};

	r = control_privilege();
        if (r != 0) {
                WARN("Failed to control privilege!");
        }

	int heyfd = heynoti_init();
	if (heyfd > 0) {
		int ret = heynoti_subscribe(heyfd, "power_off_start", _heynoti_event_power_off, NULL);
		if (ret > 0) {
			ret = heynoti_attach_handler(heyfd);
			if (ret < 0) {
				ERR("Failed to heynoti_attach_handler[%d]", ret);
			}
		} else {
			ERR("Failed to heynoti_subscribe[%d]", ret);
		}
	} else {
		ERR("Failed to heynoti_init[%d]", heyfd);
	}

	app_callback.create = quickpanel_app_create;
	app_callback.terminate = quickpanel_app_terminate;
	app_callback.pause = quickpanel_app_pause;
	app_callback.resume = quickpanel_app_resume;
	app_callback.service = quickpanel_app_service;
	app_callback.low_memory = NULL;
	app_callback.low_battery = NULL;
	app_callback.device_orientation = NULL;
	app_callback.language_changed = quickpanel_app_language_changed_cb;
	app_callback.region_format_changed = quickpanel_app_region_format_changed_cb;

	memset(&ad, 0x0, sizeof(struct appdata));

	notification_daemon_init();

	g_app_data = &ad;

	DBG("start main");
	return app_efl_main(&argc, &argv, &app_callback, (void *)&ad);
}
