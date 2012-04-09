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

#ifndef __QUICKPANEL_UI_H__
#define __QUICKPANEL_UI_H__

#include <Elementary.h>

#if !defined(PACKAGE)
#  define PACKAGE			"quickpanel"
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR			"/opt/apps/org.tizen.quickpanel/res/locale"
#endif

#if !defined(EDJDIR)
#  define EDJDIR			"/opt/apps/org.tizen.quickpanel/res/edje"
#endif

/* EDJ theme */
#define DEFAULT_NOTI_EDJ		EDJDIR"/"PACKAGE"_noti.edj"
#define DEFAULT_CUSTOM_EDJ		EDJDIR"/"PACKAGE"_theme.edj"

#define GROUP_NOTI		"quickpanel/noti"

#define _EDJ(o) elm_layout_edje_get(o)
#define _S(str)	dgettext("sys_string", str)

#define QP_PRIO_NOTI		100

#define QP_DEFAULT_WINDOW_H	1280	// Default is HD(720 X 1280)

struct appdata {
	struct {
		Evas_Object *win;
		Evas_Object *ly;
		Evas *evas;
		double h;
	} noti;

	int angle;
	double scale;

	int is_emul; // 0 : target, 1 : emul
	Ecore_Event_Handler *hdl_client_message;
	Ecore_Event_Handler *hdl_win_property;

	Evas_Object *notilist;
	Evas_Object *idletxtbox;

	E_DBus_Connection *dbus_connection;
	E_DBus_Signal_Handler *dbus_handler_size;
	E_DBus_Signal_Handler *dbus_handler_progress;
	E_DBus_Signal_Handler *dbus_handler_content;
};

typedef struct _QP_Module {
	char *name;
	/* func */
	int (*init) (void *);
	int (*fini) (void *);
	int (*suspend) (void *);
	int (*resume) (void *);
	int (*hib_enter) (void *);
	int (*hib_leave) (void *);
	void (*lang_changed) (void *);
	void (*refresh) (void *);

	/* do not modify this area */
	/* internal data */
	Eina_Bool state;
} QP_Module;

#endif				/* __QUICKPANEL_UI_H__ */
