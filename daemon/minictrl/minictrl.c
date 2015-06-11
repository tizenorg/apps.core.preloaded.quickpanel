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


#include <glib.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>
#include <string.h>
#include "common.h"
#include "quickpanel-ui.h"
#include "quickpanel_def.h"
#include "list_util.h"
#include "quickpanel_debug_util.h"
#ifdef QP_SCREENREADER_ENABLE
#include "accessibility.h"
#endif
#include "minictrl.h"
#include "vi_manager.h"

#define MINICONTROL_TYPE_STR_VIEWER "::[viewer="
#define MINICONTROL_TYPE_STR_QUICKPANEL "QUICKPANEL"
#define MINICONTROL_TYPE_STR_LOCKSCREEN "LOCKSCREEN"
#define MINICONTROL_TYPE_STR_ONGOING "_ongoing]"

static Eina_Bool _anim_init_cb(void *data);
static Eina_Bool _anim_job_cb(void *data);
static Eina_Bool _anim_done_cb(void *data);
static int _init(void *data);
static int _fini(void *data);
static int _suspend(void *data);
static int _resume(void *data);

QP_Module minictrl = {
	.name = "minictrl",
	.init = _init,
	.fini = _fini,
	.suspend = _suspend,
	.resume = _resume,
	.hib_enter = NULL,
	.hib_leave = NULL,
	.lang_changed = NULL,
	.refresh = NULL,
	.get_height = NULL,
};

struct _viewer_item {
	char *name;
	unsigned int width;
	unsigned int height;
	minicontrol_priority_e priority;
	Evas_Object *viewer;
	void *data;
};

static void _minictrl_resize_vi(Evas_Object *list,
					struct _viewer_item *item, int to_w, int to_h);

GHashTable *g_prov_table;

static int _viewer_check(const char *name)
{
	char *pos_start = NULL;
	retif(!name, 0, "name is NULL");

	if ((pos_start = strstr(name, MINICONTROL_TYPE_STR_VIEWER)) != NULL) {
		if (strstr(pos_start, MINICONTROL_TYPE_STR_QUICKPANEL) != NULL) {
			return 1;
		} else {
			return 0;
		}
	} else if (strstr(name, MINICONTROL_TYPE_STR_LOCKSCREEN) != NULL) {
		return 0;
	}

	return 1;
}

static void _viewer_freeze(Evas_Object *viewer)
{
	int freezed_count = 0;
	retif(viewer == NULL, , "Invalid parameter!");

	freezed_count = elm_object_scroll_freeze_get(viewer);

	if (freezed_count <= 0) {
		elm_object_scroll_freeze_push(viewer);
	}
}

static void _viewer_unfreeze(Evas_Object *viewer)
{
	int i = 0, freezed_count = 0;
	retif(viewer == NULL, , "Invalid parameter!");

	freezed_count = elm_object_scroll_freeze_get(viewer);

	for (i = 0 ; i < freezed_count; i++) {
		elm_object_scroll_freeze_pop(viewer);
	}
}

static Evas_Object *_get_minictrl_obj(Evas_Object *layout)
{
	retif(layout == NULL, NULL, "Invalid parameter!");

	return elm_object_part_content_get(layout, "elm.icon");
}

static void _viewer_set_size(Evas_Object *layout, void *data, int width, int height)
{
	Evas_Object *viewer = NULL;
	retif(layout == NULL, , "Invalid parameter!");
	retif(data == NULL, , "Invalid parameter!");
	retif(width < 0, , "Invalid parameter!");
	retif(height < 0, , "Invalid parameter!");
	struct appdata *ad = data;
	int max_width = 0;
	int resized_width = 0;
	int is_landscape = 0;

	viewer = _get_minictrl_obj(layout);
	retif(viewer == NULL, , "Invalid parameter!");

	is_landscape = (width > ad->win_width) ? 1 : 0;

	if (is_landscape) {
		max_width = (ad->scale * ad->win_height);
	} else {
		max_width = (ad->scale * ad->win_width);
	}
	resized_width = (width > max_width) ? max_width : width;

	SERR("minicontroller view is resized to w:%d(%d) h:%d Landscape[%d]", resized_width, width, height, is_landscape);

	evas_object_size_hint_min_set(viewer, resized_width, height);
	evas_object_size_hint_max_set(viewer, resized_width, height);
}

static void _viewer_item_free(struct _viewer_item *item)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "Invalid parameter!");
	retif(ad->list == NULL, , "Invalid parameter!");
	retif(item == NULL, , "Invalid parameter!");

	if (item->name) {
		free(item->name);
	}

	if (item->viewer) {
		quickpanel_list_util_item_unpack_by_object(ad->list, item->viewer, 0, 0);
		quickpanel_list_util_item_del_tag(item->viewer);
		if (item->viewer != NULL) {
			evas_object_del(item->viewer);
			item->viewer = NULL;
		}
	}

	free(item);
}

static Evas_Object *_minictrl_create_view(struct appdata *ad, const char *name)
{
	retif(ad == NULL, NULL, "Invalid parameter!");
	retif(ad->list == NULL, NULL, "Invalid parameter!");
	retif(name == NULL, NULL, "Invalid parameter!");

	Evas_Object *layout = NULL;

	layout = elm_layout_add(ad->list);

	elm_layout_file_set(layout, DEFAULT_EDJ,
			"quickpanel/minictrl/default");

	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	Evas_Object *viewer = minicontrol_viewer_add(layout, name);
	if (!viewer) {
		ERR("fail to add viewer - %s", name);
		if (layout) {
			evas_object_del(layout);
		}
		return NULL;
	}
	elm_object_focus_allow_set(viewer, EINA_TRUE);
	elm_object_part_content_set(layout, "elm.icon", viewer);

	Evas_Object *focus = quickpanel_accessibility_ui_get_focus_object(layout);
	elm_object_part_content_set(layout, "focus", focus);

#ifdef QP_SCREENREADER_ENABLE
	Evas_Object *ao = quickpanel_accessibility_screen_reader_object_get(layout,
			SCREEN_READER_OBJ_TYPE_ELM_OBJECT, "focus", layout);

	if (ao != NULL) {
		elm_access_info_cb_set(ao, ELM_ACCESS_TYPE, quickpanel_accessibility_info_cb,
				_NOT_LOCALIZED("Mini controller"));
	}
#endif

	return layout;
}

static int _minictrl_is_ongoing(const char *str)
{
	if (str == NULL) {
		return 0;
	}

	if (strstr(str, MINICONTROL_TYPE_STR_ONGOING) != NULL) {
		return 1;
	} else {
		return 0;
	}
}

static qp_item_type_e _minictrl_priority_to_type(minicontrol_priority_e priority)
{
	qp_item_type_e type;

	switch (priority) {
	case MINICONTROL_PRIORITY_TOP:
		type = QP_ITEM_TYPE_MINICTRL_TOP;
		break;
	case MINICONTROL_PRIORITY_MIDDLE:
		type = QP_ITEM_TYPE_MINICTRL_MIDDLE;
		break;
	case MINICONTROL_PRIORITY_LOW:
	default:
		type = QP_ITEM_TYPE_MINICTRL_LOW;
		break;
	}

	return type;
}

static void _minictrl_release_cb(void *data, Evas *e,
		Evas_Object *obj, void *event_info)
{
	struct appdata *ad;
	retif(!data, , "data is NULL");
	ad = data;

	_viewer_unfreeze(ad->scroller);
}

static void _minictrl_add(const char *name, unsigned int width,
				unsigned int height,
				minicontrol_priority_e priority,
				void *data)
{
	qp_item_data *qid = NULL;
	struct _viewer_item *vit = NULL;
	qp_item_type_e type;
	struct appdata *ad;
	Evas_Object *viewer = NULL;

	retif(!name, , "name is NULL");
	retif(!data, , "data is NULL");

	ad = data;
	retif(!ad->list, , "list is NULL");

	if (g_prov_table) {
		struct _viewer_item *found = NULL;
		found = g_hash_table_lookup(g_prov_table, name);

		if (found) {
			ERR("already have it : %s", name);
			return;
		}
	} else {
		ERR("g_prov_table is NULL");
		return;
	}

	/* elm_plug receives 'server_del' event,
	 * if it repeats connect and disconnect frequently.
	 *
	 */
	viewer = _minictrl_create_view(ad, name);
	if (!viewer) {
		ERR("Failed to create view[%s]", name);
		return;
	}
	_viewer_set_size(viewer, ad, width, height);
	quickpanel_uic_initial_resize(viewer,
			(height > QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT)
			? height : QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT);

	evas_object_event_callback_add(_get_minictrl_obj(viewer), EVAS_CALLBACK_MOUSE_UP,
			_minictrl_release_cb, ad);

	vit = malloc(sizeof(struct _viewer_item));
	if (!vit) {
		ERR("fail to alloc vit");
		if (viewer != NULL) {
			evas_object_del(viewer);
			viewer = NULL;
		}
		return;
	}

	if (_minictrl_is_ongoing(name) == 1) {
		type = QP_ITEM_TYPE_MINICTRL_ONGOING;
	} else {
		type = _minictrl_priority_to_type(priority);
	}
	qid = quickpanel_list_util_item_new(type, vit);
	if (!qid) {
		ERR("fail to alloc vit");
		if (viewer != NULL) {
			evas_object_del(viewer);
			viewer = NULL;
		}
		free(vit);
		return;
	}
	vit->name = strdup(name);
	vit->width = width;
	vit->height = height;
	vit->priority = priority;
	vit->viewer = viewer;
	vit->data = data;
	quickpanel_list_util_item_set_tag(vit->viewer, qid);
	quickpanel_list_util_sort_insert(ad->list, vit->viewer);

	g_hash_table_insert(g_prov_table, g_strdup(name), vit);

	DBG("success to add minicontrol %s", name);
	quickpanel_minictrl_rotation_report(ad->angle);
}

static void _minictrl_remove(const char *name, void *data)
{
	if (g_prov_table) {
		if (g_hash_table_remove(g_prov_table, name)) {
			DBG("success to remove %s", name);

			retif(data == NULL, , "data is NULL");
		} else {
			WARN("unknown provider name : %s", name);
		}
	}
}

static void _minictrl_update(const char *name, unsigned int width,
				unsigned int height, void *data)
{
	int old_h = 0;
	struct appdata *ad = data;
	struct _viewer_item *found = NULL;

	retif(!g_prov_table, , "data is NULL");
	retif(!ad, , "data is NULL");

	found = g_hash_table_lookup(g_prov_table, name);

	if (!found) {
		WARN("unknown provider name : %s", name);
		return;
	}

	old_h = found->height;

	if (found->viewer) {
		if (old_h != height) {
			_minictrl_resize_vi(ad->list,
				found, width, height);
		} else {
			_viewer_set_size(found->viewer, ad, width, height);
			quickpanel_uic_initial_resize(found->viewer,
					(height > QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT)
					? height : QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT);
		}
	}
}

static void _minictrl_request(const char *name, int action, int value, void *data)
{
	struct appdata *ad = data;
	retif(!name, , "name is NULL");
	retif(!ad, , "data is NULL");

	SDBG("%s %d %d", name, action, value);

	if (action == MINICONTROL_REQ_HIDE_VIEWER) {
		quickpanel_uic_close_quickpanel(true, 0);
	} else if (action == MINICONTROL_REQ_FREEZE_SCROLL_VIEWER) {
		if (ad->list != NULL) {
			ERR("freezed by %s", name);
			_viewer_freeze(ad->scroller);
		}
	} else if (action == MINICONTROL_REQ_UNFREEZE_SCROLL_VIEWER) {
		if (ad->list != NULL) {
			ERR("unfreezed by %s", name);
			_viewer_unfreeze(ad->scroller);
		}
	} 
#ifdef HAVE_X
	else if (action == MINICONTROL_REQ_REPORT_VIEWER_ANGLE) {
		if (ad->list != NULL) {
			SERR("need to broadcasting angle by %s %d", name, action);
			quickpanel_minictrl_rotation_report(ad->angle);
		}
	}
#endif
}

static void _mctrl_monitor_cb(minicontrol_action_e action,
				const char *name, unsigned int width,
				unsigned int height,
				minicontrol_priority_e priority,
				void *data)
{
	retif(!data, , "data is NULL");
	retif(!name, , "name is NULL");

	if (_viewer_check(name) == 0) {
		ERR("%s: ignored", name);
		return;
	}

	switch (action) {
	case MINICONTROL_ACTION_START:
		_minictrl_add(name, width, height, priority, data);
		break;
	case MINICONTROL_ACTION_RESIZE:
		_minictrl_update(name, width, height, data);
		break;
	case MINICONTROL_ACTION_STOP:
		_minictrl_remove(name, data);
		break;
	case MINICONTROL_ACTION_REQUEST:
		_minictrl_request(name, width, height, data);
		break;
	default:
		break;
	}
}

static void _minictrl_resize_vi(Evas_Object *list,
					struct _viewer_item *item, int to_w, int to_h)
{
	QP_VI *vi = NULL;
	retif(list == NULL, , "invalid parameter");
	retif(item == NULL, , "invalid parameter");

	vi = quickpanel_vi_new_with_data(
			VI_OP_RESIZE,
			QP_ITEM_TYPE_MINICTRL_MIDDLE,
			list,
			item->viewer,
			_anim_init_cb,
			_anim_job_cb,
			_anim_done_cb,
			_anim_done_cb,
			vi,
			item,
			to_w,
			to_h);
	quickpanel_vi_start(vi);
}

static void _anim_init_resize(void *data)
{
	QP_VI *vi = data;
	retif(vi == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	Evas_Object *item = vi->target;
	evas_object_color_set(item, 0, 0, 0, 0);
}

static void _reorder_transit_del_cb(void *data, Elm_Transit *transit)
{
	QP_VI *vi = data;
	int to_w = 0, to_h = 0;
	Evas_Object *item = NULL;
	retif(vi == NULL, , "data is NULL");
	retif(vi->target == NULL, , "invalid parameter");

	item = vi->target;
	to_w = vi->extra_flag_1;
	to_h = vi->extra_flag_2;

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "Invalid parameter!");

	_viewer_set_size(item, ad, to_w, to_h);
	quickpanel_uic_initial_resize(item,
			(to_h > QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT)
			? to_h : QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT);
}

static void _anim_job_resize(void *data)
{
	QP_VI *vi = data;
	int to_w = 0, to_h = 0;
	Elm_Transit *transit_layout_parent = NULL;
	Elm_Transit *transit_fadein = NULL;
	Evas_Object *item = NULL;
	struct _viewer_item *viewer_item = NULL;

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "Invalid parameter!");
	retif(vi == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");
	retif(vi->extra_data_2 == NULL, , "invalid parameter");

	item = vi->target;
	to_w = vi->extra_flag_1;
	to_h = vi->extra_flag_2;
	viewer_item = vi->extra_data_2;

	transit_layout_parent = quickpanel_list_util_get_reorder_transit(viewer_item->viewer, NULL, to_h - viewer_item->height);
	if (transit_layout_parent != NULL) {
		elm_transit_del_cb_set(transit_layout_parent, _reorder_transit_del_cb, vi);
	} else {
		_viewer_set_size(item, ad, to_w, to_h);
		quickpanel_uic_initial_resize(item,
				(to_h > QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT)
				? to_h : QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT);
	}

	transit_fadein = elm_transit_add();
	if (transit_fadein != NULL) {
		elm_transit_object_add(transit_fadein, item);
		elm_transit_effect_color_add(transit_fadein, 0, 0, 0, 0, 255, 255, 255, 255);
		elm_transit_duration_set(transit_fadein, 0.35);
		elm_transit_tween_mode_set(transit_fadein,
				quickpanel_vim_get_tweenmode(VI_OP_INSERT));
		elm_transit_del_cb_set(transit_fadein, quickpanel_vi_done_cb_for_transit, vi);
		elm_transit_objects_final_state_keep_set(transit_fadein, EINA_TRUE);

		if (transit_layout_parent != NULL) {
			elm_transit_chain_transit_add(transit_layout_parent, transit_fadein);
			elm_transit_go(transit_layout_parent);
		} else {
			elm_transit_go(transit_fadein);
		}
	} else {
		ERR("Failed to create all the transit");
		quickpanel_vi_done(vi);
	}
}

static void _anim_done_resize(void *data)
{
	QP_VI *vi = data;
	struct _viewer_item *viewer_item = NULL;
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "Invalid parameter!");
	retif(vi == NULL, , "invalid parameter");
	retif(vi->target == NULL, , "invalid parameter");

	Evas_Object *item = vi->target;
	viewer_item = vi->extra_data_2;

	viewer_item->width = vi->extra_flag_1;
	viewer_item->height = vi->extra_flag_2;

	_viewer_set_size(item, ad, viewer_item->width, viewer_item->height);
	quickpanel_uic_initial_resize(item,
			(viewer_item->height > QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT)
			? viewer_item->height : QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT);
	evas_object_color_set(item, 255, 255, 255, 255);
}

static Eina_Bool _anim_init_cb(void *data)
{
	QP_VI *vi = data;
	retif(vi == NULL, EINA_FALSE, "invalid parameter");

	static qp_vi_op_table anim_init_table[] = {
		{
			.op_type = VI_OP_RESIZE,
			.handler = _anim_init_resize,
		},
		{
			.op_type = VI_OP_NONE,
			.handler = NULL,
		},
	};

	int i = 0;
	for (i = 0; anim_init_table[i].op_type != VI_OP_NONE; i++) {
		if (anim_init_table[i].op_type != vi->op_type) {
			continue;
		}

		anim_init_table[i].handler(vi);
		break;
	}

	return EINA_TRUE;
}

static Eina_Bool _anim_job_cb(void *data)
{
	QP_VI *vi = data;
	retif(vi == NULL, EINA_FALSE, "invalid parameter");

	static qp_vi_op_table anim_job_table[] = {
		{
			.op_type = VI_OP_RESIZE,
			.handler = _anim_job_resize,
		},
		{
			.op_type = VI_OP_NONE,
			.handler = NULL,
		},
	};

	int i = 0;
	for (i = 0; anim_job_table[i].op_type != VI_OP_NONE; i++) {
		if (anim_job_table[i].op_type != vi->op_type) {
			continue;
		}

		anim_job_table[i].handler(vi);
		break;
	}

	return EINA_TRUE;
}

static Eina_Bool _anim_done_cb(void *data)
{
	QP_VI *vi = data;
	retif(vi == NULL, EINA_FALSE, "invalid parameter");

	static qp_vi_op_table anim_done_table[] = {
		{
			.op_type = VI_OP_RESIZE,
			.handler = _anim_done_resize,
		},
		{
			.op_type = VI_OP_NONE,
			.handler = NULL,
		},
	};

	int i = 0;
	for (i = 0; anim_done_table[i].op_type != VI_OP_NONE; i++) {
		if (anim_done_table[i].op_type != vi->op_type) {
			continue;
		}

		anim_done_table[i].handler(vi);
		break;
	}

	return EINA_TRUE;
}

static int _init(void *data)
{
	minicontrol_error_e ret;

	retif(!data, QP_FAIL, "Invalid parameter!");

	g_prov_table = g_hash_table_new_full(g_str_hash, g_str_equal,
					(GDestroyNotify)g_free,
					(GDestroyNotify)_viewer_item_free);

	ret = minicontrol_monitor_start(_mctrl_monitor_cb, data);
	if (ret != MINICONTROL_ERROR_NONE) {
		ERR("fail to minicontrol_monitor_start()- %d", ret);
		return QP_FAIL;
	}

	return QP_OK;
}

static int _fini(void *data)
{
	minicontrol_error_e ret;
	ret = minicontrol_monitor_stop();
	if (ret != MINICONTROL_ERROR_NONE) {
		ERR("fail to minicontrol_monitor_stop()- %d", ret);
	}

	if (g_prov_table) {
		g_hash_table_remove_all(g_prov_table);
		g_prov_table = NULL;
	}

	return QP_OK;
}

static int _suspend(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	if (ad->list != NULL) {
		_viewer_unfreeze(ad->scroller);
	}

	return QP_OK;
}

static int _resume(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	if (ad->list != NULL) {
		_viewer_unfreeze(ad->scroller);
	}

	return QP_OK;
}

HAPI void quickpanel_minictrl_rotation_report(int angle)
{
	if (g_prov_table != NULL) {
		if (g_hash_table_size(g_prov_table) > 0) {
			SINFO("minicontrol rotation:%d", angle);
#ifdef HAVE_X
			minicontrol_viewer_request(QP_PKG_QUICKPANEL,
					MINICONTROL_REQ_ROTATE_PROVIDER, angle);
#endif
		}
	}
}
