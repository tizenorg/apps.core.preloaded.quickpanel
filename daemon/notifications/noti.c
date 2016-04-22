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

#include <time.h>

#include <vconf.h>
#include <app_control.h>
#include <notification.h>
#include <notification_internal.h>
#include <notification_list.h>
#include <notification_ongoing_flag.h>
#include <system_settings.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <sound_manager.h>
#include <E_DBus.h>

#include "media.h"
#include "quickpanel-ui.h"
#include "quickpanel_def.h"
#include "common_uic.h"
#include "common.h"
#include "list_util.h"
#include "noti_node.h"
#include "vi_manager.h"
#include "noti_listbox.h"
#include "noti_list_item.h"
#include "noti_section.h"
#include "noti_view.h"
#include "noti.h"
#include "list_util.h"

#ifdef QP_SERVICE_NOTI_LED_ENABLE
#include "noti_led.h"
#endif

#ifdef QP_REMINDER_ENABLE
#include "reminder.h"
#endif

#ifdef QP_EMERGENCY_MODE_ENABLE
#include "emergency_mode.h"
#endif

#define QP_NOTI_ONGOING_DBUS_PATH	"/dbus/signal"
#define QP_NOTI_ONGOING_DBUS_INTERFACE	"notification.ongoing"

static struct _info {
	noti_node   *noti_node;
	Evas_Object *ongoing_noti_section_view;
	Evas_Object *noti_section_view;
	Evas_Object *noti_box;

	E_DBus_Signal_Handler *dbus_handler_size;
	E_DBus_Signal_Handler *dbus_handler_progress;
	E_DBus_Signal_Handler *dbus_handler_content;

	struct tm last_time;

	int is_ongoing_hided;
} s_info = {
	.noti_node = NULL,
	.ongoing_noti_section_view = NULL,
	.noti_section_view = NULL,
	.noti_box = NULL,

	.dbus_handler_size = NULL,
	.dbus_handler_progress = NULL,
	.dbus_handler_content = NULL,

	.is_ongoing_hided = 0,

	.last_time.tm_mday = 0,
	.last_time.tm_mon = 0,
	.last_time.tm_year = 0,
};

static void _ongoing_noti_section_deleted_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static notification_h _update_item_progress(const char *pkgname, int priv_id, double progress)
{
	char *noti_pkgname = NULL;
	int noti_priv_id = 0;

	noti_node_item *node = quickpanel_noti_node_get(s_info.noti_node, priv_id);

	if (node != NULL && node->noti != NULL) {
		notification_get_pkgname(node->noti, &noti_pkgname);
		notification_get_id(node->noti, NULL, &noti_priv_id);

		if (!pkgname || !noti_pkgname) {
			return NULL;
		}

		if (!strcmp(noti_pkgname, pkgname) && priv_id == noti_priv_id) {

			if (notification_set_progress(node->noti, progress) != NOTIFICATION_ERROR_NONE) {
				ERR("fail to set progress");
			}
			return node->noti;
		}
	}

	return NULL;
}

static notification_h _update_item_size(const char *pkgname, int priv_id, double size)
{
	char *noti_pkgname = NULL;
	int noti_priv_id = 0;

	noti_node_item *node = quickpanel_noti_node_get(s_info.noti_node, priv_id);

	if (node != NULL && node->noti != NULL) {
		notification_get_pkgname(node->noti, &noti_pkgname);
		notification_get_id(node->noti, NULL, &noti_priv_id);

		if (!pkgname || !noti_pkgname) {
			return NULL;
		}

		if (!strcmp(noti_pkgname, pkgname)
				&& priv_id == noti_priv_id) {
			notification_set_size(node->noti, size);
			return node->noti;
		}
	}

	return NULL;
}

static notification_h _update_item_content(const char *pkgname, int priv_id, char *content)
{
	char *noti_pkgname = NULL;
	int noti_priv_id = 0;
	int ret = NOTIFICATION_ERROR_NONE;

	noti_node_item *node = quickpanel_noti_node_get(s_info.noti_node, priv_id);

	if (node != NULL && node->noti != NULL) {
		notification_get_pkgname(node->noti, &noti_pkgname);
		notification_get_id(node->noti, NULL, &noti_priv_id);

		if (!pkgname || !noti_pkgname) {
			return NULL;
		}

		if (!strcmp(noti_pkgname, pkgname) && priv_id == noti_priv_id) {
			ret = notification_set_text(node->noti, NOTIFICATION_TEXT_TYPE_CONTENT, content, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
			if (ret != NOTIFICATION_ERROR_NONE) {
				ERR("Failed to set text[%d]", ret);
			}

			return node->noti;
		}
	}

	return NULL;
}

static void _update_progressbar(void *data, notification_h update_noti)
{
	int priv_id = 0;
	struct appdata *ad = data;
	noti_node_item *node = NULL;
	retif(ad == NULL, , "data is NULL");
	retif(ad->list == NULL, , "ad->list is NULL");

	if (notification_get_id(update_noti, NULL, &priv_id) == NOTIFICATION_ERROR_NONE) {
		node = quickpanel_noti_node_get(s_info.noti_node, priv_id);
	}
	retif(node == NULL, , "fail to find node of priv_id:%d", priv_id);
	retif(node->view == NULL, , "fail to find %p", node->view);

	quickpanel_noti_listbox_update_item(ad->list, node->view);
}

static void _item_progress_update_cb(void *data, DBusMessage *msg)
{
	DBusError err;
	char *pkgname = 0;
	int priv_id = 0;
	double progress = 0;
	notification_h noti = NULL;

	retif(data == NULL || msg == NULL, , "Invalid parameter!");

	dbus_error_init(&err);
	dbus_message_get_args(msg, &err,
			DBUS_TYPE_STRING, &pkgname,
			DBUS_TYPE_INT32, &priv_id,
			DBUS_TYPE_DOUBLE, &progress,
			DBUS_TYPE_INVALID);

	if (dbus_error_is_set(&err)) {
		ERR("dbus err: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	if (pkgname == NULL) {
		ERR("pkgname is null");
		return;
	}

	/* check item on the list */
	noti = _update_item_progress(pkgname, priv_id, progress);
	retif(noti == NULL, , "Can not found noti data.");

	SDBG("pkgname[%s], priv_id[%d], progress[%lf]",	pkgname, priv_id, progress);
	_update_progressbar(data, noti);
}

static void _item_size_update_cb(void *data, DBusMessage * msg)
{
	DBusError err;
	char *pkgname = 0;
	int priv_id = 0;
	double size = 0;
	notification_h noti = NULL;

	retif(data == NULL || msg == NULL, , "Invalid parameter!");

	dbus_error_init(&err);
	dbus_message_get_args(msg, &err,
			DBUS_TYPE_STRING, &pkgname,
			DBUS_TYPE_INT32, &priv_id,
			DBUS_TYPE_DOUBLE, &size, DBUS_TYPE_INVALID);
	if (dbus_error_is_set(&err)) {
		ERR("dbus err: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	if (pkgname == NULL) {
		ERR("pkgname is null");
		return;
	}

	/* check item on the list */
	noti = _update_item_size(pkgname, priv_id, size);
	retif(noti == NULL, , "Can not found noti data.");

	SDBG("pkgname[%s], priv_id[%d], progress[%lf]",
			pkgname, priv_id, size);

	_update_progressbar(data, noti);
}

static void _item_content_update_cb(void *data, DBusMessage *msg)
{
	DBusError err;
	char *pkgname = NULL;
	int priv_id = 0;
	char *content = NULL;
	notification_h noti = NULL;

	retif(data == NULL || msg == NULL, , "Invalid parameter!");

	dbus_error_init(&err);
	dbus_message_get_args(msg, &err,
			DBUS_TYPE_STRING, &pkgname,
			DBUS_TYPE_INT32, &priv_id,
			DBUS_TYPE_STRING, &content, DBUS_TYPE_INVALID);

	if (pkgname == NULL) {
		ERR("pkgname  is null");
		return;
	}
	if (content == NULL) {
		ERR("content is null");
		return;
	}

	if (dbus_error_is_set(&err)) {
		ERR("dbus err: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	SDBG("pkgname[%s], priv_id[%d], content[%s]",
			pkgname, priv_id, content);

	/* check item on the list */
	noti = _update_item_content(pkgname, priv_id, content);
	retif(noti == NULL, , "Can not found noti data.");

	_update_progressbar(data, noti);
}

static int _is_item_deletable(notification_h noti)
{
	notification_type_e type = NOTIFICATION_TYPE_NONE;
	notification_ly_type_e ly_type = NOTIFICATION_LY_NONE;
	bool ongoing_flag = false;

	notification_get_type(noti, &type);
	notification_get_layout(noti, &ly_type);
	notification_get_ongoing_flag(noti, &ongoing_flag);

	if( (type == NOTIFICATION_TYPE_ONGOING && ongoing_flag) ||
			(type == NOTIFICATION_TYPE_ONGOING && ly_type == NOTIFICATION_LY_ONGOING_PROGRESS)) {
		return 0;
	}

	return 1;
}

static int _do_noti_delete(notification_h noti)
{
	char *pkgname = NULL;
	char *caller_pkgname = NULL;
	int flags = 0, priv_id = 0, flag_delete = 0;
	notification_type_e type = NOTIFICATION_TYPE_NONE;
	int ret = NOTIFICATION_ERROR_INVALID_PARAMETER;

	quickpanel_media_play_feedback();

	retif(noti == NULL, NOTIFICATION_ERROR_INVALID_PARAMETER, "Invalid parameter!");

	notification_get_pkgname(noti, &caller_pkgname);
	//	notification_get_application(noti, &pkgname);
	if (pkgname == NULL) {
		pkgname = caller_pkgname;
	}

	notification_get_id(noti, NULL, &priv_id);
	notification_get_property(noti, &flags);
	notification_get_type(noti, &type);

	if (flags & NOTIFICATION_PROP_PERMANENT_DISPLAY) {
		flag_delete = 0;
	} else {
		flag_delete = 1;
	}

	if (flag_delete == 1 && ( type == NOTIFICATION_TYPE_NOTI || _is_item_deletable(noti)) ) {
		ret = notification_delete_by_priv_id(caller_pkgname, NOTIFICATION_TYPE_NOTI,
				priv_id);
	}

	return ret;
}

static void _do_noti_press(notification_h noti, int pressed_area)
{
	DBG("launch app");
	int ret = APP_CONTROL_ERROR_NONE;
	char *pkgname = NULL;
	char *caller_pkgname = NULL;

	bundle *responding_service_handle = NULL;
	bundle *single_service_handle = NULL;
	bundle *multi_service_handle = NULL;
	int flags = 0, group_id = 0, priv_id = 0, count = 0, flag_launch = 0,
		flag_delete = 0;
	notification_type_e type = NOTIFICATION_TYPE_NONE;

	quickpanel_media_play_feedback();

	retif(noti == NULL, , "Invalid parameter!");

	notification_get_pkgname(noti, &caller_pkgname);
	//	notification_get_application(noti, &pkgname);
	if (pkgname == NULL) {
		pkgname = caller_pkgname;
	}

	notification_get_id(noti, &group_id, &priv_id);
	notification_get_property(noti, &flags);
	notification_get_type(noti, &type);

	if (flags & NOTIFICATION_PROP_DISABLE_APP_LAUNCH) {
		flag_launch = 0;
	} else {
		flag_launch = 1;
	}

	if (flags & NOTIFICATION_PROP_DISABLE_AUTO_DELETE) {
		flag_delete = 0;
	} else {
		flag_delete = 1;
	}

	notification_get_execute_option(noti,
			NOTIFICATION_EXECUTE_TYPE_RESPONDING,
			NULL, &responding_service_handle);
	notification_get_execute_option(noti,
			NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH,
			NULL, &single_service_handle);
	notification_get_execute_option(noti,
			NOTIFICATION_EXECUTE_TYPE_MULTI_LAUNCH,
			NULL, &multi_service_handle);

	if (pressed_area == NOTI_PRESS_BUTTON_1 && responding_service_handle != NULL) {
		DBG("");
		quickpanel_uic_close_quickpanel(true, 1);
		ret = quickpanel_uic_launch_app(NULL, responding_service_handle);
	} else if (flag_launch == 1) {
		/* Hide quickpanel */
		quickpanel_uic_close_quickpanel(true, 1);

		char *text_count = NULL;
		notification_get_text(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT, &text_count);

		if (text_count != NULL) {
			count = atoi(text_count);
		} else {
			count = 1;
		}

		if (single_service_handle != NULL && multi_service_handle == NULL) {
			DBG("");
			ret = quickpanel_uic_launch_app(NULL, single_service_handle);
		}
		if (single_service_handle == NULL && multi_service_handle != NULL) {
			DBG("");
			ret = quickpanel_uic_launch_app(NULL, multi_service_handle);
		}
		if (single_service_handle != NULL && multi_service_handle != NULL) {
			DBG("");
			if (count <= 1) {
				ret = quickpanel_uic_launch_app(NULL, single_service_handle);
			} else {
				ret = quickpanel_uic_launch_app(NULL, multi_service_handle);
			}
		}
		quickpanel_uic_launch_app_inform_result(pkgname, ret);
	}

	if (flag_delete == 1 && type == NOTIFICATION_TYPE_NOTI) {
		notification_delete_by_priv_id(caller_pkgname,
				NOTIFICATION_TYPE_NOTI,
				priv_id);
	}
}

static void _notibox_delete_cb(noti_node_item *item, Evas_Object *obj)
{
	DBG("");
	retif(obj == NULL, , "Invalid parameter!");
	retif(item == NULL, , "Invalid parameter!");

	notification_h noti = item->noti;
	retif(noti == NULL, , "Invalid parameter!");

	_do_noti_delete(noti);
}

static void _notibox_button_1_cb(noti_node_item *item, Evas_Object *obj)
{
	DBG("");
	retif(item == NULL, , "Invalid parameter!");

	notification_h noti = item->noti;
	retif(noti == NULL, , "Invalid parameter!");

	_do_noti_press(noti, NOTI_PRESS_BUTTON_1);
}

static void _notibox_select_cb(noti_node_item *item, Evas_Object *obj)
{
	DBG("");
	retif(item == NULL, , "Invalid parameter!");
	notification_h noti = item->noti;
	retif(noti == NULL, , "Invalid parameter!");

	_do_noti_press(noti, NOTI_PRESS_BG);
}

static void _noti_listitem_select_cb(noti_node_item *item, Evas_Object * obj)
{
	DBG("");
	retif(item == NULL, , "Invalid parameter!");

	notification_h noti = item->noti;
	retif(noti == NULL, , "Invalid parameter!");

	_do_noti_press(noti, NOTI_PRESS_BG);
}

static inline void _ongoing_comp_n_copy(notification_h old, notification_h new)
{
	int priv_id = 0;
	int new_priv_id = 0;
	char *pkgname = NULL;
	char *new_pkgname = NULL;
	int ret = NOTIFICATION_ERROR_NONE;

	if (!old) {
		return;
	}

	if (!new) {
		return;
	}

	notification_get_id(old, NULL, &priv_id);
	notification_get_id(new, NULL, &new_priv_id);

	notification_get_pkgname(old, &pkgname);
	notification_get_pkgname(new, &new_pkgname);

	if (!pkgname || !new_pkgname) {
		return;
	}

	if (!strcmp(pkgname, new_pkgname) && priv_id == new_priv_id) {
		double percentage = 0.0;
		double size = 0.0;
		time_t insert_time = 0;
		time_t new_insert_time = 0;

		notification_get_progress(old, &percentage);
		notification_get_size(old, &size);
		if (notification_set_progress(new, percentage) != NOTIFICATION_ERROR_NONE) {
			ERR("fail to set progress");
		}
		if (notification_set_size(new, size) != NOTIFICATION_ERROR_NONE) {
			ERR("fail to set size");
		}
		notification_get_insert_time(old, &insert_time);
		notification_get_insert_time(new, &new_insert_time);

		if (insert_time == new_insert_time) {
			char *content = NULL;
			notification_get_text(old, NOTIFICATION_TEXT_TYPE_CONTENT,	&content);
			ret = notification_set_text(new, NOTIFICATION_TEXT_TYPE_CONTENT, content, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
			if (ret != NOTIFICATION_ERROR_NONE) {
				ERR("Failed to set text[%d]", ret);
			}
		}
	}
}

static void _noti_node_clear_list_cb(gpointer key, gpointer value, gpointer user_data)
{
	Evas_Object *noti_listbox = user_data;
	noti_node_item *node = (noti_node_item *)value;

	if (noti_listbox != NULL && node != NULL) {
		if (node->noti != NULL && node->view != NULL) {
			quickpanel_noti_listbox_remove_item(noti_listbox, node->view, EINA_TRUE);
		}
	}
}

static void _noti_clear_list_all(void)
{
	struct appdata *ad = quickpanel_get_app_data();

	retif(ad == NULL, , "Invalid parameter!");

	if (s_info.noti_node != NULL && s_info.noti_node->table != NULL) {
		g_hash_table_foreach(s_info.noti_node->table, _noti_node_clear_list_cb, ad->list);

		quickpanel_noti_node_remove_all(s_info.noti_node);
	}
}

static void _ongoing_noti_section_icon_state_set(int is_closed)
{
	if (s_info.ongoing_noti_section_view != NULL) {
		if (is_closed == 1) {
			elm_object_signal_emit(s_info.ongoing_noti_section_view, "button,opened", "prog");
		} else {
			elm_object_signal_emit(s_info.ongoing_noti_section_view, "button,closed", "prog");
		}
	}
}

static void _ongoing_noti_section_add(void)
{
	int noti_count;
	struct appdata *ad;

	ad = quickpanel_get_app_data();
	if (!ad) {
		ERR("Invalid parameter");
		return;
	}

	if (!ad->list) {
		ERR("Invalid list");
		return;
	}

	if (s_info.noti_node) {
		noti_count = quickpanel_noti_node_get_item_count(s_info.noti_node, NOTIFICATION_TYPE_NONE);
	} else {
		noti_count = 0;
	}

	DBG("[%d] ", noti_count);

	if (!s_info.ongoing_noti_section_view) {
		s_info.ongoing_noti_section_view = quickpanel_noti_section_create(ad->list, QP_ITEM_TYPE_ONGOING_NOTI_GROUP);
		if (s_info.ongoing_noti_section_view) {
			quickpanel_noti_section_set_deleted_cb(s_info.ongoing_noti_section_view, _ongoing_noti_section_deleted_cb, ad);
			quickpanel_noti_section_update(s_info.ongoing_noti_section_view, noti_count);

			if (s_info.is_ongoing_hided == 1) {
				DBG("Hide NOTI.SECTION");
				_ongoing_noti_section_icon_state_set(0);
			}
		}
	}
	else {
		DBG("noti section update %d ", noti_count);
		quickpanel_noti_section_update(s_info.ongoing_noti_section_view, noti_count);
	}
}

static void _ongoing_noti_section_deleted_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	DBG("");
	struct appdata *ad = data;
	s_info.ongoing_noti_section_view = NULL;
	DBG("VIM ongoing noti_section deleted");

	if (quickpanel_noti_node_get_item_count(s_info.noti_node, NOTIFICATION_TYPE_NONE) > 0) {
		_ongoing_noti_section_add();
	}

	quickpanel_noti_listbox_remove_item(ad->list, s_info.noti_section_view, 1);
	s_info.noti_section_view = NULL;
}

static void _noti_ongoing_add(Evas_Object *list, void *data, int is_prepend)
{
	Evas_Object *noti_list_item = NULL;
	notification_h noti = data;
	retif(list == NULL, , "Invalid parameter!");

	if (noti != NULL) {
		noti_list_item = quickpanel_noti_list_item_create(list, noti);

		if (noti_list_item != NULL) {
			noti_node_item *item = quickpanel_noti_node_add(s_info.noti_node, (void*)data, (void*)noti_list_item);
			if (item != NULL) {
				quickpanel_noti_list_item_node_set(noti_list_item, item);
				quickpanel_noti_list_item_set_item_selected_cb(noti_list_item, _noti_listitem_select_cb);
				quickpanel_noti_list_item_set_item_deleted_cb(noti_list_item, _notibox_delete_cb);

				if (s_info.ongoing_noti_section_view == NULL) {
					_ongoing_noti_section_add();
				}
				quickpanel_noti_listbox_add_item(list, noti_list_item, is_prepend, s_info.ongoing_noti_section_view);
			}
		} else
			ERR("fail to insert item to list : %p", data);
	}

	DBG("noti[%p] data[%p] added listbox[%p]",
			data, noti_list_item, list);
}

static void _noti_add(Evas_Object *list, void *data, int insert_pos)
{
	notification_h noti = data;
	notification_ly_type_e layout = NOTIFICATION_LY_NOTI_EVENT_SINGLE;
	Evas_Object *noti_view = NULL;

	retif(list == NULL, , "Invalid parameter!");

	if (noti != NULL) {
		notification_get_layout(noti, &layout);
		noti_view = quickpanel_noti_list_item_create(list, noti);

		if (noti_view != NULL) {
			noti_node_item *item = quickpanel_noti_node_add(s_info.noti_node, (void*)data, (void*)noti_view);
			if (item != NULL) {
				quickpanel_noti_list_item_node_set(noti_view, item);
				quickpanel_noti_list_item_set_item_selected_cb(noti_view, _notibox_select_cb);
				quickpanel_noti_list_item_set_item_button_1_cb(noti_view, _notibox_button_1_cb);
				quickpanel_noti_list_item_set_item_deleted_cb(noti_view, _notibox_delete_cb);

				if (s_info.noti_section_view == NULL) {
					_ongoing_noti_section_add();
				}
				quickpanel_noti_listbox_add_item(list, noti_view, insert_pos, s_info.noti_section_view);
			}
		} else
			ERR("fail to insert item to list : %p", data);
	}

	DBG("noti[%p] view[%p] added gridbox[%p]",
			data, noti_view, list);
}

static void _update_notilist(struct appdata *ad)
{
	DBG("");
	Evas_Object *list = NULL;
	notification_h noti = NULL;
	notification_h noti_save = NULL;
	notification_list_h list_head = NULL;
	notification_list_h list_traverse = NULL;
	int applist = NOTIFICATION_DISPLAY_APP_ALL;

	DBG("");

	retif(ad == NULL, , "Invalid parameter!");

	list = ad->list;
	retif(list == NULL, , "Failed to get noti genlist.");

	_noti_clear_list_all();

	notification_get_list(NOTIFICATION_TYPE_ONGOING, -1, &list_head);
	list_traverse = list_head;
	while (list_traverse != NULL) {
		noti = notification_list_get_data(list_traverse);
		notification_get_display_applist(noti, &applist);

		if (applist &
				NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {
			notification_clone(noti, &noti_save);
			_noti_ongoing_add(list, noti_save, LISTBOX_APPEND);
		}
		list_traverse = notification_list_get_next(list_traverse);
	}
	if (list_head != NULL) {
		notification_free_list(list_head);
		list_head = NULL;
	}

	notification_get_list(NOTIFICATION_TYPE_NOTI , -1, &list_head);
	list_traverse = list_head;
	while (list_traverse != NULL) {
		noti = notification_list_get_data(list_traverse);
		notification_get_display_applist(noti, &applist);

		if (applist &
				NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {
			notification_clone(noti, &noti_save);
			_noti_add(list, noti_save, LISTBOX_APPEND);
		}
		list_traverse = notification_list_get_next(list_traverse);
	}
	if (list_head != NULL) {
		notification_free_list(list_head);
		list_head = NULL;
	}

	if (list != NULL) {
		elm_box_recalculate(list);
	}
}

static inline void _print_debuginfo_from_noti(notification_h noti)
{
	retif(noti == NULL, , "Invalid parameter!");

	char *noti_pkgname = NULL;
	notification_type_e noti_type = NOTIFICATION_TYPE_NONE;

	notification_get_pkgname(noti, &noti_pkgname);
	notification_get_type(noti, &noti_type);

	if (noti_pkgname != NULL) {
		SERR("pkg:%s", noti_pkgname);
	}

	SERR("type:%d", noti_type);
}

static void _detailed_changed_cb(void *data, notification_type_e type, notification_op *op_list, int num_op)
{
	int i = 0;
	int op_type = 0;
	int priv_id = 0;
	struct appdata *ad = NULL;
	notification_h noti_new = NULL;
	notification_h noti_from_master = NULL;
	notification_type_e noti_type = NOTIFICATION_TYPE_NONE;
	int noti_applist = NOTIFICATION_DISPLAY_APP_ALL;
	notification_ly_type_e noti_layout = NOTIFICATION_LY_NONE;
	notification_ly_type_e old_noti_layout = NOTIFICATION_LY_NONE;

	retif(data == NULL, , "Invalid parameter!");
	ad = data;

	SERR("num_op:%d", num_op);

	for (i = 0; i < num_op; i++) {
		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_TYPE, &op_type);
		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_PRIV_ID, &priv_id);
		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_NOTI, &noti_from_master);

		SERR("noti operation:%d privid:%d", op_type, priv_id);


		switch(op_type)	{
		case NOTIFICATION_OP_INSERT: 
			DBG("NOTIFICATION_OP_INSERT");
			if (noti_from_master == NULL) {
				ERR("failed to get a notification from master");
				continue;
			}
			if (notification_clone(noti_from_master, &noti_new) != NOTIFICATION_ERROR_NONE) {
				ERR("failed to create a cloned notification");
				continue;
			}

			_print_debuginfo_from_noti(noti_new);
#ifdef QP_EMERGENCY_MODE_ENABLE
			if (quickpanel_emergency_mode_is_on()) {
				if (quickpanel_emergency_mode_notification_filter(noti_new, 0)) {
					notification_free(noti_new);
					return;
				}
			}
#endif
#ifdef QP_SERVICE_NOTI_LED_ENABLE
			quickpanel_noti_led_proc(noti_new, op_type);
#endif

			notification_get_type(noti_new, &noti_type);
			notification_get_display_applist(noti_new, &noti_applist);
			notification_get_layout(noti_new, &noti_layout);

			if (noti_applist & NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {
				noti_node_item *node = quickpanel_noti_node_get(s_info.noti_node, priv_id);
				if (node != NULL) {
					if (noti_type == NOTIFICATION_TYPE_NOTI) {
						DBG("cb after inserted:%d", priv_id);
					}
					notification_free(noti_new);
				} else {
					if (noti_type == NOTIFICATION_TYPE_NOTI) {
						_noti_add(ad->list, noti_new, LISTBOX_APPEND);
#ifdef QP_REMINDER_ENABLE
						quickpanel_reminder_start(NULL);
#endif
					} else if (noti_type == NOTIFICATION_TYPE_ONGOING) {
						_noti_ongoing_add(ad->list, noti_new, LISTBOX_PREPEND);
					} else {
						notification_free(noti_new);
					}
				}
			} else {
				notification_free(noti_new);
			}
			break;

		case NOTIFICATION_OP_DELETE:
			{
				DBG("NOTIFICATION_OP_DELETE");
				noti_node_item *node = quickpanel_noti_node_get(s_info.noti_node, priv_id);

				if (node != NULL && node->noti != NULL) {
					notification_h noti = node->noti;
					notification_get_type(noti, &noti_type);

#ifdef QP_SERVICE_NOTI_LED_ENABLE
					quickpanel_noti_led_proc(noti, op_type);
#endif
					_print_debuginfo_from_noti(noti);

					if (noti_type == NOTIFICATION_TYPE_NOTI) {
						quickpanel_noti_listbox_remove_item(ad->list, node->view, 1);
					} else if (noti_type == NOTIFICATION_TYPE_ONGOING) {
						quickpanel_noti_listbox_remove_item(ad->list, node->view, 1);
					}

					quickpanel_noti_node_remove(s_info.noti_node, priv_id);
					if (quickpanel_media_player_id_get() == priv_id) {
						quickpanel_media_player_stop();
					}
				} else {
					ERR("node = NULL or node->noti == NULL");
				}

#ifdef QP_REMINDER_ENABLE
				if (quickpanel_noti_node_get_item_count(s_info.noti_node, NOTIFICATION_TYPE_NOTI) <= 0) {
					quickpanel_reminder_stop(NULL);
				}
#endif
			}
			break;

		case NOTIFICATION_OP_UPDATE:
			{
				DBG("NOTIFICATION_OP_UPDATE");
				noti_node_item *node = quickpanel_noti_node_get(s_info.noti_node, priv_id);
				notification_h old_noti = NULL;

				DBG("Notification update priv_id[%d]", priv_id);

				if (noti_from_master == NULL) {
					ERR("failed to get a notification from master");
					continue;
				}

				if (notification_clone(noti_from_master, &noti_new) != NOTIFICATION_ERROR_NONE) {
					ERR("failed to create a cloned notification");
					continue;
				}
#ifdef QP_EMERGENCY_MODE_ENABLE
				if (quickpanel_emergency_mode_is_on()) {
					if (quickpanel_emergency_mode_notification_filter(noti_new, 0)) {
						DBG("notification filtered");
						notification_free(noti_new);
						return;
					}
				}
#endif
#ifdef QP_SERVICE_NOTI_LED_ENABLE
				quickpanel_noti_led_proc(noti_new, op_type);
#endif
				_print_debuginfo_from_noti(noti_new);
				notification_get_layout(noti_new, &noti_layout);

				if (node != NULL && node->view != NULL && node->noti != NULL) {
					notification_get_type(noti_new, &noti_type);

					notification_get_layout(node->noti, &old_noti_layout);
					if (noti_type == NOTIFICATION_TYPE_NOTI || old_noti_layout != noti_layout) {
						if (quickpanel_noti_view_is_view_handler_changed(node->view, noti_new) == 1) {
							quickpanel_noti_listbox_remove_item(ad->list, node->view, 1);
							quickpanel_noti_node_remove(s_info.noti_node, priv_id);
						} else {
							old_noti = node->noti;
							node->noti = noti_new;
							quickpanel_noti_listbox_update_item(ad->list, node->view);
						}
					} else if (noti_type == NOTIFICATION_TYPE_ONGOING) {
						old_noti = node->noti;
						node->noti = noti_new;

						quickpanel_noti_listbox_update_item(ad->list, node->view);
					} else {
						notification_free(noti_new);
					}

					if (old_noti != NULL) {
						notification_free(old_noti);
					}
				} else {
					notification_get_display_applist(noti_new, &noti_applist);

					if (noti_applist & NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {
						if (noti_type == NOTIFICATION_TYPE_NOTI) {
							_noti_add(ad->list, noti_new, LISTBOX_PREPEND);
						} else if (noti_type == NOTIFICATION_TYPE_ONGOING) {
							_noti_ongoing_add(ad->list, noti_new, LISTBOX_PREPEND);
						} else {
							notification_free(noti_new);
						}
					} else {
						notification_free(noti_new);
					}
				}
			}
			break;

		case NOTIFICATION_OP_SERVICE_READY:
			_update_notilist(ad);

#ifdef QP_SERVICE_NOTI_LED_ENABLE
			quickpanel_noti_led_init(ad, s_info.noti_node);
#endif
			quickpanel_vim_set_state_ready();

#ifdef QP_REMINDER_ENABLE
			if (quickpanel_noti_node_get_item_count(s_info.noti_node, NOTIFICATION_TYPE_NOTI) > 0) {
				quickpanel_reminder_start(NULL);
			} else {
				quickpanel_reminder_stop(NULL);
			}
#endif
			//quickpanel_chg_init();
			break;

		default:
			SERR("Unknown op type");
			break;
		}
	}

	int noti_count = quickpanel_noti_node_get_item_count(s_info.noti_node, NOTIFICATION_TYPE_NOTI);
	int ongoing_noti_count = quickpanel_noti_node_get_item_count(s_info.noti_node, NOTIFICATION_TYPE_ONGOING);

	if (s_info.noti_section_view != NULL) {
		quickpanel_noti_section_update(s_info.noti_section_view, noti_count+ongoing_noti_count);
	}
	if (s_info.ongoing_noti_section_view != NULL) {
		quickpanel_noti_section_update(s_info.ongoing_noti_section_view, noti_count+ongoing_noti_count);
	}

	SERR("current noti count:%d, ongoing:%d", noti_count, ongoing_noti_count);
}

static void _update_sim_status_cb(keynode_t *node, void *data)
{
	struct appdata *ad = data;

	if (ad != NULL && ad->list != NULL) {
		if (notification_is_service_ready() == 1) {
			_update_notilist(ad);
		}
	}
}

static Eina_Bool _noti_callback_register_idler_cb(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, EINA_FALSE, "Invalid parameter!");

	notification_register_detailed_changed_cb(_detailed_changed_cb, ad);

	return EINA_FALSE;
}

static int _register_event_handler(struct appdata *ad)
{
	int ret = 0;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");
	retif(ad->dbus_connection == NULL, QP_FAIL, "Invalid parameter!");

	s_info.dbus_handler_size =
		e_dbus_signal_handler_add(ad->dbus_connection, NULL,
				QP_NOTI_ONGOING_DBUS_PATH,
				QP_NOTI_ONGOING_DBUS_INTERFACE, "update_progress",
				_item_progress_update_cb,
				ad);
	if (s_info.dbus_handler_size == NULL) {
		ERR("fail to add size signal");
	}

	s_info.dbus_handler_progress =
		e_dbus_signal_handler_add(ad->dbus_connection, NULL,
				QP_NOTI_ONGOING_DBUS_PATH,
				QP_NOTI_ONGOING_DBUS_INTERFACE, "update_size",
				_item_size_update_cb,
				ad);
	if (s_info.dbus_handler_progress == NULL) {
		ERR("fail to add progress signal");
	}

	s_info.dbus_handler_content =
		e_dbus_signal_handler_add(ad->dbus_connection, NULL,
				QP_NOTI_ONGOING_DBUS_PATH,
				QP_NOTI_ONGOING_DBUS_INTERFACE, "update_content",
				_item_content_update_cb,
				ad);
	if (s_info.dbus_handler_content == NULL) {
		ERR("fail to add content signal");
	}

	/* Notify vconf key */
	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT, _update_sim_status_cb, (void *)ad);
	if (ret != 0) {
		ERR("Failed to register SIM_SLOT change callback!");
	}

	/* Register notification changed cb */
	ecore_idler_add(_noti_callback_register_idler_cb, ad);

	return ret;
}

static int _unregister_event_handler(struct appdata *ad)
{
	int ret = 0;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");
	retif(ad->dbus_connection == NULL, QP_FAIL, "Invalid parameter!");

	/* Unregister notification changed cb */
	notification_unregister_detailed_changed_cb(_detailed_changed_cb, (void *)ad);

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT, _update_sim_status_cb);
	if (ret != 0) {
		ERR("Failed to ignore SIM_SLOT change callback!");
	}

	/* Delete dbus signal */
	if (s_info.dbus_handler_size != NULL) {
		e_dbus_signal_handler_del(ad->dbus_connection, s_info.dbus_handler_size);
		s_info.dbus_handler_size = NULL;
	}
	if (s_info.dbus_handler_progress != NULL) {
		e_dbus_signal_handler_del(ad->dbus_connection, s_info.dbus_handler_progress);
		s_info.dbus_handler_progress = NULL;
	}
	if (s_info.dbus_handler_content != NULL) {
		e_dbus_signal_handler_del(ad->dbus_connection, s_info.dbus_handler_content);
		s_info.dbus_handler_content = NULL;
	}

	return QP_OK;
}

/*static void _quickpanel_noti_init(void *data)
  {
  struct appdata *ad = NULL;

  retif(data == NULL, , "Invalid parameter!");
  ad = data;

  retif(ad->list == NULL, , "Invalid parameter!");

  DBG("wr");

  if (s_info.noti_box == NULL) {
  s_info.noti_box = quickpanel_noti_listbox_create(ad->list
  , quickpanel_get_app_data(), QP_ITEM_TYPE_ONGOING_NOTI);
  quickpanel_noti_listbox_set_item_deleted_cb(s_info.noti_box, _quickpanel_list_box_deleted_cb);
  quickpanel_list_util_sort_insert(ad->list, s_info.noti_box);
  }
  }

  static void _quickpanel_noti_fini(void *data)
  {
  struct appdata *ad = NULL;

  retif(data == NULL, , "Invalid parameter!");
  ad = data;

  retif(ad->list == NULL, , "Invalid parameter!");

  DBG("dr");
  }*/

static void _on_time_changed(keynode_t *key, void *data)
{
	struct appdata *ad = data;
	time_t current_time;
	struct tm loc_time;

	if (!key) {
		/**
		 * @todo
		 * Todo something for this case.
		 */
	}

	current_time = time(NULL);
	localtime_r(&current_time, &loc_time);

	if (loc_time.tm_yday != s_info.last_time.tm_yday || loc_time.tm_year != s_info.last_time.tm_year) {
		_update_notilist(ad);
	}

	s_info.last_time = loc_time;
}

static void _noti_time_init(void *data)
{
	int ret = 0;
	struct appdata *ad = data;
	retif_nomsg(ad == NULL, );
	time_t current_time;

	current_time = time(NULL);
	localtime_r(&current_time, &s_info.last_time);

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_TIMEZONE_INT, _on_time_changed, data);
	msgif(ret != 0, "failed to set key(%s) : %d", VCONFKEY_SETAPPL_TIMEZONE_INT, ret);
	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SVC_ROAM, _on_time_changed, data);
	msgif(ret != 0, "failed to set key(%s) : %d", VCONFKEY_TELEPHONY_SVC_ROAM, ret);
	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_TIMEZONE_ID, _on_time_changed, data);
	msgif(ret != 0, "failed to set key(%s) : %d", VCONFKEY_SETAPPL_TIMEZONE_ID, ret);
}

static int _init(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	quickpanel_noti_node_create(&s_info.noti_node);

	//_quickpanel_noti_init(ad);

	_register_event_handler(ad);

	// NOTI TIME
	_noti_time_init(data);

	return QP_OK;
}

static void _noti_time_fini(void *data)
{
	int ret = 0;
	struct appdata *ad = data;
	retif_nomsg(ad == NULL, );

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_TIMEZONE_INT, _on_time_changed);
	msgif(ret != 0, "failed to set key(%s) : %d", VCONFKEY_SETAPPL_TIMEZONE_INT, ret);
	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_TIMEZONE_ID, _on_time_changed);
	msgif(ret != 0, "failed to set key(%s) : %d", VCONFKEY_SETAPPL_TIMEZONE_ID, ret);
	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SVC_ROAM, _on_time_changed);
	msgif(ret != 0, "failed to set key(%s) : %d", VCONFKEY_TELEPHONY_SVC_ROAM, ret);
}

static int _fini(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

#ifdef QP_SERVICE_NOTI_LED_ENABLE
	quickpanel_noti_led_fini(ad);
#endif

	/* Unregister event handler */
	_unregister_event_handler(data);

	_noti_clear_list_all();

	//_quickpanel_noti_fini(ad);

	if (s_info.noti_node != NULL) {
		quickpanel_noti_node_destroy(&s_info.noti_node);
	}

	// NOTI TIME
	_noti_time_fini(data);

	return QP_OK;
}

static int _suspend(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	return QP_OK;
}

static void _noti_node_ongoing_update_cb(gpointer key, gpointer value, gpointer user_data)
{
	notification_type_e noti_type = NOTIFICATION_TYPE_NONE;
	Evas_Object *noti_listbox = user_data;
	noti_node_item *node = (noti_node_item *)value;

	if (noti_listbox != NULL && node != NULL) {
		if (node->noti != NULL && node->view != NULL) {
			notification_get_type(node->noti, &noti_type);
			if (noti_type == NOTIFICATION_TYPE_ONGOING) {
				quickpanel_noti_listbox_update_item(noti_listbox, node->view);
			}
		}
	}
}

static int _resume(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	if (ad->list != NULL && s_info.noti_node != NULL) {
		if (quickpanel_noti_node_get_item_count(s_info.noti_node, QP_ITEM_TYPE_ONGOING_NOTI) > 0) {
			if (s_info.noti_node->table != NULL) {
				g_hash_table_foreach(s_info.noti_node->table, _noti_node_ongoing_update_cb, ad->list);
			}
		}
	}

	return QP_OK;
}

static void _refresh(void *data)
{
	struct appdata *ad = NULL;

	retif(data == NULL, , "Invalid parameter!");
	ad = data;

	quickpanel_noti_listbox_rotation(ad->list, ad->angle);
}

static void _lang_changed(void *data)
{
	int noti_count = 0;
	int ongoing_noti_count = 0;
	struct appdata *ad = data;

	retif(ad == NULL, , "Invalid parameter!");

	if (notification_is_service_ready() == 1) {

		_update_notilist(ad);

		noti_count = quickpanel_noti_node_get_item_count(s_info.noti_node, NOTIFICATION_TYPE_NOTI);
		ongoing_noti_count = quickpanel_noti_node_get_item_count(s_info.noti_node, NOTIFICATION_TYPE_ONGOING);

		if (s_info.noti_section_view != NULL) {
			quickpanel_noti_section_update(s_info.noti_section_view, noti_count+ongoing_noti_count);
		}
		if (s_info.ongoing_noti_section_view != NULL) {
			quickpanel_noti_section_update(s_info.ongoing_noti_section_view, noti_count+ongoing_noti_count);
		}
	}
}

HAPI int quickpanel_noti_get_count(void)
{
	return quickpanel_noti_node_get_item_count(s_info.noti_node, NOTIFICATION_TYPE_NONE);
}

HAPI int quickpanel_noti_get_type_count(notification_type_e noti_type)
{
	return quickpanel_noti_node_get_item_count(s_info.noti_node, noti_type);
}

HAPI int quickpanel_noti_get_geometry(int *limit_h, int *limit_partial_h, int *limit_partial_w)
{
	retif(limit_h == NULL, 0, "invalid parameter");
	retif(limit_partial_h == NULL, 0, "invalid parameter");
	retif(limit_partial_w == NULL, 0, "invalid parameter");
	struct appdata *ad = quickpanel_get_app_data();

	return quickpanel_noti_listbox_get_geometry(ad->list, limit_h, limit_partial_h, limit_partial_w);
}

HAPI noti_node_item *quickpanel_noti_node_get_by_priv_id(int priv_id)
{
	retif(s_info.noti_node == NULL, NULL, "invalid parameter");

	return quickpanel_noti_node_get(s_info.noti_node, priv_id);
}

HAPI noti_node_item *quickpanel_noti_node_get_first_noti(void)
{
	// get box list
	Eina_List *l;
	Eina_List *l_next;
	Evas_Object *obj = NULL;
	Eina_List *item_list = NULL;
	noti_node_item *node = NULL;
	noti_node_item *node_first_noti = NULL;
	notification_type_e type = NOTIFICATION_TYPE_NONE;
	struct appdata *ad = quickpanel_get_app_data();

	item_list = elm_box_children_get(ad->list);
	retif(item_list == NULL, NULL, "invalid parameter");

	EINA_LIST_FOREACH_SAFE(item_list, l, l_next, obj) {
		if (obj != NULL) {
			node = quickpanel_noti_list_item_node_get(obj);
			if (node) {
				notification_h noti = node->noti;
				if (noti) {
					notification_get_type(noti, &type);
					if (type == NOTIFICATION_TYPE_NOTI) {
						node_first_noti = node;
						break;
					}
				}
			}
		}
	}

	if (item_list != NULL) {
		eina_list_free(item_list);
	}

	return node_first_noti;
}

HAPI void quickpanel_noti_closing_trigger_set(void)
{
	struct appdata *ad = quickpanel_get_app_data();

	retif(ad == NULL, , "invalid parameter");

	quickpanel_noti_listbox_closing_trigger_set(ad->list);
}

static void _opened(void *data)
{
	if (elm_config_access_get() == EINA_TRUE) {
		elm_access_say(_NOT_LOCALIZED("Notification panel"));
	}
}

HAPI void quickpanel_noti_set_clear_all_status()
{
	notification_h noti;
	notification_list_h list_head;
	notification_list_h list_traverse;
	bool ongoing_cnt = false;
	int ret;

	list_head = NULL;
	ret = notification_get_list(NOTIFICATION_TYPE_ONGOING, -1, &list_head);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Unable to get the list of notification");
		return;
	}

	list_traverse = list_head;
	while (list_traverse != NULL) {
		noti = notification_list_get_data(list_traverse);
		if (_is_item_deletable(noti)) {
			ongoing_cnt++;
		}
		list_traverse = notification_list_get_next(list_traverse);
	}

	if (list_head != NULL) {
		notification_free_list(list_head);
	}

	if (ongoing_cnt == 0 && quickpanel_noti_node_get_item_count(s_info.noti_node, NOTIFICATION_TYPE_NOTI) <= 0) {
		INFO("NOTI SECTION CLEAR ALL HIDE");
		elm_object_signal_emit(s_info.ongoing_noti_section_view, "notifaction,section,clear_all,hide", "base");
	} else {
		INFO("NOTI SECTION CLEAR ALL SHOW");
		elm_object_signal_emit(s_info.ongoing_noti_section_view, "notifaction,section,clear_all,show", "base");
	}
}

HAPI void quickpanel_noti_on_clear_all_clicked(void *data, Evas_Object *obj, void *info)
{
	quickpanel_media_play_feedback();
	LOGI("NOTI CLEAR ALL CLICKED");

	DBG("");
	notification_h noti;
	notification_list_h list_head;
	notification_list_h list_traverse;
	int ret;

	quickpanel_noti_closing_trigger_set();
	notification_clear(NOTIFICATION_TYPE_NOTI);

	list_head = NULL;
	ret = notification_get_list(NOTIFICATION_TYPE_ONGOING, -1, &list_head);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Unable to get the list of notifications");
		return;
	}

	list_traverse = list_head;
	while (list_traverse != NULL) {
		noti = notification_list_get_data(list_traverse);
		if (_is_item_deletable(noti)) {
			char *caller_pkgname;
			int priv_id;

			ret = notification_get_id(noti, NULL, &priv_id);
			if (ret != NOTIFICATION_ERROR_NONE) {
				ERR("Unable to get ID from noti object: %p", noti);
				priv_id = 0;
			}
			ret = notification_get_pkgname(noti, &caller_pkgname);
			if (ret != NOTIFICATION_ERROR_NONE) {
				ERR("Unable to get caller package name: %p", noti);
				caller_pkgname = NULL;
			}

			notification_delete_by_priv_id(caller_pkgname, NOTIFICATION_TYPE_NOTI, priv_id);
		}

		list_traverse = notification_list_get_next(list_traverse);
	}

	if (list_head != NULL) {
		notification_free_list(list_head);
		list_head = NULL;
	}

	quickpanel_uic_close_quickpanel(EINA_FALSE, EINA_FALSE);

}

static Eina_Bool _notification_time_format_changed_cb(void *data)
{
	struct appdata *ad = data;

	_update_notilist(ad);

	return ECORE_CALLBACK_CANCEL;
}

HAPI void quickpanel_noti_update_by_system_time_changed_setting_cb(system_settings_key_e key, void *data)
{
	// struct appdata *ad = data;
	if (key == SYSTEM_SETTINGS_KEY_TIME_CHANGED || key == SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY) {
		_on_time_changed(NULL, data);
	} else { //key == SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR
		_notification_time_format_changed_cb(data);
	}
}

HAPI void quickpanel_noti_update_by_system_time_changed_vconf_cb(keynode_t *key, void *data)
{
	_notification_time_format_changed_cb(data);
}

HAPI void quickpanel_noti_init_noti_section(void)
{
	if (s_info.ongoing_noti_section_view == NULL) {
		_ongoing_noti_section_add();
	}
}

QP_Module noti = {
	.name = "noti",
	.init = _init,
	.fini = _fini,
	.suspend = _suspend,
	.resume = _resume,
	.lang_changed = _lang_changed,
	.hib_enter = NULL,
	.hib_leave = NULL,
	.refresh = _refresh,
	.get_height = NULL,
	.qp_opened = _opened,
};

/* End of a file */
