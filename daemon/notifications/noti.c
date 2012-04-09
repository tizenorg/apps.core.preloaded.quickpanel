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

#include <aul.h>
#include <appsvc.h>

#include <time.h>
#include <vconf.h>
#include <appcore-common.h>
#include <Ecore_X.h>

#include <unicode/uloc.h>
#include <unicode/udat.h>
#include <unicode/udatpg.h>
#include <unicode/ustring.h>

#include <notification.h>

#include "quickpanel-ui.h"
#include "common.h"

#define QP_DEFAULT_ICON	ICONDIR"/quickpanel_icon_default.png"
#define QP_NOTI_DAY_DEC	(24 * 60 * 60)

static int suspended = 0;

static notification_list_h g_notification_list = NULL;
static notification_list_h g_notification_ongoing_list = NULL;

static Elm_Genlist_Item_Class *itc_noti;
static Elm_Genlist_Item_Class *g_itc;

static Evas_Object *g_window = NULL;

static int quickpanel_noti_init(void *data);
static int quickpanel_noti_fini(void *data);
static int quickpanel_noti_suspend(void *data);
static int quickpanel_noti_resume(void *data);
static void quickpanel_noti_lang_changed(void *data);

QP_Module noti = {
	.name = "noti",
	.init = quickpanel_noti_init,
	.fini = quickpanel_noti_fini,
	.suspend = quickpanel_noti_suspend,
	.resume = quickpanel_noti_resume,
	.lang_changed = quickpanel_noti_lang_changed,
};

static void _quickpanel_noti_clear_clicked_cb(void *data, Evas_Object * obj,
					      void *event_info)
{
	int count = 0;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	noti_err =
	    notification_get_count(NOTIFICATION_TYPE_NOTI, NULL,
				   NOTIFICATION_GROUP_ID_NONE,
				   NOTIFICATION_PRIV_ID_NONE, &count);

	if (noti_err == NOTIFICATION_ERROR_NONE && count > 0) {
		notifiation_clear(NOTIFICATION_TYPE_NOTI);
	}

	DBG("Clear Clicked : noti_err(%d), count(%d)", noti_err, count);
}

static char *_quickpanel_noti_get_progress(notification_h noti, char *buf,
					   int buf_len)
{
	double size = 0.0;
	double percentage = 0.0;

	retif(noti == NULL, NULL, "Invalid parameter!");

	notification_get_size(noti, &size);
	notification_get_progress(noti, &percentage);

	if (percentage < 1 && percentage > 0) {
		if (snprintf(buf, buf_len, "%d%%", (int)(percentage * 100)) <=
		    0) {
			return NULL;
		}
		return buf;
	} else if (size > 0) {
		if (size > (1 << 30)) {
			if (snprintf
			    (buf, buf_len, "%.1lfGB",
			     size / 1000000000.0) <= 0) {
				return NULL;
			}
			return buf;
		} else if (size > (1 << 20)) {
			if (snprintf(buf, buf_len, "%.1lfMB", size / 1000000.0)
			    <= 0) {
				return NULL;
			}
			return buf;
		} else if (size > (1 << 10)) {
			if (snprintf(buf, buf_len, "%.1lfKB", size / 1000.0) <=
			    0) {
				return NULL;
			}
			return buf;
		} else {
			if (snprintf(buf, buf_len, "%lfB", size) <= 0) {
				return NULL;
			}
			return buf;
		}
	}

	return NULL;
}

static notification_h _quickpanel_noti_update_item_progress(const char *pkgname,
							    int priv_id,
							    double progress)
{
	notification_h noti = NULL;
	notification_list_h head = NULL;
	char *noti_pkgname = NULL;
	int noti_priv_id = 0;

	if (g_notification_ongoing_list) {
		head = notification_list_get_head(g_notification_ongoing_list);

		while (head != NULL) {
			noti = notification_list_get_data(head);
			notification_get_pkgname(noti, &noti_pkgname);
			notification_get_id(noti, NULL, &noti_priv_id);
			if (!strcmp(noti_pkgname, pkgname)
			    && priv_id == noti_priv_id) {
				notification_set_progress(noti, progress);
				return noti;
			}
			head = notification_list_get_next(head);
		}
	}

	if (g_notification_list) {
		head = notification_list_get_head(g_notification_list);

		while (head != NULL) {
			noti = notification_list_get_data(head);
			notification_get_pkgname(noti, &noti_pkgname);
			notification_get_id(noti, NULL, &noti_priv_id);
			if (!strcmp(noti_pkgname, pkgname)
			    && priv_id == noti_priv_id) {
				notification_set_progress(noti, progress);
				return noti;
			}
			head = notification_list_get_next(head);
		}
	}

	return NULL;
}

static notification_h _quickpanel_noti_update_item_size(const char *pkgname,
							int priv_id,
							double size)
{
	notification_h noti = NULL;
	notification_list_h head = NULL;
	char *noti_pkgname = NULL;
	int noti_priv_id = 0;

	if (g_notification_ongoing_list) {
		head = notification_list_get_head(g_notification_ongoing_list);

		while (head != NULL) {
			noti = notification_list_get_data(head);
			notification_get_pkgname(noti, &noti_pkgname);
			notification_get_id(noti, NULL, &noti_priv_id);
			if (!strcmp(noti_pkgname, pkgname)
			    && priv_id == noti_priv_id) {
				notification_set_size(noti, size);
				return noti;
			}
			head = notification_list_get_next(head);
		}
	}

	if (g_notification_list) {
		head = notification_list_get_head(g_notification_list);

		while (head != NULL) {
			noti = notification_list_get_data(head);
			notification_get_pkgname(noti, &noti_pkgname);
			notification_get_id(noti, NULL, &noti_priv_id);
			if (!strcmp(noti_pkgname, pkgname)
			    && priv_id == noti_priv_id) {
				notification_set_size(noti, size);
				return noti;
			}
			head = notification_list_get_next(head);
		}
	}

	return NULL;
}

static notification_h _quickpanel_noti_update_item_content(const char *pkgname,
							int priv_id,
							char *content)
{
	notification_h noti = NULL;
	notification_list_h head = NULL;
	char *noti_pkgname = NULL;
	int noti_priv_id = 0;

	if (g_notification_ongoing_list) {
		head = notification_list_get_head(g_notification_ongoing_list);

		while (head != NULL) {
			noti = notification_list_get_data(head);
			notification_get_pkgname(noti, &noti_pkgname);
			notification_get_id(noti, NULL, &noti_priv_id);
			if (!strcmp(noti_pkgname, pkgname)
			    && priv_id == noti_priv_id) {
				notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
					content, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
				return noti;
			}
			head = notification_list_get_next(head);
		}
	}

	if (g_notification_list) {
		head = notification_list_get_head(g_notification_list);

		while (head != NULL) {
			noti = notification_list_get_data(head);
			notification_get_pkgname(noti, &noti_pkgname);
			notification_get_id(noti, NULL, &noti_priv_id);
			if (!strcmp(noti_pkgname, pkgname)
			    && priv_id == noti_priv_id) {
				notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
					content, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
				return noti;
			}
			head = notification_list_get_next(head);
		}
	}

	return NULL;
}

static void _quickpanel_noti_update_progressbar(void *data,
						notification_h update_noti)
{
	Evas_Object *eo = (Evas_Object *) data;
	Elm_Object_Item *head = NULL;
	Elm_Object_Item *it = NULL;
	notification_h noti = NULL;

	head = elm_genlist_first_item_get(eo);
	retif(head == NULL,, "no first item in genlist");

	for (it = head; it != NULL; it = elm_genlist_item_next_get(it)) {
		noti = elm_object_item_data_get(it);
		if (noti == update_noti || update_noti == NULL) {
			elm_genlist_item_update(it);
		}
	}
}

static void _quickpanel_noti_item_progress_update_cb(void *data,
						     DBusMessage * msg)
{
	DBusError err;
	char *pkgname = 0;
	int priv_id = 0;
	double progress = 0;
	notification_h noti = NULL;

	retif(data == NULL || msg == NULL,, "Invalid parameter!");

	dbus_error_init(&err);
	dbus_message_get_args(msg, &err,
			      DBUS_TYPE_STRING, &pkgname,
			      DBUS_TYPE_INT32, &priv_id,
			      DBUS_TYPE_DOUBLE, &progress, DBUS_TYPE_INVALID);

	/* check item on the list */
	noti =
	    _quickpanel_noti_update_item_progress(pkgname, priv_id, progress);
	retif(noti == NULL,, "Can not found noti data.");

	if (!suspended) {
		_quickpanel_noti_update_progressbar(data, noti);
	}
}

static void _quickpanel_noti_item_size_update_cb(void *data, DBusMessage * msg)
{
	DBusError err;
	char *pkgname = 0;
	int priv_id = 0;
	double size = 0;
	notification_h noti = NULL;

	retif(data == NULL || msg == NULL,, "Invalid parameter!");

	dbus_error_init(&err);
	dbus_message_get_args(msg, &err,
			      DBUS_TYPE_STRING, &pkgname,
			      DBUS_TYPE_INT32, &priv_id,
			      DBUS_TYPE_DOUBLE, &size, DBUS_TYPE_INVALID);

	/* check item on the list */
	noti = _quickpanel_noti_update_item_size(pkgname, priv_id, size);
	retif(noti == NULL,, "Can not found noti data.");

	if (!suspended) {
		_quickpanel_noti_update_progressbar(data, noti);
	}
}

static void _quickpanel_noti_item_content_update_cb(void *data, DBusMessage * msg)
{
	DBusError err;
	char *pkgname = NULL;
	int priv_id = 0;
	char *content = NULL;
	notification_h noti = NULL;

	retif(data == NULL || msg == NULL,, "Invalid parameter!");

	dbus_error_init(&err);
	dbus_message_get_args(msg, &err,
			      DBUS_TYPE_STRING, &pkgname,
			      DBUS_TYPE_INT32, &priv_id,
			      DBUS_TYPE_STRING, &content, DBUS_TYPE_INVALID);

	INFO("content update : %s(%d) : %s", pkgname, priv_id, content);

	/* check item on the list */
	noti = _quickpanel_noti_update_item_content(pkgname, priv_id, content);
	retif(noti == NULL,, "Can not found noti data.");

	if (!suspended) {
		_quickpanel_noti_update_progressbar(data, noti);
	}
}

static char *_quickpanel_noti_get_time(time_t t, char *buf, int buf_len)
{
	UErrorCode status = U_ZERO_ERROR;
	UDateTimePatternGenerator *generator;
	UDateFormat *formatter;
	UChar skeleton[40] = { 0 };
	UChar pattern[40] = { 0 };
	UChar formatted[40] = { 0 };
	int32_t patternCapacity, formattedCapacity;
	int32_t skeletonLength, patternLength, formattedLength;
	UDate date;
	const char *locale;
	const char customSkeleton[] = UDAT_YEAR_NUM_MONTH_DAY;
	char bf1[32] = { 0, };
	enum appcore_time_format timeformat;

	struct tm loc_time;
	time_t today, yesterday;
	int ret = 0;

	today = time(NULL);
	localtime_r(&today, &loc_time);

	loc_time.tm_sec = 0;
	loc_time.tm_min = 0;
	loc_time.tm_hour = 0;
	today = mktime(&loc_time);

	yesterday = today - QP_NOTI_DAY_DEC;

	localtime_r(&t, &loc_time);

	if (t >= yesterday && t < today) {
		ret = snprintf(buf, buf_len, _S("IDS_COM_BODY_YESTERDAY"));
	} else if (t < yesterday) {
		/* set UDate  from time_t */
		date = (UDate) t *1000;

		/* get default locale  */
		uloc_setDefault(__secure_getenv("LC_TIME"), &status);	/* for thread saftey  */
		locale = uloc_getDefault();

		/* open datetime pattern generator */
		generator = udatpg_open(locale, &status);
		if (generator == NULL)
			return NULL;

		/* calculate pattern string capacity */
		patternCapacity =
		    (int32_t) (sizeof(pattern) / sizeof((pattern)[0]));

		/* ascii to unicode for input skeleton */
		u_uastrcpy(skeleton, customSkeleton);

		/* get skeleton length */
		skeletonLength = strlen(customSkeleton);

		/* get best pattern using skeleton */
		patternLength =
		    udatpg_getBestPattern(generator, skeleton, skeletonLength,
					  pattern, patternCapacity, &status);

		/* open datetime formatter using best pattern */
		formatter =
		    udat_open(UDAT_IGNORE, UDAT_DEFAULT, locale, NULL, -1,
			      pattern, patternLength, &status);
		if (formatter == NULL) {
			udatpg_close(generator);
			return NULL;
		}

		/* calculate formatted string capacity */
		formattedCapacity =
		    (int32_t) (sizeof(formatted) / sizeof((formatted)[0]));

		/* formatting date using formatter by best pattern */
		formattedLength =
		    udat_format(formatter, date, formatted, formattedCapacity,
				NULL, &status);

		/* unicode to ascii to display */
		u_austrcpy(bf1, formatted);

		/* close datetime pattern generator */
		udatpg_close(generator);

		/* close datetime formatter */
		udat_close(formatter);

		ret = snprintf(buf, buf_len, "%s", bf1);
	} else {
		ret = appcore_get_timeformat(&timeformat);
		if (ret == 0 && timeformat == APPCORE_TIME_FORMAT_24) {
			ret = strftime(buf, buf_len, "%H:%M", &loc_time);
		} else {
			strftime(bf1, sizeof(bf1), "%l:%M", &loc_time);

			if (loc_time.tm_hour >= 0 && loc_time.tm_hour < 12) {
				ret = snprintf(buf, buf_len, "%s%s", bf1, "AM");
			} else {
				ret = snprintf(buf, buf_len, "%s%s", bf1, "PM");
			}
		}

	}

	return ret <= 0 ? NULL : buf;
}

static Evas_Object *_quickpanel_noti_gl_get_content(void *data, Evas_Object * obj,
						 const char *part)
{
	notification_h noti = (notification_h) data;
	Evas_Object *ic = NULL;
	char *icon_path = NULL;
	char *thumbnail_path = NULL;
	char *ret_path = NULL;
	double size = 0.0;
	double percentage = 0.0;
	notification_type_e type = NOTIFICATION_TYPE_NONE;

	retif(noti == NULL, NULL, "Invalid parameter!");

	if (!strncmp
	    (part, "elm.swallow.progress", strlen("elm.swallow.progress"))) {
		notification_get_type(noti, &type);
		if (type == NOTIFICATION_TYPE_ONGOING) {
			notification_get_size(noti, &size);
			notification_get_progress(noti, &percentage);

			if (percentage > 0 && percentage <= 1) {
				ic = elm_progressbar_add(obj);
				if (ic == NULL) {
					return NULL;
				}

				elm_object_style_set(ic, "list_progress");
				elm_progressbar_value_set(ic, percentage);
				elm_progressbar_horizontal_set(ic, EINA_TRUE);
				elm_progressbar_pulse(ic, EINA_FALSE);
			} else if (size > 0) {
				ic = elm_progressbar_add(obj);
				if (ic == NULL) {
					return NULL;
				}
				elm_object_style_set(ic, "pending_list");
				elm_progressbar_horizontal_set(ic, EINA_TRUE);
				elm_progressbar_pulse(ic, EINA_TRUE);
			}
		}
		return ic;
	}

	ic = elm_icon_add(obj);
	retif(ic == NULL, NULL, "Failed to create elm icon.");

	notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_THUMBNAIL,
			       &thumbnail_path);
	notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, &icon_path);

	if (!strncmp
	    (part, "elm.swallow.thumbnail", strlen("elm.swallow.thumbnail"))) {
		if (thumbnail_path == NULL) {
			ret_path = icon_path;
		} else {
			ret_path = thumbnail_path;
		}

		elm_icon_resizable_set(ic, EINA_FALSE, EINA_TRUE);

		if (ret_path == NULL
		    || (elm_icon_file_set(ic, ret_path, NULL) == EINA_FALSE)) {
			elm_icon_file_set(ic, QP_DEFAULT_ICON, NULL);
		}
	} else
	    if (!strncmp(part, "elm.swallow.icon", strlen("elm.swallow.icon")))
	{
		if (thumbnail_path == NULL) {
			ret_path = NULL;
		} else {
			ret_path = icon_path;
		}

		if (ret_path != NULL) {
			elm_icon_file_set(ic, ret_path, NULL);
		}
	}

	return ic;
}

static char *_quickpanel_noti_gl_get_text(void *data, Evas_Object * obj,
					   const char *part)
{
	notification_h noti = (notification_h) data;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	char *text = NULL;
	char *str = NULL;
	char *loc_str = NULL;
	char *get_loc_str = NULL;
	char *domain = NULL;
	char *dir = NULL;
	char *temp = NULL;
	char *pkgname = NULL;
	char *caller_pkgname = NULL;
	int group_id = 0, priv_id = 0, count = 0, unread_count = 0, temp_count;
	char buf[128] = { 0, };
	time_t time;
	notification_type_e type = NOTIFICATION_TYPE_NONE;
	notification_count_display_type_e count_display =
	    NOTIFICATION_COUNT_DISPLAY_TYPE_NONE;

	retif(noti == NULL, NULL, "Invalid parameter!");

	// Set text domain
	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL) {
		bindtextdomain(domain, dir);
	}
	// Get pkgname & id
	notification_get_pkgname(noti, &pkgname);
	notification_get_application(noti, &caller_pkgname);
	notification_get_id(noti, &group_id, &priv_id);
	notification_get_type(noti, &type);

	if (!strcmp(part, "elm.text.title")) {
		notification_get_count(type, pkgname,
				       group_id, priv_id, &count);

		if (count > 1) {
			/* Multi event */
			noti_err =
			    notification_get_text(noti,
						  NOTIFICATION_TEXT_TYPE_GROUP_TITLE,
						  &text);
			if (noti_err != NOTIFICATION_ERROR_NONE) {
				text = NULL;
			}
		} else {
			/* Single event */
			noti_err =
			    notification_get_text(noti,
						  NOTIFICATION_TEXT_TYPE_TITLE,
						  &text);
			if (noti_err != NOTIFICATION_ERROR_NONE) {
				text = NULL;
			}
		}
	} else if (!strcmp(part, "elm.text.content")) {
		notification_get_count(type, pkgname,
				       group_id, priv_id, &count);

		if (count > 1) {
			/* Multi event */
			noti_err =
			    notification_get_text(noti,
						  NOTIFICATION_TEXT_TYPE_GROUP_CONTENT,
						  &text);
			if (noti_err != NOTIFICATION_ERROR_NONE) {
				text = NULL;
			}

			if (text == NULL) {
				/* Default string */
				if (!strcmp
				    (caller_pkgname, "org.tizen.phone")) {
					snprintf(buf, sizeof(buf), "%d %s",
						 count,
						 _S("IDS_COM_POP_MISSED_CALLS"));
				} else
				    if (!strcmp
					(caller_pkgname,
					 "org.tizen.message")) {
					snprintf(buf, sizeof(buf), "%d %s",
						 count,
						 _S("IDS_COM_POP_NEW_MESSAGES"));
				}/* else {
					snprintf(buf, sizeof(buf), "%d %s",
						 count,
						 _S("IDS_COM_POP_MISSED_EVENT"));
				}*/
				text = buf;
			}
		} else {
			/* Single event */
			noti_err =
			    notification_get_text(noti,
						  NOTIFICATION_TEXT_TYPE_CONTENT,
						  &text);
			if (noti_err != NOTIFICATION_ERROR_NONE) {
				text = NULL;
			}

			/* Default string */
			if (text == NULL) {
				if (!strcmp
				    (caller_pkgname, "org.tizen.phone")) {
					snprintf(buf, sizeof(buf), "%s",
						 _S("IDS_COM_POP_MISSED_CALL"));
				}
				text = buf;
			}
		}
	} else if (!strcmp(part, "elm.text.time")) {
		if (type == NOTIFICATION_TYPE_ONGOING) {
			text =
			    _quickpanel_noti_get_progress(noti, buf,
							  sizeof(buf));
		} else {
			notification_get_time(noti, &time);

			if ((int)time > 0) {
				text =
				    _quickpanel_noti_get_time(time, buf,
							      sizeof(buf));
			} else {
				notification_get_insert_time(noti, &time);
				text =

				    _quickpanel_noti_get_time(time, buf,
							      sizeof(buf));
			}
		}
	}

	if (text != NULL) {
		return strdup(text);
	}

	return NULL;
}

static Eina_Bool _quickpanel_noti_gl_get_state(void *data, Evas_Object * obj,
					       const char *part)
{
	notification_h noti = (notification_h) data;
	char *pkgname = NULL;
	int group_id = 0, priv_id = 0, count = 0;
	char *content = NULL;
	time_t time;

	retif(noti == NULL, EINA_FALSE, "Invalid parameter!");

	notification_get_pkgname(noti, &pkgname);
	notification_get_id(noti, &group_id, &priv_id);

	if (!strcmp(part, "elm.text.content")) {
		 notification_get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, &content);
		if (content != NULL) {
			return EINA_TRUE;
		}
	} else if (!strcmp(part, "elm.text.time")) {
		notification_get_time(noti, &time);

		if ((int)time > 0) {
			return EINA_TRUE;
		}
	}

	return EINA_FALSE;
}

static void quickpanel_noti_select_cb(void *data, Evas_Object * obj,
				      void *event_info)
{
	notification_h noti = (notification_h) data;
	char *pkgname = NULL;
	char *caller_pkgname = NULL;
	bundle *args = NULL;
	bundle *group_args = NULL;
	bundle *single_service_handle = NULL;
	bundle *multi_service_handle = NULL;
	int flags = 0, group_id = 0, priv_id = 0, count = 0, flag_launch =
	    0, flag_delete = 0;
	notification_type_e type = NOTIFICATION_TYPE_NONE;

	elm_genlist_item_selected_set((Elm_Object_Item *) event_info,
				      EINA_FALSE);

	retif(noti == NULL,, "Invalid parameter!");

	notification_get_pkgname(noti, &caller_pkgname);
	notification_get_application(noti, &pkgname);
	if (pkgname == NULL) {
		pkgname = caller_pkgname;
	}

	notification_get_id(noti, &group_id, &priv_id);
	notification_get_property(noti, &flags);
	notification_get_type(noti, &type);

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
	notification_get_execute_option(noti,
					NOTIFICATION_EXECUTE_TYPE_MULTI_LAUNCH,
					NULL, &multi_service_handle);

	if (flag_launch == 1) {
		if (group_id != NOTIFICATION_GROUP_ID_NONE) {
			notification_get_count(type,
					       caller_pkgname, group_id,
					       priv_id, &count);
		} else {
			count = 1;
		}

		if (count > 1 && multi_service_handle != NULL) {
			appsvc_run_service(multi_service_handle, 0, NULL, NULL);
		} else if (single_service_handle != NULL) {
			appsvc_run_service(single_service_handle, 0, NULL,
					   NULL);
		} else {
			notification_get_args(noti, &args, &group_args);

			/* AUL launch, this will be removed */
			if (count > 1 && group_args != NULL) {
				aul_launch_app(pkgname, group_args);
			} else {
				aul_launch_app(pkgname, args);
			}
		}

		// Hide quickpanel
		Ecore_X_Window zone;
		zone = ecore_x_e_illume_zone_get(elm_win_xwindow_get(g_window));
		ecore_x_e_illume_quickpanel_state_send(zone,
						       ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
	}

	if (flag_delete == 1 && type == NOTIFICATION_TYPE_NOTI) {
		notification_delete_group_by_priv_id(caller_pkgname,
						     NOTIFICATION_TYPE_NOTI,
						     priv_id);
	}
}

static Evas_Object *_quickpanel_noti_gl_get_group_content(void *data,
						       Evas_Object * obj,
						       const char *part)
{
	Evas_Object *eo = NULL;

	eo = elm_button_add(obj);
	retif(eo == NULL, NULL, "Failed to create clear button!");

	elm_object_style_set(eo, "noticlear");

	elm_object_text_set(eo, _S("IDS_COM_POP_CLEAR"));
	evas_object_smart_callback_add(eo, "clicked",
				       _quickpanel_noti_clear_clicked_cb, NULL);

	return eo;
}

static char *_quickpanel_noti_gl_get_group_text(void *data, Evas_Object * obj,
						 const char *part)
{
	char buf[128] = { 0, };
	int noti_count = 0;
	struct appdata *ad = (struct appdata *)data;

	retif(ad == NULL, NULL, "Invalid parameter!");

	if (!strncmp(part, "elm.text", 8)) {
		memset(buf, 0x00, sizeof(buf));

		notification_get_count(NOTIFICATION_TYPE_NONE, NULL,
				       NOTIFICATION_GROUP_ID_NONE,
				       NOTIFICATION_PRIV_ID_NONE,
				       &noti_count);

		snprintf(buf, sizeof(buf), _("IDS_QP_BODY_NOTIFICATIONS_HPD"),
			 noti_count);

		return strdup(buf);
	}

	return NULL;
}

static void _quickpanel_noti_get_new_list(void)
{
	notification_list_h new_noti_list = NULL;
	notification_list_h head = NULL;
	notification_list_h new_head = NULL;
	notification_h noti = NULL;
	notification_h new_noti = NULL;
	notification_type_e new_type = NOTIFICATION_TYPE_NONE;
	char *pkgname = NULL;
	char *new_pkgname = NULL;
	int priv_id = 0;
	int new_priv_id = 0;
	double percentage = 0.0;
	double size = 0.0;

	if (g_notification_ongoing_list != NULL) {
		notification_free_list(g_notification_ongoing_list);
		g_notification_ongoing_list = new_noti_list;
	}

	/* Get all list */
	notification_get_grouping_list(NOTIFICATION_TYPE_NONE, -1,
				       &new_noti_list);
	if (g_notification_list != NULL) {
		head = notification_list_get_head(g_notification_list);
		while (head != NULL) {
			new_head = notification_list_get_head(new_noti_list);
			while (new_head != NULL) {
				noti = notification_list_get_data(head);
				new_noti = notification_list_get_data(new_head);

				notification_get_type(new_noti, &new_type);
				if (new_type == NOTIFICATION_TYPE_ONGOING) {
					notification_get_id(noti, NULL,
							    &priv_id);
					notification_get_id(new_noti, NULL,
							    &new_priv_id);

					notification_get_pkgname(noti,
								 &pkgname);
					notification_get_pkgname(new_noti,
								 &new_pkgname);

					if (!strcmp(pkgname, new_pkgname)
					    && priv_id == new_priv_id) {
						notification_get_progress(noti,
									  &percentage);
						notification_get_size(noti,
								      &size);
						notification_set_progress
						    (new_noti, percentage);
						notification_set_size(new_noti,
								      size);
					}
				}

				new_head = notification_list_get_next(new_head);
			}
			head = notification_list_get_next(head);
		}

		notification_free_list(g_notification_list);
		g_notification_list = new_noti_list;
	} else {
		g_notification_list = new_noti_list;
	}
}

void _quickpanel_noti_update_notilist(struct appdata *ad)
{
	Evas_Object *eo = ad->notilist;
	Elm_Object_Item *grp = NULL;
	notification_h noti = NULL;
	notification_list_h get_list = NULL;
	int r;
	int applist = NOTIFICATION_DISPLAY_APP_ALL;

	DBG("%s(%d)", __FUNCTION__, __LINE__);

	retif(ad == NULL,, "Invalid parameter!");

	eo = ad->notilist;
	retif(eo == NULL,, "Failed to get noti genlist.");

	/* Clear genlist */
	elm_genlist_clear(eo);

	/* Update notification list */
	_quickpanel_noti_get_new_list();

	/* append ongoing data to genlist */
	if (g_notification_ongoing_list) {
		get_list =
		    notification_list_get_head(g_notification_ongoing_list);
		noti = notification_list_get_data(get_list);

		while (get_list != NULL) {
			notification_get_display_applist(noti, &applist);

			if (applist &
			    NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {
				elm_genlist_item_append(eo, itc_noti, noti,
							NULL,
							ELM_GENLIST_ITEM_NONE,
							quickpanel_noti_select_cb,
							noti);
			}

			get_list = notification_list_get_next(get_list);
			noti = notification_list_get_data(get_list);
		}
	}

	/* append noti title to genlist */
	grp = elm_genlist_item_append(eo, g_itc, ad,
				      NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);

	/* append noti data to genlist */
	if (g_notification_list) {
		get_list = notification_list_get_head(g_notification_list);
		noti = notification_list_get_data(get_list);

		while (get_list != NULL) {
			notification_get_display_applist(noti, &applist);

			if (applist &
			    NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {
				elm_genlist_item_append(eo, itc_noti, noti,
							grp,
							ELM_GENLIST_ITEM_NONE,
							quickpanel_noti_select_cb,
							noti);
			}

			get_list = notification_list_get_next(get_list);
			noti = notification_list_get_data(get_list);
		}
	}
}

static void _quickpanel_noti_delete_volatil_data(void)
{
	notification_list_h noti_list = NULL;
	notification_list_h noti_list_head = NULL;
	notification_h noti = NULL;
	int property = 0;

	notification_get_grouping_list(NOTIFICATION_TYPE_NONE, -1, &noti_list);

	noti_list_head = noti_list;

	while (noti_list != NULL) {
		noti = notification_list_get_data(noti_list);
		notification_get_property(noti, &property);

		if (property & NOTIFICATION_PROP_VOLATILE_DISPLAY) {
			notification_set_property(noti,
						  property |
						  NOTIFICATION_PROP_DISABLE_UPDATE_ON_DELETE);
			notification_delete(noti);
		}

		noti_list = notification_list_get_next(noti_list);
	}

	notification_free_list(noti_list_head);

	notification_update(NULL);
}

static void _quickpanel_noti_changed_cb(void *data, notification_type_e type)
{
	_quickpanel_noti_update_notilist(data);
}

static void _quickpanel_noti_update_sim_status_cb(keynode_t * node, void *data)
{
	struct appdata *ad = data;

	if (ad != NULL && ad->notilist != NULL) {
		_quickpanel_noti_update_notilist(ad);
	}
}

static int _quickpanel_noti_register_event_handler(struct appdata *ad)
{
	int ret = 0;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	g_window = ad->noti.win;

	/* Add dbus signal */
	e_dbus_init();
	ad->dbus_connection = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (ad->dbus_connection == NULL) {
		ERR("noti register : failed to get dbus bus");
		return -1;
	}

	ad->dbus_handler_size = e_dbus_signal_handler_add(ad->dbus_connection,
							  NULL,
							  "/dbus/signal",
							  "notification.ongoing",
							  "update_progress",
							  _quickpanel_noti_item_progress_update_cb,
							  ad->notilist);
	if (ad->dbus_handler_size == NULL) {
		ERR("noti register : failed to add size signal");
	}

	ad->dbus_handler_progress =
	    e_dbus_signal_handler_add(ad->dbus_connection, NULL, "/dbus/signal",
				      "notification.ongoing", "update_size",
				      _quickpanel_noti_item_size_update_cb,
				      ad->notilist);
	if (ad->dbus_handler_progress == NULL) {
		ERR("noti register : failed to add progress signal");
	}

	ad->dbus_handler_content =
	    e_dbus_signal_handler_add(ad->dbus_connection, NULL, "/dbus/signal",
				      "notification.ongoing", "update_content",
				      _quickpanel_noti_item_content_update_cb,
				      ad->notilist);
	if (ad->dbus_handler_content == NULL) {
		ERR("noti register : failed to add content signal");
	}

	/* Notify vconf key */
	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,
				       _quickpanel_noti_update_sim_status_cb,
				       (void *)ad);
	if (ret != 0) {
		ERR("Failed to register SIM_SLOT change callback!");
	}

	/* Register notification changed cb */
	notification_resister_changed_cb(_quickpanel_noti_changed_cb, ad);

	/* Update notification list */
	_quickpanel_noti_update_notilist(ad);

	return ret;
}

static int _quickpanel_noti_unregister_event_handler(struct appdata *ad)
{
	int ret = 0;

	/* Unregister notification changed cb */
	notification_unresister_changed_cb(_quickpanel_noti_changed_cb);

	/* Ignore vconf key */
	ret =
	    vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,
				     _quickpanel_noti_update_sim_status_cb);
	if (ret != 0) {
		ERR("Failed to ignore SIM_SLOT change callback!");
	}

	/* Delete dbus signal */
	if (ad->dbus_handler_size != NULL) {
		e_dbus_signal_handler_del(ad->dbus_connection,
					  ad->dbus_handler_size);
		ad->dbus_handler_size = NULL;
	}
	if (ad->dbus_handler_progress != NULL) {
		e_dbus_signal_handler_del(ad->dbus_connection,
					  ad->dbus_handler_progress);
		ad->dbus_handler_progress = NULL;
	}
	if (ad->dbus_handler_content != NULL) {
		e_dbus_signal_handler_del(ad->dbus_connection,
					  ad->dbus_handler_content);
		ad->dbus_handler_content = NULL;
	}

	if (ad->dbus_connection != NULL) {
		e_dbus_connection_close(ad->dbus_connection);
		ad->dbus_connection = NULL;
	}

	return QP_OK;
}

static void _quickpanel_noti_gl_style_init(void)
{
	Elm_Genlist_Item_Class *noti = NULL;
	Elm_Genlist_Item_Class *group = NULL;

	/* item style for noti items*/
	noti = elm_genlist_item_class_new();
	if (noti) {
		noti->item_style = "notification_item";
		noti->func.text_get = _quickpanel_noti_gl_get_text;
		noti->func.content_get = _quickpanel_noti_gl_get_content;
		noti->func.state_get = _quickpanel_noti_gl_get_state;
		itc_noti = noti;
	}

	/* item style for noti group title */
	group = elm_genlist_item_class_new();
	if (group) {
		group->item_style = "qp_group_title";
		group->func.text_get = _quickpanel_noti_gl_get_group_text;
		group->func.content_get = _quickpanel_noti_gl_get_group_content;
		g_itc = group;
	}
}

static void _quickpanel_noti_gl_style_fini(void)
{
	if (itc_noti) {
		elm_genlist_item_class_free(itc_noti);
		itc_noti = NULL;
	}

	if (g_itc) {
		elm_genlist_item_class_free(g_itc);
		g_itc = NULL;
	}
}

static Evas_Object *_quickpanel_noti_create_notilist(Evas_Object * parent)
{
	Evas_Object *gl = NULL;

	gl = elm_genlist_add(parent);
	retif(gl == NULL, NULL, "Failed to add elm genlist.");

	return gl;
}

static int _quickpanel_noti_add_layout(struct appdata *ad)
{
	Evas_Object *eo = NULL;
	char buf[1024] = { 0, };

	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	eo = _quickpanel_noti_create_notilist(ad->noti.ly);
	retif(eo == NULL, QP_FAIL, "Failed to create notification list!");

	_quickpanel_noti_gl_style_init();

	edje_object_part_swallow(_EDJ(ad->noti.ly), "qp.noti.swallow.notilist",
				 eo);
	ad->notilist = eo;

	elm_theme_extension_add(NULL, DEFAULT_CUSTOM_EDJ);

	return QP_OK;
}

static void _quickpanel_noti_del_layout(struct appdata *ad)
{
	retif(ad == NULL,, "Invalid parameter!");

	if (ad->notilist) {
		evas_object_hide(ad->notilist);
		evas_object_del(ad->notilist);
		ad->notilist = NULL;
	}

	_quickpanel_noti_gl_style_fini();
}

static int quickpanel_noti_init(void *data)
{
	struct appdata *ad = data;
	int ret = 0;

	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	/* Add noti layout */
	ret = _quickpanel_noti_add_layout(ad);
	retif(ret != QP_OK, QP_FAIL, "Failed to add noti layout!");

	/* Remove ongoing and volatile noti data */
	notifiation_clear(NOTIFICATION_TYPE_ONGOING);

	_quickpanel_noti_delete_volatil_data();

	/* Register event handler */
	_quickpanel_noti_register_event_handler(ad);

	return QP_OK;
}

static int quickpanel_noti_fini(void *data)
{
	/* Remove notification list */
	if (g_notification_ongoing_list != NULL) {
		notification_free_list(g_notification_ongoing_list);

		g_notification_ongoing_list = NULL;
	}

	if (g_notification_list != NULL) {
		notification_free_list(g_notification_list);

		g_notification_list = NULL;
	}

	/* Unregister event handler */
	_quickpanel_noti_unregister_event_handler(data);

	/* Delete noti layout */
	_quickpanel_noti_del_layout((struct appdata *)data);

	return QP_OK;
}

static int quickpanel_noti_suspend(void *data)
{
	suspended = 1;

	return QP_OK;
}

static int quickpanel_noti_resume(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	suspended = 0;
	_quickpanel_noti_update_progressbar(ad->notilist, NULL);

	return QP_OK;
}

void quickpanel_noti_lang_changed(void *data)
{
	struct appdata *ad = data;

	retif(ad == NULL,, "Invalid parameter!");

	_quickpanel_noti_update_notilist(ad);
}
