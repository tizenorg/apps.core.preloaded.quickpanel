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

#include <voice_control_setting.h>
#include <app_control_internal.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "common.h"
#include "common_uic.h"
#include "voice_control.h"
#include "quickpanel-ui.h"
#include "list_util.h"


static int _init(void *data);
static int _fini(void *data);
Evas_Object* _voice_control_view_create(void *data);
static void _voice_control_register_event(void *data);
static bool _check_voice_control_enabled(void);

QP_Module voice_control = {
	.name = "voice_control",
	.init = _init,
	.fini = _fini,
	.suspend = NULL,
	.resume = NULL,
	.hib_enter = NULL,
	.hib_leave = NULL,
	.lang_changed = NULL,
	.refresh = NULL,
	.get_height = NULL,
	.qp_opened = NULL,
	.qp_closed = NULL,
};

static Evas_Object *g_layout = NULL;

static void _voice_control_view_destroy(void *data)
{
	DBG("_voice_control_view_destroy");
	struct appdata *ad = data;

	if (ad == NULL)
	{
		ERR("invalid data");
		return;
	}

	if (g_layout == NULL)
	{
		ERR("g_layout is not exist");
		return;
	}

	quickpanel_list_util_item_unpack_by_object(ad->list, g_layout, 0, 0);
	quickpanel_list_util_item_del_tag(g_layout);
	elm_object_signal_emit(ad->ly, "voice_icon.hide", "quickpanel.prog");

	if (g_layout != NULL) {
		evas_object_del(g_layout);
		g_layout = NULL;
	}
}

static int _init(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	if (0 != vc_setting_initialize()) {
		ERR("Fail to init");
		return QP_FAIL;
	}

	_voice_control_register_event(data);

	if (_check_voice_control_enabled()) { //when module restart
		_voice_control_view_create(ad);
	}

	return QP_OK;
}

static int _fini(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	if (0 != vc_setting_deinitialize()) {
		ERR("Fail to vc_setting_deinitialize");
	}

	_voice_control_view_destroy(ad);

	return QP_OK;
}

static void _vc_enabled_changed_cb(bool enabled, void* user_data)
{
	DBG("_vc_enabled_changed_cb");
	struct appdata *ad = user_data;

	if( !enabled ) { //deactivated voice controller
		_voice_control_view_destroy(ad);
	} else {
		_voice_control_view_create(ad);
	}
}

static void _voice_control_register_event(void *data)
{
	DBG("_voice_control_register_event");
	if (0 != vc_setting_set_enabled_changed_cb(_vc_enabled_changed_cb, data)) {
		ERR("Fail to set enabled cb");
		return;
	}
}

static bool _check_voice_control_enabled(void)
{
	bool enabled = false;

	if (0 != vc_setting_get_enabled(&enabled)) {
		ERR("Fail to get enabled");
	}
	DBG("_check_voice_control_enabled enabled %d",enabled);

	return enabled;
}

static void _button_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	DBG("_button_clicked_cb");
	int ret = 0;
	quickpanel_uic_launch_ug_by_appcontrol(VOICE_CONTOL_REF_APP, NULL);
	quickpanel_uic_launch_app_inform_result(VOICE_CONTOL_REF_APP, ret);
	quickpanel_uic_close_quickpanel(true, 1);

}

Evas_Object* _voice_control_view_create(void *data)
{
	DBG("_voice_control_view_create");
	struct appdata *ad = data;
	Evas_Object *layout = NULL;
	Eina_Bool ret = EINA_FALSE;

	if (!_check_voice_control_enabled()) {
		ERR("voice control is not enabled.");
		return layout;
	}
	if (ad->win == NULL)
	{
		ERR("invalid parent");
		return layout;
	}

	if (g_layout)
	{
		ERR("voice control view is already created.");
		return g_layout;
	}

	layout = elm_layout_add(ad->win);
	if (layout == NULL)
	{
		ERR("Failed to create voice control layout");
		return layout;
	}

	g_layout = layout;

	ret = elm_layout_file_set(layout, VOICE_CONTOL_EDJ, "quickpanel/voice_control/default");
	if (ret == EINA_FALSE)
	{
		ERR("Failed to set layout file[%d]", ret);
		evas_object_del(layout);
		return NULL;
	}

	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	elm_object_signal_callback_add(layout, "button_clicked" , "", _button_clicked_cb, ad);
	elm_object_signal_emit(ad->ly, "voice_icon.show", "quickpanel.prog");

	// attach to list
	qp_item_data *qid = quickpanel_list_util_item_new(QP_ITEM_TYPE_VOICE_CONTOL, layout);
	quickpanel_list_util_item_set_tag(layout, qid);
	quickpanel_list_util_sort_insert(ad->list, layout);
	quickpanel_uic_initial_resize(layout, QP_VOICE_CONTOL_HEIGHT);

	return layout;
}

