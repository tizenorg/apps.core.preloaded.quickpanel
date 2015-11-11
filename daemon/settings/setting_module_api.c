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
#include "quickpanel_def.h"
#include "quickpanel-ui.h"
#include "settings.h"
#include "setting_utils.h"
#include "setting_module_api.h"
#include "settings_icon_common.h"

#ifdef QP_SCREENREADER_ENABLE
#include "accessibility.h"
#endif

#define E_DATA_CONTAINER_TYPE "container_type"

static qp_setting_icon_container_type _icon_container_type_get(Evas_Object *view)
{
	retif(view == NULL, QP_SETTING_ICON_CONTAINER_NONE, "invalid parameter");

	return (qp_setting_icon_container_type)evas_object_data_get(view, E_DATA_CONTAINER_TYPE);
}

static void _icon_view_add(QP_Module_Setting *module, Evas_Object *view ,qp_setting_icon_container_type container_type)
{
	retif(module == NULL, , "invalid parameter");
	retif(view == NULL, , "invalid parameter");

	if (eina_list_data_find(module->view_list, view) == NULL) {
		evas_object_data_set(view, E_DATA_CONTAINER_TYPE, (void *)container_type);
		module->view_list = eina_list_append(module->view_list, view);
	}
}

static void _icon_view_del(QP_Module_Setting *module, Evas_Object *view)
{
	retif(module == NULL, , "invalid parameter");
	retif(view == NULL, , "invalid parameter");

	module->view_list = eina_list_remove(module->view_list, view);
}

HAPI Evas_Object *quickpanel_setting_module_icon_create(QP_Module_Setting *module, Evas_Object *parent)
{
	Evas_Object *view = NULL;
	retif(module == NULL, NULL, "invalid parameter");
	retif(parent == NULL, NULL, "invalid parameter");

	view = quickpanel_setting_icon_new(parent);
	retif(view == NULL, NULL, "failed to create icon");

	if (module->label_get != NULL) {
		quickpanel_setting_icon_text_set(view, module->label_get(), ICON_VIEW_STATE_OFF);
	}
	if (module->handler_press != NULL) {
		if (module->is_disable_feedback) {
			quickpanel_setting_icon_click_cb_without_feedback_add(view, module->handler_press, module);
		} else {
			quickpanel_setting_icon_click_cb_add(view, module->handler_press, module);
		}
	}
	evas_object_data_set(view, E_DATA_MODULE_INFO, module);
	evas_object_show(view);

	return view;
}

HAPI QP_Module_Setting *quickpanel_setting_module_get_from_icon(Evas_Object *icon)
{
	return evas_object_data_get(icon, E_DATA_MODULE_INFO);
}

HAPI void quickpanel_setting_module_icon_add(QP_Module_Setting *module, Evas_Object *icon, qp_setting_icon_container_type container_type)
{
	_icon_view_add(module, icon, container_type);
}

HAPI Evas_Object *quickpanel_setting_module_icon_get(QP_Module_Setting *module, qp_setting_icon_container_type container_type)
{
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *view = NULL;
	retif(module == NULL, NULL, "invalid parameter");

	if (module->view_update != NULL) {
		EINA_LIST_FOREACH_SAFE(module->view_list, l, l_next, view) {
			if (_icon_container_type_get(view) == container_type) {
				return view;
			}
		}
	}

	return NULL;
}

HAPI void quickpanel_setting_module_icon_remove(QP_Module_Setting *module, Evas_Object *icon)
{
	_icon_view_del(module, icon);
}

HAPI void quickpanel_setting_module_icon_state_set(QP_Module_Setting *module, int state)
{
	retif(module == NULL, , "invalid parameter");
	retif(module->loader == NULL, , "invalid parameter");

	module->loader->state = state;
}

HAPI int quickpanel_setting_module_icon_state_get(QP_Module_Setting *module)
{
	retif(module == NULL, FLAG_TURN_OFF, "invalid parameter");
	retif(module->loader == NULL, FLAG_TURN_OFF, "invalid parameter");

	return module->loader->state;
}

HAPI void quickpanel_setting_module_icon_view_update(QP_Module_Setting *module, int flag_extra_1, int flag_extra_2)
{
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *view = NULL;
	retif(module == NULL, , "invalid parameter");

	int status = quickpanel_setting_module_icon_state_get(module);

	if (module->view_update != NULL) {
		EINA_LIST_FOREACH_SAFE(module->view_list, l, l_next, view) {
			module->view_update(view, status, flag_extra_1, flag_extra_2);
		}
	}
}

HAPI void quickpanel_setting_module_icon_view_update_text(QP_Module_Setting *module)
{
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *view = NULL;
	retif(module == NULL, , "invalid parameter");

	if (module->view_update != NULL && module->label_get != NULL) {
		EINA_LIST_FOREACH_SAFE(module->view_list, l, l_next, view) {
			quickpanel_setting_icon_text_set(view, module->label_get(), quickpanel_setting_module_icon_state_get(module));
		}
	}
}

HAPI void quickpanel_setting_module_icon_status_update(QP_Module_Setting *module, int flag_extra_1, int flag_extra_2)
{
	retif(module == NULL, , "invalid parameter");

	if (module->status_update != NULL) {
		module->status_update(module, flag_extra_1, flag_extra_2);
	}
}

HAPI int quickpanel_setting_module_is_icon_clickable(QP_Module_Setting *module)
{
	retif(module == NULL, 0, "invalid parameter");
	retif(module->loader == NULL, 0, "invalid parameter");

	if (module->loader->timer != NULL) {
		return 0;
	}
	if (module->loader->state_icon == STATE_ICON_BUSY) {
		return 0;
	}

	return 1;
}

static Eina_Bool _timer_expire_cb(void *data)
{
	retif(data == NULL, ECORE_CALLBACK_CANCEL, "invalid parameter");

	quickpanel_setting_module_icon_timer_del(data);
	quickpanel_setting_module_icon_status_update(data, FLAG_VALUE_VOID, FLAG_VALUE_VOID);

	return ECORE_CALLBACK_CANCEL;
}

HAPI void quickpanel_setting_module_icon_timer_add(QP_Module_Setting *module)
{
	retif(module == NULL, , "invalid parameter");
	retif(module->loader == NULL, , "invalid parameter");

	quickpanel_setting_module_icon_timer_del(module);
	module->loader->timer = ecore_timer_add(TIMER_COUNT, _timer_expire_cb, module);
}

HAPI void quickpanel_setting_module_icon_timer_del(QP_Module_Setting *module)
{
	retif(module == NULL, , "invalid parameter");
	retif(module->loader == NULL, , "invalid parameter");

	if (module->loader->timer != NULL) {
		ecore_timer_del(module->loader->timer);
		module->loader->timer = NULL;
	}
}

#ifdef __PROGRESSBAR_ENABLED__
static Evas_Object *_progressbar_get(Evas_Object *parent)
{
	Evas_Object *content = NULL;

	content = elm_progressbar_add(parent);
	elm_progressbar_unit_format_set(content, "%0.0f%%");
	retif(!content, NULL, "fail to elm_progressbar_add");

	elm_object_style_set(content, "quickpanel_style");
	evas_object_size_hint_weight_set(content,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_progressbar_pulse(content, EINA_TRUE);
	evas_object_show(content);

	return content;
}

static void _progressbar_set(Evas_Object *view, int is_enable, int is_request_on)
{
	Evas_Object *content = NULL;
	Evas_Object *content_old = NULL;
	retif(view == NULL, , "invalid parameter");

	if (is_enable == FLAG_ENABLE) {
		content_old = quickpanel_setting_icon_content_get(view);
		if (content_old != NULL) {
			evas_object_del(content_old);
			content_old = NULL;
		}
		content = _progressbar_get(view);
		quickpanel_setting_icon_content_set(view, content);

		quickpanel_setting_icon_state_progress_set(view);
		quickpanel_setting_icon_state_set(view, ICON_VIEW_STATE_DIM);
	}
}
#endif

HAPI void quickpanel_setting_module_progress_mode_set(QP_Module_Setting *module, int is_enable, int is_request_on)
{
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *view = NULL;
	retif(module == NULL, , "invalid parameter");
	retif(module->loader == NULL, , "invalid parameter");

	EINA_LIST_FOREACH_SAFE(module->view_list, l, l_next, view) {
#ifdef __PROGRESSBAR_ENABLED__
		_progressbar_set(view, is_enable, is_request_on);
#else
		if (is_enable) {
			quickpanel_setting_icon_state_progress_set(view);
		}
#endif
	}

	if (is_enable == FLAG_ENABLE) {
		module->loader->state_icon = STATE_ICON_BUSY;
	} else {
		module->loader->state_icon = STATE_ICON_IDLE;
	}
}

HAPI void quickpanel_setting_module_icon_destroy(QP_Module_Setting *module, Evas_Object *icon)
{
	retif(module == NULL, , "invalid parameter");
	retif(icon == NULL, , "invalid parameter");

	_icon_view_del(module, icon);
	quickpanel_setting_icon_click_cb_del(icon, module->handler_press);
	evas_object_del(icon);
	icon = NULL;
}
