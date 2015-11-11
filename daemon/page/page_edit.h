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


#ifndef __QUICKPANEL_PAGE_EDIT_H__
#define __QUICKPANEL_PAGE_EDIT_H__

extern Evas_Object *quickpanel_page_edit_create(Evas_Object *parent, void *data);
extern Evas_Object *quickpanel_page_edit_view_get(const char *view_name);
extern void quickpanel_page_edit_view_set(const char *view_name, Evas_Object *view);
extern void quickpanel_page_edit_freeze_set(Eina_Bool is_freeze);
extern Eina_Bool quickpanel_page_edit_is_page_showed(void);

#endif
