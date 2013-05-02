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

#ifdef QP_SERVICE_NOTI_LED_ENABLE
#ifndef _QP_NOTI_LED_DEF_
#define _QP_NOTI_LED_DEF_

#include <notification.h>
#include <dd-led.h>
#include "quickpanel-ui.h"

#if !defined(VENDOR)
#define NOTI_LED_CALL_PKGNAME "com.samsung.call"
#define NOTI_LED_VTCALL_PKGNAME "com.samsung.vtmain"
#else
#define NOTI_LED_CALL_PKGNAME VENDOR".call"
#define NOTI_LED_VTCALL_PKGNAME VENDOR".vtmain"
#endif
#define NOTI_LED_MSG_PKGNAME "/usr/bin/msg-server"

void quickpanel_service_noti_led_on(notification_h noti);
void quickpanel_service_noti_led_off(notification_h noti);
void quickpanel_service_noti_init(void *data);
void quickpanel_service_noti_fini(void *data);

#endif
#endif
