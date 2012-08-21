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
/* notification */
extern QP_Module noti;
extern QP_Module ticker;
/* idle test */
extern QP_Module idletxt;

static QP_Module *modules[] = {
#ifdef QP_MINICTRL_ENABLE
	&minictrl,
#endif /* QP_MINICTRL_ENABLE */
	&noti,
	&ticker,
	&idletxt
};

int init_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->init)
			modules[i]->init(data);
	}

	return QP_OK;
}

int fini_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->fini)
			modules[i]->fini(data);
	}

	return QP_OK;
}

int suspend_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->suspend)
			modules[i]->suspend(data);
	}

	return QP_OK;
}

int resume_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->resume)
			modules[i]->resume(data);
	}

	return QP_OK;
}

int hib_enter_modules(void *data)
{
	int i;

	retif(data == NULL, QP_FAIL, "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->hib_enter)
			modules[i]->hib_enter(data);
	}

	return QP_OK;
}

int hib_leave_modules(void *data)
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

void lang_change_modules(void *data)
{
	int i;
	retif(data == NULL, , "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->lang_changed)
			modules[i]->lang_changed(data);
	}
}

void refresh_modules(void *data)
{
	int i;
	retif(data == NULL, , "Invalid parameter!");

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
		if (modules[i]->refresh)
			modules[i]->refresh(data);
	}
}
