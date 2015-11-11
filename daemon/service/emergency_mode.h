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


#ifndef _QP_EMERGENCY_MODE_DEF_
#define _QP_EMERGENCY_MODE_DEF_

#define PACKAGE_EMERGENCY_MODE_SETTING "setting-emergency-efl"

extern void quickpanel_emergency_mode_init(void *data);
extern void quickpanel_emergency_mode_fini(void *data);
extern int quickpanel_emergency_mode_is_permitted_app(const char *appid);
extern int quickpanel_emergency_mode_is_on(void);
extern int quickpanel_emergency_mode_notification_filter(notification_h noti, int is_delete);
extern int quickpanel_emergency_mode_syspopup_launch(void);

#endif
