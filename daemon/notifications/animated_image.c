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
#include "common_uic.h"
#include "animated_image.h"
#include "quickpanel-ui.h"

static int _init(void *data);
static int _fini(void *data);
static int _suspend(void *data);
static int _resume(void *data);

QP_Module animated_image = {
	.name = "animated_image",
	.init = _init,
	.fini = _fini,
	.suspend = _suspend,
	.resume = _resume,
	.lang_changed = NULL,
	.refresh = NULL
};

static Eina_List *g_animated_image_list = NULL;
static char g_animated_image_group_name[32] = {0,};

static void _animated_image_list_add(Evas_Object *image)
{
	retif(image == NULL, ,"invalid parameter");

	g_animated_image_list = eina_list_append(g_animated_image_list, image);
}

static void _animated_image_play(Eina_Bool on)
{
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;
	Evas_Object *entry_obj = NULL;

	retif_nomsg(g_animated_image_list == NULL, );

	EINA_LIST_FOREACH_SAFE(g_animated_image_list, l, ln, entry_obj) {
		if (entry_obj == NULL) {
			continue;
		}

		if (on == EINA_TRUE) {
			if (elm_image_animated_play_get(entry_obj) == EINA_FALSE) {
				elm_image_animated_play_set(entry_obj, EINA_TRUE);
			}
		} else {
			if (elm_image_animated_play_get(entry_obj) == EINA_TRUE) {
				elm_image_animated_play_set(entry_obj, EINA_FALSE);
			}
		}
	}
}

static void _animated_image_deleted_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	retif(obj == NULL, , "obj is NULL");
	retif(g_animated_image_list == NULL, , "list is empty");

	g_animated_image_list = eina_list_remove(g_animated_image_list, obj);
}

HAPI void quickpanel_animated_image_add(Evas_Object *image)
{
	retif(image == NULL, , "image is NULL");

	if (elm_image_animated_available_get(image) == EINA_TRUE) {
		elm_image_animated_set(image, EINA_TRUE);
		if (quickpanel_uic_is_suspended() == 0) {
			elm_image_animated_play_set(image, EINA_TRUE);
		} else {
			elm_image_animated_play_set(image, EINA_FALSE);
		}
		_animated_image_list_add(image);
		evas_object_event_callback_add(image, EVAS_CALLBACK_DEL, _animated_image_deleted_cb, NULL);
	}
}

HAPI char *quickpanel_animated_image_get_groupname(const char *path)
{
	static int s_image_index = 0;

	if (path != NULL) {
		if (strstr(path, "gif") != NULL || strstr(path, "GIF") != NULL) {
			snprintf(g_animated_image_group_name, sizeof(g_animated_image_group_name),
					"%d:EVAS", s_image_index++);

			return g_animated_image_group_name;
		}
	}

	return NULL;
}

/*****************************************************************************
 *
 * Util functions
 *
 *****************************************************************************/
static int _init(void *data)
{
	return QP_OK;
}

static int _fini(void *data)
{
	return QP_OK;
}

static int _suspend(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	SDBG("animated image going to be suspened");
	_animated_image_play(EINA_FALSE);

	return QP_OK;
}

static int _resume(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	SDBG("animated image going to be resumed");
	_animated_image_play(EINA_TRUE);

	return QP_OK;
}
