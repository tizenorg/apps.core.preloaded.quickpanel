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

#ifndef __NOTI_WIN_H__
#define __NOTI_WIN_H__
#include <Evas.h>

enum Noti_Orient {
	NOTI_ORIENT_TOP = 0,
	NOTI_ORIENT_BOTTOM,
	NOTI_ORIENT_LAST
	} ;

/* Creates and return a new window (of widget type elm_win) of width equal to
root window
*/
Evas_Object *noti_win_add(Evas_Object *parent);

/* Sets an Evas Object as content of the notification window created using
noti_win_add
*/
void noti_win_content_set(Evas_Object *obj, Evas_Object *content);

/* Sets the orientation of the notification window, this can be of type
Noti_Orient
*/
void noti_win_orient_set(Evas_Object *obj, enum Noti_Orient orient);
#endif
