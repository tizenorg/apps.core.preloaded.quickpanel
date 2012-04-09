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

#include <appcore-efl.h>
#include "common.h"
#include "modules.h"

/*******************************************************************
  *
  * MODULES
  *
  *****************************************************************/
/* searchbar */
/* extern QP_Module searchbar; */
/* notification */
extern QP_Module noti;
extern QP_Module ticker;
/* idle test */
extern QP_Module idletxt;

static QP_Module *modules[] = {
	&noti,
	&ticker,
	&idletxt
};

int init_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->init) {
			modules[i]->init(data);
		}
	}

	return QP_OK;
}

int fini_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->fini) {
			modules[i]->fini(data);
		}
	}

	return QP_OK;
}

int suspend_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->suspend) {
			modules[i]->suspend(data);
		}
	}

	return QP_OK;
}

int resume_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->resume) {
			modules[i]->resume(data);
		}
	}

	return QP_OK;
}

int hib_enter_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->hib_enter) {
			modules[i]->hib_enter(data);
		}
	}

	return QP_OK;
}

int hib_leave_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->hib_leave) {
			modules[i]->hib_leave(data);
		}
	}

	return QP_OK;
}

/******************************************************************
  *
  * LANGUAGE
  *
  ****************************************************************/

void lang_change_modules(void *data)
{
	int i;
	retif(data == NULL,, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->lang_changed) {
			modules[i]->lang_changed(data);
		}
	}
}

void refresh_modules(void *data)
{
	int i;
	retif(data == NULL,, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->refresh) {
			modules[i]->refresh(data);
		}
	}
}
