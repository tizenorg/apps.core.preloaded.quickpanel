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

#include "quickpanel-ui.h"
#include "common.h"
#include "list_util.h"
#include "noti_node.h"

static void _noti_node_free(noti_node_item *node);

void noti_node_create(noti_node **handle)
{
	retif(handle == NULL, , "Invalid parameter!");

	*handle = (noti_node *)malloc(sizeof(noti_node));

	if (*handle != NULL) {
		(*handle)->table = g_hash_table_new_full(g_direct_hash, g_direct_equal,
							NULL,
							(GDestroyNotify)_noti_node_free);

		(*handle)->n_ongoing = 0;
		(*handle)->n_noti = 0;
	} else {
		*handle = NULL;
	}
}

void noti_node_destroy(noti_node **handle)
{
	retif(handle == NULL, , "Invalid parameter!");
	retif(*handle == NULL, , "Invalid parameter!");

	g_hash_table_remove_all((*handle)->table);
	g_hash_table_destroy((*handle)->table);
	(*handle)->table = NULL;

	free((*handle));
	*handle = NULL;
}

noti_node_item *noti_node_add(noti_node *handle, notification_h noti, void *view)
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

		if (noti_type == NOTIFICATION_TYPE_NOTI)
			handle->n_noti++;
		else if (noti_type == NOTIFICATION_TYPE_ONGOING)
			handle->n_ongoing++;

		return node;
	}

	return NULL;
}

void noti_node_remove(noti_node *handle, int priv_id)
{
	notification_type_e noti_type = NOTIFICATION_TYPE_NONE;

	retif(handle == NULL, , "Invalid parameter!");
	retif(handle->table == NULL, , "Invalid parameter!");

	noti_node_item *item = noti_node_get(handle, priv_id);

	if (item != NULL) {
		if (item->noti != NULL) {
			notification_get_type(item->noti, &noti_type);

			if (noti_type == NOTIFICATION_TYPE_NOTI)
				handle->n_noti--;
			else if (noti_type == NOTIFICATION_TYPE_ONGOING)
				handle->n_ongoing--;
		}

		notification_free(item->noti);
		item->noti = NULL;
		item->view = NULL;

		if (g_hash_table_remove(handle->table, GINT_TO_POINTER(priv_id)))
		{
			INFO("success to remove %d", priv_id);
		}
	}
}

noti_node_item *noti_node_get(noti_node *handle, int priv_id)
{
	retif(handle == NULL, NULL, "Invalid parameter!");
	retif(handle->table == NULL, NULL, "Invalid parameter!");

	return (noti_node_item *)g_hash_table_lookup
			(handle->table, GINT_TO_POINTER(priv_id));
}

int noti_node_get_item_count(noti_node *handle, notification_type_e noti_type)
{
	retif(handle == NULL, 0, "Invalid parameter!");

	if (noti_type == NOTIFICATION_TYPE_NOTI)
		return handle->n_noti;
	else if (noti_type == NOTIFICATION_TYPE_ONGOING)
		return handle->n_ongoing;

	return 0;
}

static void _noti_node_free(noti_node_item *node)
{
	retif(node == NULL, , "Invalid parameter!");

	DBG("item_node is freed:%p", node);

	free(node);
}
