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


#ifndef __SETTING_ICON_COMMON_H__
#define __SETTING_ICON_COMMON_H__

int quickpanel_setting_icon_click_cb_add(Evas_Object *icon,
			Edje_Signal_Cb func, void *data);
int quickpanel_setting_icon_click_cb_without_feedback_add(Evas_Object *icon,
			Edje_Signal_Cb func, void *data);
int quickpanel_setting_icon_click_cb_del(Evas_Object *icon, Edje_Signal_Cb func);
void quickpanel_setting_icon_handler_longpress(const char *pkgname, void *data);

#endif /* __SETTING_ICON_COMMON_H__ */
