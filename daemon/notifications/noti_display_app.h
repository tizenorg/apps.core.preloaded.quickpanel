/*
* Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All Rights Reserved.
*
* This file is part of quickpanel
*
* Written by [Rajeev Ranjan] <rajeev.r@samsung.com>
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

#ifndef __NOTI_DISPLAY_APP_H__
#define __NOTI_DISPLAY_APP_H__
#include <Elementary.h>

/* Initializes the notification daemon */
void notification_daemon_init();
/* Sets the window which rotation will be used to rotate the notification popup
while displaying */
void notification_daemon_win_set(Evas_Object *win);
/* Terminates the notification daemon */
void notification_daemon_shutdown();
#endif
