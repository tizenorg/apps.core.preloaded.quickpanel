/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.tizenopensource.org/license
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
#include "quickpanel_debug_util.h"

#define QP_OK	(0)
#define QP_FAIL	(-1)

#ifdef _DLOG_USED
#define LOG_TAG "quickpanel"
#include <dlog.h>

#define ERR(str,args...) 		LOGE("%s[%d]\t " #str "\n", __func__, __LINE__, ##args)
#define DBG(str,args...) 		LOGD("%s[%d]\t " #str "\n", __func__, __LINE__, ##args)
#define INFO(str,args...) 		LOGI(#str"\n", ##args)
#elif FILE_DEBUG /*_DLOG_USED*/
#define ERR(str,args...) 		debug_printf("%s[%d]\t " #str "\n", __func__, __LINE__, ##args)
#define DBG(str,args...) 		debug_printf("%s[%d]\t " #str "\n", __func__, __LINE__, ##args)
#define INFO(str,args...) 		debug_printf(#str"\n", ##args)
#else /*_DLOG_USED*/
#define ERR(str,args...) 		fprintf( stderr, "%s[%d]\t " #str "\n", __func__, __LINE__, ##args)
#define DBG(str,args...) 		fprintf( stderr, "%s[%d]\t " #str "\n", __func__, __LINE__, ##args)
#define INFO(str,args...) 		fprintf( stderr, #str"\n", ##args)
#endif /*_DLOG_USED*/

#define retif(cond, ret, str, args...) do{\
	if(cond) {\
		DBG(str, ##args);\
		return ret;\
	}\
}while(0);

#define gotoif(cond, target, str, args...) do{\
	if(cond) {\
		DBG(str, ##args);\
		goto target;\
	}\
}while(0);

#endif				/* __QP_COMMON_H_ */
