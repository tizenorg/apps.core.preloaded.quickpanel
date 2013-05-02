/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _QP_NOTI_UTIL_DEF_
#define _QP_NOTI_UTIL_DEF_

#include <notification.h>

HAPI int quickpanel_noti_get_event_count_from_noti(notification_h noti);
int quickpanel_noti_get_event_count_by_pkgname(const char *pkgname);
char *quickpanel_noti_get_time(time_t t, char *buf, int buf_len);

#endif
