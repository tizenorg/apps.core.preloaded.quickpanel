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

#include <dlog.h>
#include <vconf.h>

#include <tapi_common.h>
#include <ITapiSim.h>
#include <TelCall.h>
#include <ITapiCall.h>

#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "handler_controller.h"
#include "setting_utils.h"
#include "list_util.h"
#include "quickpanel-ui.h"
#include "common.h"

HAPI void quickpanel_handler_text_set(char *text)
{
	struct appdata *ad = quickpanel_get_app_data();
	if (!ad) {
		ERR("Could not get application data");
		return;
	}

	Evas_Object *layout = ad->view_root;
	if (!layout) {
		ERR("Could not get view_root");
		return;
	}

	if (text) {
		elm_object_part_text_set(layout, "qp.handler.text", text);
		elm_object_signal_emit(layout, "show", "qp.handler.text");
	} else {
		elm_object_part_text_set(layout, "qp.handler.text", " ");
		elm_object_signal_emit(layout, "hide", "qp.handler.text");
	}
}

HAPI void quickpanel_handler_set_visibility(Eina_Bool visible)
{
	struct appdata *ad = quickpanel_get_app_data();
	if (!ad) {
		ERR("Could not get application data");
		return;
	}

	Evas_Object *layout = ad->view_root;
	if (!layout) {
		ERR("Could not get view_root");
		return;
	}

	if (visible == EINA_FALSE) {
		elm_object_signal_emit(layout, "qp.handler.text,hide", "qp.handler.text");
	} else {
		elm_object_signal_emit(layout, "qp.handler.text,show", "qp.handler.text");
	}
}





