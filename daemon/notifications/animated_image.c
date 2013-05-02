/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "animated_image.h"

static int quickpanel_animated_image_init(void *data);
static int quickpanel_animated_image_fini(void *data);
static int quickpanel_animated_image_suspend(void *data);
static int quickpanel_animated_image_resume(void *data);

QP_Module animated_image = {
	.name = "animated_image",
	.init = quickpanel_animated_image_init,
	.fini = quickpanel_animated_image_fini,
	.suspend = quickpanel_animated_image_suspend,
	.resume = quickpanel_animated_image_resume,
	.lang_changed = NULL,
	.refresh = NULL
};

static Eina_List *g_animated_image_list = NULL;

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

	retif(g_animated_image_list == NULL, ,"list is empty");

	EINA_LIST_FOREACH_SAFE(g_animated_image_list, l, ln, entry_obj) {
		if (entry_obj == NULL) continue;

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

HAPI void quickpanel_animated_image_add(Evas_Object *image) {
	retif(image == NULL, , "image is NULL");

	if (elm_image_animated_available_get(image) == EINA_TRUE) {
		elm_image_animated_set(image, EINA_TRUE);
		if (quickpanel_is_suspended() == 0) {
			elm_image_animated_play_set(image, EINA_TRUE);
		} else {
			elm_image_animated_play_set(image, EINA_FALSE);
		}
		_animated_image_list_add(image);
		evas_object_event_callback_add(image, EVAS_CALLBACK_DEL, _animated_image_deleted_cb, NULL);
	}
}

/*****************************************************************************
 *
 * Util functions
 *
 *****************************************************************************/
static int quickpanel_animated_image_init(void *data)
{
	return QP_OK;
}

static int quickpanel_animated_image_fini(void *data)
{
	return QP_OK;
}

static int quickpanel_animated_image_suspend(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	INFO("animated image going to be suspened");
	_animated_image_play(EINA_FALSE);

	return QP_OK;
}

static int quickpanel_animated_image_resume(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	INFO("animated image going to be resumed");
	_animated_image_play(EINA_TRUE);

	return QP_OK;
}
