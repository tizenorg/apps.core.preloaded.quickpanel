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


#ifndef __DATETIME_H__
#define __DATETIME_H__

extern void quickpanel_datetime_datentime_event_set(int is_clickable);
extern void quickpanel_datetime_editing_icon_visibility_set(int is_visible);
extern void quickpanel_datetime_view_update(char *date, char *time, char *meridiem, int meridiem_type);

#endif	/* __DATETIME_H__ */
