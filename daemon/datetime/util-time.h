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


#ifndef __UTIL_TIME_H__
#define __UTIL_TIME_H__

#define UTIL_TIME_MERIDIEM_TYPE_NONE 0
#define UTIL_TIME_MERIDIEM_TYPE_PRE 1
#define UTIL_TIME_MERIDIEM_TYPE_POST 2

enum qp_time_format {
	QP_TIME_FORMAT_UNKNOWN,
	QP_TIME_FORMAT_12,
	QP_TIME_FORMAT_24,
};

extern void quickpanel_util_time_timer_enable_set(int is_enable);

#endif				/* __UTIL_TIME_H__ */
