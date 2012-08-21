/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <glib.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>
#include "common.h"
#include "quickpanel-ui.h"
#include "list_util.h"

static int quickpanel_minictrl_init(void *data);
static int quickpanel_minictrl_fini(void *data);
static int quickpanel_minictrl_get_height(void *data);

QP_Module minictrl = {
	.name = "minictrl",
	.init = quickpanel_minictrl_init,
	.fini = quickpanel_minictrl_fini,
	.suspend = NULL,
	.resume = NULL,
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

	evas_object_size_hint_min_set(viewer, item->width , item->height);

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

	itc->item_style = "qp_item/default";
	itc->func.text_get = NULL;
	itc->func.content_get = _minictrl_gl_get_content;
	itc->func.state_get = _minictrl_gl_get_state;
	itc->func.del = _minictrl_gl_del;

	return itc;
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
	evas_object_size_hint_min_set(viewer, width, height);

	itc = _minictrl_gl_style_get();
	if (!itc) {
		ERR("fail to _minictrl_gl_style_get()");
		evas_object_del(viewer);
		return;
	}

	vit = malloc(sizeof(struct _viewer_item));
	if (!vit) {
		ERR("fail to alloc vit");
		evas_object_del(viewer);
		elm_genlist_item_class_free(itc);
		return;
	}

	type = _minictrl_priority_to_type(priority);
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

	if (found->viewer)
		evas_object_size_hint_min_set(found->viewer, width , height);

	if (found->it) {
		elm_genlist_item_update(found->it);
		quickpanel_ui_update_height(ad);
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

static int quickpanel_minictrl_get_height(void *data)
{
	unsigned int height_minictrl = 0;

	if (g_prov_table != NULL) {
		g_hash_table_foreach(g_prov_table, _quickpanel_minictrl_hf_sum_height, &height_minictrl);
	}

	return height_minictrl;
}
