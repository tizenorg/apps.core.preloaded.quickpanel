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

#ifndef __QP_COMMON_H_
#define __QP_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <Elementary.h>
#include "quickpanel_debug_util.h"

#define QP_OK	(0)
#define QP_FAIL	(-1)

#ifdef _DLOG_USED
#define LOG_TAG "QUICKPANEL"
#include <dlog.h>

#define HAPI __attribute__((visibility("hidden")))

#define DBG(fmt , args...) \
	do { \
		LOGD("[%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define INFO(fmt , args...) \
	do { \
		LOGI("[%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define WARN(fmt , args...) \
	do { \
		LOGI("[%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define ERR(fmt , args...) \
	do { \
		LOGE("[%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#elif FILE_DEBUG /*_DLOG_USED*/
#define DBG(fmt , args...) \
	do { \
		debug_printf("[D]%s : %d] "fmt"\n", \
			__func__, __LINE__, ##args); \
	} while (0)

#define INFO(fmt , args...) \
	do { \
		debug_printf("[I][%s : %d] "fmt"\n",\
			__func__, __LINE__, ##args); \
	} while (0)

#define WARN(fmt , args...) \
	do { \
		debug_printf("[W][%s : %d] "fmt"\n", \
			__func__, __LINE__, ##args); \
	} while (0)

#define ERR(fmt , args...) \
	do { \
		debug_printf("[E][%s : %d] "fmt"\n", \
			__func__, __LINE__, ##args); \
	} while (0)

#else /*_DLOG_USED*/
#define DBG(fmt , args...) \
	do { \
		fprintf("[D][%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define INFO(fmt , args...) \
	do { \
		fprintf("[I][%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define WARN(fmt , args...) \
	do { \
		fprintf("[W][%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define ERR(fmt , args...) \
	do { \
		fprintf("[E][%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)
#endif /*_DLOG_USED*/

#define msgif(cond, str, args...) do { \
	if (cond) { \
		ERR(str, ##args);\
	} \
} while (0);

#define retif(cond, ret, str, args...) do { \
	if (cond) { \
		ERR(str, ##args);\
		return ret;\
	} \
} while (0);

#define gotoif(cond, target, str, args...) do { \
	if (cond) { \
		WARN(str, ##args); \
		goto target; \
	} \
} while (0);


void quickpanel_util_char_replace(char *text, char s, char t);
void quickpanel_ui_set_current_popup(Evas_Object *popup);
void quickpanel_ui_del_current_popup(void);

#endif				/* __QP_COMMON_H_ */
