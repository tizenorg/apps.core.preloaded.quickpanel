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

#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "quickpanel-ui.h"
#include "common.h"
#include "pager.h"
#include "datetime.h"

#define EVAS_DATA_PAGE_HANDLER "page_handler"

static inline void _scroll_hold(Evas_Object *viewer)
{
	int hold_count = 0;
	retif(viewer == NULL, , "Invalid parameter!");

	hold_count = elm_object_scroll_hold_get(viewer);

	if (hold_count <= 0) {
		elm_object_scroll_hold_push(viewer);
	}
}

static inline void _scroll_unhold(Evas_Object *viewer)
{
	int i = 0, hold_count = 0;
	retif(viewer == NULL, , "Invalid parameter!");

	hold_count = elm_object_scroll_hold_get(viewer);

	for (i = 0 ; i < hold_count; i++) {
		elm_object_scroll_hold_pop(viewer);
	}
}

static inline void _scroll_freeze(Evas_Object *viewer)
{
	int freezed_count = 0;
	retif(viewer == NULL, , "Invalid parameter!");

	freezed_count = elm_object_scroll_freeze_get(viewer);

	if (freezed_count <= 0) {
		elm_object_scroll_freeze_push(viewer);
	}
}

static inline void _scroll_unfreeze(Evas_Object *viewer)
{
	int i = 0, freezed_count = 0;
	retif(viewer == NULL, , "Invalid parameter!");

	freezed_count = elm_object_scroll_freeze_get(viewer);

	for (i = 0 ; i < freezed_count; i++) {
		elm_object_scroll_freeze_pop(viewer);
	}
}

HAPI void quickpanel_page_handler_set(Evas_Object *page, QP_Page_Handler *handler)
{
	retif(page == NULL, , "invalid parameter");

	evas_object_data_set(page, EVAS_DATA_PAGE_HANDLER, handler);
}

HAPI QP_Page_Handler *quickpanel_page_handler_get(Evas_Object *page)
{
	retif(page == NULL, NULL, "invalid parameter");

	return evas_object_data_get(page, EVAS_DATA_PAGE_HANDLER);
}

HAPI void quickpanel_page_scroll_freeze_set(Eina_Bool is_freeze)
{
	Evas_Object *pager_scroller = quickpanel_pager_view_get("SCROLLER");
	retif(pager_scroller == NULL, , "pager null");

	if (is_freeze) {
		_scroll_freeze(pager_scroller);
	} else {
		_scroll_unfreeze(pager_scroller);
	}
}

HAPI void quickpanel_page_scroll_hold_set(Eina_Bool is_freeze)
{
	Evas_Object *pager_scroller = quickpanel_pager_view_get("SCROLLER");
	retif(pager_scroller == NULL, , "pager null");

	if (is_freeze) {
		_scroll_hold(pager_scroller);
	} else {
		_scroll_unhold(pager_scroller);
	}
}

HAPI void quickpanel_page_get_recoordinated_pos(int local_x, int local_y, int *x, int *y)
{
	int rot_x = 0;
	int rot_y = 0;
	int width = 0;
	int height = 0;
	retif(x == NULL && y == NULL, , "invalid parameter");

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid parameter");

	elm_win_screen_size_get(ad->win, NULL, NULL, &width, &height);

	switch (ad->angle) {
	case 0:
		rot_x = local_x;
		rot_y = local_y;
		break;
	case 90:
		rot_x = height - local_y;
		rot_y = local_x;
		break;
	case 180:
		rot_x = width - local_x;
		rot_y = height - local_y;
		break;
	case 270:
		rot_x = local_y;
		rot_y = width - local_x;
		break;
	default:
		break;
	}

	if (x != NULL) {
		*x = rot_x;
	}

	if (y != NULL) {
		*y = rot_y;
	}
}

HAPI void quickpanel_page_get_touched_pos(int *x, int *y)
{
	int rot_x = 0;
	int rot_y = 0;
	int local_x = 0;
	int local_y = 0;
	retif(x == NULL && y == NULL, , "invalid parameter");

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid parameter");

#if defined(WINSYS_X11)
	ecore_x_pointer_last_xy_get(&local_x, &local_y);
#endif

	quickpanel_page_get_recoordinated_pos(local_x, local_y, &rot_x, &rot_y);

	if (x != NULL) {
		*x = rot_x;
	}

	if (y != NULL) {
		*y = rot_y;
	}
}

HAPI void quickpanel_page_editing_icon_visible_status_update(void)
{
	int is_visible = 0;
	struct appdata *ad;

	ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid parameter");

	if (quickpanel_pager_current_page_get() == PAGE_IDX_EDITING) {
		is_visible = 1;
	} else {
		is_visible = 0;
	}

	quickpanel_datetime_editing_icon_visibility_set(is_visible);
}
