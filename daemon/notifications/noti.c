/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aul.h>
#include <appsvc.h>

#include <time.h>
#include <vconf.h>
#include <appcore-common.h>
#include <app_service.h>
#include <runtime_info.h>
#include <Ecore_X.h>

#include <unicode/uloc.h>
#include <unicode/udat.h>
#include <unicode/udatpg.h>
#include <unicode/ustring.h>

#include <notification.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "list_util.h"
#include "quickpanel_theme_def.h"

#ifndef VCONFKEY_QUICKPANEL_STARTED
#define VCONFKEY_QUICKPANEL_STARTED "memory/private/"PACKAGE_NAME"/started"
#endif /* VCONFKEY_QUICKPANEL_STARTED */

#define QP_DEFAULT_ICON	ICONDIR"/quickpanel_icon_default.png"
#define QP_NOTI_DAY_DEC	(24 * 60 * 60)

#define QP_NOTI_ONGOING_DBUS_PATH	"/dbus/signal"
#define QP_NOTI_ONGOING_DBUS_INTERFACE	"notification.ongoing"

static int suspended;

static notification_list_h g_notification_list;
static notification_list_h g_notification_ongoing_list;

static Elm_Genlist_Item_Class *itc_noti;
static Elm_Genlist_Item_Class *itc_ongoing;
static Elm_Genlist_Item_Class *g_itc;

static Evas_Object *g_window;

static Elm_Object_Item *noti_group;
static Elm_Object_Item *ongoing_first;
static Elm_Object_Item *noti_first;

static int quickpanel_noti_init(void *data);
static int quickpanel_noti_fini(void *data);
static int quickpanel_noti_suspend(void *data);
static int quickpanel_noti_resume(void *data);
static void quickpanel_noti_lang_changed(void *data);
static int quickpanel_noti_get_height(void *data);

QP_Module noti = {
	.name = "noti",
	.init = quickpanel_noti_init,
	.fini = quickpanel_noti_fini,
	.suspend = quickpanel_noti_suspend,
	.resume = quickpanel_noti_resume,
	.lang_changed = quickpanel_noti_lang_changed,
	.hib_enter = NULL,
	.hib_leave = NULL,
	.refresh = NULL,
	.get_height = quickpanel_noti_get_height,
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

	if (noti_err == NOTIFICATION_ERROR_NONE && count > 0)
		notifiation_clear(NOTIFICATION_TYPE_NOTI);

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
		if (snprintf(buf, buf_len, "%d%%", (int)(percentage * 100))
			<= 0)
			return NULL;

		return buf;
	} else if (size > 0) {
		if (size > (1 << 30)) {
			if (snprintf(buf, buf_len, "%.1lfGB",
				size / 1000000000.0) <= 0)
				return NULL;

			return buf;
		} else if (size > (1 << 20)) {
			if (snprintf(buf, buf_len, "%.1lfMB",
				size / 1000000.0) <= 0)
				return NULL;

			return buf;
		} else if (size > (1 << 10)) {
			if (snprintf(buf, buf_len, "%.1lfKB",
				size / 1000.0) <= 0)
				return NULL;

			return buf;
		} else {
			if (snprintf(buf, buf_len, "%lfB", size) <= 0)
				return NULL;

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
				notification_set_text(noti,
					NOTIFICATION_TEXT_TYPE_CONTENT,
					content, NULL,
					NOTIFICATION_VARIABLE_TYPE_NONE);
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
				notification_set_text(noti,
					NOTIFICATION_TEXT_TYPE_CONTENT,
					content, NULL,
					NOTIFICATION_VARIABLE_TYPE_NONE);
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
	struct appdata *ad = NULL;
	Elm_Object_Item *found = NULL;

	retif(!data, , "data is NULL");
	ad = data;

	retif(!ad->list, , "ad->list is NULL");

	if (ad->show_setting)
		found = quickpanel_list_util_find_item_by_type(ad->list,
				update_noti, ongoing_first,
				QP_ITEM_TYPE_ONGOING_NOTI);
	else
		found = quickpanel_list_util_find_item_by_type(ad->list,
				update_noti, noti_first,
				QP_ITEM_TYPE_NOTI);

	retif(!found, , "fail to find %p related gl item", update_noti);

	elm_genlist_item_fields_update(found, "*", ELM_GENLIST_ITEM_FIELD_ALL);
}

static void _quickpanel_noti_item_progress_update_cb(void *data,
						DBusMessage *msg)
{
	DBusError err;
	char *pkgname = 0;
	int priv_id = 0;
	double progress = 0;
	notification_h noti = NULL;

	retif(data == NULL || msg == NULL, , "Invalid parameter!");

	dbus_error_init(&err);
	dbus_message_get_args(msg, &err,
			DBUS_TYPE_STRING, &pkgname,
			DBUS_TYPE_INT32, &priv_id,
			DBUS_TYPE_DOUBLE, &progress,
			DBUS_TYPE_INVALID);

	if (dbus_error_is_set(&err)) {
		ERR("dbus err: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	/* check item on the list */
	noti = _quickpanel_noti_update_item_progress(pkgname,
						priv_id, progress);
	retif(noti == NULL, , "Can not found noti data.");

	DBG("pkgname[%s], priv_id[%d], progress[%lf]",
				pkgname, priv_id, progress);
	if (!suspended)
		_quickpanel_noti_update_progressbar(data, noti);
}

static void _quickpanel_noti_item_size_update_cb(void *data, DBusMessage * msg)
{
	DBusError err;
	char *pkgname = 0;
	int priv_id = 0;
	double size = 0;
	notification_h noti = NULL;

	retif(data == NULL || msg == NULL, , "Invalid parameter!");

	dbus_error_init(&err);
	dbus_message_get_args(msg, &err,
			DBUS_TYPE_STRING, &pkgname,
			DBUS_TYPE_INT32, &priv_id,
			DBUS_TYPE_DOUBLE, &size, DBUS_TYPE_INVALID);
	if (dbus_error_is_set(&err)) {
		ERR("dbus err: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	/* check item on the list */
	noti = _quickpanel_noti_update_item_size(pkgname, priv_id, size);
	retif(noti == NULL, , "Can not found noti data.");

	DBG("pkgname[%s], priv_id[%d], progress[%lf]",
				pkgname, priv_id, size);

	if (!suspended)
		_quickpanel_noti_update_progressbar(data, noti);
}

static void _quickpanel_noti_item_content_update_cb(void *data,
						DBusMessage *msg)
{
	DBusError err;
	char *pkgname = NULL;
	int priv_id = 0;
	char *content = NULL;
	notification_h noti = NULL;

	retif(data == NULL || msg == NULL, , "Invalid parameter!");

	dbus_error_init(&err);
	dbus_message_get_args(msg, &err,
			DBUS_TYPE_STRING, &pkgname,
			DBUS_TYPE_INT32, &priv_id,
			DBUS_TYPE_STRING, &content, DBUS_TYPE_INVALID);

	if (dbus_error_is_set(&err)) {
		ERR("dbus err: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	DBG("pkgname[%s], priv_id[%d], content[%s]",
				pkgname, priv_id, content);

	/* check item on the list */
	noti = _quickpanel_noti_update_item_content(pkgname, priv_id, content);
	retif(noti == NULL, , "Can not found noti data.");

	if (!suspended)
		_quickpanel_noti_update_progressbar(data, noti);
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
	bool is_24hour_enabled = FALSE;

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
		date = (UDate) t * 1000;

		/* get default locale  */
		/* for thread saftey  */
		uloc_setDefault(__secure_getenv("LC_TIME"), &status);
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
		ret = runtime_info_get_value_bool(
					RUNTIME_INFO_KEY_24HOUR_CLOCK_FORMAT_ENABLED, &is_24hour_enabled);
		if (ret == RUNTIME_INFO_ERROR_NONE && is_24hour_enabled == TRUE) {
			ret = strftime(buf, buf_len, "%H:%M", &loc_time);
		} else {
			strftime(bf1, sizeof(bf1), "%l:%M", &loc_time);

			if (loc_time.tm_hour >= 0 && loc_time.tm_hour < 12)
				ret = snprintf(buf, buf_len, "%s%s", bf1, "AM");
			else
				ret = snprintf(buf, buf_len, "%s%s", bf1, "PM");
		}

	}

	return ret <= 0 ? NULL : buf;
}

static Evas_Object *_quickpanel_noti_gl_get_content(void *data,
					Evas_Object *obj, const char *part)
{
	qp_item_data *qid = NULL;
	notification_h noti = NULL;
	Evas_Object *ic = NULL;
	char *icon_path = NULL;
	char *thumbnail_path = NULL;
	char *ret_path = NULL;
	double size = 0.0;
	double percentage = 0.0;
	notification_type_e type = NOTIFICATION_TYPE_NONE;

	retif(!data, NULL, "data is NULL");
	qid = data;

	noti = quickpanel_list_util_item_get_data(qid);
	retif(noti == NULL, NULL, "noti is NULL");

	if (!strncmp
	    (part, "elm.swallow.progress", strlen("elm.swallow.progress"))) {
		notification_get_type(noti, &type);
		if (type == NOTIFICATION_TYPE_ONGOING) {
			notification_get_size(noti, &size);
			notification_get_progress(noti, &percentage);

			if (percentage > 0 && percentage <= 1) {
				ic = elm_progressbar_add(obj);
				if (ic == NULL)
					return NULL;

				elm_object_style_set(ic, "list_progress");
				elm_progressbar_value_set(ic, percentage);
				elm_progressbar_horizontal_set(ic, EINA_TRUE);
				elm_progressbar_pulse(ic, EINA_FALSE);
			} else if (size > 0) {
				ic = elm_progressbar_add(obj);
				if (ic == NULL)
					return NULL;

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
		if (thumbnail_path == NULL)
			ret_path = icon_path;
		else
			ret_path = thumbnail_path;

		elm_icon_resizable_set(ic, EINA_FALSE, EINA_TRUE);

		if (ret_path == NULL
		    || (elm_icon_file_set(ic, ret_path, NULL) == EINA_FALSE))
			elm_icon_file_set(ic, QP_DEFAULT_ICON, NULL);
	} else if (!strncmp(part, "elm.swallow.icon",
			strlen("elm.swallow.icon"))) {
		if (thumbnail_path == NULL)
			ret_path = NULL;
		else
			ret_path = icon_path;

		if (ret_path != NULL)
			elm_icon_file_set(ic, ret_path, NULL);
	}

	return ic;
}

static char *_quickpanel_noti_gl_get_text(void *data, Evas_Object * obj,
					   const char *part)
{
	qp_item_data *qid = NULL;
	notification_h noti = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	char *text = NULL;
	char *domain = NULL;
	char *dir = NULL;
	char *pkgname = NULL;
	char *caller_pkgname = NULL;
	int group_id = 0, priv_id = 0, count = 0;
	char buf[128] = { 0, };
	time_t time;
	notification_type_e type = NOTIFICATION_TYPE_NONE;

	retif(!data, NULL, "data is NULL");
	qid = data;

	noti = quickpanel_list_util_item_get_data(qid);
	retif(noti == NULL, NULL, "noti is NULL");

	/* Set text domain */
	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL)
		bindtextdomain(domain, dir);

	/* Get pkgname & id */
	notification_get_pkgname(noti, &pkgname);
	notification_get_application(noti, &caller_pkgname);
	notification_get_id(noti, &group_id, &priv_id);
	notification_get_type(noti, &type);

	if (!strcmp(part, "elm.text.title")) {
		notification_get_count(type, pkgname,
				       group_id, priv_id, &count);
		if (count > 1) {
			/* Multi event */
			noti_err = notification_get_text(noti,
					NOTIFICATION_TEXT_TYPE_GROUP_CONTENT,
					&text);

			if (noti_err == NOTIFICATION_ERROR_NONE && text == NULL) {
				text = NULL;
			} else {
				noti_err = notification_get_text(noti,
						NOTIFICATION_TEXT_TYPE_GROUP_TITLE,
						&text);
				if (noti_err != NOTIFICATION_ERROR_NONE)
					text = NULL;
			}
		} else {
			/* Single event */
			noti_err = notification_get_text(noti,
					NOTIFICATION_TEXT_TYPE_CONTENT,
					&text);

			if (noti_err == NOTIFICATION_ERROR_NONE && text == NULL) {
				text = NULL;
			} else {
				noti_err = notification_get_text(noti,
						NOTIFICATION_TEXT_TYPE_TITLE,
						&text);
				if (noti_err != NOTIFICATION_ERROR_NONE)
					text = NULL;
			}
		}
	} else if (!strcmp(part, "elm.text.title.only")) {
		notification_get_count(type, pkgname,
				       group_id, priv_id, &count);
		if (count > 1) {
			/* Multi event */
			noti_err = notification_get_text(noti,
					NOTIFICATION_TEXT_TYPE_GROUP_CONTENT,
					&text);

			if (noti_err != NOTIFICATION_ERROR_NONE || text != NULL) {
				text = NULL;
			} else {
				noti_err = notification_get_text(noti,
						NOTIFICATION_TEXT_TYPE_GROUP_TITLE,
						&text);
				if (noti_err != NOTIFICATION_ERROR_NONE)
					text = NULL;
			}
		} else {
			/* Single event */
			noti_err = notification_get_text(noti,
					NOTIFICATION_TEXT_TYPE_CONTENT,
					&text);
			DBG("result:%d, %x", noti_err, text);
			if (noti_err != NOTIFICATION_ERROR_NONE || text != NULL) {
				text = NULL;
			} else {
				noti_err = notification_get_text(noti,
						NOTIFICATION_TEXT_TYPE_TITLE,
						&text);
				if (noti_err != NOTIFICATION_ERROR_NONE)
					text = NULL;
			}
		}
	} else if (!strcmp(part, "elm.text.content")) {
		notification_get_count(type, pkgname,
				       group_id, priv_id, &count);

		if (count > 1) {
			/* Multi event */
			noti_err = notification_get_text(noti,
					NOTIFICATION_TEXT_TYPE_GROUP_CONTENT,
					&text);
			if (noti_err != NOTIFICATION_ERROR_NONE)
				text = NULL;

			if (text == NULL) {
				/* Default string */
				if (!strcmp(caller_pkgname,
					 VENDOR".message")) {
					snprintf(buf, sizeof(buf), "%d %s",
						count,
						_S("IDS_COM_POP_NEW_MESSAGES"));
					text = buf;
				}
			}
		} else {
			/* Single event */
			noti_err = notification_get_text(noti,
						NOTIFICATION_TEXT_TYPE_CONTENT,
						&text);
			if (noti_err != NOTIFICATION_ERROR_NONE)
				text = NULL;
		}
	} else if (!strcmp(part, "elm.text.time")) {
		if (type == NOTIFICATION_TYPE_ONGOING)
			text = _quickpanel_noti_get_progress(noti, buf,
							  sizeof(buf));
		else {
			notification_get_count(type, pkgname,
					       group_id, priv_id, &count);

			if (count <= 1) {
				notification_get_time(noti, &time);

				if ((int)time > 0)
					text = _quickpanel_noti_get_time(time, buf,
									  sizeof(buf));
				else {
					notification_get_insert_time(noti, &time);
					text = _quickpanel_noti_get_time(time, buf,
									  sizeof(buf));
				}
			}
		}
	}

	if (text != NULL)
		return strdup(text);

	return NULL;
}

static char *_quickpanel_ongoing_noti_gl_get_text(void *data, Evas_Object * obj,
					   const char *part)
{
	qp_item_data *qid = NULL;
	notification_h noti = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	char *text = NULL;
	char *domain = NULL;
	char *dir = NULL;
	char *pkgname = NULL;
	char *caller_pkgname = NULL;
	int group_id = 0, priv_id = 0, count = 0;
	char buf[128] = { 0, };
	time_t time;
	notification_type_e type = NOTIFICATION_TYPE_NONE;
	double size = 0.0;
	double percentage = 0.0;
	int isProgressBarEnabled = 1;

	retif(!data, NULL, "data is NULL");
	qid = data;

	noti = quickpanel_list_util_item_get_data(qid);
	retif(noti == NULL, NULL, "noti is NULL");

	/* Set text domain */
	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL)
		bindtextdomain(domain, dir);

	/* Get pkgname & id */
	notification_get_pkgname(noti, &pkgname);
	notification_get_application(noti, &caller_pkgname);
	notification_get_id(noti, &group_id, &priv_id);
	notification_get_type(noti, &type);
	notification_get_size(noti, &size);
	notification_get_progress(noti, &percentage);

	DBG("percentage:%f size:%f", percentage, size);

	if (percentage <= 0.0 && size <= 0.0) {
		isProgressBarEnabled = 0;
	}

	if (!strcmp(part, "elm.text.title")) {
		notification_get_count(type, pkgname,
				       group_id, priv_id, &count);
		if (count > 1) {
			/* Multi event */
			noti_err = notification_get_text(noti,
					NOTIFICATION_TEXT_TYPE_GROUP_TITLE,
					&text);
			if (noti_err != NOTIFICATION_ERROR_NONE)
				text = NULL;
		} else {
			/* Single event */
			noti_err = notification_get_text(noti,
									NOTIFICATION_TEXT_TYPE_TITLE,
									&text);
			if (noti_err != NOTIFICATION_ERROR_NONE)
				text = NULL;
		}
	} else if (!strcmp(part, "elm.text.content")) {

		if (isProgressBarEnabled == 0)
			return NULL;

		notification_get_count(type, pkgname,
				       group_id, priv_id, &count);

		if (count > 1) {
			/* Multi event */
			noti_err = notification_get_text(noti,
					NOTIFICATION_TEXT_TYPE_GROUP_CONTENT,
					&text);
			if (noti_err != NOTIFICATION_ERROR_NONE)
				text = NULL;

			if (text == NULL) {
				/* Default string */
				if (!strcmp(caller_pkgname,
					 VENDOR".message")) {
					snprintf(buf, sizeof(buf), "%d %s",
						count,
						_S("IDS_COM_POP_NEW_MESSAGES"));
					text = buf;
				}
			}
		} else {
			/* Single event */
			noti_err = notification_get_text(noti,
						NOTIFICATION_TEXT_TYPE_CONTENT,
						&text);
			if (noti_err != NOTIFICATION_ERROR_NONE)
				text = NULL;
		}
	} else if (!strcmp(part, "elm.text.content.no.progress")) {

		if (isProgressBarEnabled == 1)
			return NULL;

		notification_get_count(type, pkgname,
				       group_id, priv_id, &count);

		if (count > 1) {
			/* Multi event */
			noti_err = notification_get_text(noti,
					NOTIFICATION_TEXT_TYPE_GROUP_CONTENT,
					&text);
			if (noti_err != NOTIFICATION_ERROR_NONE)
				text = NULL;

			if (text == NULL) {
				/* Default string */
				if (!strcmp(caller_pkgname,
					 VENDOR".message")) {
					snprintf(buf, sizeof(buf), "%d %s",
						count,
						_S("IDS_COM_POP_NEW_MESSAGES"));
					text = buf;
				}
			}
		} else {
			/* Single event */
			noti_err = notification_get_text(noti,
						NOTIFICATION_TEXT_TYPE_CONTENT,
						&text);
			if (noti_err != NOTIFICATION_ERROR_NONE)
				text = NULL;
		}
	} else if (!strcmp(part, "elm.text.time")) {

		if (isProgressBarEnabled == 0)
			return NULL;

		text = _quickpanel_noti_get_progress(noti, buf,
									  sizeof(buf));
	}

	if (text != NULL)
		return strdup(text);

	return NULL;
}

static Eina_Bool _quickpanel_noti_gl_get_state(void *data, Evas_Object * obj,
					       const char *part)
{
	qp_item_data *qid = NULL;
	notification_h noti = NULL;
	char *pkgname = NULL;
	int group_id = 0, priv_id = 0;
	char *content = NULL;
	time_t time;

	retif(!data, EINA_FALSE, "data is NULL");
	qid = data;

	noti = quickpanel_list_util_item_get_data(qid);
	retif(noti == NULL, EINA_FALSE, "noti is NULL");

	notification_get_pkgname(noti, &pkgname);
	notification_get_id(noti, &group_id, &priv_id);

	if (!strcmp(part, "elm.text.content")) {
		notification_get_text(noti,
			NOTIFICATION_TEXT_TYPE_CONTENT, &content);
		if (content != NULL)
			return EINA_TRUE;
	} else if (!strcmp(part, "elm.text.time")) {
		notification_get_time(noti, &time);

		if ((int)time > 0)
			return EINA_TRUE;
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

	DBG("quickpanel_noti_select_cb called");

	elm_genlist_item_selected_set((Elm_Object_Item *) event_info,
				      EINA_FALSE);

	retif(noti == NULL, , "Invalid parameter!");

	notification_get_pkgname(noti, &caller_pkgname);
	notification_get_application(noti, &pkgname);
	if (pkgname == NULL)
		pkgname = caller_pkgname;

	notification_get_id(noti, &group_id, &priv_id);
	notification_get_property(noti, &flags);
	notification_get_type(noti, &type);

	if (flags & NOTIFICATION_PROP_DISABLE_APP_LAUNCH)
		flag_launch = 0;
	else
		flag_launch = 1;

	if (flags & NOTIFICATION_PROP_DISABLE_AUTO_DELETE)
		flag_delete = 0;
	else
		flag_delete = 1;

	notification_get_execute_option(noti,
				NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH,
				NULL, &single_service_handle);
	notification_get_execute_option(noti,
				NOTIFICATION_EXECUTE_TYPE_MULTI_LAUNCH,
				NULL, &multi_service_handle);

	if (flag_launch == 1) {
		if (group_id != NOTIFICATION_GROUP_ID_NONE)
			notification_get_count(type,
					caller_pkgname, group_id,
					priv_id, &count);
		else
			count = 1;

		if (count > 1 && multi_service_handle != NULL)
			appsvc_run_service(multi_service_handle, 0, NULL, NULL);
		else if (single_service_handle != NULL)
			appsvc_run_service(single_service_handle, 0, NULL,
					NULL);
		else {
			notification_get_args(noti, &args, &group_args);

			/* AUL launch, this will be removed */
			if (count > 1 && group_args != NULL)
				aul_launch_app(pkgname, group_args);
			else
				aul_launch_app(pkgname, args);
		}

		/* Hide quickpanel */
		Ecore_X_Window zone;
		zone = ecore_x_e_illume_zone_get(elm_win_xwindow_get(g_window));
		ecore_x_e_illume_quickpanel_state_send(zone,
				ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
	}

	if (flag_delete == 1 && type == NOTIFICATION_TYPE_NOTI)
		notification_delete_group_by_priv_id(caller_pkgname,
					NOTIFICATION_TYPE_NOTI,
					priv_id);
}

static Evas_Object *_quickpanel_noti_gl_get_group_content(void *data,
						Evas_Object *obj,
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
	struct appdata *ad = NULL;
	qp_item_data *qid = NULL;

	retif(!data, NULL, "data is NULL");
	qid = data;

	ad = quickpanel_list_util_item_get_data(qid);
	retif(ad == NULL, NULL, "appdata is NULL");

	if (!strncmp(part, "elm.text", 8)) {
		const char *format = NULL;
		memset(buf, 0x00, sizeof(buf));

		if (ad->show_setting) {
			notification_get_count(NOTIFICATION_TYPE_NOTI, NULL,
					       NOTIFICATION_GROUP_ID_NONE,
					       NOTIFICATION_PRIV_ID_NONE,
					       &noti_count);
		} else {
			notification_get_count(NOTIFICATION_TYPE_NONE, NULL,
					       NOTIFICATION_GROUP_ID_NONE,
					       NOTIFICATION_PRIV_ID_NONE,
					       &noti_count);
		}

		format = _("IDS_QP_BODY_NOTIFICATIONS_HPD");
		snprintf(buf, sizeof(buf), format, noti_count);

		return strdup(buf);
	}

	return NULL;
}

static void _quickpanel_noti_gl_del(void *data, Evas_Object *obj)
{
	if (data) {
		quickpanel_list_util_del_count(data);
		free(data);
	}

	return;
}

static void _quickpanel_noti_gl_style_init(void)
{
	Elm_Genlist_Item_Class *noti = NULL;
	Elm_Genlist_Item_Class *noti_ongoing = NULL;
	Elm_Genlist_Item_Class *group = NULL;

	/* item style for noti items*/
	noti = elm_genlist_item_class_new();
	if (noti) {
		noti->item_style = "notification_item";
		noti->func.text_get = _quickpanel_noti_gl_get_text;
		noti->func.content_get = _quickpanel_noti_gl_get_content;
		noti->func.state_get = _quickpanel_noti_gl_get_state;
		noti->func.del = _quickpanel_noti_gl_del;
		itc_noti = noti;
	}

	noti_ongoing = elm_genlist_item_class_new();
	if (noti_ongoing) {
		noti_ongoing->item_style = "notification_ongoing_item";
		noti_ongoing->func.text_get = _quickpanel_ongoing_noti_gl_get_text;
		noti_ongoing->func.content_get = _quickpanel_noti_gl_get_content;
		noti_ongoing->func.state_get = _quickpanel_noti_gl_get_state;
		noti_ongoing->func.del = _quickpanel_noti_gl_del;
		itc_ongoing = noti_ongoing;
	}

	/* item style for noti group title */
	group = elm_genlist_item_class_new();
	if (group) {
		group->item_style = "qp_group_title";
		group->func.text_get = _quickpanel_noti_gl_get_group_text;
		group->func.content_get = _quickpanel_noti_gl_get_group_content;
		noti->func.del = _quickpanel_noti_gl_del;
		g_itc = group;
	}
}

static void _quickpanel_noti_gl_style_fini(void)
{
	if (itc_noti) {
		elm_genlist_item_class_free(itc_noti);
		itc_noti = NULL;
	}

	if (itc_ongoing) {
		elm_genlist_item_class_free(itc_ongoing);
		itc_ongoing = NULL;
	}

	if (g_itc) {
		elm_genlist_item_class_free(g_itc);
		g_itc = NULL;
	}
}

static inline void __ongoing_comp_n_copy(notification_h old, notification_h new)
{
	int priv_id = 0;
	int new_priv_id = 0;
	char *pkgname = NULL;
	char *new_pkgname = NULL;

	if (!old)
		return;

	if (!new)
		return;

	notification_get_id(old, NULL, &priv_id);
	notification_get_id(new, NULL, &new_priv_id);

	notification_get_pkgname(old, &pkgname);
	notification_get_pkgname(new, &new_pkgname);

	if (!pkgname || !new_pkgname)
		return;

	if (!strcmp(pkgname, new_pkgname) && priv_id == new_priv_id) {
		double percentage = 0.0;
		double size = 0.0;
		time_t insert_time = 0;
		time_t new_insert_time = 0;

		notification_get_progress(old, &percentage);
		notification_get_size(old, &size);
		notification_set_progress(new, percentage);
		notification_set_size(new, size);
		notification_get_insert_time(old, &insert_time);
		notification_get_insert_time(new, &new_insert_time);

		if (insert_time == new_insert_time) {
			char *content = NULL;
			notification_get_text(old,
				NOTIFICATION_TEXT_TYPE_CONTENT,	&content);
			notification_set_text(new,
				NOTIFICATION_TEXT_TYPE_CONTENT,	content,
				NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
		}
	}
}

static void _quickpanel_noti_get_new_divided_list(void)
{
	notification_list_h new_noti_list = NULL;
	notification_list_h head = NULL;
	notification_list_h new_head = NULL;
	notification_h noti = NULL;
	notification_h new_noti = NULL;

	/* Get ongoing list */
	notification_get_grouping_list(NOTIFICATION_TYPE_ONGOING, -1,
				&new_noti_list);
	if (g_notification_ongoing_list != NULL) {
		head = notification_list_get_head(g_notification_ongoing_list);
		while (head != NULL) {
			new_head = notification_list_get_head(new_noti_list);
			while (new_head != NULL) {
				noti = notification_list_get_data(head);
				new_noti = notification_list_get_data(new_head);

				__ongoing_comp_n_copy(noti, new_noti);

				new_head = notification_list_get_next(new_head);
			}
			head = notification_list_get_next(head);
		}

		notification_free_list(g_notification_ongoing_list);
		g_notification_ongoing_list = new_noti_list;
	} else {
		g_notification_ongoing_list = new_noti_list;
	}

	/* Get noti list */
	notification_get_grouping_list(NOTIFICATION_TYPE_NOTI, -1,
				       &new_noti_list);
	if (g_notification_list != NULL) {
		notification_free_list(g_notification_list);
		g_notification_list = new_noti_list;
	}

	g_notification_list = new_noti_list;
}

static void _quickpanel_noti_get_new_list(void)
{
	notification_list_h new_noti_list = NULL;
	notification_list_h head = NULL;
	notification_list_h new_head = NULL;
	notification_h noti = NULL;
	notification_h new_noti = NULL;
	notification_type_e new_type = NOTIFICATION_TYPE_NONE;

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

				if (new_type == NOTIFICATION_TYPE_ONGOING)
					__ongoing_comp_n_copy(noti, new_noti);

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

static void _quickpanel_noti_clear_ongoinglist(Evas_Object *list)
{
	if (!list)
		return;

	if (!ongoing_first)
		return;

	quickpanel_list_util_item_del_by_type(list, ongoing_first,
			QP_ITEM_TYPE_ONGOING_NOTI);
}

static void _quickpanel_noti_clear_notilist(Evas_Object *list)
{
	if (!list)
		return;

	if (!noti_first)
		return;

	quickpanel_list_util_item_del_by_type(list, noti_first,
			QP_ITEM_TYPE_NOTI);
}

static void _quickpanel_noti_clear_list_all(Evas_Object *list)
{
	_quickpanel_noti_clear_ongoinglist(list);
	ongoing_first = NULL;

	if (noti_group) {
		_quickpanel_noti_clear_notilist(list);
		noti_first = NULL;

		elm_object_item_del(noti_group);
		noti_group = NULL;
		quickpanel_list_util_del_count_by_itemtype(QP_ITEM_TYPE_NOTI_GROUP);
	}
}

static void _quickpanel_noti_ongoing_add(Evas_Object *list, void *data)
{
	qp_item_data *qid = NULL;
	Elm_Object_Item *it = NULL;

	if (!list)
		return;

	qid = quickpanel_list_util_item_new(QP_ITEM_TYPE_ONGOING_NOTI, data);
	if (!qid)
		return;

	it = quickpanel_list_util_sort_insert(list, itc_ongoing, qid, NULL,
			ELM_GENLIST_ITEM_NONE, quickpanel_noti_select_cb, data);

	if (it)
		ongoing_first = it;
	else
		ERR("fail to insert item to list : %p", data);

	DBG("noti ongoing[%p] data[%p] added, it[%p]", qid, data, it);
}

static void _quickpanel_noti_group_add(Evas_Object *list, void *data)
{
	qp_item_data *qid = NULL;
	Elm_Object_Item *it = NULL;

	if (!list)
		return;

	qid = quickpanel_list_util_item_new(QP_ITEM_TYPE_NOTI_GROUP, data);
	if (!qid)
		return;

	it = quickpanel_list_util_sort_insert(list, g_itc, qid, NULL,
		ELM_GENLIST_ITEM_GROUP, NULL, NULL);

	if (it)
		noti_group = it;
	else
		ERR("fail to insert item to list : %p", data);

	DBG("noti group[%p] data[%p] added, it[%p]", qid, data, it);
}

static void _quickpanel_noti_noti_add(Evas_Object *list, void *data)
{
	qp_item_data *qid = NULL;
	Elm_Object_Item *it = NULL;

	if (!list)
		return;

	qid = quickpanel_list_util_item_new(QP_ITEM_TYPE_NOTI, data);
	if (!qid)
		return;

	if (!noti_group)
		_quickpanel_noti_group_add(list, data);

	it = quickpanel_list_util_sort_insert(list, itc_noti, qid, noti_group,
		ELM_GENLIST_ITEM_NONE, quickpanel_noti_select_cb, data);

	if (it)
		noti_first = it;
	else
		ERR("fail to insert item to list : %p", data);

	DBG("noti[%p] data[%p] added, it[%p]", qid, data, it);
}


void _quickpanel_noti_update_notilist(struct appdata *ad)
{
	Evas_Object *list = NULL;
	notification_h noti = NULL;
	notification_list_h get_list = NULL;
	int applist = NOTIFICATION_DISPLAY_APP_ALL;

	DBG("");

	retif(ad == NULL, , "Invalid parameter!");

	list = ad->list;
	retif(list == NULL, , "Failed to get noti genlist.");

	/* Clear genlist */
	_quickpanel_noti_clear_list_all(list);

	/* Update notification list */
	if (ad->show_setting)
		_quickpanel_noti_get_new_divided_list();
	else
		_quickpanel_noti_get_new_list();

	/* append ongoing data to genlist */
	if (g_notification_ongoing_list) {
		get_list =
		    notification_list_get_tail(g_notification_ongoing_list);
		noti = notification_list_get_data(get_list);

		while (get_list != NULL) {
			notification_get_display_applist(noti, &applist);

			if (applist &
			    NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY)
				_quickpanel_noti_ongoing_add(list, noti);

			get_list = notification_list_get_prev(get_list);
			noti = notification_list_get_data(get_list);
		}
	}

	/* append noti data to genlist */
	if (g_notification_list) {
		get_list = notification_list_get_tail(g_notification_list);
		noti = notification_list_get_data(get_list);

		while (get_list != NULL) {
			notification_get_display_applist(noti, &applist);

			if (applist &
			    NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY)
				_quickpanel_noti_noti_add(list, noti);

			get_list = notification_list_get_prev(get_list);
			noti = notification_list_get_data(get_list);
		}
	}

	quickpanel_ui_update_height(ad);
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

static void _quickpanel_noti_update_sim_status_cb(keynode_t *node, void *data)
{
	struct appdata *ad = data;

	if (ad != NULL && ad->list != NULL)
		_quickpanel_noti_update_notilist(ad);
}

static int _quickpanel_noti_register_event_handler(struct appdata *ad)
{
	int ret = 0;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	/* Add dbus signal */
	e_dbus_init();
	ad->dbus_connection = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (ad->dbus_connection == NULL) {
		ERR("noti register : failed to get dbus bus");
		return -1;
	}

	ad->dbus_handler_size =
		e_dbus_signal_handler_add(ad->dbus_connection, NULL,
			QP_NOTI_ONGOING_DBUS_PATH,
			QP_NOTI_ONGOING_DBUS_INTERFACE, "update_progress",
			_quickpanel_noti_item_progress_update_cb,
			ad);
	if (ad->dbus_handler_size == NULL)
		ERR("fail to add size signal");

	ad->dbus_handler_progress =
		e_dbus_signal_handler_add(ad->dbus_connection, NULL,
			QP_NOTI_ONGOING_DBUS_PATH,
			QP_NOTI_ONGOING_DBUS_INTERFACE, "update_size",
			_quickpanel_noti_item_size_update_cb,
			ad);
	if (ad->dbus_handler_progress == NULL)
		ERR("fail to add progress signal");

	ad->dbus_handler_content =
		e_dbus_signal_handler_add(ad->dbus_connection, NULL,
			QP_NOTI_ONGOING_DBUS_PATH,
			QP_NOTI_ONGOING_DBUS_INTERFACE, "update_content",
			_quickpanel_noti_item_content_update_cb,
			ad);
	if (ad->dbus_handler_content == NULL)
		ERR("fail to add content signal");

	/* Notify vconf key */
	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,
				       _quickpanel_noti_update_sim_status_cb,
				       (void *)ad);
	if (ret != 0)
		ERR("Failed to register SIM_SLOT change callback!");

	/* Register notification changed cb */
	notification_resister_changed_cb(_quickpanel_noti_changed_cb, ad);

	return ret;
}

static int _quickpanel_noti_unregister_event_handler(struct appdata *ad)
{
	int ret = 0;

	/* Unregister notification changed cb */
	notification_unresister_changed_cb(_quickpanel_noti_changed_cb);

	/* Ignore vconf key */
	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,
				_quickpanel_noti_update_sim_status_cb);
	if (ret != 0)
		ERR("Failed to ignore SIM_SLOT change callback!");

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

static int _quickpanel_noti_check_first_start(void)
{
	int status = 0;
	int ret = 0;

	ret = vconf_get_bool(VCONFKEY_QUICKPANEL_STARTED, &status);
	if (ret) {
		INFO("fail to get %s", VCONFKEY_QUICKPANEL_STARTED);
		/* reboot */
		ret = vconf_set_bool(VCONFKEY_QUICKPANEL_STARTED, 1);
		INFO("set : %s, result : %d", VCONFKEY_QUICKPANEL_STARTED, ret);
	}

	if (status)
		return 0;

	return 1;
}

static int quickpanel_noti_init(void *data)
{
	struct appdata *ad = data;
	int is_first = 0;

	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	g_window = ad->win;

	is_first = _quickpanel_noti_check_first_start();
	if (is_first) {
		/* Remove ongoing and volatile noti data */
		notifiation_clear(NOTIFICATION_TYPE_ONGOING);
		_quickpanel_noti_delete_volatil_data();
	}

	_quickpanel_noti_gl_style_init();

	/* Register event handler */
	_quickpanel_noti_register_event_handler(ad);

	/* Update notification list */
	_quickpanel_noti_update_notilist(ad);

	return QP_OK;
}

static int quickpanel_noti_fini(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

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

	_quickpanel_noti_clear_list_all(ad->list);

	_quickpanel_noti_gl_style_fini();

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

	if (ad->list)
		quickpanel_list_util_item_update_by_type(ad->list,
				ongoing_first, QP_ITEM_TYPE_ONGOING_NOTI);

	return QP_OK;
}

void quickpanel_noti_lang_changed(void *data)
{
	struct appdata *ad = data;

	retif(ad == NULL, , "Invalid parameter!");

	_quickpanel_noti_update_notilist(ad);
}

static int quickpanel_noti_get_height(void *data)
{
	int height = 0;
	struct appdata *ad = data;

	retif(ad == NULL, 0, "Invalid parameter!");

	qp_item_count *noti_item_count = quickpanel_list_util_get_item_count();

	if (noti_item_count != NULL) {
		height = noti_item_count->group*QP_THEME_LIST_ITEM_GROUP_HEIGHT
				+ noti_item_count->noti*QP_THEME_LIST_ITEM_NOTI_HEIGHT
				+ noti_item_count->ongoing*QP_THEME_LIST_ITEM_ONGOING_HEIGHT;
	}
	return height * ad->scale;
}
