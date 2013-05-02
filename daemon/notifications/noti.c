/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <appsvc.h>

#include <time.h>
#include <vconf.h>
#include <appcore-common.h>
#include <app_service.h>
#include <Ecore_X.h>
#include <notification.h>

#include "quickpanel-ui.h"
#include "quickpanel_def.h"
#include "common.h"
#include "noti_node.h"
#include "noti_gridbox.h"
#include "noti_box.h"
#include "noti_listbox.h"
#include "noti_list_item.h"
#include "noti_section.h"
#include "noti.h"
#include "list_util.h"
#ifdef QP_SERVICE_NOTI_LED_ENABLE
#include "noti_led.h"
#endif

#ifndef VCONFKEY_QUICKPANEL_STARTED
#define VCONFKEY_QUICKPANEL_STARTED "memory/private/"PACKAGE_NAME"/started"
#endif /* VCONFKEY_QUICKPANEL_STARTED */

#define QP_NOTI_ONGOING_DBUS_PATH	"/dbus/signal"
#define QP_NOTI_ONGOING_DBUS_INTERFACE	"notification.ongoing"

static noti_node *g_noti_node;
static Evas_Object *g_noti_section;
static Evas_Object *g_noti_listbox;
static Evas_Object *g_noti_gridbox;

static int quickpanel_noti_init(void *data);
static int quickpanel_noti_fini(void *data);
static int quickpanel_noti_suspend(void *data);
static int quickpanel_noti_resume(void *data);
static void quickpanel_noti_lang_changed(void *data);
static void quickpanel_noti_refresh(void *data);

QP_Module noti = {
	.name = "noti",
	.init = quickpanel_noti_init,
	.fini = quickpanel_noti_fini,
	.suspend = quickpanel_noti_suspend,
	.resume = quickpanel_noti_resume,
	.lang_changed = quickpanel_noti_lang_changed,
	.hib_enter = NULL,
	.hib_leave = NULL,
	.refresh = quickpanel_noti_refresh,
	.get_height = NULL,
};

static notification_h _quickpanel_noti_update_item_progress(const char *pkgname,
							    int priv_id,
							    double progress)
{
	char *noti_pkgname = NULL;
	int noti_priv_id = 0;

	noti_node_item *node = noti_node_get(g_noti_node, priv_id);

	if (node != NULL && node->noti != NULL) {
		notification_get_pkgname(node->noti, &noti_pkgname);
		notification_get_id(node->noti, NULL, &noti_priv_id);
		if (!strcmp(noti_pkgname, pkgname)
		    && priv_id == noti_priv_id) {

			if (notification_set_progress(node->noti, progress) != NOTIFICATION_ERROR_NONE) {
				ERR("fail to set progress");
			}
			return node->noti;
		}
	}

	return NULL;
}

static notification_h _quickpanel_noti_update_item_size(const char *pkgname,
							int priv_id,
							double size)
{
	char *noti_pkgname = NULL;
	int noti_priv_id = 0;

	noti_node_item *node = noti_node_get(g_noti_node, priv_id);

	if (node != NULL && node->noti != NULL) {
		notification_get_pkgname(node->noti, &noti_pkgname);
		notification_get_id(node->noti, NULL, &noti_priv_id);
		if (!strcmp(noti_pkgname, pkgname)
		    && priv_id == noti_priv_id) {
			notification_set_size(node->noti, size);
			return node->noti;
		}
	}

	return NULL;
}

static notification_h _quickpanel_noti_update_item_content(const char *pkgname,
							int priv_id,
							char *content)
{
	char *noti_pkgname = NULL;
	int noti_priv_id = 0;

	noti_node_item *node = noti_node_get(g_noti_node, priv_id);

	if (node != NULL && node->noti != NULL) {
		notification_get_pkgname(node->noti, &noti_pkgname);
		notification_get_id(node->noti, NULL, &noti_priv_id);
		if (!strcmp(noti_pkgname, pkgname)
		    && priv_id == noti_priv_id) {
			notification_set_text(node->noti,
				NOTIFICATION_TEXT_TYPE_CONTENT,
				content, NULL,
				NOTIFICATION_VARIABLE_TYPE_NONE);
			return node->noti;
		}
	}

	return NULL;
}

static void _quickpanel_noti_update_progressbar(void *data,
						notification_h update_noti)
{
	struct appdata *ad = NULL;
	Elm_Object_Item *found = NULL;
	noti_node_item *node = NULL;

	retif(!data, , "data is NULL");
	ad = data;

	retif(!ad->list, , "ad->list is NULL");

	int priv_id = 0;

	if (notification_get_id(update_noti, NULL, &priv_id) == NOTIFICATION_ERROR_NONE) {
		node = noti_node_get(g_noti_node, priv_id);

		if (node != NULL) {
			found = node->view;
		}
	}

	retif(node == NULL, , "fail to find node of priv_id:%d", priv_id);
	retif(node->view == NULL, , "fail to find %p", node->view);

	listbox_update_item(g_noti_listbox, node->view);
}

static void _quickpanel_noti_item_progress_update_cb(void *data,
						DBusMessage *msg)
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
	noti = _quickpanel_noti_update_item_progress(pkgname,
						priv_id, progress);
	retif(noti == NULL, , "Can not found noti data.");

	DBG("pkgname[%s], priv_id[%d], progress[%lf]",
				pkgname, priv_id, progress);
	if (!quickpanel_is_suspended())
		_quickpanel_noti_update_progressbar(data, noti);
}

static void _quickpanel_noti_item_size_update_cb(void *data, DBusMessage * msg)
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
	noti = _quickpanel_noti_update_item_size(pkgname, priv_id, size);
	retif(noti == NULL, , "Can not found noti data.");

	DBG("pkgname[%s], priv_id[%d], progress[%lf]",
				pkgname, priv_id, size);

	if (!quickpanel_is_suspended())
		_quickpanel_noti_update_progressbar(data, noti);
}

static void _quickpanel_noti_item_content_update_cb(void *data,
						DBusMessage *msg)
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

	DBG("pkgname[%s], priv_id[%d], content[%s]",
				pkgname, priv_id, content);

	/* check item on the list */
	noti = _quickpanel_noti_update_item_content(pkgname, priv_id, content);
	retif(noti == NULL, , "Can not found noti data.");

	if (!quickpanel_is_suspended())
		_quickpanel_noti_update_progressbar(data, noti);
}

static void _quickpanel_do_noti_delete(notification_h noti) {
	char *pkgname = NULL;
	char *caller_pkgname = NULL;
	int flags = 0, priv_id = 0, flag_delete = 0;
	notification_type_e type = NOTIFICATION_TYPE_NONE;

	quickpanel_play_feedback();

	retif(noti == NULL, , "Invalid parameter!");

	notification_get_pkgname(noti, &caller_pkgname);
	notification_get_application(noti, &pkgname);
	if (pkgname == NULL)
		pkgname = caller_pkgname;

	notification_get_id(noti, NULL, &priv_id);
	notification_get_property(noti, &flags);
	notification_get_type(noti, &type);

	if (flags & NOTIFICATION_PROP_PERMANENT_DISPLAY)
		flag_delete = 0;
	else
		flag_delete = 1;

	if (flag_delete == 1 && type == NOTIFICATION_TYPE_NOTI) {
		notification_delete_by_priv_id(caller_pkgname, NOTIFICATION_TYPE_NOTI,
				priv_id);
	}
}

static void _quickpanel_do_noti_press(notification_h noti, int pressed_area) {
	int ret = -1;
	char *pkgname = NULL;
	char *caller_pkgname = NULL;
	bundle *args = NULL;
	bundle *group_args = NULL;
	bundle *responding_service_handle = NULL;
	bundle *single_service_handle = NULL;
	bundle *multi_service_handle = NULL;
	int flags = 0, group_id = 0, priv_id = 0, count = 0, flag_launch = 0,
			flag_delete = 0;
	notification_type_e type = NOTIFICATION_TYPE_NONE;

	quickpanel_play_feedback();

	retif(noti == NULL, , "Invalid parameter!");

	notification_get_pkgname(noti, &caller_pkgname);
	notification_get_application(noti, &pkgname);
	if (pkgname == NULL)
		pkgname = caller_pkgname;

	notification_get_id(noti, &group_id, &priv_id);
	notification_get_property(noti, &flags);
	notification_get_type(noti, &type);

	if (flags & NOTIFICATION_PROP_DISABLE_APP_LAUNCH)
		flag_launch = 0;
	else
		flag_launch = 1;

	if (flags & NOTIFICATION_PROP_DISABLE_AUTO_DELETE)
		flag_delete = 0;
	else
		flag_delete = 1;

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
		quickpanel_close_quickpanel(true);
		ret = quickpanel_launch_app(NULL, responding_service_handle);
		quickpanel_launch_app_inform_result(pkgname, ret);
	} else if (flag_launch == 1) {
		/* Hide quickpanel */
		quickpanel_close_quickpanel(true);

		char *text_count = NULL;
		notification_get_text(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT, &text_count);

		if (text_count != NULL) {
			count = atoi(text_count);
		} else {
			count = 1;
		}

		if (single_service_handle != NULL && multi_service_handle == NULL) {
			DBG("");
			ret = quickpanel_launch_app(NULL, single_service_handle);
			quickpanel_launch_app_inform_result(pkgname, ret);
		}
		if (single_service_handle == NULL && multi_service_handle != NULL) {
			DBG("");
			ret = quickpanel_launch_app(NULL, multi_service_handle);
			quickpanel_launch_app_inform_result(pkgname, ret);
		}
		if (single_service_handle != NULL && multi_service_handle != NULL) {
			DBG("");
			if (count <= 1) {
				ret = quickpanel_launch_app(NULL, single_service_handle);
				quickpanel_launch_app_inform_result(pkgname, ret);
			} else {
				ret = quickpanel_launch_app(NULL, multi_service_handle);
				quickpanel_launch_app_inform_result(pkgname, ret);
			}
		}
		if (single_service_handle == NULL && multi_service_handle == NULL) {
			DBG("");
			notification_get_args(noti, &args, &group_args);

			if (count > 1 && group_args != NULL) {
				ret = quickpanel_launch_app(pkgname, group_args);
				quickpanel_launch_app_inform_result(pkgname, ret);
			} else {
				ret = quickpanel_launch_app(pkgname, args);
				quickpanel_launch_app_inform_result(pkgname, ret);
			}
		}
	}

	if (flag_delete == 1 && type == NOTIFICATION_TYPE_NOTI) {
		notification_delete_by_priv_id(caller_pkgname,
				NOTIFICATION_TYPE_NOTI,
				priv_id);
	}
}

static void quickpanel_notibox_delete_cb(void *data, Evas_Object * obj) {
	DBG("");
	noti_node_item *item = data;
	retif(item == NULL, , "Invalid parameter!");

	notification_h noti = item->noti;
	retif(noti == NULL, , "Invalid parameter!");

	_quickpanel_do_noti_delete(noti);

}

static void quickpanel_notibox_button_1_cb(void *data, Evas_Object * obj) {
	DBG("");
	noti_node_item *item = data;
	retif(item == NULL, , "Invalid parameter!");

	notification_h noti = item->noti;
	retif(noti == NULL, , "Invalid parameter!");

	_quickpanel_do_noti_press(noti, NOTI_PRESS_BUTTON_1);
}

static void quickpanel_notibox_select_cb(void *data, Evas_Object * obj) {
	DBG("");
	noti_node_item *item = data;
	retif(item == NULL, , "Invalid parameter!");

	notification_h noti = item->noti;
	retif(noti == NULL, , "Invalid parameter!");

	_quickpanel_do_noti_press(noti, NOTI_PRESS_BG);
}

static void quickpanel_noti_listitem_select_cb(void *data, Evas_Object * obj) {
	DBG("");
	noti_node_item *item = data;
	retif(item == NULL, , "Invalid parameter!");

	notification_h noti = item->noti;
	retif(noti == NULL, , "Invalid parameter!");

	_quickpanel_do_noti_press(noti, NOTI_PRESS_BG);
}

static inline void __ongoing_comp_n_copy(notification_h old, notification_h new)
{
	int priv_id = 0;
	int new_priv_id = 0;
	char *pkgname = NULL;
	char *new_pkgname = NULL;

	if (!old)
		return;

	if (!new)
		return;

	notification_get_id(old, NULL, &priv_id);
	notification_get_id(new, NULL, &new_priv_id);

	notification_get_pkgname(old, &pkgname);
	notification_get_pkgname(new, &new_pkgname);

	if (!pkgname || !new_pkgname)
		return;

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
			notification_get_text(old,
				NOTIFICATION_TEXT_TYPE_CONTENT,	&content);
			notification_set_text(new,
				NOTIFICATION_TEXT_TYPE_CONTENT,	content,
				NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
		}
	}
}

static void _quickpanel_noti_clear_ongoinglist()
{
	if (g_noti_listbox != NULL) {
		listbox_remove_all_item(g_noti_listbox, EINA_FALSE);
	}
}

static void _quickpanel_noti_clear_notilist(void)
{
	if (g_noti_gridbox != NULL) {
		gridbox_remove_all_item(g_noti_gridbox, EINA_FALSE);
	}
}

static void _quickpanel_noti_clear_list_all(void)
{
	_quickpanel_noti_clear_ongoinglist();
	_quickpanel_noti_clear_notilist();
}

static void _quickpanel_noti_section_add(void)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "Invalid parameter!");
	retif(ad->list == NULL, , "Invalid parameter!");

	if (g_noti_section == NULL) {
		g_noti_section = noti_section_create(ad->list);
		if (g_noti_section != NULL) {
			quickpanel_list_util_sort_insert(ad->list, g_noti_section);
			DBG("noti section[%p]", g_noti_section);
		}
	}
}

static void _quickpanel_noti_section_remove(void)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "Invalid parameter!");
	retif(ad->list == NULL, , "Invalid parameter!");

	if (g_noti_section != NULL) {
		quickpanel_list_util_item_unpack_by_type(ad->list, QP_ITEM_TYPE_NOTI_GROUP);
		noti_section_remove(g_noti_section);
		g_noti_section = NULL;
	}
}

static void _quickpanel_noti_box_deleted_cb(void *data, Evas_Object *obj) {
	int priv_id = -1;

	retif(data == NULL, , "Invalid parameter!");
	DBG("deleting:%p", data);

	noti_node_item *item = data;
	notification_h noti = item->noti;

	if (noti != NULL) {
		notification_get_id(noti, NULL, &priv_id);
		noti_node_remove(g_noti_node, priv_id);
	}
}

static void _quickpanel_list_box_deleted_cb(void *data, Evas_Object *obj) {
	int priv_id = -1;

	retif(data == NULL, , "Invalid parameter!");
	DBG("deleting:%p", data);

	noti_node_item *item = data;
	notification_h noti = item->noti;

	if (noti != NULL) {
		notification_get_id(noti, NULL, &priv_id);
		noti_node_remove(g_noti_node, priv_id);
	}
}

static void _quickpanel_noti_ongoing_add(Evas_Object *list, void *data, int is_prepend)
{
	Evas_Object *noti_list_item = NULL;
	notification_ly_type_e layout = NOTIFICATION_LY_ONGOING_EVENT;

	retif(list == NULL, , "Invalid parameter!");
	notification_h noti = data;

	if (noti != NULL) {
		notification_get_layout(noti, &layout);
		noti_list_item = noti_list_item_create(g_noti_listbox, layout);

		if (noti_list_item != NULL) {
			noti_node_item *item = noti_node_add(g_noti_node, (void*)data, (void*)noti_list_item);
			if (item != NULL) {
				noti_list_item_node_set(noti_list_item, item);
				noti_list_item_set_item_selected_cb(noti_list_item, quickpanel_noti_listitem_select_cb);
				listbox_add_item(g_noti_listbox, noti_list_item, is_prepend);
			}
		} else
			ERR("fail to insert item to list : %p", data);
	}

	DBG("noti[%p] data[%p] added listbox[%p]",
			data, noti_list_item, g_noti_listbox);
}

static void _quickpanel_noti_noti_add(Evas_Object *list, void *data, int is_prepend)
{
	notification_h noti = data;
	notification_ly_type_e layout = NOTIFICATION_LY_NOTI_EVENT_SINGLE;
	Evas_Object *noti_box = NULL;

	retif(list == NULL, , "Invalid parameter!");

	if (g_noti_section == NULL) {
		_quickpanel_noti_section_add();
	}

	if (noti != NULL) {
		notification_get_layout(noti, &layout);
		Evas_Object *noti_box = noti_box_create(g_noti_gridbox, layout);

		if (noti_box != NULL) {
			noti_node_item *item = noti_node_add(g_noti_node, (void*)data, (void*)noti_box);
			if (item != NULL) {
				noti_box_node_set(noti_box, item);
				noti_box_set_item_selected_cb(noti_box, quickpanel_notibox_select_cb);
				noti_box_set_item_button_1_cb(noti_box, quickpanel_notibox_button_1_cb);
				noti_box_set_item_deleted_cb(noti_box, quickpanel_notibox_delete_cb);
				gridbox_add_item(g_noti_gridbox, noti_box, is_prepend);
			}
		} else
			ERR("fail to insert item to list : %p", data);
	}

	int noti_count =
			noti_node_get_item_count(g_noti_node, NOTIFICATION_TYPE_NOTI);

	if (g_noti_section != NULL) {
		noti_section_update(g_noti_section, noti_count);
	}

	DBG("noti[%p] view[%p] added gridbox[%p]",
			data, noti_box, g_noti_gridbox);
}

static void _quickpanel_noti_update_notilist(struct appdata *ad)
{
	Evas_Object *list = NULL;
	notification_h noti = NULL;
	notification_h noti_save = NULL;
	notification_list_h get_list = NULL;
	int applist = NOTIFICATION_DISPLAY_APP_ALL;

	DBG("");

	retif(ad == NULL, , "Invalid parameter!");

	list = ad->list;
	retif(list == NULL, , "Failed to get noti genlist.");

	_quickpanel_noti_clear_list_all();

	notification_get_list(NOTIFICATION_TYPE_ONGOING, -1, &get_list);
	while (get_list != NULL) {
		noti = notification_list_get_data(get_list);
		notification_get_display_applist(noti, &applist);

		if (applist &
		    NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {
			notification_clone(noti, &noti_save);
			_quickpanel_noti_ongoing_add(list, noti_save, LISTBOX_APPEND);
		}
		get_list = notification_list_get_next(get_list);
	}
	notification_free_list(get_list);

	notification_get_list(NOTIFICATION_TYPE_NOTI , -1, &get_list);
	while (get_list != NULL) {
		noti = notification_list_get_data(get_list);
		notification_get_display_applist(noti, &applist);

		if (applist &
		    NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {
			notification_clone(noti, &noti_save);
			_quickpanel_noti_noti_add(list, noti_save, GRIDBOX_APPEND);
		}
		get_list = notification_list_get_next(get_list);
	}
	notification_free_list(get_list);

	if (g_noti_gridbox != NULL) {
		elm_box_recalculate(g_noti_gridbox);
	}
}

static void _quickpanel_noti_delete_volatil_data(void)
{
	notification_list_h noti_list = NULL;
	notification_list_h noti_list_head = NULL;
	notification_h noti = NULL;
	int property = 0;

	notification_get_grouping_list(NOTIFICATION_TYPE_NONE, -1, &noti_list);

	noti_list_head = noti_list;

	while (noti_list != NULL) {
		noti = notification_list_get_data(noti_list);
		notification_get_property(noti, &property);

		if (property & NOTIFICATION_PROP_VOLATILE_DISPLAY) {
			notification_set_property(noti,
				property |
				NOTIFICATION_PROP_DISABLE_UPDATE_ON_DELETE);
			notification_delete(noti);
		}

		noti_list = notification_list_get_next(noti_list);
	}

	notification_free_list(noti_list_head);

	notification_update(NULL);
}

inline static void _print_debuginfo_from_noti(notification_h noti) {
	retif(noti == NULL, , "Invalid parameter!");

	char *noti_pkgname = NULL;
	char *noti_launch_pkgname = NULL;
	notification_type_e noti_type = NOTIFICATION_TYPE_NONE;

	notification_get_pkgname(noti, &noti_pkgname);
	notification_get_application(noti, &noti_launch_pkgname);
	notification_get_type(noti, &noti_type);

	if (noti_pkgname != NULL) {
		ERR("pkg:%s", noti_pkgname);
	}
	if (noti_launch_pkgname != NULL) {
		ERR("pkgl:%s", noti_launch_pkgname);
	}

	ERR("type:%d", noti_type);
}

static void _quickpanel_noti_detailed_changed_cb(void *data, notification_type_e type, notification_op *op_list, int num_op)
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

	retif(data == NULL, , "Invalid parameter!");
	ad = data;

	ERR("num_op:%d", num_op);

	for (i = 0; i < num_op; i++) {
		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_TYPE, &op_type);
		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_PRIV_ID, &priv_id);
		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_NOTI, &noti_from_master);

		ERR("noti operation:%d privid:%d", op_type, priv_id);

		if (op_type == NOTIFICATION_OP_INSERT) {

			if (noti_from_master == NULL) {
				ERR("failed to get a notification from master");
				continue;
			}
			if (notification_clone(noti_from_master, &noti_new) != NOTIFICATION_ERROR_NONE) {
				ERR("failed to create a cloned notification");
				continue;
			}

			_print_debuginfo_from_noti(noti_new);
#ifdef QP_SERVICE_NOTI_LED_ENABLE
			quickpanel_service_noti_led_on(noti_new);
#endif

			notification_get_type(noti_new, &noti_type);
			notification_get_display_applist(noti_new, &noti_applist);
			notification_get_layout(noti_new, &noti_layout);

			if (noti_applist & NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {
				noti_node_item *node = noti_node_get(g_noti_node, priv_id);
				if (node != NULL) {
					if (noti_type == NOTIFICATION_TYPE_NOTI) {
						DBG("cb after inserted:%d", priv_id);
					}
					notification_free(noti_new);
				} else {
					if (noti_type == NOTIFICATION_TYPE_NOTI) {
						_quickpanel_noti_noti_add(ad->list, noti_new, GRIDBOX_PREPEND);
					} else if (noti_type == NOTIFICATION_TYPE_ONGOING) {
						_quickpanel_noti_ongoing_add(ad->list, noti_new, LISTBOX_PREPEND);
					}
				}
			} else {
				notification_free(noti_new);
			}
		} else if (op_type == NOTIFICATION_OP_DELETE) {
			noti_node_item *node = noti_node_get(g_noti_node, priv_id);

			if (node != NULL && node->noti != NULL) {
				notification_h noti = node->noti;
				notification_get_type(noti, &noti_type);

#ifdef QP_SERVICE_NOTI_LED_ENABLE
				quickpanel_service_noti_led_off(noti);
#endif
				_print_debuginfo_from_noti(noti);

				if (noti_type == NOTIFICATION_TYPE_NOTI) {
					gridbox_remove_item(g_noti_gridbox, node->view, 0);
				} else if (noti_type == NOTIFICATION_TYPE_ONGOING) {
					listbox_remove_item(g_noti_listbox, node->view, 0);
				}
				noti_node_remove(g_noti_node, priv_id);
			}
		} else if (op_type == NOTIFICATION_OP_UPDATE) {
			noti_node_item *node = noti_node_get(g_noti_node, priv_id);
			notification_h old_noti = NULL;

			if (noti_from_master == NULL) {
				ERR("failed to get a notification from master");
				continue;
			}
			if (notification_clone(noti_from_master, &noti_new) != NOTIFICATION_ERROR_NONE) {
				ERR("failed to create a cloned notification");
				continue;
			}

#ifdef QP_SERVICE_NOTI_LED_ENABLE
			quickpanel_service_noti_led_on(noti_new);
#endif
			_print_debuginfo_from_noti(noti_new);

			if (node != NULL && node->view != NULL && node->noti != NULL) {
				notification_get_type(noti_new, &noti_type);

				if (noti_type == NOTIFICATION_TYPE_NOTI) {
					gridbox_remove_item(g_noti_gridbox, node->view, 0);
					_quickpanel_noti_noti_add(ad->list, noti_new, GRIDBOX_PREPEND);
/*
					gridbox_remove_and_add_item(g_noti_gridbox, node->view,
							_quickpanel_noti_noti_add
							,ad->list, noti_new, GRIDBOX_PREPEND);
*/
				} else if (noti_type == NOTIFICATION_TYPE_ONGOING) {
					old_noti = node->noti;
					node->noti = noti_new;

					listbox_update_item(g_noti_listbox, node->view);
				}

				if (old_noti != NULL) {
					notification_free(old_noti);
				}
			} else {
				notification_get_display_applist(noti_new, &noti_applist);

				if (noti_applist & NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {
					if (noti_type == NOTIFICATION_TYPE_NOTI) {
						_quickpanel_noti_noti_add(ad->list, noti_new, GRIDBOX_PREPEND);
					} else if (noti_type == NOTIFICATION_TYPE_ONGOING) {
						_quickpanel_noti_ongoing_add(ad->list, noti_new, GRIDBOX_PREPEND);
					}
				} else {
					notification_free(noti_new);
				}
			}
		} else if (op_type == NOTIFICATION_OP_REFRESH) {
			_quickpanel_noti_update_notilist(ad);
		} else if (op_type == NOTIFICATION_OP_SERVICE_READY) {
			_quickpanel_noti_update_notilist(ad);

#ifdef QP_SERVICE_NOTI_LED_ENABLE
			quickpanel_service_noti_init(ad);
			quickpanel_service_noti_led_on(NULL);
#endif
		}
	}

	int noti_count = 0;

	if ((noti_count = noti_node_get_item_count(g_noti_node, NOTIFICATION_TYPE_NOTI))
			<= 0) {
		_quickpanel_noti_clear_notilist();
		_quickpanel_noti_section_remove();
	} else {
		if (g_noti_section != NULL) {
			noti_section_update(g_noti_section, noti_count);
		}
	}

	ERR("current noti count:%d", noti_count);
}

static void _quickpanel_noti_update_desktop_cb(keynode_t *node, void *data)
{
	char *event = NULL;
	char type[10] = {0,};
	char package[1024] = {0,};

	event = vconf_get_str(vconf_keynode_get_name(node));
	retif(NULL == event, , "invalid event");

	DBG("%s", event);

	if (sscanf(event, "%10[^:]:%1023s", type, package) != 2) {
		DBG("Failed to parse the event format : [%s], [%s]", type, package);
	}

	if (strncasecmp(type, "delete", strlen(type)) == 0) {
		notification_delete_all_by_type(package, NOTIFICATION_TYPE_NONE);
	}

	if (event != NULL)
		free(event);
}

static void _quickpanel_noti_update_sim_status_cb(keynode_t *node, void *data)
{
	struct appdata *ad = data;

	if (ad != NULL && ad->list != NULL) {
		_quickpanel_noti_update_notilist(ad);
	}
}

static int _quickpanel_noti_register_event_handler(struct appdata *ad)
{
	int ret = 0;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	/* Add dbus signal */
	e_dbus_init();
	ad->dbus_connection = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (ad->dbus_connection == NULL) {
		ERR("noti register : failed to get dbus bus");
		return -1;
	}

	ad->dbus_handler_size =
		e_dbus_signal_handler_add(ad->dbus_connection, NULL,
			QP_NOTI_ONGOING_DBUS_PATH,
			QP_NOTI_ONGOING_DBUS_INTERFACE, "update_progress",
			_quickpanel_noti_item_progress_update_cb,
			ad);
	if (ad->dbus_handler_size == NULL)
		ERR("fail to add size signal");

	ad->dbus_handler_progress =
		e_dbus_signal_handler_add(ad->dbus_connection, NULL,
			QP_NOTI_ONGOING_DBUS_PATH,
			QP_NOTI_ONGOING_DBUS_INTERFACE, "update_size",
			_quickpanel_noti_item_size_update_cb,
			ad);
	if (ad->dbus_handler_progress == NULL)
		ERR("fail to add progress signal");

	ad->dbus_handler_content =
		e_dbus_signal_handler_add(ad->dbus_connection, NULL,
			QP_NOTI_ONGOING_DBUS_PATH,
			QP_NOTI_ONGOING_DBUS_INTERFACE, "update_content",
			_quickpanel_noti_item_content_update_cb,
			ad);
	if (ad->dbus_handler_content == NULL)
		ERR("fail to add content signal");

	/* Notify vconf key */
	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,
				       _quickpanel_noti_update_sim_status_cb,
				       (void *)ad);
	if (ret != 0)
		ERR("Failed to register SIM_SLOT change callback!");

	/* Notify vconf key */
	ret = vconf_notify_key_changed(VCONFKEY_MENUSCREEN_DESKTOP,
				       _quickpanel_noti_update_desktop_cb,
				       (void *)ad);
	if (ret != 0)
		ERR("Failed to register DESKTOP change callback!");

	/* Register notification changed cb */
	notification_register_detailed_changed_cb(_quickpanel_noti_detailed_changed_cb, ad);

	return ret;
}

static int _quickpanel_noti_unregister_event_handler(struct appdata *ad)
{
	int ret = 0;

	/* Unregister notification changed cb */
	notification_unregister_detailed_changed_cb(_quickpanel_noti_detailed_changed_cb, (void *)ad);

	/* Ignore vconf key */
	ret = vconf_ignore_key_changed(VCONFKEY_MENUSCREEN_DESKTOP,
			_quickpanel_noti_update_desktop_cb);
	if (ret != 0)
		ERR("Failed to ignore DESKTOP change callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,
				_quickpanel_noti_update_sim_status_cb);
	if (ret != 0)
		ERR("Failed to ignore SIM_SLOT change callback!");

	/* Delete dbus signal */
	if (ad->dbus_handler_size != NULL) {
		e_dbus_signal_handler_del(ad->dbus_connection,
				ad->dbus_handler_size);
		ad->dbus_handler_size = NULL;
	}
	if (ad->dbus_handler_progress != NULL) {
		e_dbus_signal_handler_del(ad->dbus_connection,
				ad->dbus_handler_progress);
		ad->dbus_handler_progress = NULL;
	}
	if (ad->dbus_handler_content != NULL) {
		e_dbus_signal_handler_del(ad->dbus_connection,
				ad->dbus_handler_content);
		ad->dbus_handler_content = NULL;
	}

	if (ad->dbus_connection != NULL) {
		e_dbus_connection_close(ad->dbus_connection);
		ad->dbus_connection = NULL;
	}

	return QP_OK;
}

static int _quickpanel_noti_check_first_start(void)
{
	int status = 0;
	int ret = 0;

	ret = vconf_get_bool(VCONFKEY_QUICKPANEL_STARTED, &status);
	if (ret == 0 && status == 0) {
		/* reboot */
		ret = vconf_set_bool(VCONFKEY_QUICKPANEL_STARTED, 1);
		INFO("set : %s, result : %d", VCONFKEY_QUICKPANEL_STARTED, ret);
	}

	if (status)
		return 0;

	return 1;
}

static void _quickpanel_noti_init(void *data)
{
	struct appdata *ad = NULL;

	retif(data == NULL, , "Invalid parameter!");
	ad = data;

	retif(ad->list == NULL, , "Invalid parameter!");

	DBG("wr");

	if (g_noti_listbox == NULL) {
		g_noti_listbox = listbox_create(ad->list, quickpanel_get_app_data());
		listbox_set_item_deleted_cb(g_noti_listbox, _quickpanel_list_box_deleted_cb);
		quickpanel_list_util_sort_insert(ad->list, g_noti_listbox);
	}

	if (g_noti_gridbox == NULL) {
		g_noti_gridbox = gridbox_create(ad->list, quickpanel_get_app_data());
		gridbox_set_item_deleted_cb(g_noti_gridbox, _quickpanel_noti_box_deleted_cb);
		quickpanel_list_util_sort_insert(ad->list, g_noti_gridbox);
	}
}

static void _quickpanel_noti_fini(void *data)
{
	struct appdata *ad = NULL;

	retif(data == NULL, , "Invalid parameter!");
	ad = data;

	retif(ad->list == NULL, , "Invalid parameter!");

	DBG("dr");

	if (g_noti_listbox != NULL) {
		quickpanel_list_util_item_unpack_by_object(ad->list
				, g_noti_listbox);
		listbox_remove(g_noti_listbox);
		g_noti_listbox = NULL;
	}

	if (g_noti_gridbox != NULL) {
		quickpanel_list_util_item_unpack_by_object(ad->list
				, g_noti_gridbox);
		gridbox_remove(g_noti_gridbox);
		g_noti_gridbox = NULL;
	}
}

static void _quickpanel_noti_cleanup(void *data) {
	notifiation_clear(NOTIFICATION_TYPE_ONGOING);
	_quickpanel_noti_delete_volatil_data();
}

static int quickpanel_noti_init(void *data)
{
	struct appdata *ad = data;
	int is_first = 0;

	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	noti_node_create(&g_noti_node);

	is_first = _quickpanel_noti_check_first_start();
	if (is_first) {
		if (notification_is_service_ready()) {
			_quickpanel_noti_cleanup(ad);
		} else {
			notification_add_deffered_task(_quickpanel_noti_cleanup, ad);
		}
	}

	_quickpanel_noti_init(ad);

	_quickpanel_noti_register_event_handler(ad);

	return QP_OK;
}

static int quickpanel_noti_fini(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

#ifdef QP_SERVICE_NOTI_LED_ENABLE
	quickpanel_service_noti_fini(ad);
	quickpanel_service_noti_led_off(NULL);
#endif

	/* Unregister event handler */
	_quickpanel_noti_unregister_event_handler(data);

	_quickpanel_noti_clear_list_all();

	_quickpanel_noti_fini(ad);

	if (g_noti_node != NULL) {
		noti_node_destroy(&g_noti_node);
	}

	return QP_OK;
}

static int quickpanel_noti_suspend(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	return QP_OK;
}

static int quickpanel_noti_resume(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	if (ad->list) {
		listbox_update(g_noti_listbox);
	}

	return QP_OK;
}

static void quickpanel_noti_refresh(void *data) {
	struct appdata *ad = NULL;

	retif(data == NULL, , "Invalid parameter!");
	ad = data;

	if (g_noti_gridbox != NULL) {
		gridbox_rotation(g_noti_gridbox, ad->angle);
	}
}

static void quickpanel_noti_lang_changed(void *data)
{
	struct appdata *ad = data;

	retif(ad == NULL, , "Invalid parameter!");

	_quickpanel_noti_update_notilist(ad);
}
