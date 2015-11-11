/* * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Elementary.h>
#include <E_DBus.h>

#include <unicode/uloc.h>
#include <unicode/udat.h>
#include <unicode/udatpg.h>
#include <unicode/ustring.h>

#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <runtime_info.h>
#include <vconf.h>
#include <system_settings.h>
#include <notification_list.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "noti_util.h"

#define QP_NOTI_DAY_DEC	(24 * 60 * 60)
#define QP_NOTI_TIME_LEN_LIMIT 12

HAPI int quickpanel_noti_util_get_event_count_from_noti(notification_h noti)
{
	char *text_count = NULL;

	retif(noti == NULL, 0, "Invalid parameter!");

	notification_get_text(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT, &text_count);
	if (text_count != NULL) {
		return atoi(text_count);
	}
	return 1;
}

HAPI int quickpanel_noti_util_get_event_count_by_pkgname(const char *pkgname)
{
	int count = 0;
	notification_h noti = NULL;
	notification_list_h noti_list = NULL;

	retif(pkgname == NULL, 0, "Invalid parameter!");

	notification_get_detail_list(pkgname, NOTIFICATION_GROUP_ID_NONE, NOTIFICATION_PRIV_ID_NONE, -1, &noti_list);
	if (noti_list != NULL) {
		noti = notification_list_get_data(noti_list);
		if (noti != NULL) {
			count = quickpanel_noti_util_get_event_count_from_noti(noti);
		}
		notification_free_list(noti_list);
		return count;
	}

	return 0;
}

static char* _get_locale(void)
{
	char locale_tmp[32] = { 0, };
	char *locale = NULL;
	int ret = 0;

	ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, &locale);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "ailed to set key(%s) : %d", SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, ret);


	if (locale == NULL) {
		ERR("vconf_get_str() failed : region format");
		return strdup("en_US");
	}

	strncpy(locale_tmp, locale, sizeof(locale_tmp) - 1);

	// remove .UTF-8
	if (strlen(locale_tmp) > 0) {
		char *p = strstr(locale_tmp, ".UTF-8");
		if (p) {
			*p = 0;
		}
	}

	free(locale);

	if (strlen(locale_tmp) > 0) {
		return strdup(locale_tmp);
	}

	return strdup("en_US");
}

static char* _get_timezone_from_vconf(void)
{
	char *szTimezone = NULL;
	szTimezone = vconf_get_str(VCONFKEY_SETAPPL_TIMEZONE_ID);
	if (szTimezone == NULL) {
		ERR("Cannot get time zone.");
		return strdup("N/A");
	}

	return szTimezone;
}

static char* _get_timezone(void)
{
	char buf[1024] = {0,};
	ssize_t len = readlink("/opt/etc/localtime", buf, sizeof(buf)-1);

	if (len != -1) {
		buf[len] = '\0';
	} else {
		ERR("failed to get a timezone information");
		return _get_timezone_from_vconf();
	}

	return strdup(buf + 20);
}

HAPI char *quickpanel_noti_util_get_time(time_t t, char *buf, int buf_len)
{
	int ret = 0;
	UErrorCode status = U_ZERO_ERROR;
	UDate date;
	UDateTimePatternGenerator *generator = NULL;
	UDateFormat *formatter = NULL;
	UChar utimezone_id[40] = {0,};
	UChar skeleton[40] = { 0 };
	UChar pattern[40] = { 0 };
	UChar formatted[40] = { 0 };
	int32_t patternCapacity, formattedCapacity;
	int32_t skeletonLength, patternLength;
	time_t today;
	struct tm loc_time;
	char *timezone = NULL;
	char *locale = NULL;
	char bf1[32] = { 0, };
	bool is_24hour_enabled = FALSE;
	int is_show_time = 0;

	today = time(NULL);
	localtime_r(&today, &loc_time);

	loc_time.tm_sec = 0;
	loc_time.tm_min = 0;
	loc_time.tm_hour = 0;
	today = mktime(&loc_time);

	localtime_r(&t, &loc_time);

	if (buf == NULL) {
		return NULL;
	}

	if (t < today) {
		/* ascii to unicode for input skeleton */
		u_uastrcpy(skeleton, UDAT_ABBR_MONTH_DAY);
		skeletonLength = strlen(UDAT_ABBR_MONTH_DAY);
		is_show_time = 0;
	} else {
		ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, &is_24hour_enabled);

		if (ret == SYSTEM_SETTINGS_ERROR_NONE && is_24hour_enabled == true) {
			/* ascii to unicode for input skeleton */
			u_uastrcpy(skeleton, "HHmm");
			skeletonLength = strlen("HHmm");
		} else {
			/* ascii to unicode for input skeleton */
			u_uastrcpy(skeleton, "hhmm");
			skeletonLength = strlen("hhmm");
		}
		is_show_time = 1;
	}

	/* set UDate  from time_t */
	date = (UDate)t * 1000;

	patternCapacity =
		(int32_t) (sizeof(pattern) / sizeof((pattern)[0]));

	timezone = _get_timezone();
	locale = _get_locale();

	if (u_uastrncpy(utimezone_id, timezone, 40) == NULL) {
		ERR("u_uastrncpy() error.");
		ret = 0;
		goto err;
	}

	ucal_setDefaultTimeZone(utimezone_id , &status);
	if (U_FAILURE(status)) {
		ERR("ucal_setDefaultTimeZone() is failed.");
		ret = 0;
		goto err;
	}

#ifdef HAVE___SECURE_GETENV
	uloc_setDefault(__secure_getenv("LC_TIME"), &status);
#elif defined HAVE_SECURE_GETENV
	uloc_setDefault(secure_getenv("LC_TIME"), &status);
#else
	uloc_setDefault(getenv("LC_TIME"), &status);
#endif
	if (U_FAILURE(status)) {
		ERR("uloc_setDefault() is failed.");
		ret = 0;
		goto err;
	}

	/* open datetime pattern generator */
	generator = udatpg_open(locale, &status);
	if (generator == NULL) {
		ret = 0;
		goto err;
	}

	/* get best pattern using skeleton */
	patternLength =
		udatpg_getBestPattern(generator, skeleton, skeletonLength,
				pattern, patternCapacity, &status);

	/* open datetime formatter using best pattern */
	formatter =
		udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1,
				pattern, patternLength, &status);
	if (formatter == NULL) {
		ret = 0;
		goto err;
	}

	/* calculate formatted string capacity */
	formattedCapacity =
		(int32_t) (sizeof(formatted) / sizeof((formatted)[0]));

	/* formatting date using formatter by best pattern */
	udat_format(formatter, date, formatted, formattedCapacity,
			NULL, &status);

	/* unicode to ascii to display */
	u_austrcpy(bf1, formatted);
	ret = snprintf(buf, buf_len, "%s", bf1);

	if (is_show_time == 1 && strlen(buf) > QP_NOTI_TIME_LEN_LIMIT) {
		if (is_24hour_enabled == TRUE) {
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

err:
	if (timezone) {
		free(timezone);
		timezone = NULL;
	}

	if (locale) {
		free(locale);
		locale = NULL;
	}

	if (generator) {
		udatpg_close(generator);
		generator = NULL;
	}

	if (formatter) {
		udat_close(formatter);
		formatter = NULL;
	}

	return ret <= 0 ? NULL : buf;
}
