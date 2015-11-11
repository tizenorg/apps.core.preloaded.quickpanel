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
#include <glib.h>

#include <string.h>
#include <vconf.h>
#include <notification.h>
#include <system_settings.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "list_util.h"
#include "quickpanel_def.h"
#include "vi_manager.h"
#include "noti_node.h"
#include "noti_list_item.h"
#include "noti.h"
#include "noti_util.h"
#include "animated_icon.h"
#include "noti_list_item.h"

#ifdef QP_SCREENREADER_ENABLE
#include "accessibility.h"
#endif

#ifdef QP_ANIMATED_IMAGE_ENABLE
#include "animated_image.h"
#endif

#define NOTI_LAYOUT_LISTTYPE 0
#define NOTI_LAYOUT_BOXTYPE 1

#define E_DATA_VIEW_HANDLER_KEY "view_handler_cache"

extern Noti_View_H noti_view_listtype_h;
extern Noti_View_H noti_view_boxtype_h;

static struct _info {
	Noti_View_H *view_handlers[NOTI_LAYOUT_BOXTYPE + 1];
} s_info = {
	.view_handlers = {
		&noti_view_listtype_h,
		&noti_view_boxtype_h,
	},
};

#ifdef BOX_TYPE_SUPPORTED
static int _is_image_exist(notification_h noti, notification_image_type_e image_type)
{
	char *image = NULL;

	notification_get_image(noti, image_type, &image);

	if (image == NULL) {
		return 0;
	}

	if (strncasecmp(image, "(null)", strlen(image)) == 0) {
		return 0;
	}

	return 1;
}

static int _is_text_exist(notification_h noti, notification_text_type_e text_type)
{
	char *text = NULL;

	notification_get_text(noti, text_type, &text);

	if (text == NULL) {
		return 0;
	}

	return 1;
}
#endif

static void _view_handler_set(Evas_Object *item, Noti_View_H *handler)
{
	retif(item == NULL, , "Invalid parameter!");
	retif(handler == NULL, , "Invalid parameter!");

	evas_object_data_set(item, E_DATA_VIEW_HANDLER_KEY, handler);
}

static Noti_View_H *_view_handler_cached_get(Evas_Object *item)
{
	retif(item == NULL, NULL, "Invalid parameter!");

	return (Noti_View_H *)evas_object_data_get(item, E_DATA_VIEW_HANDLER_KEY);
}

static Noti_View_H *_view_handler_get_by_contents(notification_h noti)
{
#ifdef BOX_TYPE_SUPPORTED	// Box type is not supported in Kiran
	int ret = 0;

	ret += _is_text_exist(noti, NOTIFICATION_TEXT_TYPE_INFO_1);
	ret += _is_text_exist(noti, NOTIFICATION_TEXT_TYPE_INFO_2);
	ret += _is_image_exist(noti, NOTIFICATION_IMAGE_TYPE_BACKGROUND);

	if (ret > 0) {
		return s_info.view_handlers[NOTI_LAYOUT_BOXTYPE];
	}
#endif
	return s_info.view_handlers[NOTI_LAYOUT_LISTTYPE];
}

static void _view_handler_del(Evas_Object *item)
{
	retif(item == NULL, , "Invalid parameter!");

	evas_object_data_del(item, E_DATA_VIEW_HANDLER_KEY);
}

static Evas_Object *_create(notification_h noti, Evas_Object *parent)
{
	Evas_Object *view = NULL;
	Noti_View_H *view_handler = NULL;
	retif(parent == NULL, NULL, "Invalid parameter!");
	retif(noti == NULL, NULL, "Invalid parameter!");

	view_handler = _view_handler_get_by_contents(noti);
	if (view_handler != NULL && view_handler->create != NULL) {
		view = view_handler->create(noti, parent);
		if (view == NULL) {
			ERR("failed to create notification view(%s)", view_handler->name);
		} else {
			_view_handler_set(view, view_handler);
		}
	} else {
		ERR("create handler isn't supported");
	}

	return view;
}

static void _update(noti_node_item *noti_node, notification_ly_type_e layout, Evas_Object *item)
{
	Noti_View_H *view_handler = NULL;
	retif(item == NULL, , "Invalid parameter!");
	retif(noti_node == NULL, , "Invalid parameter!");
	retif(noti_node->noti == NULL, , "Invalid parameter!");

	view_handler = _view_handler_get_by_contents(noti_node->noti);
	if (view_handler != NULL && view_handler->update != NULL) {
		_view_handler_set(item, view_handler);
		view_handler->update(noti_node, layout, item);
	} else {
		ERR("update handler isn't supported");
	}
}

static void _remove(noti_node_item *noti_node, notification_ly_type_e layout, Evas_Object *item)
{
	Noti_View_H *view_handler = NULL;
	retif(item == NULL, , "Invalid parameter!");
	retif(noti_node == NULL, , "Invalid parameter!");
	retif(noti_node->noti == NULL, , "Invalid parameter!");

	view_handler = _view_handler_cached_get(item);
	if (view_handler != NULL && view_handler->remove != NULL) {
		_view_handler_del(item);
		view_handler->remove(noti_node, layout, item);
	} else {
		ERR("remove handler isn't supported");
	}
}

HAPI int quickpanel_noti_view_is_view_handler_changed(Evas_Object *item, notification_h noti)
{
	Noti_View_H *view_handler_old = NULL;
	Noti_View_H *view_handler_new = NULL;

	view_handler_old = _view_handler_cached_get(item);
	view_handler_new = _view_handler_get_by_contents(noti);

	if (view_handler_old != view_handler_new) {
		return 1;
	}

	return 0;
}

Noti_View_H noti_view_h = {
	.name 			= "noti_view",
	.create			= _create,
	.update			= _update,
	.remove			= _remove,
};
