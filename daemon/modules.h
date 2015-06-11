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


#ifndef __QP_MODULES_H__
#define __QP_MODULES_H__

#include <stdlib.h>
#include "quickpanel-ui.h"

extern int quickpanel_modules_init(void *data);
extern int quickpanel_modules_fini(void *data);
extern int quickpanel_modules_suspend(void *data);
extern int quickpanel_modules_resume(void *data);
extern int quickpanel_modules_hib_enter(void *data);
extern int quickpanel_modules_hib_leave(void *data);
extern void quickpanel_modules_lang_change(void *data);
extern void quickpanel_modules_refresh(void *data);
extern int quickpanel_modules_opened(void *data);
extern int quickpanel_modules_closed(void *data);

#endif /* __QP_MODULES_H__ */
