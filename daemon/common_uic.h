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


#ifndef __QP_COMMON_UIC_H_
#define __QP_COMMON_UIC_H_

typedef enum {
	OPENED_NO_REASON = 0,
	OPENED_BY_CMD_HIDE_LAUNCH = 1,
	OPENED_BY_CMD_SHOW_SETTINGS = 2,
} qp_open_reason;

extern Evas_Object *quickpanel_uic_load_edj(Evas_Object * parent, const char *file, const char *group, int is_just_load);
extern int quickpanel_uic_launch_app(char *app_id, void *data);
extern int quickpanel_uic_launch_ug_by_appcontrol(const char *package, void *data);
extern int quickpanel_uic_is_emul(void);
extern int quickpanel_uic_is_suspended(void);
extern int quickpanel_uic_is_opened(void);
extern void quickpanel_uic_launch_app_inform_result(const char *pkgname, int retcode);
extern void quickpanel_uic_initial_resize(Evas_Object *obj, int height);
extern void quickpanel_uic_close_quickpanel(bool is_check_lock, int is_delay_needed);
extern void quickpanel_uic_open_quickpanel(int reason);
extern void quickpanel_uic_toggle_openning_quickpanel(void);
extern void quickpanel_uic_opened_reason_set(int reason);
extern int quickpanel_uic_opened_reason_get(void);

#endif				/* __QP_COMMON_UIC_H_ */
