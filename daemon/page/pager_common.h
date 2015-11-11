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


#ifndef __QUICKPANEL_PAGER_COMMON_H__
#define __QUICKPANEL_PAGER_COMMON_H__

extern void quickpanel_page_handler_set(Evas_Object *page, QP_Page_Handler *handler);
extern QP_Page_Handler *quickpanel_page_handler_get(Evas_Object *page);
extern void quickpanel_page_scroll_hold_set(Eina_Bool is_freeze);
extern void quickpanel_page_scroll_freeze_set(Eina_Bool is_freeze);
extern void quickpanel_page_get_touched_pos(int *x, int *y);
extern void quickpanel_page_get_recoordinated_pos(int local_x, int local_y, int *x, int *y);
extern void quickpanel_page_editing_icon_visible_status_update(void);

#endif
