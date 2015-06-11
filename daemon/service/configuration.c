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


#include <system_settings.h>
#include "common.h"
#include "configuration.h"

static struct _s_configuration_info {
	int longpress_threshold;
} s_configuration_info = {
#ifdef HAVE_X
	.longpress_threshold = SYSTEM_SETTINGS_TAP_AND_HOLD_DELAY_SHORT,
#else
	.longpress_threshold = 0,
#endif
};

static void _conf_longpress_threshold_cb(system_settings_key_e key, void *user_data)
{
#ifdef HAVE_X
	int delay = SYSTEM_SETTINGS_TAP_AND_HOLD_DELAY_SHORT; /* default 0.5 sec */
#else
	int delay = 0.5;
#endif

#ifdef HAVE_X
	if (SYSTEM_SETTINGS_KEY_TAP_AND_HOLD_DELAY == key)
#endif
	{
#ifdef HAVE_X
		if (system_settings_get_value_int(SYSTEM_SETTINGS_KEY_TAP_AND_HOLD_DELAY, &delay) != 0) {
			ERR("Failed to get tap and hold delay");
			return;
		}
#endif
		if (s_configuration_info.longpress_threshold != delay) {
			s_configuration_info.longpress_threshold = delay;
		}

		DBG("Current tap and hold delay : [%d] msec", delay);
	}
}

HAPI void quickpanel_conf_init(void *data)
{
#ifdef HAVE_X
	if (system_settings_get_value_int(SYSTEM_SETTINGS_KEY_TAP_AND_HOLD_DELAY,
			&s_configuration_info.longpress_threshold) != 0) {
		ERR("Failed to get tap and hold delay");
	}

	if (system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_TAP_AND_HOLD_DELAY,
			_conf_longpress_threshold_cb, NULL) != 0) {
		ERR("Failed to set tap and hold delay changed callback");
	}
#endif
}

HAPI void quickpanel_conf_fini(void *data)
{
#ifdef HAVE_X
   if (system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_TAP_AND_HOLD_DELAY) != 0) {
		ERR("Failed to unset tab and hold delay changed callback");
	}
#endif
}

HAPI double quickpanel_conf_longpress_time_get(void)
{
   return (double)(s_configuration_info.longpress_threshold)/(double)1000.0;
}
