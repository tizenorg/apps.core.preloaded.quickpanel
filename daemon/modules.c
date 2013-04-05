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

#include "common.h"
#include "modules.h"

/*******************************************************************
  *
  * MODULES
  *
  *****************************************************************/
/* searchbar */
/* extern QP_Module searchbar; */
#ifdef QP_MINICTRL_ENABLE
extern QP_Module minictrl;
#endif /* QP_MINICTRL_ENABLE */

#ifdef QP_BRIGHTNESS_ENABLE
/* brightness */
extern QP_Module brightness_ctrl;
#endif /* QP_BRIGHTNESS_ENABLE */
/* notification */
extern QP_Module noti;
extern QP_Module ticker;
extern QP_Module ticker_status;
/* idle test */
extern QP_Module idletxt;

static QP_Module *modules[] = {
#ifdef QP_MINICTRL_ENABLE
	&minictrl,
#endif /* QP_MINICTRL_ENABLE */
#ifdef QP_BRIGHTNESS_ENABLE
	&brightness_ctrl,
#endif /* QP_BRIGHTNESS_ENABLE */
	&noti,
	&ticker,
	&ticker_status,
	&idletxt
};

HAPI int init_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->init)
			modules[i]->init(data);
	}

	return QP_OK;
}

HAPI int fini_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->fini)
			modules[i]->fini(data);
	}

	return QP_OK;
}

HAPI int suspend_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->suspend)
			modules[i]->suspend(data);
	}

	return QP_OK;
}

HAPI int resume_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->resume)
			modules[i]->resume(data);
	}

	return QP_OK;
}

HAPI int hib_enter_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->hib_enter)
			modules[i]->hib_enter(data);
	}

	return QP_OK;
}

HAPI int hib_leave_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->hib_leave)
			modules[i]->hib_leave(data);
	}

	return QP_OK;
}

/******************************************************************
  *
  * LANGUAGE
  *
  ****************************************************************/

HAPI void lang_change_modules(void *data)
{
	int i;
	retif(data == NULL, , "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->lang_changed)
			modules[i]->lang_changed(data);
	}
}

HAPI void refresh_modules(void *data)
{
	int i;
	retif(data == NULL, , "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->refresh)
			modules[i]->refresh(data);
	}
}

/******************************************************************
  *
  * Quickpanel open/close Events
  *
  ****************************************************************/
HAPI int qp_opened_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->qp_opened)
			modules[i]->qp_opened(data);
	}

	return QP_OK;
}

HAPI int qp_closed_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->qp_closed)
			modules[i]->qp_closed(data);
	}

	return QP_OK;
}
