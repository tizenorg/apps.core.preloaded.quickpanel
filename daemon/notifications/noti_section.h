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

#ifndef __QUICKPANEL_NOTI_SECTION_H__
#define __QUICKPANEL_NOTI_SECTION_H__

extern Evas_Object *quickpanel_noti_section_create(Evas_Object *parent, qp_item_type_e type);
extern void quickpanel_noti_section_update(Evas_Object *noti_section, int noti_count);
extern void quickpanel_noti_section_remove(Evas_Object *noti_section) ;
extern void quickpanel_noti_section_set_deleted_cb(Evas_Object *noti_section, Evas_Object_Event_Cb func, void *data);

#endif
