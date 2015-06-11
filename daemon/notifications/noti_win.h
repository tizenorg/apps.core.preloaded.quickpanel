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
Evas_Object *quickpanel_noti_win_add(Evas_Object *parent);

/* Sets an Evas Object as content of the notification window created using
noti_win_add
*/
void quickpanel_noti_win_content_set(Evas_Object *obj, Evas_Object *content, int btn_cnt);

/* Sets the orientation of the notification window, this can be of type
Noti_Orient
*/
void quickpanel_noti_win_orient_set(Evas_Object *obj, enum Noti_Orient orient);
#endif
