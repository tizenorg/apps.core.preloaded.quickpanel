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

#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <notification.h>
#include <notification_internal.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "list_util.h"
#include "noti_node.h"

static void _noti_node_free(noti_node_item *node);

HAPI void quickpanel_noti_node_create(noti_node **handle)
{
	retif(handle == NULL, , "Invalid parameter!");

	*handle = (noti_node *)malloc(sizeof(noti_node));

	if (*handle != NULL) {
		(*handle)->table = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)_noti_node_free);
		(*handle)->n_ongoing = 0;
		(*handle)->n_noti = 0;
	} else {
		*handle = NULL;
	}
}

HAPI void quickpanel_noti_node_destroy(noti_node **handle)
{
	retif(handle == NULL, , "Invalid parameter!");
	retif(*handle == NULL, , "Invalid parameter!");

	g_hash_table_remove_all((*handle)->table);
	g_hash_table_destroy((*handle)->table);
	(*handle)->table = NULL;

	free((*handle));
	*handle = NULL;
}

HAPI noti_node_item *quickpanel_noti_node_add(noti_node *handle, notification_h noti, void *view)
{
	int priv_id = 0;
	notification_type_e noti_type = NOTIFICATION_TYPE_NONE;
	noti_node_item *node = NULL;

	retif(handle == NULL || noti == NULL, NULL, "Invalid parameter!");

	if (notification_get_id(noti, NULL, &priv_id) == NOTIFICATION_ERROR_NONE) {
		node = malloc(sizeof(noti_node_item));
		if (!node) {
			ERR("fail to alloc item");
			return NULL;
		}

		node->noti = noti;
		node->view = view;

		g_hash_table_insert(handle->table, GINT_TO_POINTER(priv_id), (gpointer *)node);

		notification_get_type(noti, &noti_type);

		if (noti_type == NOTIFICATION_TYPE_NOTI) {
			handle->n_noti++;
		} else if (noti_type == NOTIFICATION_TYPE_ONGOING) {
			handle->n_ongoing++;
		}

		DBG("n_noti = [%d] n_ongoing = [%d]", handle->n_noti, handle->n_ongoing);
		return node;
	}

	return NULL;
}

HAPI void quickpanel_noti_node_remove(noti_node *handle, int priv_id)
{
	notification_type_e noti_type = NOTIFICATION_TYPE_NONE;

	retif(handle == NULL, , "Invalid parameter!");
	retif(handle->table == NULL, , "Invalid parameter!");

	noti_node_item *item = quickpanel_noti_node_get(handle, priv_id);

	if (item != NULL) {
		if (item->noti != NULL) {
			notification_get_type(item->noti, &noti_type);

			if (noti_type == NOTIFICATION_TYPE_NOTI) {
				handle->n_noti--;
			} else if (noti_type == NOTIFICATION_TYPE_ONGOING) {
				handle->n_ongoing--;
			}
		}

		notification_free(item->noti);
		item->noti = NULL;
		item->view = NULL;

		if (g_hash_table_remove(handle->table, GINT_TO_POINTER(priv_id))) {
			DBG("success to remove %d", priv_id);
		}
	}
}

HAPI void quickpanel_noti_node_remove_all(noti_node *handle)
{
	retif(handle == NULL, , "Invalid parameter!");
	retif(handle->table == NULL, , "Invalid parameter!");

	g_hash_table_remove_all(handle->table);
	handle->n_noti = 0;
	handle->n_ongoing = 0;
	DBG("all the nodes are removed");
}

HAPI noti_node_item *quickpanel_noti_node_get(noti_node *handle, int priv_id)
{
	retif(handle == NULL, NULL, "Invalid parameter!");
	retif(handle->table == NULL, NULL, "Invalid parameter!");

	return (noti_node_item *)g_hash_table_lookup(handle->table, GINT_TO_POINTER(priv_id));
}

HAPI int quickpanel_noti_node_get_item_count(noti_node *handle, notification_type_e noti_type)
{
	retif(handle == NULL, 0, "Invalid parameter!");

	DBG("n_noti %d , n_ongoing %d ", handle->n_noti, handle->n_ongoing);

	if (noti_type == NOTIFICATION_TYPE_NOTI) {
		return handle->n_noti;
	} else if (noti_type == NOTIFICATION_TYPE_ONGOING) {
		return handle->n_ongoing;
	} else if (noti_type == NOTIFICATION_TYPE_NONE) {
		return handle->n_noti + handle->n_ongoing;
	}

	return 0;
}

static void _noti_node_free(noti_node_item *node)
{
	retif(node == NULL, , "Invalid parameter!");

	DBG("item_node is freed:%p", node);

	free(node);
}
