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

#endif /* __QP_MODULES_H__ */
