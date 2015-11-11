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

#include <Elementary.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>


#include "common.h"
#include "quickpanel-ui.h"
#include "modules.h"

/*******************************************************************
 *
 * MODULES
 *
 *****************************************************************/

#ifdef QP_SETTING_ENABLE
/* setting */
extern QP_Module settings;
extern QP_Module settings_view_featured;
extern QP_Module settings_view_all;
#endif /* QP_SETTING_ENABLE */

#ifdef QP_MINICTRL_ENABLE
extern QP_Module minictrl;
#endif /* QP_MINICTRL_ENABLE */

#ifdef QP_BRIGHTNESS_ENABLE
/* brightness */
extern QP_Module brightness_ctrl;
#endif /* QP_BRIGHTNESS_ENABLE */

#ifdef QP_ANIMATED_IMAGE_ENABLE
extern QP_Module animated_image;
#endif

extern QP_Module vi_manager;
extern QP_Module pager;

/* notification */
extern QP_Module noti;
extern QP_Module activenoti;
extern QP_Module qp_datetime_controller;
extern QP_Module qp_datetime_view;

/* voice control */
#ifdef QP_VOICE_CONTROL_ENABLE
extern QP_Module voice_control;
#endif

/* do not change the order of modules, result may be changed up to order */
static QP_Module *modules[] = {
	&vi_manager,
	&pager,
	&qp_datetime_controller,
	&qp_datetime_view,
#ifdef QP_SETTING_ENABLE
	&settings,
	&settings_view_featured,
	&settings_view_all,
#endif /* QP_SETTING_ENABLE */
#ifdef QP_MINICTRL_ENABLE
	&minictrl,
#endif /* QP_MINICTRL_ENABLE */
#ifdef QP_BRIGHTNESS_ENABLE
	&brightness_ctrl,
#endif /* QP_BRIGHTNESS_ENABLE */
	&noti,
	&activenoti,
#ifdef QP_ANIMATED_IMAGE_ENABLE
	&animated_image,
#endif
#ifdef QP_VOICE_CONTROL_ENABLE
	&voice_control,
#endif
};

HAPI int quickpanel_modules_init(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->init) {
			modules[i]->init(data);
		}

		if (modules[i]->init_job_cb) {
			ecore_job_add(modules[i]->init_job_cb, data);
		}
	}

	return QP_OK;
}

HAPI int quickpanel_modules_fini(void *data)
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

HAPI int quickpanel_modules_suspend(void *data)
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

HAPI int quickpanel_modules_resume(void *data)
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

HAPI int quickpanel_modules_hib_enter(void *data)
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

HAPI int quickpanel_modules_hib_leave(void *data)
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

HAPI void quickpanel_modules_lang_change(void *data)
{
	int i;
	retif(data == NULL, , "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->lang_changed) {
			modules[i]->lang_changed(data);
		}
	}
}

HAPI void quickpanel_modules_refresh(void *data)
{
	int i;
	retif(data == NULL, , "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->refresh) {
			modules[i]->refresh(data);
		}
	}
}

/******************************************************************
 *
 * Quickpanel open/close Events
 *
 ****************************************************************/
HAPI int quickpanel_modules_opened(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->qp_opened) {
			modules[i]->qp_opened(data);
		}
	}

	return QP_OK;
}

HAPI int quickpanel_modules_closed(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->qp_closed) {
			modules[i]->qp_closed(data);
		}
	}

	return QP_OK;
}
