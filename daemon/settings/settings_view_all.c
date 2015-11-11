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
#include "settings_view_all.h"
#include "pager.h"
#include "pager_common.h"
#include "page_setting_all.h"
#include "accessibility.h"

#ifdef QP_EMERGENCY_MODE_ENABLE
#include "emergency_mode.h"
#endif

static int _init(void *data);
static int _fini(void *data);
static int _resume(void *data);
static void _opened(void *data);
static void _closed(void *data);
static void _refresh(void *data);
static void _view_icons_add(void);
static void _view_icons_del(void);

QP_Module settings_view_all = {
	.name = "settings_view_all",
	.init = _init,
	.fini = _fini,
	.resume = _resume,
	.qp_opened = _opened,
	.qp_closed = _closed,
	.refresh = _refresh,
	.lang_changed = NULL,
};

static int _icons_rotation_set(int angle)
{
	Evas_Object *icon = NULL;
	Eina_List *icons = NULL;
	Evas_Object *section = NULL;
	Eina_List *l = NULL;
	const char *signal = NULL;

	retif(angle < 0, -1, "angle is %d", angle);

	section = quickpanel_page_setting_all_view_get("ACTIVE.BUTTONS");
	retif(!section, -1, "box is NULL");

	icons = elm_box_children_get(section);
	retif(!icons, -1, "icons list is NULL");

	if (angle % 180 == 0) {
		signal = "icon.portrait";
	} else {
		signal = "icon.landscape";
	}

	EINA_LIST_FOREACH(icons, l, icon) {
		elm_object_signal_emit(icon, signal, "quickpanl.prog");
		edje_object_message_signal_process(_EDJ(icon));
	}

	eina_list_free(icons);

	return 0;
}

static Evas_Object *_icon_create(QP_Module_Setting *module, Evas_Object *parent) {
	retif(module == NULL, NULL, "Invalid parameter!");
	retif(parent == NULL, NULL, "Invalid parameter!");

	return quickpanel_setting_module_icon_create(module, parent);
}

static Evas_Object *_divider_create(Evas_Object *parent) {
	Evas_Object *divider = NULL;
	retif(parent == NULL, NULL, "invalid parameter");

	struct appdata *ad = (struct appdata*)quickpanel_get_app_data();
	retif(ad == NULL, NULL, "application data is NULL");

	divider = quickpanel_uic_load_edj(parent, DEFAULT_EDJ, "quickpanel/setting_icon_divider", 0);

	retif(divider == NULL, NULL, "failed to load divider");

	return divider;
}

static void _icon_pack(Evas_Object *gridbox, Evas_Object *icon, int need_divider)
{
	Evas_Object *divider = NULL;
	retif(gridbox == NULL, , "Invalid parameter!");
	retif(icon == NULL, , "Invalid parameter!");

	quickpanel_settings_gridbox_item_add(gridbox, icon, SETTINGS_GRIDBOX_ITEM_ICON, SETTINGS_GRIDBOX_APPEND);

	if (need_divider == 1) {
		divider = _divider_create(gridbox);
		if (divider != NULL) {
			quickpanel_settings_gridbox_item_add(gridbox, divider,
					SETTINGS_GRIDBOX_ITEM_DIVIDER, SETTINGS_GRIDBOX_APPEND);
		}
	}
}

static void _view_icons_add(void)
{
	int index = 0, total = 0;
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *gridbox = NULL;
	Evas_Object *icon = NULL;
	QP_Module_Setting *module = NULL;
	Eina_List *list_all_icon = NULL;
	Evas_Object *layout = quickpanel_page_setting_all_view_get("LAYOUT");
	retif(layout == NULL, , "Invalid parameter!");

	quickpanel_settings_all_list_get(&list_all_icon);
	retif(list_all_icon == NULL, , "Invalid parameter!");

	gridbox = quickpanel_page_setting_all_view_get("ACTIVE.BUTTONS");
	if (gridbox == NULL) {
		gridbox = quickpanel_settings_gridbox_create(layout, quickpanel_get_app_data());
		quickpanel_page_setting_all_view_set("ACTIVE.BUTTONS", gridbox);
	}

	total = eina_list_count(list_all_icon);
	DBG("total:%d", total);
	EINA_LIST_FOREACH_SAFE(list_all_icon, l, l_next, module) {
		icon = _icon_create(module, gridbox);
		if (icon != NULL) {
			_icon_pack(gridbox, icon, (index != total - 1) ? 1 : 0);
			quickpanel_setting_module_icon_add(module, icon, QP_SETTING_ICON_CONTAINER_ALL_LIST);
			quickpanel_setting_module_icon_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
		}
		DBG("all list:%s", module->name);
		index++;
	}

	eina_list_free(list_all_icon);
}

static void _view_icons_del(void)
{
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *icon = NULL;
	QP_Module_Setting *module = NULL;
	Eina_List *list_icons = NULL;
	Evas_Object *box = quickpanel_page_setting_all_view_get("ACTIVE.BUTTONS");
	retif(box == NULL, , "Invalid parameter!");

	list_icons = elm_box_children_get(box);

	EINA_LIST_FOREACH_SAFE(list_icons, l, l_next, icon) {
		module = quickpanel_setting_module_get_from_icon(icon);
		quickpanel_setting_module_icon_remove(module, icon);
		elm_box_unpack(box, icon);
		if (icon != NULL) {
			evas_object_del(icon);
			icon = NULL;
		}
	}

	eina_list_free(list_icons);
}

static int _view_create(void *data)
{
	_view_icons_add();

	return QP_OK;
}

static int _view_destroy(void *data)
{
	_view_icons_del();

	return QP_OK;
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

	_view_create(data);

	return QP_OK;
}

static int _fini(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	_view_destroy(ad);

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
}

static void _closed(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");
}

static void _refresh(void *data) {
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	_icons_rotation_set(ad->angle);
}

HAPI void quickpanel_setting_view_all_reload(Eina_List *list_all_module)
{
	int index = 0, total = 0;
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *gridbox = NULL;
	Evas_Object *icon = NULL;
	QP_Module_Setting *module = NULL;
	retif(list_all_module == NULL, , "Invalid parameter!");

	gridbox = quickpanel_page_setting_all_view_get("ACTIVE.BUTTONS");
	retif(gridbox == NULL, , "Invalid parameter!");

	quickpanel_settings_gridbox_unpack_all(gridbox);

	total = eina_list_count(list_all_module);
	DBG("total:%d", total);
	EINA_LIST_FOREACH_SAFE(list_all_module, l, l_next, module) {
		if ((icon = quickpanel_setting_module_icon_get(module,
						QP_SETTING_ICON_CONTAINER_ALL_LIST)) == NULL) {
			icon = _icon_create(module, gridbox);
		}
		if (icon != NULL) {
			_icon_pack(gridbox, icon, (index != total - 1) ? 1 : 0);
			quickpanel_setting_module_icon_add(module, icon, QP_SETTING_ICON_CONTAINER_ALL_LIST);
			quickpanel_setting_module_icon_status_update(module, FLAG_VALUE_VOID, FLAG_VALUE_VOID);
		}
		DBG("all list:%s", module->name);
		index++;
	}
}
