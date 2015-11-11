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


#ifndef __QUICKPANEL_NOTI_NODE_H__
#define __QUICKPANEL_NOTI_NODE_H__

typedef struct _noti_node {
	GHashTable *table;
	int n_ongoing;
	int n_noti;
} noti_node;

typedef struct _noti_node_item {
	notification_h noti;
	void *view;
} noti_node_item;

extern void quickpanel_quickpanel_noti_node_create(noti_node **handle);
extern void quickpanel_noti_node_destroy(noti_node **handle);
extern noti_node_item *quickpanel_noti_node_add(noti_node *handle, notification_h noti, void *view);
extern void quickpanel_noti_node_remove(noti_node *handle, int priv_id);
extern void quickpanel_noti_node_remove_all(noti_node *handle);
extern noti_node_item *quickpanel_noti_node_get(noti_node *handle, int priv_id);
extern int quickpanel_noti_node_get_item_count(noti_node *handle, notification_type_e noti_type);
extern void quickpanel_noti_node_create(noti_node **handle);

#endif
