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


#ifndef _QP_SMART_ALERT_DEF_
#define _QP_SMART_ALERT_DEF_


#if !defined(VENDOR)
#define SMART_ALARM_CALL_PKGNAME "org.tizen.call-notification"
#define SMART_ALARM_VTCALL_PKGNAME "org.tizen.vtmain"
#define SMART_ALARM_MSG_PKGNAME "org.tizen.message"
#else
#define SMART_ALARM_CALL_PKGNAME VENDOR".call-notification"
#define SMART_ALARM_VTCALL_PKGNAME VENDOR".vtmain"
#define SMART_ALARM_MSG_PKGNAME VENDOR".message"
#endif

extern void quickpanel_smart_alert_update_info(notification_h noti);

#endif
