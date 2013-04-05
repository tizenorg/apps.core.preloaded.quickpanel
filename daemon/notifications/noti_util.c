/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unicode/uloc.h>
#include <unicode/udat.h>
#include <unicode/udatpg.h>
#include <unicode/ustring.h>
#include <runtime_info.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "noti_util.h"

#define QP_NOTI_DAY_DEC	(24 * 60 * 60)

HAPI int quickpanel_noti_get_event_count_from_noti(notification_h noti) {
	char *text_count = NULL;

	retif(noti == NULL, 0, "Invalid parameter!");

	notification_get_text(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT, &text_count);
	if (text_count != NULL) {
		return atoi(text_count);
	}
	return 1;
}

HAPI int quickpanel_noti_get_event_count_by_pkgname(const char *pkgname) {
	int count = 0;
	notification_h noti = NULL;
	notification_list_h noti_list = NULL;

	retif(pkgname == NULL, 0, "Invalid parameter!");

	DBG("%s", pkgname);

	notification_get_detail_list(pkgname, NOTIFICATION_GROUP_ID_NONE, NOTIFICATION_PRIV_ID_NONE, -1, &noti_list);
	if (noti_list != NULL) {
		noti = notification_list_get_data(noti_list);
		if (noti != NULL) {
			count = quickpanel_noti_get_event_count_from_noti(noti);
		}
		notification_free_list(noti_list);
		return count;
	}

	return 0;
}

HAPI char *quickpanel_noti_get_time(time_t t, char *buf, int buf_len)
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
