/*                                                                                                                                                    
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All Rights Reserved. 
 *                                                                                 
 * This file is part of quickpanel  
 *
 * Written by Jeonghoon Park <jh1979.park@samsung.com> Youngjoo Park <yjoo93.park@samsung.com>
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
