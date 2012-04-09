/*                                                                                                                                                    
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All Rights Reserved. 
 *                                                                                 
 * This file is part of quickpanel  
 *
 * Written by Jeonghoon Park <jh1979.park@samsung.com> Youngjoo Park <yjoo93.park@samsung.com>
 *                                                                                 
 * PROPRIETARY/CONFIDENTIAL                                                        
 *                                                                                 
 * This software is the confidential and proprietary information of                
 * SAMSUNG ELECTRONICS ("Confidential Information").                               
 * You shall not disclose such Confidential Information and shall use it only   
 * in accordance with the terms of the license agreement you entered into          
 * with SAMSUNG ELECTRONICS.                                                       
 * SAMSUNG make no representations or warranties about the suitability of          
 * the software, either express or implied, including but not limited to           
 * the implied warranties of merchantability, fitness for a particular purpose, 
 * or non-infringement. SAMSUNG shall not be liable for any damages suffered by 
 * licensee as a result of using, modifying or distributing this software or    
 * its derivatives.                                                                
 *                                                                                 
 */ 

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <appcore-efl.h>
#include <Ecore_X.h>
#include <heynoti.h>
#include <vconf.h>
#include <aul.h>
#include <unistd.h>
#include <privilege-control.h>
#include <iniparser.h>

#include "common.h"
#include "quickpanel-ui.h"
#include "modules.h"
#include "notifications/noti_display_app.h"

#define HIBERNATION_ENTER_NOTI	"HIBERNATION_ENTER"
#define HIBERNATION_LEAVE_NOTI	"HIBERNATION_LEAVE"

/* HD base size */
#define QP_NOTI_WINDOW_H	144
#define QP_INDICATOR_H		50
#define QP_HANDLE_H			50

/* heynoti handle */
static int g_hdl_heynoti = 0;

/* binary information */
#define QP_EMUL_STR	"emul"
#define QP_EMUL_STR_LEN	4
#define QP_BIN_INFO_PATH	"/etc/info.ini"

static int common_cache_flush(void *evas);

/**********************************************************************************
  *
  * HIBERNATION
  *
  ********************************************************************************/
static void _hibernation_enter_cb(void *data)
{
	struct appdata *ad = data;

	INFO(" >>>>>>>>>>>>>>> ENTER HIBERNATION!! <<<<<<<<<<<<<<<< ");
	hib_enter_modules(data);
	if (ad) {
		common_cache_flush(ad->noti.evas);
	}
}

static void _hibernation_leave_cb(void *data)
{
	hib_leave_modules(data);
	INFO(" >>>>>>>>>>>>>>> LEAVE HIBERNATION!! <<<<<<<<<<<<<<<< ");
}

/**********************************************************************************
  *
  * UI
  *
  ********************************************************************************/

static Eina_Bool quickpanel_ui_refresh_cb(void *data)
{
	INFO(" >>>>>>>>>>>>>>> Refresh QP Setting modules!! <<<<<<<<<<<<<<<< ");

	refresh_modules(data);

	return EINA_FALSE;
}

static int quickpanel_ui_lang_changed_cb(void *data)
{
	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	INFO(" >>>>>>>>>>>>>>> LANGUAGE CHANGED!! <<<<<<<<<<<<<<<< ");
	lang_change_modules(data);

	return QP_OK;
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

static int quickpanel_ui_low_battery_cb(void *data)
{
	return QP_OK;
}

static int _quickpanel_get_mini_win_height(void *data, int angle)
{
	struct appdata *ad = data;
	Ecore_X_Atom qp_list_atom;
	Ecore_X_Window root;
	Ecore_X_Window *qp_lists;
	int num_qp_lists, i;
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int border = 0;
	unsigned int depth = 0;
	int rel_x, rel_y;
	Ecore_X_Display *dpy = NULL;
	int ret_height = 0;
	int ret_width = 0;
	int ret_mini_height = 0;

	root = ecore_x_window_root_first_get();

	Ecore_X_Window noti_xwin = elm_win_xwindow_get(ad->noti.win);

	qp_list_atom = ecore_x_atom_get("_E_ILLUME_QUICKPANEL_WINDOW_LIST");
	if (!qp_list_atom) {
		return 0;
	}

	num_qp_lists =
	    ecore_x_window_prop_window_list_get(root, qp_list_atom, &qp_lists);

	if (num_qp_lists > 0) {
		for (i = 0; i < num_qp_lists; i++) {
			dpy = ecore_x_display_get();

			ecore_x_window_size_get(qp_lists[i], &width, &height);
			if (noti_xwin == qp_lists[i]) {
				INFO("[%d] Notification window : (%d X %d)", i,
				     width, height);
			} else {
				INFO("[%d] Other window : (%d X %d)", i, width,
				     height);
				ret_height = ret_height + height;
				ret_width = ret_width + width;

				if (height > width) {
					ret_mini_height += width;
				} else {
					ret_mini_height += height;
				}
			}
		}
	}

	if (qp_lists) {
		free(qp_lists);
	}

	INFO("ret width : %d, ret height : %d, ret_mini_height : %d", ret_width,
	     ret_height, ret_mini_height);

	return ret_mini_height;
}

static int _resize_noti_win(void *data, int new_angle)
{
	struct appdata *ad = (struct appdata *)data;
	int w = 0, h = 0;
	int diff = (ad->angle > new_angle) ?
	    (ad->angle - new_angle) : (new_angle - ad->angle);
	int mini_height = 0;

	mini_height = _quickpanel_get_mini_win_height(data, new_angle);

	int tot_h = QP_HANDLE_H * ad->scale + mini_height;

	/* get indicator height */
	ecore_x_e_illume_indicator_geometry_get(ecore_x_window_root_first_get(),
						NULL, NULL, NULL, &h);
	if (h <= 0)
		h = QP_INDICATOR_H;

	tot_h += h;

	ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
	if (diff % 180 != 0) {
		if (ad->angle % 180 == 0) {
			INFO("(2-1)Resize noti window to %d x %d diff : %d, angle : %d,  xwin(%dx%d), mini_h : %d", w - tot_h, h, diff, ad->angle, w, h, mini_height);
			evas_object_resize(ad->noti.win, w - tot_h, h);
		} else {
			INFO("(2-2)Resize noti window to %d x %d diff : %d, angle : %d, xwin(%dx%d), mini_h : %d", h - tot_h, w, diff, ad->angle, w, h, mini_height);
			evas_object_resize(ad->noti.win, h - tot_h, w);
		}
	}
	return 0;
}

static Eina_Bool quickpanel_ui_client_message_cb(void *data, int type,
						 void *event)
{
	struct appdata *ad = data;
	Ecore_X_Event_Client_Message *ev =
	    (Ecore_X_Event_Client_Message *) event;
	int new_angle;

	retif(data == NULL
	      || event == NULL, ECORE_CALLBACK_RENEW, "Invalid parameter!");

	if (ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE) {
		new_angle = ev->data.l[0];
		if (new_angle != ad->angle) {
			INFO("ROTATION: %d", new_angle);
			_resize_noti_win(ad, new_angle);
			elm_win_rotation_with_resize_set(ad->noti.win,
							 new_angle);
			ad->angle = new_angle;
		}

		ecore_idler_add(quickpanel_ui_refresh_cb, ad);
	}
	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool quickpanel_ui_window_property_cb(void *data, int type,
						  void *event)
{
	Ecore_X_Event_Window_Property *e = event;
	struct appdata *ad = data;
	Ecore_X_Window root, mini_win;
	Ecore_X_Atom qp_list_atom;
	Ecore_X_Atom mini_atom;
	int mini_height = 0, tot_h = 0, w = 0, h = 0;

	if (e == NULL || ad == NULL)
		return ECORE_CALLBACK_RENEW;

	qp_list_atom = ecore_x_atom_get("_E_ILLUME_QUICKPANEL_WINDOW_LIST");

	if (e->atom == qp_list_atom) {
		INFO("QuickPanel Window ADDED!");

		mini_height = _quickpanel_get_mini_win_height(data, ad->angle);
		tot_h = QP_HANDLE_H * ad->scale + mini_height;

		ecore_x_e_illume_indicator_geometry_get
		    (ecore_x_window_root_first_get(), NULL, NULL, NULL, &h);
		if (h <= 0) {
			h = QP_INDICATOR_H;
		}

		tot_h += h;

		ecore_x_window_size_get(ecore_x_window_root_first_get(), &w,
					&h);

		if (ad->angle % 180 == 0) {
			INFO("(1)Resize noti window to %d x %d angle : %d,  xwin(%dx%d), mini_h : %d", w, h - tot_h, ad->angle, w, h, mini_height);
			evas_object_resize(ad->noti.win, w, h - tot_h);
		} else {
			INFO("(2)Resize noti window to %d x %d angle : %d,  xwin(%dx%d), mini_h : %d", h, w - tot_h, ad->angle, w, h, mini_height);
			evas_object_resize(ad->noti.win, h, w - tot_h);
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
		elm_win_title_set(eo, name);
		elm_win_borderless_set(eo, EINA_TRUE);
		elm_win_autodel_set(eo, EINA_TRUE);
		elm_win_alpha_set(eo, EINA_TRUE);

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

static Evas_Object *_quickpanel_ui_load_edj(Evas_Object * win, const char *file,
					    const char *group)
{
	Eina_Bool r;
	Evas_Object *eo = NULL;

	retif(win == NULL, NULL, "Invalid parameter!");

	eo = elm_layout_add(win);
	retif(eo == NULL, NULL, "Failed to add layout object!");

	r = elm_layout_file_set(eo, file, group);
	retif(r != EINA_TRUE, NULL, "Failed to set edje object file!");

	evas_object_size_hint_weight_set(eo,
					 EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, eo);
	evas_object_show(eo);

	return eo;
}

static int _quickpanel_ui_create_noti(struct appdata *ad)
{
	char buf[1024] = { 0, };

	/* create noti window */
	ad->noti.win =
	    _quickpanel_ui_window_add("Quickpanel Noti Wiondow", QP_PRIO_NOTI);
	if (ad->noti.win == NULL) {
		ERR("ui create : failed to create noti window.");
	}

	/* load noti edje */
	snprintf(buf, sizeof(buf), "%s/%s_noti.edj", EDJDIR, PACKAGE);

	ad->noti.ly = _quickpanel_ui_load_edj(ad->noti.win, buf, GROUP_NOTI);
	if (ad->noti.ly == NULL) {
		INFO("ui create : failed to load %s", buf);
		/* load default theme */
		ad->noti.ly =
		    _quickpanel_ui_load_edj(ad->noti.win, DEFAULT_NOTI_EDJ,
					    GROUP_NOTI);
		if (ad->noti.ly == NULL) {
			ERR("ui create : failed to create noti theme.");
		}
	}

	/* get noti evas */
	ad->noti.evas = evas_object_evas_get(ad->noti.win);

	return 0;
}

static int _quickpanel_ui_create_win(void *data)
{
	struct appdata *ad = data;
	int w = 0;
	int h = 0;
	retif(data == NULL, QP_FAIL, "Invialid parameter!");

	/* Get resolution and scale */
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
	ad->scale = elm_config_scale_get();
	if (ad->scale < 0) {
		ad->scale = 1.0;
	}

	/* Create window */
	_quickpanel_ui_create_noti(ad);

	/* Resize window */
	if (ad->noti.win != NULL) {
		ad->noti.h =
		    h - ad->scale * (QP_INDICATOR_H + QP_HANDLE_H);
		evas_object_resize(ad->noti.win, w, ad->noti.h);
	}

	return 0;
}

static int _quickpanel_ui_delete_win(void *data)
{
	struct appdata *ad = data;
	retif(data == NULL, QP_FAIL, "Invialid parameter!");

	/* delete noti window */
	if (ad->noti.ly != NULL) {
		evas_object_del(ad->noti.ly);
		ad->noti.ly = NULL;
	}
	if (ad->noti.win != NULL) {
		evas_object_del(ad->noti.win);
		ad->noti.win = NULL;
	}
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

	/* Register window rotate event */
	hdl =
	    ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
				    quickpanel_ui_client_message_cb, ad);
	if (hdl == NULL) {
		ERR("ui init ecore : failed to add handler(ECORE_X_EVENT_CLIENT_MESSAGE)");
	}

	ad->hdl_client_message = hdl;

	/* Register quickpanel window list changed event */
	hdl =
	    ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,
				    quickpanel_ui_window_property_cb, ad);
	if (hdl == NULL) {
		ERR("ui init ecore : failed to add handler(ECORE_X_EVENT_WINDOW_PROPERTY)");
	}

	ad->hdl_win_property = hdl;
}

static void _quickpanel_ui_fini_ecore_event(struct appdata *ad)
{
	if (ad->hdl_client_message != NULL) {
		ecore_event_handler_del(ad->hdl_client_message);
		ad->hdl_client_message = NULL;
	}

	if (ad->hdl_win_property != NULL) {
		ecore_event_handler_del(ad->hdl_win_property);
		ad->hdl_win_property = NULL;
	}
}

static void _quickpanel_ui_init_appcore_event(struct appdata *ad)
{
	/* Register language chagned event */
	appcore_set_event_callback(APPCORE_EVENT_LANG_CHANGE,
				   quickpanel_ui_lang_changed_cb, ad);

	/* Register low battery event */
	appcore_set_event_callback(APPCORE_EVENT_LOW_BATTERY,
				   quickpanel_ui_low_battery_cb, ad);
}

static int _quickpanel_ui_check_emul(void)
{
	dictionary *dic = NULL;
	const char *bin_ver = NULL;
	char *str = NULL;
	char *pos = NULL;
	const char emul[QP_EMUL_STR_LEN + 1] = {0,};

	dic = iniparser_load(QP_BIN_INFO_PATH);
	if (dic == NULL) {
		/* When failed to get the info, let's regard the binary as an emulator one */
		return 1;
	}

	bin_ver = (const char*)iniparser_getstr(dic, "Version:Build");
	if (bin_ver != NULL) {
		str = strdup(bin_ver);
		if (str != NULL) {
			pos = str;
			while (*pos++) {
				if ('_' == *pos) {
					*pos = ' ';
				}
			}
			sscanf(str, "%*s %4s", emul);
		}
		free(str);
	}

	if (dic != NULL) {
		iniparser_freedict(dic);
		dic = NULL;
	}

	if (!strncmp(emul, QP_EMUL_STR, QP_EMUL_STR_LEN)) {
		return 1;
	}

	return 0;
}

/**********************************************************************************
  *
  * Appcore interface
  *
  ********************************************************************************/

static int app_create(void *data)
{
	pid_t pid;
	int r;

	pid = setsid();
	if (pid < 0) {
		fprintf(stderr, "[QUICKPANEL] Failed to set session id!\n");
	}

	r = control_privilege();
	if (r != 0) {
		fprintf(stderr, "[QUICKPANEL] Failed to control privilege!\n");
		return -1;
	}

	r = nice(2);
	if (r == -1)
		fprintf(stderr, "[QUICKPANEL] Failed to set nice value!\n");

	return 0;
}

static int app_terminate(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "invalid data.");

	/* fini quickpanel modules */
	fini_modules(ad);

	common_cache_flush(ad->noti.evas);

	/* unregister system event callback */
	_quickpanel_ui_fini_heynoti();

	notification_daemon_shutdown();

	_quickpanel_ui_fini_ecore_event(ad);

	/* delete quickpanel window */
	_quickpanel_ui_delete_win(ad);

	INFO(" >>>>>>>>>>>>>>> QUICKPANEL IS TERMINATED!! <<<<<<<<<<<<<<<< ");
	return 0;
}

static int app_pause(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "invalid data.");

	suspend_modules(ad);

	common_cache_flush(ad->noti.evas);

	return 0;
}

static int app_resume(void *data)
{
	resume_modules(data);

	return 0;
}

static int app_reset(bundle * b, void *data)
{
	struct appdata *ad = data;
	int ret = 0;

	retif(ad == NULL, QP_FAIL, "Invialid parameter!");

	INFO(" >>>>>>>>>>>>>>> QUICKPANEL IS STARTED!! <<<<<<<<<<<<<<<< ");
	/* Check emulator */
	ad->is_emul = _quickpanel_ui_check_emul();
	INFO("quickpanel run in %s", ad->is_emul? "Emul":"Device");

	/* create quickpanel window */
	ret = _quickpanel_ui_create_win(ad);
	retif(ret != QP_OK, QP_FAIL, "Failed to create window!");

	/* init internationalization */
	ret = appcore_set_i18n(PACKAGE, LOCALEDIR);
	if (ret != 0) {
		ERR("qp reset : fail to set i18n.");
		return -1;
	}
	notification_daemon_win_set(ad->noti.win);
	/* register system event callback */
	_quickpanel_ui_init_appcore_event(ad);

	_quickpanel_ui_init_ecore_event(ad);

	_quickpanel_ui_init_heynoti(ad);

	/* init quickpanel modules */
	init_modules(ad);

	ecore_idler_add(quickpanel_ui_refresh_cb, ad);

	return 0;
}

int main(int argc, char *argv[])
{
	struct appdata ad;
	struct appcore_ops ops = {
		.create = app_create,
		.terminate = app_terminate,
		.pause = app_pause,
		.resume = app_resume,
		.reset = app_reset,
	};

	memset(&ad, 0x0, sizeof(struct appdata));
	ops.data = &ad;
	notification_daemon_init();

	return appcore_efl_main(PACKAGE, &argc, &argv, &ops);
}
