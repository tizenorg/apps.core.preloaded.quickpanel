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


#ifndef __QUICKPANEL_PAGER_H__
#define __QUICKPANEL_PAGER_H__


typedef struct _QP_Page_Handler {
	int status;
	char *name;
	/* func */
	void (*content_resize)(int width, int height, const char *signal);
	void (*mapbuf_enable_set)(Eina_Bool is_enable);
	int (*down_cb)(void *, void *);
	int (*move_cb)(void *, void *);
	int (*up_cb)(void *, void *);
	int (*scroll_start_cb)(void *, void *);
	int (*scroll_done_cb)(void *, void *);
	int (*page_changed_cb)(void *, void *);
} QP_Page_Handler;

typedef enum _qp_pager_page_type {
	PAGE_IDX_MAIN = 0,
	PAGE_IDX_EDITING,	// Not supported
} qp_pager_page_type;

extern Evas_Object *quickpanel_pager_new(Evas_Object *parent, void *data);
extern void quickpanel_pager_destroy(void);
extern Evas_Object *quickpanel_pager_view_get(const char *view_name);
extern int quickpanel_pager_current_page_get(void);
extern void quickpanel_pager_page_set(int page_index, int need_resize);
extern void quickpanel_pager_mapbuf_set(int is_enable);

#endif
