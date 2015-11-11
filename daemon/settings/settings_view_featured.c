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

#include <notification.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "common.h"
#include "common_uic.h"
#include "quickpanel-ui.h"
#include "quickpanel_def.h"
#include "modules.h"
#include "preference.h"
#include "settings.h"
#include "setting_utils.h"
#include "settings_gridbox.h"
#include "setting_module_api.h"
#include "settings_view_featured.h"
#include "pager.h"
#include "pager_common.h"
#include "accessibility.h"

#ifdef QP_EMERGENCY_MODE_ENABLE
#include "emergency_mode.h"
#endif

static int _init(void *data);
static void _init_job_cb(void *data);
static int _fini(void *data);
static int _resume(void *data);
static void _opened(void *data);
static void _closed(void *data);
static void _refresh(void *data);
static void _lang_changed(void *data);

QP_Module settings_view_featured = {
	.name = "settings_view_featured",
	.init = _init,
	.init_job_cb = _init_job_cb,
	.fini = _fini,
	.resume = _resume,
	.qp_opened = _opened,
	.qp_closed = _closed,
	.refresh = _refresh,
	.lang_changed = _lang_changed,
};

static void _view_layout_create(void *data)
{
	Evas_Object *box = NULL;
	Evas_Object *container = NULL;
	struct appdata *ad = data;
	retif(!ad->ly, , "layout is NULL!");

	container = quickpanel_uic_load_edj(ad->ly, DEFAULT_EDJ, "quickpanel/setting_container_wvga", 0);

	retif(container == NULL, , "failed to load container");

	box = elm_box_add(container);
	if (!box) {
		if (container != NULL) {
			evas_object_del(container);
			container = NULL;
		}
		return;
	}

	elm_object_style_set(box, "effect");

	evas_object_size_hint_weight_set(box, 0.0 , EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(box, EINA_TRUE);
	evas_object_show(box);

	elm_object_part_content_set(container, QP_SETTING_SCROLLER_PART_WVGA, box);

	quickpanel_setting_layout_set(ad->ly, container);
}

static void _view_icons_add(void *data)
{
	int index = 0, total = 0;
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *icon = NULL;
	QP_Module_Setting *module = NULL;
	Eina_List *list_featured_icon = NULL;
	struct appdata *ad = (struct appdata *)data;
	Evas_Object *box = quickpanel_setting_box_get(ad->ly);
	retif(box == NULL, , "Invalid parameter!");

	quickpanel_settings_featured_list_get(&list_featured_icon);
	retif(list_featured_icon == NULL, , "Invalid parameter!");

	total = eina_list_count(list_featured_icon);
	EINA_LIST_FOREACH_SAFE(list_featured_icon, l, l_next, module) {
		icon = quickpanel_setting_module_icon_create(module, box);
		quickpanel_setting_module_icon_add(module, icon, QP_SETTING_ICON_CONTAINER_FEATURED);
		quickpanel_setting_icon_pack(box, icon, (index == total - 1) ? 0 : 1);
		quickpanel_setting_module_icon_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
		index++;
	}

	eina_list_free(list_featured_icon);
}

static void _view_icons_del(void *data)
{
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *icon = NULL;
	QP_Module_Setting *module = NULL;
	Eina_List *list_featured_icon = NULL;
	struct appdata *ad = (struct appdata *)data;
	retif(ad == NULL, , "Invalid parameter!");
	Evas_Object *box = quickpanel_setting_box_get(ad->ly);
	retif(box == NULL, , "Invalid parameter!");
	list_featured_icon = elm_box_children_get(box);

	EINA_LIST_FOREACH_SAFE(list_featured_icon, l, l_next, icon) {
		module = quickpanel_setting_module_get_from_icon(icon);
		quickpanel_setting_module_icon_remove(module, icon);
		elm_box_unpack(box, icon);
		if (icon != NULL) {
			evas_object_del(icon);
			icon = NULL;
		}
	}

	eina_list_free(list_featured_icon);
}

/*****************************************************************************
 *
 * Util functions
 *
 *****************************************************************************/
static int _init(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

#ifdef QP_EMERGENCY_MODE_ENABLE
	if (quickpanel_emergency_mode_is_on()) {
		elm_object_signal_emit(ad->ly, "quickpanel.setting.hide", "quickpanel.prog");
		return QP_OK;
	}
#endif

	_view_layout_create(data);
	_view_icons_add(data);

	elm_object_signal_emit(ad->ly, "quickpanel.setting.show", "quickpanel.prog");

	return QP_OK;
}

static void _init_job_cb(void *data)
{
	struct appdata *ad = data;
	Evas_Object *scroller = NULL;
	retif(ad == NULL, , "Invalid parameter!");

	quickpanel_setting_set_scroll_page_width(ad);
	quickpanel_setting_stop(ad->ly, 0);
	scroller = quickpanel_setting_scroller_get(ad->ly);
	evas_object_show(scroller);
}

static int _fini(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	_view_icons_del(ad);
	quickpanel_setting_layout_remove(ad->ly);
	elm_object_signal_emit(ad->ly, "quickpanel.setting.hide", "quickpanel.prog");

	return QP_OK;
}

static int _resume(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	return QP_OK;
}

static void _opened(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	quickpanel_setting_view_featured_initial_focus_set();
}

static void _closed(void *data)
{
	struct appdata *ad = data;
	Evas_Object *focused_obj = NULL;
	retif(ad == NULL, , "Invalid parameter!");

	quickpanel_setting_stop(ad->ly, 0);

	if (ad->win != NULL) {
		focused_obj = elm_object_focused_object_get(ad->win);
		if (focused_obj != NULL) {
			elm_object_focus_set(focused_obj, EINA_FALSE);
		}
	}
}

static void _refresh(void *data) {
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	quickpanel_setting_container_rotation_set(ad->ly, ad->angle);
	quickpanel_setting_icons_rotation_set(ad->ly, ad->angle);
	if (ad->is_opened == 0) {
		quickpanel_setting_stop(ad->ly, 1);
	}
}

static void _lang_changed(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	if (ad->is_opened == 0) {
		quickpanel_setting_stop(ad->ly, 0);
	}
}

HAPI Eina_Bool quickpanel_settings_is_in_left_edge(void)
{
	if (quickpanel_setting_scroll_page_get(quickpanel_get_app_data()) == 0) {
		return EINA_TRUE;
	}

	return EINA_FALSE;
}

HAPI void quickpanel_setting_view_featured_reload(Eina_List *list_all_module, int num_featured)
{
	int index = 0, total = 0;
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *box = NULL;
	Evas_Object *icon = NULL;
	QP_Module_Setting *module = NULL;
	struct appdata *ad = quickpanel_get_app_data();
	retif(list_all_module == NULL, , "Invalid parameter!");

	box = quickpanel_setting_box_get(ad->ly);
	retif(box == NULL, , "invalid parameter");

	quickpanel_setting_icon_unpack_all(box);

	total = eina_list_count(list_all_module);
	DBG("total:%d", total);
	EINA_LIST_FOREACH_SAFE(list_all_module, l, l_next, module) {
		if (index < num_featured) {
			if ((icon = quickpanel_setting_module_icon_get(module,
							QP_SETTING_ICON_CONTAINER_FEATURED)) == NULL) {
				icon = quickpanel_setting_module_icon_create(module, box);
			}
			if (icon != NULL) {
				quickpanel_setting_module_icon_add(module, icon, QP_SETTING_ICON_CONTAINER_FEATURED);
				quickpanel_setting_icon_pack(box, icon, (index == num_featured - 1) ? 0 : 1);
				quickpanel_setting_module_icon_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
			}
			DBG("all list:%s", module->name);
		} else {
			if ((icon = quickpanel_setting_module_icon_get(module,
							QP_SETTING_ICON_CONTAINER_FEATURED)) != NULL) {
				quickpanel_setting_module_icon_remove(module, icon);
				evas_object_del(icon);
				icon = NULL;
			}
		}

		index++;
	}
}

HAPI void quickpanel_setting_view_featured_initial_focus_set(void)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "Invalid parameter!");

	Evas_Object *obj_first = NULL;
	Eina_List *list_featured_icon = NULL;
	Evas_Object *box = quickpanel_setting_box_get(ad->ly);
	retif(box == NULL, , "Invalid parameter!");

#ifdef QP_EMERGENCY_MODE_ENABLE
	if (quickpanel_emergency_mode_is_on()) {
		return;
	}
#endif

	if (quickpanel_uic_opened_reason_get() == OPENED_BY_CMD_SHOW_SETTINGS) {
		return;
	}

	list_featured_icon = elm_box_children_get(box);
	if (list_featured_icon != NULL) {
		obj_first = eina_list_nth(list_featured_icon, 0);
		if (obj_first != NULL) {
			elm_object_focus_set(obj_first, EINA_FALSE);
			elm_object_focus_set(obj_first, EINA_TRUE);
		}
		eina_list_free(list_featured_icon);
	}
}

HAPI void quickpanel_setting_view_featured_brightness_init(Evas_Object *brightness_view)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "Invalid parameter!");

	Evas_Object *container = NULL;

	container = quickpanel_setting_layout_get(ad->ly, QP_SETTING_BASE_PART);

	retif(container == NULL, ,"Failed to get container");

	elm_object_part_content_set(container, QP_SETTING_BRIGHTNESS_PART_WVGA, brightness_view);
}
