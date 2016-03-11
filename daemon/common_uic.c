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
#include <sys/utsname.h>
#include <Ecore_Input.h>

#include <app.h>
#include <vconf.h>
#include <notification.h>
#include <app_control_internal.h>
#include <bundle_internal.h>
#include <system_info.h>
#include <common_uic.h>

#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "common.h"
#include "quickpanel-ui.h"


/* binary information */
#define QP_EMUL_STR		"Emulator"
#define DEL_TIMER_VALUE	0.480
#define SYSTEM_INFO_KEY_MODEL "http://tizen.org/system/model_name"
static Ecore_Timer *_close_timer = NULL;


static void _quickpanel_move_data_to_service(const char *key, const char *val, void *data)
{
	retif(data == NULL || key == NULL || val == NULL, , "Invialid parameter!");

	app_control_h service = data;
	app_control_add_extra_data(service, key, val);
}

HAPI Evas_Object *quickpanel_uic_load_edj(Evas_Object * parent, const char *file, const char *group, int is_just_load)
{
	Eina_Bool r;
	Evas_Object *eo = NULL;

	retif(parent == NULL, NULL, "Invalid parameter!");

	eo = elm_layout_add(parent);
	retif(eo == NULL, NULL, "Failed to add layout object!");

	r = elm_layout_file_set(eo, file, group);
	retif(r != EINA_TRUE, NULL, "Failed to set edje object file[%s-%s]!", file, group);

	evas_object_size_hint_weight_set(eo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(eo, EVAS_HINT_FILL, EVAS_HINT_FILL);

	if (is_just_load == 1) {
		elm_win_resize_object_add(parent, eo);
	}
	evas_object_show(eo);

	return eo;
}

HAPI int quickpanel_uic_is_opened(void)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, 0, "invalid data.");

	return ad->is_opened;
}

HAPI int quickpanel_uic_is_suspended(void)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, 0, "invalid data.");

	return ad->is_suspended;
}

HAPI int quickpanel_uic_is_emul(void)
{
	int is_emul = 0;
	char *info = NULL;

	if (system_info_get_platform_string(SYSTEM_INFO_KEY_MODEL, &info) == 0) {
		if (info == NULL) {
			return 0;
		}
		if (!strncmp(QP_EMUL_STR, info, strlen(info))) {
			is_emul = 1;
		}
	}

	free(info);

	return is_emul;
}

HAPI void quickpanel_uic_initial_resize(Evas_Object *obj, int height)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid data.");

	height = (height % 2 != 0) ? height + 1 : height;

	if (ad->angle == 90 || ad->angle == 270) {
		evas_object_resize(obj, ad->win_height, height * ad->scale);
	} else  {
		evas_object_resize(obj, ad->win_width, height * ad->scale);
	}
}

HAPI int quickpanel_uic_launch_app(char *app_id, void *data)
{
	int ret = APP_CONTROL_ERROR_NONE;
	app_control_h service = NULL;
	char *app_id_from_service = NULL;

	retif(app_id == NULL && data == NULL, APP_CONTROL_ERROR_INVALID_PARAMETER, "Invialid parameter!");

	ret = app_control_create(&service);
	if (ret != APP_CONTROL_ERROR_NONE) {
		ERR("app_control_create() return error : %d", ret);
		return ret;
	}
	retif(service == NULL, APP_CONTROL_ERROR_INVALID_PARAMETER, "fail to create service handle!");

	if (app_id != NULL) {
		app_control_set_operation(service, APP_CONTROL_OPERATION_DEFAULT);
		app_control_set_app_id(service, app_id);

		if (data != NULL) {
			bundle_iterate((bundle *)data, _quickpanel_move_data_to_service, service);
		}
	} else {
		if (data != NULL) {
			ret = app_control_import_from_bundle(service, data);
			if (ret != APP_CONTROL_ERROR_NONE) {
				ERR("Failed to import[%d]", ret);
			}
		}
	}

	ret = app_control_send_launch_request(service, NULL, NULL);
	if (ret != APP_CONTROL_ERROR_NONE) {
		ERR("app_control_send_launch_request() is failed : %d", ret);
		app_control_get_app_id(service, &app_id_from_service);
		if (app_id_from_service != NULL) {
			quickpanel_uic_launch_app_inform_result(app_id_from_service, ret);
			free(app_id_from_service);
		} else {
			quickpanel_uic_launch_app_inform_result(app_id, ret);
		}
		app_control_destroy(service);
		return ret;
	}
	app_control_destroy(service);
	return ret;
}

HAPI int quickpanel_uic_launch_ug_by_appcontrol(const char *package, void *data)
{
	int ret = APP_CONTROL_ERROR_NONE;
	app_control_h service = NULL;
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, APP_CONTROL_ERROR_INVALID_PARAMETER, "ad null");
	retif(package == NULL, APP_CONTROL_ERROR_INVALID_PARAMETER, "package null");

	ret = app_control_create(&service);
	if (ret != APP_CONTROL_ERROR_NONE) {
		ERR("app_control_create() return error : %d", ret);
		return ret;
	}
	retif(service == NULL, APP_CONTROL_ERROR_INVALID_PARAMETER, "fail to create service handle!");

	app_control_set_operation(service, APP_CONTROL_OPERATION_DEFAULT);
	app_control_set_app_id(service, package);

	if (data != NULL) {
		bundle_iterate((bundle *)data, _quickpanel_move_data_to_service, service);
	}

	ret = app_control_send_launch_request(service, NULL, NULL);
	if (ret != APP_CONTROL_ERROR_NONE) {
		ERR("app_control_send_launch_request() is failed : %d", ret);
		app_control_destroy(service);
		return ret;
	}
	app_control_destroy(service);
	return ret;
}

HAPI void quickpanel_uic_launch_app_inform_result(const char *pkgname, int retcode)
{
	retif(retcode == APP_CONTROL_ERROR_NONE, , "retcode = APP_CONTROL_ERROR_NONE!");
	retif(pkgname == NULL && retcode != APP_CONTROL_ERROR_APP_NOT_FOUND, , "Invialid parameter!");

	const char *msg = NULL;
	char *app_label = NULL;

	if (retcode == APP_CONTROL_ERROR_APP_NOT_FOUND) {
		notification_status_message_post(_NOT_LOCALIZED("Unable to find application to perform this action."));
	} else {
		Eina_Strbuf *strbuf = eina_strbuf_new();
		char *format = _("IDS_QP_TPOP_UNABLE_TO_OPEN_PS");

		if (strbuf != NULL) {
			app_label = quickpanel_common_ui_get_pkginfo_label(pkgname);
			if (app_label != NULL) {
				eina_strbuf_append_printf(strbuf, format, app_label);
				free(app_label);
			} else {
				eina_strbuf_append_printf(strbuf, format, pkgname);
			}
			eina_strbuf_append_printf(strbuf, "(%x)", retcode);
			msg = eina_strbuf_string_get(strbuf);

			if (msg != NULL) {
				notification_status_message_post(msg);
			}
			eina_strbuf_free(strbuf);
		}
	}
}

HAPI void quickpanel_uic_open_quickpanel(int reason)
{
	struct appdata *ad = quickpanel_get_app_data();

	DBG("reason:%d", reason);

	retif(ad == NULL, , "Invalid parameter!");
	retif(ad->win == NULL, , "Invalid parameter!");

	ERR("Not yet implemented");

}

HAPI void quickpanel_uic_opened_reason_set(int reason)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "Invalid parameter!");

	ad->opening_reason = reason;
}

HAPI int quickpanel_uic_opened_reason_get(void)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, OPENED_NO_REASON, "Invalid parameter!");

	return ad->opening_reason;
}

static void _quickpanel_close(void)
{
	struct appdata *ad = quickpanel_get_app_data();

	DBG("");

	if (!ad || !ad->win) {
		ERR("Invalid parameter");
		return;
	}

	int ret = 0;
	ret = tzsh_quickpanel_service_hide(ad->quickpanel_service);
	if(ret != 0) {
		ERR("failed tzsh_quickpanel_service_hide");
	}

}

static Eina_Bool _quickpanel_close_timer_cb(void *data)
{
	if (_close_timer != NULL) {
		_close_timer = NULL;
	}
	_quickpanel_close();

	return ECORE_CALLBACK_CANCEL;
}

HAPI void quickpanel_uic_close_quickpanel(bool is_check_lock, int is_delay_needed) {
	int ret = 0;
	int is_lock_launched = VCONFKEY_IDLE_UNLOCK;

	if (is_check_lock == true) {
		if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &is_lock_launched) == 0) {
			if (is_lock_launched == VCONFKEY_IDLE_LOCK) {
				ret = vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, VCONFKEY_IDLE_UNLOCK);
				if (ret == 0) {
					ERR("unlock the lockscreen from quickpanel");
				} else {
					ERR("failed to unlock the lockscreen from quickpanel");
				}
			}
		}
	}

	if (is_delay_needed) {
		if( _close_timer == NULL ) {
			_close_timer = ecore_timer_add(DEL_TIMER_VALUE, _quickpanel_close_timer_cb, NULL);
		}
	} else {
		_quickpanel_close();
	}
}

HAPI void quickpanel_uic_toggle_openning_quickpanel(void)
{
	/* TO DO */
}
