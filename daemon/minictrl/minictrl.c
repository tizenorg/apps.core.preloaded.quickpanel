/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the License);
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

#include <glib.h>
#include <Ecore_X.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>
#include <string.h>
#include "common.h"
#include "quickpanel-ui.h"
#include "list_util.h"
#include "quickpanel_debug_util.h"

#define QP_R_MARGIN 12
#define MINICONTROL_WIDTH_P_MAX 692
#define MINICONTROL_WIDTH_L_MAX 1252

#define MINICONTROL_TYPE_STR_ONGOING "_ongoing]"

static int quickpanel_minictrl_init(void *data);
static int quickpanel_minictrl_fini(void *data);
static unsigned int quickpanel_minictrl_get_height(void *data);
static int quickpanel_minictrl_suspend(void *data);
static int quickpanel_minictrl_resume(void *data);

QP_Module minictrl = {
	.name = "minictrl",
	.init = quickpanel_minictrl_init,
	.fini = quickpanel_minictrl_fini,
	.suspend = quickpanel_minictrl_suspend,
	.resume = quickpanel_minictrl_resume,
	.hib_enter = NULL,
	.hib_leave = NULL,
	.lang_changed = NULL,
	.refresh = NULL,
	.get_height = quickpanel_minictrl_get_height,
};

struct _viewer_item {
	char *name;
	unsigned int width;
	unsigned int height;
	minicontrol_priority_e priority;
	Evas_Object *viewer;
	Elm_Object_Item *it;
	void *data;
};

GHashTable *g_prov_table;

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

static void _viewer_set_size(Evas_Object *viewer, void *data, int width, int height)
{
	retif(viewer == NULL, , "Invalid parameter!");
	retif(data == NULL, , "Invalid parameter!");
	retif(width < 0, , "Invalid parameter!");
	retif(height < 0, , "Invalid parameter!");
	struct appdata *ad = data;
	int max_width = 0;
	int resized_width = 0;

	if (ad->angle == 90 || ad->angle == 270) {
		max_width = (ad->scale * MINICONTROL_WIDTH_L_MAX) - 1;
	} else {
		max_width = (ad->scale * MINICONTROL_WIDTH_P_MAX) - 1;
	}
	resized_width = (width > max_width) ? max_width : width;
	evas_object_size_hint_min_set(viewer, resized_width, height);
}

static void _viewer_item_free(struct _viewer_item *item)
{
	if (!item)
		return;

	if (item->name)
		free(item->name);

	if (item->it)
		elm_object_item_del(item->it);

	if (item->viewer) {
		evas_object_unref(item->viewer);
		evas_object_del(item->viewer);
	}

	free(item);
}

#if 0
static Evas_Object *_minictrl_load_viewer(Evas_Object *parent,
					struct _viewer_item *item)
{
	Evas_Object *viewer = NULL;

	if (!parent) {
		ERR("parent is NULL");
		return NULL;
	}

	if (!item) {
		ERR("item is NULL");
		return NULL;
	}

	if (!item->name) {
		ERR("item name is NULL");
		return NULL;
	}

	viewer = minicontrol_viewer_add(parent, item->name);
	if (!viewer) {
		ERR("fail to create viewer for [%s]", item->name);
		return NULL;
	}

	evas_object_size_hint_min_set(viewer, item->width - QP_R_MARGIN , item->height);

	return viewer;
}
#endif

static Evas_Object *_minictrl_gl_get_content(void *data, Evas_Object * obj,
					const char *part)
{
	Evas_Object *content = NULL;
	qp_item_data *qid = NULL;
	struct _viewer_item *item = NULL;

	retif(data == NULL, NULL, "Invalid parameter!");
	qid = data;

	item = quickpanel_list_util_item_get_data(qid);
	retif(!item, NULL, "item is NULL");

	if (strcmp(part, "elm.icon") == 0)
		content = item->viewer;

	return content;
}


static Eina_Bool _minictrl_gl_get_state(void *data, Evas_Object *obj,
					const char *part)
{
	return EINA_FALSE;
}

static void _minictrl_gl_del(void *data, Evas_Object *obj)
{
	if (data) {
		quickpanel_list_util_del_count(data);
		free(data);
	}

	return;
}

static Elm_Genlist_Item_Class *_minictrl_gl_style_get(void)
{
	Elm_Genlist_Item_Class *itc = NULL;

	itc = elm_genlist_item_class_new();
	if (!itc) {
		ERR("fail to elm_genlist_item_class_new()");
		return NULL;
	}

	itc->item_style = "minicontrol/default";
	itc->func.text_get = NULL;
	itc->func.content_get = _minictrl_gl_get_content;
	itc->func.state_get = _minictrl_gl_get_state;
	itc->func.del = _minictrl_gl_del;

	return itc;
}

static int _minictrl_is_ongoing(const char *str)
{
	if (str == NULL) return 0;

	if (strstr(str, MINICONTROL_TYPE_STR_ONGOING) != NULL) {
		return 1;
	} else {
		return 0;
	}
}

qp_item_type_e _minictrl_priority_to_type(minicontrol_priority_e priority)
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
		Evas_Object *obj, void *event_info) {
	struct appdata *ad;
	retif(!data, , "data is NULL");
	ad = data;

	DBG("");
	_viewer_unfreeze(ad->list);
}

static void _minictrl_add(const char *name, unsigned int width,
				unsigned int height,
				minicontrol_priority_e priority,
				void *data)
{
	qp_item_data *qid = NULL;
	Elm_Genlist_Item_Class *itc = NULL;
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
	viewer = minicontrol_viewer_add(ad->list, name);
	if (!viewer) {
		ERR("fail to add viewer - %s", name);
		return;
	}
	evas_object_ref(viewer);
	_viewer_set_size(viewer, ad, width, height);

	itc = _minictrl_gl_style_get();
	if (!itc) {
		ERR("fail to _minictrl_gl_style_get()");
		evas_object_del(viewer);
		return;
	}

	evas_object_event_callback_add(viewer, EVAS_CALLBACK_MOUSE_UP,
			_minictrl_release_cb, ad);

	vit = malloc(sizeof(struct _viewer_item));
	if (!vit) {
		ERR("fail to alloc vit");
		evas_object_del(viewer);
		elm_genlist_item_class_free(itc);
		return;
	}


	if (_minictrl_is_ongoing(name) == 1) {
		DBG("QP_ITEM_TYPE_MINICTRL_ONGOING is added");
		type = QP_ITEM_TYPE_MINICTRL_ONGOING;
	} else {
		type = _minictrl_priority_to_type(priority);
	}
	qid = quickpanel_list_util_item_new(type, vit);
	if (!qid) {
		ERR("fail to alloc vit");
		evas_object_del(viewer);
		elm_genlist_item_class_free(itc);
		free(vit);
		return;
	}
	vit->name = strdup(name);
	vit->width = width;
	vit->height = height;
	vit->priority = priority;
	vit->viewer = viewer;
	vit->data = data;
	vit->it = quickpanel_list_util_sort_insert(ad->list, itc, qid, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);

	g_hash_table_insert(g_prov_table, g_strdup(name), vit);

	INFO("success to add %s", name);

	elm_genlist_item_class_free(itc);

	quickpanel_ui_update_height(ad);
}

static void _minictrl_remove(const char *name, void *data)
{
	if (g_prov_table) {
		if (g_hash_table_remove(g_prov_table, name))
		{
			INFO("success to remove %s", name);

			retif(data == NULL, , "data is NULL");
			quickpanel_ui_update_height(data);
		}
		else
			WARN("unknown provider name : %s", name);
	}
}

static void _minictrl_update(const char *name, unsigned int width,
				unsigned int height, void *data)
{
	struct _viewer_item *found = NULL;
	struct appdata *ad = NULL;

	if (!g_prov_table)
		return;

	retif(!data, , "data is NULL");
	ad = data;

	found = g_hash_table_lookup(g_prov_table, name);

	if (!found) {
		WARN("unknown provider name : %s", name);
		return;
	}

	found->width = width;
	found->height = height;

	if (found->viewer) {
		_viewer_set_size(found->viewer, ad, width, height);
	}

	if (found->it) {
		elm_genlist_item_update(found->it);
		quickpanel_ui_update_height(ad);
	}
}

static void _minictrl_request(const char *name, int action, void *data)
{
	struct appdata *ad = NULL;
	retif(!name, , "name is NULL");
	retif(!data, , "data is NULL");
	ad = data;

	if (action == MINICONTROL_REQ_HIDE_VIEWER) {
		quickpanel_close_quickpanel(true);
	}
	if (action == MINICONTROL_REQ_FREEZE_SCROLL_VIEWER) {
		if (ad->list != NULL) {
			ERR("freezed by %s", name);
			_viewer_freeze(ad->list);
		}
	}
	if (action == MINICONTROL_REQ_UNFREEZE_SCROLL_VIEWER) {
		if (ad->list != NULL) {
			ERR("unfreezed by %s", name);
			_viewer_unfreeze(ad->list);
		}
	}
}

static void _mctrl_monitor_cb(minicontrol_action_e action,
				const char *name, unsigned int width,
				unsigned int height,
				minicontrol_priority_e priority,
				void *data)
{
	retif(!data, , "data is NULL");
	retif(!name, , "name is NULL");

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
		_minictrl_request(name, width, data);
		break;
	default:
		break;
	}
}

static int quickpanel_minictrl_init(void *data)
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

static int quickpanel_minictrl_fini(void *data)
{
	minicontrol_error_e ret;
	ret = minicontrol_monitor_stop();
	if (ret != MINICONTROL_ERROR_NONE)
		ERR("fail to minicontrol_monitor_stop()- %d", ret);

	if (g_prov_table) {
		g_hash_table_remove_all(g_prov_table);
		g_prov_table = NULL;
	}

	return QP_OK;
}


static void _quickpanel_minictrl_hf_sum_height(gpointer key, gpointer value, gpointer data)
{
	struct _viewer_item *item = value;

	if (item != NULL && data != NULL) {
		*((unsigned int *)data) += item->height;
	}
}

static unsigned int quickpanel_minictrl_get_height(void *data)
{
	unsigned int height_minictrl = 0;

	if (g_prov_table != NULL) {
		g_hash_table_foreach(g_prov_table, _quickpanel_minictrl_hf_sum_height, &height_minictrl);
	}

	return height_minictrl;
}

static int quickpanel_minictrl_suspend(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	if (ad->list != NULL) {
		_viewer_unfreeze(ad->list);
	}

	return QP_OK;
}

static int quickpanel_minictrl_resume(void *data)
{
	int i = 0, freezed_count;
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	if (ad->list != NULL) {
		_viewer_unfreeze(ad->list);
	}

	return QP_OK;
}
