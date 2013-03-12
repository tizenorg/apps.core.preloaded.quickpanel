/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __QP_MODULES_H__
#define __QP_MODULES_H__

#include <stdlib.h>
#include "quickpanel-ui.h"

extern int init_modules(void *data);
extern int fini_modules(void *data);
extern int suspend_modules(void *data);
extern int resume_modules(void *data);
extern int hib_enter_modules(void *data);
extern int hib_leave_modules(void *data);
extern void lang_change_modules(void *data);
extern void refresh_modules(void *data);
extern int qp_opened_modules(void *data);
extern int qp_closed_modules(void *data);

#endif /* __QP_MODULES_H__ */
