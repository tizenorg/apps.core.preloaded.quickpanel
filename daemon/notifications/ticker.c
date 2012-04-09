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

#include <Elementary.h>
#include <Ecore_X.h>
#include <appcore-common.h>
#include <vconf.h>
#include <svi.h>
#include <mm_sound.h>
#include <aul.h>
#include <notification.h>
#include <time.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "noti_win.h"

#define QP_TICKER_DURATION	5
#define QP_TICKER_DETAIL_DURATION 6

#define TICKER_MSG_LEN				1024
#define DEFAULT_ICON ICONDIR		"/quickpanel_icon_default.png"

static Evas_Object *g_window = NULL;
static Evas_Object *g_ticker = NULL;
static Ecore_Timer *g_timer = NULL;
static notification_list_h g_latest_noti_list = NULL;
static int g_svi = 0;

static int quickpanel_ticker_init(void *data);
static int quickpanel_ticker_fini(void *data);
static int quickpanel_ticker_enter_hib(void *data);
static int quickpanel_ticker_leave_hib(void *data);

QP_Module ticker = {
	.name = "ticker",
	.init = quickpanel_ticker_init,
	.fini = quickpanel_ticker_fini,
	.hib_enter = quickpanel_ticker_enter_hib,
	.hib_leave = quickpanel_ticker_leave_hib,
	.lang_changed = NULL
};

static int latest_inserted_time;

/**************************************************************************************
 *
 * (Static) Util functions
 *
 *************************************************************************************/

static int _quickpanel_ticker_check_setting_event_value(notification_h noti)
{
	char *pkgname = NULL;
	char key[512] = { 0, };
	int ret = 0;
	int boolval = 0;

	notification_get_application(noti, &pkgname);

	if (pkgname == NULL) {
		notification_get_pkgname(noti, &pkgname);
	}

	if (pkgname == NULL) {
		return -1;	// Ticker is not displaying.
	}

	if (!strcmp(pkgname, "org.tizen.message")) {
		ret =
		    vconf_get_bool
		    (VCONFKEY_SETAPPL_STATE_TICKER_NOTI_MESSAGES_BOOL,
		     &boolval);
		if (ret == 0 && boolval == 0) {
			return -1;	// Ticker is not displaying.
		}

	} else if (!strcmp(pkgname, "org.tizen.email")) {
		ret =
		    vconf_get_bool
		    (VCONFKEY_SETAPPL_STATE_TICKER_NOTI_EMAIL_BOOL, &boolval);
		if (ret == 0 && boolval == 0) {
			return -1;	// Ticker is not displaying.
		}
	} else if (!strcmp(pkgname, "org.tizen.download-provider")) {
		ret =
		    vconf_get_bool
		    (VCONFKEY_SETAPPL_STATE_TICKER_NOTI_FILE_TRAN_BOOL,
		     &boolval);
		if (ret == 0 && boolval == 0) {
			return -1;	// Ticker is not displaying.
		}
	}
	// TODO: VCONFKEY_SETAPPL_STATE_TICKER_NOTI_VMAIL_BOOL, VCONFKEY_SETAPPL_STATE_TICKER_NOTI_SNS_BOOL

	// Check 3rd App
	snprintf(key, sizeof(key), "db/app-settings/noti-enabled/%s", pkgname);
	ret = vconf_get_int(key, &boolval);
	INFO("Key : %s == %d, ret : %d", key, boolval, ret);
	if (ret == 0 && boolval == 0) {
		return -1;	// Ticker is not displaying.
	}

	return 0;		// Displaying ticker!
}

static Eina_Bool _quickpanel_ticker_hide(void *data)
{
	INFO("_quickpanel_ticker_hide");
	char *pkgname = NULL;
	int applist = 0;
	int priv_id = 0;

	if (g_ticker) {
		evas_object_hide(g_ticker);
		evas_object_del(g_ticker);
		g_ticker = NULL;
	}

	if (g_timer) {
		g_timer = NULL;
	}

	if (g_latest_noti_list) {
		// delete here only ticker noti display item
		notification_h noti =
		    notification_list_get_data(g_latest_noti_list);
		notification_get_display_applist(noti, &applist);
		if (applist & NOTIFICATION_DISPLAY_APP_TICKER) {
			if (applist &
			    NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {

			} else {
				//delete
				notification_get_pkgname(noti, &pkgname);
				notification_get_id(noti, NULL, &priv_id);
				INFO("hide : pkgname : %s, priv_id : %d",
				     pkgname, priv_id);
				notification_delete_by_priv_id(pkgname,
							       NOTIFICATION_TYPE_NONE,
							       priv_id);
			}
		}

		notification_free_list(g_latest_noti_list);
		g_latest_noti_list = NULL;
	}
}

static void _quickpanel_ticker_detail_hide_cb(void *data, Evas * e,
					      Evas_Object * obj,
					      void *event_info)
{
	INFO("_quickpanel_ticker_detail_hide_cb");
	Evas_Object *tickernoti = (Evas_Object *) data;

	if (g_timer) {
		ecore_timer_del(g_timer);
		g_timer = NULL;
	}

	evas_object_hide(tickernoti);
	_quickpanel_ticker_hide(NULL);
}

static void _quickpanel_ticker_detail_show_cb(void *data, Evas * e,
					      Evas_Object * obj,
					      void *event_info)
{
	INFO("_quickpanel_ticker_detail_show_cb");
	Evas_Object *tickernoti = (Evas_Object *) data;

	if (g_timer) {
		ecore_timer_delay(g_timer, QP_TICKER_DETAIL_DURATION);
	}

	evas_object_hide(tickernoti);

	/* Use style "default" for detailview mode and "info" for text only mode */
	elm_object_style_set(tickernoti,"default");
	evas_object_show(tickernoti);
}

static void _quickpanel_ticker_clicked_cb(void *data, Evas_Object * obj,
					  void *event_info)
{
	INFO("_quickpanel_ticker_clicked_cb");
	notification_h noti = (notification_h) data;
	char *caller_pkgname = NULL;
	char *pkgname = NULL;
	bundle *args = NULL;
	bundle *single_service_handle = NULL;
	int group_id = 0;
	int priv_id = 0;
	int flags = 0;
	int ret = 0;
	int val = 0;
	int flag_launch = 0;
	int flag_delete = 0;
	int type = NOTIFICATION_TYPE_NONE;

	retif(noti == NULL,, "Invalid parameter!");

	/* Check idle lock state */
	ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val);
	if(ret != 0 || val == VCONFKEY_IDLE_LOCK) {
		/* If Lock state, there is not any action when clicked. */
		return;
	}

	notification_get_pkgname(noti, &caller_pkgname);
	notification_get_application(noti, &pkgname);
	if (pkgname == NULL) {
		pkgname = caller_pkgname;
	}

	notification_get_id(noti, &group_id, &priv_id);
	notification_get_property(noti, &flags);

	if (flags & NOTIFICATION_PROP_DISABLE_APP_LAUNCH) {
		flag_launch = 0;
	} else {
		flag_launch = 1;
	}

	if (flags & NOTIFICATION_PROP_DISABLE_AUTO_DELETE) {
		flag_delete = 0;
	} else {
		flag_delete = 1;
	}


	notification_get_execute_option(noti,
					NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH,
					NULL, &single_service_handle);

	if (flag_launch == 1) {
		if (single_service_handle != NULL) {
			appsvc_run_service(single_service_handle, 0, NULL,
					   NULL);
		} else {
			notification_get_args(noti, &args, NULL);
			aul_launch_app(pkgname, args);
		}

		// Hide quickpanel
		Ecore_X_Window zone;
		zone = ecore_x_e_illume_zone_get(elm_win_xwindow_get(g_window));
		ecore_x_e_illume_quickpanel_state_send(zone,
							   ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
	}

	notification_get_type(noti, &type);
	if (flag_delete == 1 && type == NOTIFICATION_TYPE_NOTI) {
		notification_delete_group_by_priv_id(caller_pkgname,
							 NOTIFICATION_TYPE_NOTI,
							 priv_id);
	}
}

static void _quickpanel_ticker_button_clicked_cb(void *data, Evas_Object * obj,
						 void *event_info)
{
	if (g_timer) {
		ecore_timer_del(g_timer);
		g_timer = NULL;
	}

	_quickpanel_ticker_hide(NULL);
}

static Evas_Object *_quickpanel_ticker_create_button(Evas_Object * parent,
						     notification_h noti)
{
	Evas_Object *button = NULL;
	int ret = 0;
	int val = 0;

	retif(noti == NULL || parent == NULL, NULL, "Invalid parameter!");

	/* Check idle lock state */
	ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val);
	if(ret != 0 || val == VCONFKEY_IDLE_LOCK) {
		/* If Lock state, button is diabled */
		return NULL;
	}

	button = elm_button_add(parent);
	elm_object_style_set(button, "tickernoti");
	elm_object_text_set(button, _S("IDS_COM_BODY_CLOSE"));
	evas_object_smart_callback_add(button, "clicked",
				       _quickpanel_ticker_button_clicked_cb,
				       noti);

	return button;
}

static Evas_Object *_quickpanel_ticker_create_icon(Evas_Object * parent,
						   notification_h noti)
{
	char *icon_path = NULL;
	Evas_Object *icon = NULL;

	retif(noti == NULL || parent == NULL, NULL, "Invalid parameter!");

	notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, &icon_path);
	icon = elm_icon_add(parent);

	if (icon_path == NULL
	    || (elm_icon_file_set(icon, icon_path, NULL) == EINA_FALSE)) {
		elm_icon_file_set(icon, DEFAULT_ICON, NULL);
		elm_icon_resizable_set(icon, EINA_TRUE, EINA_TRUE);
	}

	return icon;
}

static char *_quickpanel_ticker_get_label(notification_h noti)
{
	char buf[TICKER_MSG_LEN] = { 0, };
	int len = 0;
	char *domain = NULL;
	char *dir = NULL;
	char *get_title = NULL;
	char *get_content = NULL;
	char *result_title = NULL;
	char *result_content = NULL;
	char *title_utf8 = NULL;
	char *content_utf8 = NULL;
	int ret = 0;
	int boolval = 0;
	char *pkgname = NULL;

	retif(noti == NULL, NULL, "Invalid parameter!");

	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL)
		bindtextdomain(domain, dir);

	notification_get_text(noti, NOTIFICATION_TEXT_TYPE_TITLE,
			&result_title);

	ret = vconf_get_bool(
		VCONFKEY_SETAPPL_STATE_TICKER_NOTI_DISPLAY_CONTENT_BOOL,
		&boolval);

	notification_get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
			&result_content);
	if (result_title)
		title_utf8 = elm_entry_utf8_to_markup(result_title);

	if (result_content)
		content_utf8 = elm_entry_utf8_to_markup(result_content);

	if (title_utf8 && content_utf8) {
		len = snprintf(buf, sizeof(buf),
			"<font_size=26><color=#BABABA>%s</color></font>"
			"<br><font_size=29><color=#F4F4F4>%s</color></font>",
			title_utf8, content_utf8);
	} else if (title_utf8) {
		len = snprintf(buf, sizeof(buf),
			"<font_size=29><color=#BABABA>%s</color></font>",
			title_utf8);
	}

	if (title_utf8)
		free(title_utf8);

	if (content_utf8)
		free(content_utf8);

	if (len > 0)
		return strdup(buf);

	return NULL;
}

static void _noti_hide_cb(void *data, Evas_Object *obj,
	const char *emission, const char *source)
{
	if (g_timer) {
		ecore_timer_del(g_timer);
		g_timer = NULL;
	}

	_quickpanel_ticker_hide(NULL);
}

static Evas_Object *_quickpanel_ticker_create_tickernoti(void *data)
{
	notification_h noti = (notification_h) data;
	Evas_Object *tickernoti = NULL;
	Evas_Object *icon = NULL;
	Evas_Object *detail = NULL;
	Evas_Object *button = NULL;
	char *buf = NULL;
	const char *data_win_height = NULL;
	int noti_height = 50;

	retif(noti == NULL, NULL, "Invalid parameter!");

	// tickernoti
	tickernoti = noti_win_add(NULL);
	retif(tickernoti == NULL, NULL, "Failed to add elm tickernoti.");

	// detail view
	detail = elm_layout_add(tickernoti);
	if (!detail) {
		ERR("Failed to get detailview.");
		evas_object_del(tickernoti);
		return NULL;
	}
	elm_layout_theme_set(detail, "tickernoti", "base", "default");
	elm_object_signal_callback_add(detail, "request,hide", "", _noti_hide_cb, NULL);
	data_win_height = (char *)elm_layout_data_get(detail, "height");
	if (data_win_height != NULL && elm_config_scale_get() > 0.0)
		noti_height = (int)(elm_config_scale_get() * atoi(data_win_height));
	evas_object_size_hint_min_set(detail, 1, noti_height);
	noti_win_content_set(tickernoti, detail);
	// icon for detail view
	icon = _quickpanel_ticker_create_icon(detail, noti);
	if (icon != NULL)
		elm_object_part_content_set(detail, "icon", icon);

	// button for detail view
	button = _quickpanel_ticker_create_button(detail, noti);
	if (button != NULL)
		elm_object_part_content_set(detail, "button", button);

	// label
	buf = _quickpanel_ticker_get_label(noti);
	if (buf != NULL) {
		elm_object_part_text_set(detail, "elm.text", buf);
		free(buf);
	}

	/* Use style "default" for detailview mode and "info" for text only mode */
	elm_object_style_set(tickernoti,"default");

	return tickernoti;
}

static int _quickpanel_ticker_get_angle(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	Ecore_X_Window xwin, root;
	int ret = 0, angle = 0, count = 0;
	unsigned char *prop_data = NULL;

	xwin = elm_win_xwindow_get(ad->noti.win);
	root = ecore_x_window_root_get(xwin);

	ret =
	    ecore_x_window_prop_property_get(root,
					     ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE,
					     ECORE_X_ATOM_CARDINAL, 32,
					     &prop_data, &count);

	if (ret && prop_data) {
		memcpy(&angle, prop_data, sizeof(int));

		if (prop_data) {
			free(prop_data);
		}

		return angle;
	} else {
		ERR("Fail to get angle");
		if (prop_data) {
			free(prop_data);
		}

		return -1;
	}
}

void _quickpanel_ticker_noti_changed_cb(void *data, notification_type_e type)
{
	notification_list_h noti_list = NULL;
	notification_h noti = NULL;
	notification_h noti_prev = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	int angle = 0;
	int current_inserted_time = 0;
	time_t insert_time;
	int flags = 0;
	int applist = NOTIFICATION_DISPLAY_APP_ALL;
	int ret = 0;

	INFO("_quickpanel_ticker_noti_changed_cb");

	// Get latest item
	noti_err = notification_get_grouping_list(type, 1, &noti_list);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_get_grouping_list : %d", noti_err);
		return;
	}

	noti = notification_list_get_data(noti_list);
	if (noti == NULL) {
		ERR("Fail to notification_list_get_data");
		notification_free_list(noti_list);
		return;
	}

	noti_err = notification_get_insert_time(noti, &insert_time);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_get_insert_time(%d)", noti_err);
		notification_free_list(noti_list);
		return;
	}

	char *pkgname = NULL;
	int priv_id = 0;
	// Save latest item's inserted time
	current_inserted_time = (int)insert_time;
	if (latest_inserted_time >= current_inserted_time) {
		// delete temporary here only ticker noti display item
		notification_get_display_applist(noti, &applist);
		if (applist & NOTIFICATION_DISPLAY_APP_TICKER) {
			if (applist &
			    NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {

			} else {
				//delete
				notification_get_pkgname(noti, &pkgname);
				notification_get_id(noti, NULL, &priv_id);
				notification_delete_by_priv_id(pkgname,
							       NOTIFICATION_TYPE_NONE,
							       priv_id);
			}
		}

		notification_free_list(noti_list);
		return;
	}
	latest_inserted_time = current_inserted_time;

	// Play sound
	notification_sound_type_e nsound_type = NOTIFICATION_SOUND_TYPE_NONE;
	const char *nsound_path = NULL;

	notification_get_sound(noti, &nsound_type, &nsound_path);
	DBG("Sound : %d, %s", nsound_type, nsound_path);
	if (nsound_type > NOTIFICATION_SOUND_TYPE_NONE
	    || nsound_type < NOTIFICATION_SOUND_TYPE_MAX) {
		if (g_svi == 0) {
			svi_init(&g_svi);
		}

		switch (nsound_type) {
		case NOTIFICATION_SOUND_TYPE_DEFAULT:
			svi_play_sound(g_svi, SVI_SND_OPERATION_NEWCHAT);
			break;
		case NOTIFICATION_SOUND_TYPE_USER_DATA:
			mm_sound_play_sound(nsound_path,
					    VOLUME_TYPE_NOTIFICATION, NULL,
					    NULL, NULL);
			break;
		default:
			break;
		}
	}
	// Play Vibration
	notification_vibration_type_e nvibration_type =
	    NOTIFICATION_VIBRATION_TYPE_NONE;
	const char *nvibration_path = NULL;

	notification_get_vibration(noti, &nvibration_type, &nvibration_path);
	DBG("Vibration : %d, %s", nvibration_type, nvibration_path);
	if (nvibration_type > NOTIFICATION_VIBRATION_TYPE_NONE
	    || nvibration_type < NOTIFICATION_VIBRATION_TYPE_MAX) {
		if (g_svi == 0) {
			svi_init(&g_svi);
		}

		switch (nvibration_type) {
		case NOTIFICATION_SOUND_TYPE_DEFAULT:
			svi_play_vib(g_svi, SVI_VIB_OPERATION_NEWCHAT);
			break;
		case NOTIFICATION_SOUND_TYPE_USER_DATA:
			break;
		default:
			break;
		}
	}
	// Skip if previous ticker is still shown
	if (g_ticker) {
		// delete temporary here only ticker noti display item
		notification_get_display_applist(noti, &applist);
		if (applist & NOTIFICATION_DISPLAY_APP_TICKER) {
			if (applist &
			    NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {

			} else {
				//delete
				notification_get_pkgname(noti, &pkgname);
				notification_get_id(noti, NULL, &priv_id);
				notification_delete_by_priv_id(pkgname,
							       NOTIFICATION_TYPE_NONE,
							       priv_id);
			}
		}

		notification_free_list(noti_list);
		return;
	}
	// Check setting's event  notificcation
	ret = _quickpanel_ticker_check_setting_event_value(noti);

	// Check tickernoti flag
	notification_get_property(noti, &flags);
	notification_get_display_applist(noti, &applist);

	if (flags & NOTIFICATION_PROP_DISABLE_TICKERNOTI || ret < 0) {
		INFO("Disable tickernoti ret : %d", ret);
	} else if (applist & NOTIFICATION_DISPLAY_APP_TICKER) {
		// Display ticker
		// Add timer
		if (g_timer) {
			ecore_timer_del(g_timer);
		}
		g_timer =
		    ecore_timer_add(QP_TICKER_DURATION, _quickpanel_ticker_hide,
				    NULL);

		// Show ticker
		g_ticker = _quickpanel_ticker_create_tickernoti(noti);
		if (g_ticker == NULL) {
			ERR("Fail to create tickernoti");
			return;
		}
		// Set rotate
		angle = _quickpanel_ticker_get_angle(data);
		if (angle > 0) {
			elm_win_rotation_with_resize_set(g_ticker, angle);
		}

		evas_object_show(g_ticker);

		evas_object_event_callback_add(g_ticker, EVAS_CALLBACK_SHOW,
					       _quickpanel_ticker_detail_show_cb,
					       g_ticker);
		evas_object_event_callback_add(g_ticker, EVAS_CALLBACK_HIDE,
					       _quickpanel_ticker_detail_hide_cb,
					       g_ticker);

		evas_object_smart_callback_add(g_ticker, "clicked",
					       _quickpanel_ticker_clicked_cb,
					       noti);
	}

	if (g_latest_noti_list) {
		notification_free_list(g_latest_noti_list);
	}
	g_latest_noti_list = noti_list;
}

/**************************************************************************************
 *
 * Util functions
 *
 *************************************************************************************/
static int quickpanel_ticker_init(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	latest_inserted_time = time(NULL);
	g_window = ad->noti.win;

	notification_resister_changed_cb(_quickpanel_ticker_noti_changed_cb,
					 data);

	return QP_OK;
}

static int quickpanel_ticker_fini(void *data)
{
	_quickpanel_ticker_hide(NULL);

	if (g_svi != 0) {
		svi_fini(g_svi);
		g_svi = 0;
	}

	return QP_OK;
}

static int quickpanel_ticker_enter_hib(void *data)
{
	return QP_OK;
}

static int quickpanel_ticker_leave_hib(void *data)
{
	return QP_OK;
}
