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


#include <Elementary.h>
#include <ctype.h>
#include <glib.h>

#include <unicode/utypes.h>
#include <unicode/putil.h>
#include <unicode/uiter.h>
#include <unicode/udat.h>
#include <unicode/udatpg.h>
#include <unicode/ustring.h>

#include <app_control.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <dlog.h>
#include <system_settings.h>
#include <utils_i18n.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <notification.h>
#include <E_DBus.h>

#include "common.h"

#include "quickpanel-ui.h"
#include "util-time.h"
#include "datetime.h"
#include "noti_node.h"
#include "noti.h"

#define TIME_ZONEINFO_PATH      "/usr/share/zoneinfo/"
#define TIME_ZONEINFO_PATH_LEN  (strlen(TIME_ZONEINFO_PATH))
#define BUF_FORMATTER 64

static const char *colon = ":";
static const char *ratio = "&#x2236;";
static int _init(void *data);
static int _fini(void *data);
static void _lang_changed(void *data);
static void _util_time_heartbeat_do(void);



QP_Module qp_datetime_controller = {
	.name = "qp_datetime_controller",
	.init = _init,
	.fini = _fini,
	.suspend = NULL,
	.resume = NULL,
	.lang_changed = _lang_changed,
	.refresh = NULL,
};

static struct info {
	int is_initialized;
	Ecore_Timer *timer;
	int is_timer_enabled;
	UDateFormat *formatter_date;
	UDateFormat *formatter_time;
	UDateFormat *formatter_ampm;
	UDateTimePatternGenerator *generator;
	UDateTimePatternGenerator *date_generator;
	int timeformat;
	char *timeregion_format;
	char *dateregion_format;
	char *timezone_id;
	Eina_Bool is_pre_meridiem;
} s_info = {
	.is_initialized = 0,
	.timer = NULL,
	.is_timer_enabled = 0,
	.formatter_date = NULL,
	.formatter_time = NULL,
	.formatter_ampm = NULL,
	.generator = NULL,
	.date_generator = NULL,
	.timeformat = QP_TIME_FORMAT_24,
	.timeregion_format = NULL,
	.dateregion_format = NULL,
	.timezone_id = NULL,
	.is_pre_meridiem = EINA_FALSE,
};

static Eina_Bool _timer_cb(void *data);

static UChar *uastrcpy(const char *chars)
{
	int len = 0;
	UChar *str = NULL;
	len = strlen(chars);
	str = (UChar *) malloc(sizeof(UChar) *(len + 1));
	if (!str) {
		return NULL;
	}
	u_uastrcpy(str, chars);
	return str;
}

static void ICU_set_timezone(const char *timezone)
{
	DBG("ICU_set_timezone = %s ", timezone);
	UErrorCode ec = U_ZERO_ERROR;
	UChar *str = uastrcpy(timezone);
	retif(str == NULL, , "uastrcpy error!");

	ucal_setDefaultTimeZone(str, &ec);
	if (U_SUCCESS(ec)) {
		DBG("ucal_setDefaultTimeZone() SUCCESS ");
	} else {
		ERR("ucal_setDefaultTimeZone() FAILED : %s ",
				u_errorName(ec));
	}
	free(str);
}

static char *_get_locale(void)
{
	char locale_tmp[32] = { 0, };
	char *locale = NULL; //vconf_get_str(VCONFKEY_REGIONFORMAT);
	int ret = 0;

	ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, &locale);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to ignore key(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY) : %d", ret);

	if (locale == NULL) {
		ERR("vconf_get_str() failed : region format");
		return strdup("en_GB");
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

	return strdup("en_GB");
}

/*static char *_get_locale_for_date(void)
  {
  char *locale = vconf_get_str(VCONFKEY_REGIONFORMAT);
  if (locale == NULL) {
  ERR("vconf_get_str() failed : region format");
  return strdup("en_GB.UTF8");
  }

  if (strlen(locale) > 0) {
  return locale;
  }

  return strdup("en_GB.UTF8");
  }*/

static inline char *_extend_heap(char *buffer, int *sz, int incsz)
{
	char *tmp;

	*sz += incsz;
	tmp = realloc(buffer, *sz);
	if (!tmp) {
		ERR("Heap");
		return NULL;
	}

	return tmp;
}

static char *_string_replacer(char *src, const char *pattern, const char *replace)
{
	char *ptr;
	char *tmp = NULL;
	char *ret = NULL;
	int idx = 0;
	int out_idx = 0;
	int out_sz = 0;
	enum {
		STATE_START,
		STATE_FIND,
		STATE_CHECK,
		STATE_END,
	} state;

	if (!src || !pattern) {
		return NULL;
	}

	out_sz = strlen(src);
	ret = strdup(src);
	if (!ret) {
		ERR("Heap");
		return NULL;
	}

	out_idx = 0;
	for (state = STATE_START, ptr = src; state != STATE_END; ptr++) {
		switch (state) {
		case STATE_START:
			if (*ptr == '\0') {
				state = STATE_END;
			} else if (!isblank(*ptr)) {
				state = STATE_FIND;
				ptr--;
			}
			break;
		case STATE_FIND:
			if (*ptr == '\0') {
				state = STATE_END;
			} else if (*ptr == *pattern) {
				state = STATE_CHECK;
				ptr--;
				idx = 0;
			} else if (*ptr == '-') {
				state = STATE_CHECK;
				*ptr = *pattern;
				ptr--;
				idx = 0;
			} else {
				ret[out_idx] = *ptr;
				out_idx++;
				if (out_idx == out_sz) {
					tmp = _extend_heap(ret, &out_sz, strlen(replace) + 1);
					if (!tmp) {
						free(ret);
						return NULL;
					}
					ret = tmp;
				}
			}
			break;
		case STATE_CHECK:
			if (!pattern[idx]) {
				/*!
				 * If there is no space for copying the replacement,
				 * Extend size of the return buffer.
				 */
				if (out_sz - out_idx < strlen(replace) + 1) {
					tmp = _extend_heap(ret, &out_sz, strlen(replace) + 1);
					if (!tmp) {
						free(ret);
						return NULL;
					}
					ret = tmp;
				}

				strcpy(ret + out_idx, replace);
				out_idx += strlen(replace);

				state = STATE_FIND;
				ptr--;
			} else if (*ptr != pattern[idx]) {
				ptr -= idx;

				/* Copy the first matched character */
				ret[out_idx] = *ptr;
				out_idx++;
				if (out_idx == out_sz) {
					tmp = _extend_heap(ret, &out_sz, strlen(replace) + 1);
					if (!tmp) {
						free(ret);
						return NULL;
					}

					ret = tmp;
				}

				state = STATE_FIND;
			} else {
				idx++;
			}
			break;
		default:
			break;
		}
	}

	ret[out_idx] = '\0';
	return ret;
}

static UDateTimePatternGenerator *__util_time_generator_get(void *data)
{
	UErrorCode status = U_ZERO_ERROR;
	UDateTimePatternGenerator *generator = NULL;

	struct appdata *ad = data;
	retif_nomsg(ad == NULL, NULL);
	retif_nomsg(s_info.timeregion_format == NULL, NULL);

	generator = udatpg_open(s_info.timeregion_format, &status);
	if (U_FAILURE(status)) {
		ERR("udatpg_open() failed");
		generator = NULL;
		return NULL;
	}
	return generator;
}

static UDateTimePatternGenerator *__util_date_generator_get(void *data)
{
	UErrorCode status = U_ZERO_ERROR;
	UDateTimePatternGenerator *generator = NULL;

	struct appdata *ad = data;
	retif_nomsg(ad == NULL, NULL);
	retif_nomsg(s_info.dateregion_format == NULL, NULL);

	generator = udatpg_open(s_info.dateregion_format, &status);
	if (U_FAILURE(status)) {
		ERR("udatpg_open() failed");
		generator = NULL;
		return NULL;
	}
	return generator;
}

static UDateFormat *__util_time_date_formatter_get(void *data, const char *timezone_id, const char *skeleton)
{
	UErrorCode status = U_ZERO_ERROR;

	UChar u_skeleton[BUF_FORMATTER] = {0,};
	int skeleton_len = 0;

	UChar u_best_pattern[BUF_FORMATTER] = {0,};
	int32_t u_best_pattern_capacity;
	UDateFormat *formatter = NULL;

	struct appdata *ad = data;
	retif_nomsg(ad == NULL, NULL);
	retif_nomsg(s_info.date_generator == NULL, NULL);

	u_uastrncpy(u_skeleton, skeleton, strlen(skeleton));
	skeleton_len = u_strlen(u_skeleton);

	u_best_pattern_capacity =
		(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));

	udatpg_getBestPattern(s_info.date_generator, u_skeleton, skeleton_len,
			u_best_pattern, u_best_pattern_capacity, &status);
	if (U_FAILURE(status)) {
		ERR("udatpg_getBestPattern() failed");
		return NULL;
	}

	UChar u_timezone_id[BUF_FORMATTER] = {0,};
	if (timezone_id == NULL) {
		u_uastrncpy(u_timezone_id, s_info.timezone_id, sizeof(u_timezone_id));
		formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, s_info.dateregion_format, u_timezone_id, -1,
				u_best_pattern, -1, &status);
	} else {
		u_uastrncpy(u_timezone_id, timezone_id, sizeof(u_timezone_id));
		formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, s_info.dateregion_format, u_timezone_id, -1,
				u_best_pattern, -1, &status);
	}
	if (U_FAILURE(status)) {
		ERR("udat_open() failed");
		return NULL;
	}

	char a_best_pattern[BUF_FORMATTER] = {0,};
	u_austrcpy(a_best_pattern, u_best_pattern);

	return formatter;
}

static UDateFormat *__util_time_ampm_formatter_get(void *data, const char *timezone_id)
{
	UErrorCode status = U_ZERO_ERROR;

	UChar u_best_pattern[BUF_FORMATTER] = {0,};
	UDateFormat *formatter = NULL;

	struct appdata *ad = data;
	retif_nomsg(ad == NULL, NULL);

	u_uastrcpy(u_best_pattern, "a");

	UChar u_timezone_id[BUF_FORMATTER] = {0,};
	if (timezone_id == NULL) {
		u_uastrncpy(u_timezone_id, s_info.timezone_id, sizeof(u_timezone_id));
		formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, s_info.timeregion_format, u_timezone_id, -1,
				u_best_pattern, -1, &status);
	} else {
		u_uastrncpy(u_timezone_id, timezone_id, sizeof(u_timezone_id));
		formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, s_info.timeregion_format, u_timezone_id, -1,
				u_best_pattern, -1, &status);
	}
	if (U_FAILURE(status)) {
		ERR("udat_open() failed");
		return NULL;
	}

	char a_best_pattern[BUF_FORMATTER] = {0,};
	u_austrcpy(a_best_pattern, u_best_pattern);

	return formatter;
}

static UDateFormat *__util_time_time_formatter_get(void *data, int time_format, const char *timezone_id)
{
	char buf[BUF_FORMATTER] = {0,};
	UErrorCode status = U_ZERO_ERROR;
	UChar u_pattern[BUF_FORMATTER] = {0,};
	UChar u_best_pattern[BUF_FORMATTER] = {0,};
	int32_t u_best_pattern_capacity;
	char a_best_pattern[BUF_FORMATTER] = {0,};

	UDateFormat *formatter = NULL;

	struct appdata *ad = data;
	retif_nomsg(ad == NULL, NULL);
	retif_nomsg(s_info.generator == NULL, NULL);

	if (time_format == QP_TIME_FORMAT_24) {
		snprintf(buf, sizeof(buf)-1, "%s", "HH:mm");
	} else {
		/* set time format 12 */
		snprintf(buf, sizeof(buf)-1, "%s", "h:mm");
	}

	if (u_uastrncpy(u_pattern, buf, sizeof(u_pattern)) == NULL) {
		ERR("u_uastrncpy() is failed.");
		return NULL;
	}

	u_best_pattern_capacity =
		(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));

	udatpg_getBestPattern(s_info.generator, u_pattern, u_strlen(u_pattern),
			u_best_pattern, u_best_pattern_capacity, &status);
	if (U_FAILURE(status)) {
		ERR("udatpg_getBestPattern() failed");
		return NULL;
	}

	u_austrcpy(a_best_pattern, u_best_pattern);

	if (a_best_pattern[0] == 'a') {
		s_info.is_pre_meridiem = EINA_TRUE;
	} else {
		s_info.is_pre_meridiem = EINA_FALSE;
	}

	u_uastrcpy(u_best_pattern, buf);

	UChar u_timezone_id[BUF_FORMATTER] = {0,};
	if (timezone_id == NULL) {
		u_uastrncpy(u_timezone_id, s_info.timezone_id, sizeof(u_timezone_id));
		formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, s_info.timeregion_format, u_timezone_id, -1,
				u_best_pattern, -1, &status);
	} else {
		u_uastrncpy(u_timezone_id, timezone_id, sizeof(u_timezone_id));
		formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, s_info.timeregion_format, u_timezone_id, -1,
				u_best_pattern, -1, &status);
	}
	if (U_FAILURE(status)) {
		ERR("udat_open() failed");
		return NULL;
	}

	return formatter;
}

static void _util_time_formatters_create(void *data)
{
	struct appdata *ad = data;
	retif_nomsg(ad == NULL, );

	if (s_info.generator == NULL) {
		s_info.generator = __util_time_generator_get(ad);
	}

	if (s_info.date_generator == NULL) {
		s_info.date_generator = __util_date_generator_get(ad);
	}

	if (s_info.formatter_date == NULL) {
		s_info.formatter_date = __util_time_date_formatter_get(ad, NULL, "MMMMEd");
	}

	if (s_info.timeformat == QP_TIME_FORMAT_12) {
		if (s_info.formatter_ampm == NULL) {
			s_info.formatter_ampm = __util_time_ampm_formatter_get(ad, NULL);
		}
	}

	if (s_info.formatter_time == NULL) {
		s_info.formatter_time = __util_time_time_formatter_get(ad, s_info.timeformat, NULL);
	}
}

static void _util_time_formatters_destroy(void *data)
{
	struct appdata *ad = data;
	retif_nomsg(ad == NULL, );

	if (s_info.date_generator) {
		udat_close(s_info.date_generator);
		s_info.date_generator = NULL;
	}

	if (s_info.generator) {
		udat_close(s_info.generator);
		s_info.generator = NULL;
	}

	if (s_info.formatter_date) {
		udat_close(s_info.formatter_date);
		s_info.formatter_date = NULL;
	}
	if (s_info.formatter_time) {
		udat_close(s_info.formatter_time);
		s_info.formatter_time = NULL;
	}
	if (s_info.formatter_ampm) {
		udat_close(s_info.formatter_ampm);
		s_info.formatter_ampm = NULL;
	}
}

static char *_util_time_regionformat_get(void)
{
	return _get_locale();
}

static char *_util_date_regionformat_get(void)
{
	return _get_locale();
}

static char* _get_timezone_from_vconf(void)
{
	char *szTimezone = NULL;
	szTimezone = vconf_get_str(VCONFKEY_SETAPPL_TIMEZONE_ID);
	if (szTimezone == NULL || strlen(szTimezone) == 0)
	{
		ERR("QUICKPANEL TIMEZONE - Cannot get time zone.");
		return strdup("N/A");
	}

	return szTimezone;
}

static char *_util_time_timezone_id_get(void)
{
	char buf[1024] = {0,};
	ssize_t len = readlink("/opt/etc/localtime", buf, sizeof(buf)-1);

	INFO("QUICKPANEL TIMEZONE -  %s",  buf);

	if (len != -1) {
		buf[len] = '\0';
	} else {
		ERR("QUICKPANEL TIMEZONE - failed to get a timezone information");
		return _get_timezone_from_vconf();
	}

	return strdup(buf + 20);
}

static int _util_time_formatted_time_get(UDateFormat *formatter, time_t tt, char *buf, int buf_len)
{
	i18n_udate u_time = (i18n_udate)(tt) * 1000;
	i18n_uchar u_formatted_str[BUF_FORMATTER] = {0, };
	int32_t u_formatted_str_capacity;
	int32_t formatted_str_len = -1;
	int status = I18N_ERROR_INVALID_PARAMETER;

	u_formatted_str_capacity =
		(int32_t)(sizeof(u_formatted_str) / sizeof((u_formatted_str)[0]));

	status = i18n_udate_format_date(formatter, u_time, u_formatted_str, u_formatted_str_capacity, NULL, &formatted_str_len);
	if (status != I18N_ERROR_NONE) {
		ERR("i18n_udate_format_date() failed");
		return -1;
	}

	if (formatted_str_len <= 0) {
		ERR("formatted_str_len is less than 0");
	}

	buf = i18n_ustring_copy_au_n(buf, u_formatted_str, (int32_t)buf_len);
	DBG("date:(%d)[%s][%d]", formatted_str_len, buf, tt);

	return (int)u_strlen(u_formatted_str);
}

static void _formatter_create(void *data)
{
	int ret = 0;
	struct appdata *ad = data;
	retif_nomsg(ad == NULL, );
	bool status = false;

	ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, &status);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to ignore key(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR) : %d", ret);

	if (status == true){
		s_info.timeformat = QP_TIME_FORMAT_24;
	}else{
		s_info.timeformat = QP_TIME_FORMAT_12;
	}

	if (s_info.timeregion_format == NULL) {
		s_info.timeregion_format = _util_time_regionformat_get();
	}

	if (s_info.dateregion_format == NULL) {
		s_info.dateregion_format = _util_date_regionformat_get();
	}

	if (s_info.timezone_id == NULL) {
		s_info.timezone_id = _util_time_timezone_id_get();
	}

	ICU_set_timezone(s_info.timezone_id);

	_util_time_formatters_create(ad);

	s_info.is_initialized = 1;
	DBG("%d %s %s", s_info.timeformat, s_info.timeregion_format, s_info.timezone_id);
}

static void _formatter_destory(void *data)
{
	struct appdata *ad = data;
	retif_nomsg(ad == NULL, );

	if (s_info.timeregion_format) {
		free(s_info.timeregion_format);
		s_info.timeregion_format = NULL;
	}
	if (s_info.dateregion_format) {
		free(s_info.dateregion_format);
		s_info.dateregion_format = NULL;
	}
	if (s_info.timezone_id) {
		free(s_info.timezone_id);
		s_info.timezone_id = NULL;
	}

	_util_time_formatters_destroy(ad);

	s_info.is_initialized = 0;
}

static void _util_time_setting_changed_cb(system_settings_key_e key, void *data)
{
	struct appdata *ad = data;

	_formatter_destory(ad);
	_formatter_create(ad);

	_util_time_heartbeat_do();

	//upate noti time information.
	quickpanel_noti_update_by_system_time_changed_setting_cb(key, ad);
}

static void _util_time_vconf_changed_cb(keynode_t *key, void *data)
{
	struct appdata *ad = data;

	_formatter_destory(ad);
	_formatter_create(ad);

	_util_time_heartbeat_do();

	//upate noti time information.
	quickpanel_noti_update_by_system_time_changed_vconf_cb(key, ad);
}

static void _time_event_deattach(void *data)
{
	int ret = 0;
	struct appdata *ad = data;
	retif_nomsg(ad == NULL, );

	/* unregister vconf cbs */
	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_TIMEZONE_INT, _util_time_vconf_changed_cb);
	msgif(ret != 0, "failed to set key(%s) : %d", VCONFKEY_SETAPPL_TIMEZONE_INT, ret);
	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_TIMEZONE_ID, _util_time_vconf_changed_cb);
	msgif(ret != 0, "failed to set key(%s) : %d", VCONFKEY_SETAPPL_TIMEZONE_ID, ret);
	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SVC_ROAM, _util_time_vconf_changed_cb);
	msgif(ret != 0, "failed to set key(%s) : %d", VCONFKEY_TELEPHONY_SVC_ROAM, ret);

	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_TIME_CHANGED);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to set key(%d) : %d", SYSTEM_SETTINGS_KEY_TIME_CHANGED, ret);
	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to set key(%d) : %d", SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, ret);
	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to set key(%d) : %d", SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, ret);
}

static void _time_event_attach(void *data)
{
	int ret = 0;
	struct appdata *ad = data;
	retif_nomsg(ad == NULL, );

	/* register vconf cbs */
	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_TIMEZONE_INT, _util_time_vconf_changed_cb, data);
	msgif(ret != 0, "failed to set key(%s) : %d", VCONFKEY_SETAPPL_TIMEZONE_INT, ret);
	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_TIMEZONE_ID, _util_time_vconf_changed_cb, data);
	msgif(ret != 0, "failed to set key(%s) : %d", VCONFKEY_SETAPPL_TIMEZONE_ID, ret);
	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SVC_ROAM, _util_time_vconf_changed_cb, data);
	msgif(ret != 0, "failed to set key(%s) : %d", VCONFKEY_TELEPHONY_SVC_ROAM, ret);

	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_TIME_CHANGED, _util_time_setting_changed_cb, data);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to set key(%d) : %d", SYSTEM_SETTINGS_KEY_TIME_CHANGED, ret);
	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, _util_time_setting_changed_cb, data);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to set key(%d) : %d", SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, ret);
	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, _util_time_setting_changed_cb, data);
	msgif(ret != SYSTEM_SETTINGS_ERROR_NONE, "failed to set key(%d) : %d", SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, ret);

}

static void _util_time_get(int is_current_time, time_t tt_a, char **str_date, char **str_time, char **str_meridiem)
{
	time_t tt;
	struct tm st;
	char buf_date[512] = {0,};
	char buf_time[512] = {0,};
	char buf_ampm[512] = {0,};

	char *convert_formatted_str = NULL;

	if (is_current_time == 1) {
		tt = time(NULL);
	} else {
		tt = tt_a;
	}
	localtime_r(&tt, &st);

	/* date */
	_util_time_formatted_time_get(s_info.formatter_date, tt, buf_date, sizeof(buf_date));

	/* time */
	if (s_info.timeformat == QP_TIME_FORMAT_24) {
		_util_time_formatted_time_get(s_info.formatter_time, tt, buf_time, sizeof(buf_time)-1);
	} else {
		_util_time_formatted_time_get(s_info.formatter_time, tt, buf_time, sizeof(buf_time)-1);
		int ampm_len = _util_time_formatted_time_get(s_info.formatter_ampm, tt, buf_ampm, sizeof(buf_ampm)-1);
		if (ampm_len > 4) {
			if (st.tm_hour >= 0 && st.tm_hour < 12) {
				snprintf(buf_ampm, sizeof(buf_ampm)-1, "AM");
			} else {
				snprintf(buf_ampm, sizeof(buf_ampm)-1, "PM");
			}
		}
	}

	if (strstr(s_info.timeregion_format, "ar_")) {
		convert_formatted_str = strdup(buf_time);
	} else {
		convert_formatted_str = _string_replacer(buf_time, colon, ratio);
	}

	if (str_date != NULL) {
		*str_date = strdup(buf_date);
	}

	if (str_meridiem != NULL) {
		*str_meridiem = strdup(buf_ampm);
	}

	if (convert_formatted_str)
	{
		if (str_time != NULL) {
			*str_time = strdup(convert_formatted_str);
		}
		free(convert_formatted_str);
	}
}

static void _timer_add(void)
{
	time_t tt;
	struct tm st;

	tt = time(NULL);
	localtime_r(&tt, &st);

	s_info.timer = ecore_timer_add(60 - st.tm_sec, _timer_cb, NULL);
}

static void _timer_del(void)
{
	if (s_info.timer != NULL) {
		ecore_timer_del(s_info.timer);
		s_info.timer = NULL;
	}
}

static Eina_Bool _timer_cb(void *data)
{
	_util_time_heartbeat_do();

	if (s_info.is_timer_enabled ==1) {
		_timer_del();
		_timer_add();
	}
	return ECORE_CALLBACK_CANCEL;
}

static int _init(void *data)
{
	_formatter_create(data);
	_time_event_attach(data);

	return QP_OK;
}

static int _fini(void *data)
{
	_time_event_deattach(data);
	_formatter_destory(data);

	return QP_OK;
}

static void _lang_changed(void *data)
{
	_util_time_vconf_changed_cb(NULL, data);
}

static void _util_time_heartbeat_do(void)
{
	int type_meridiem = UTIL_TIME_MERIDIEM_TYPE_NONE;
	char *str_date = NULL;
	char *str_time = NULL;
	char *str_meridiem = NULL;

	if (s_info.is_initialized == 0) {
		ERR("time information ins't initialized");
		return;
	}

	_util_time_get(1, 0, &str_date, &str_time, &str_meridiem);

	if (str_meridiem != NULL) {
		if (s_info.is_pre_meridiem == EINA_TRUE) {
			type_meridiem = UTIL_TIME_MERIDIEM_TYPE_PRE;
		} else {
			type_meridiem = UTIL_TIME_MERIDIEM_TYPE_POST;
		}
	}
	quickpanel_datetime_view_update(str_date, str_time, str_meridiem, type_meridiem);

	if (str_date) {
		free(str_date);
	}
	if (str_time) {
		free(str_time);
	}
	if (str_meridiem) {
		free(str_meridiem);
	}
}

HAPI void quickpanel_util_time_timer_enable_set(int is_enable)
{
	_timer_del();

	if (is_enable == 1) {
		_timer_add();
	}

	_util_time_heartbeat_do();

	s_info.is_timer_enabled = is_enable;
}
