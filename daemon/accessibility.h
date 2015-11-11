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



#ifndef __ACCESSIBILITY_H__
#define __ACCESSIBILITY_H__

typedef enum {
	SCREEN_READER_OBJ_TYPE_ELM_OBJECT,
	SCREEN_READER_OBJ_TYPE_EDJ_OBJECT,
} screen_reader_object_type_e;

extern Evas_Object *quickpanel_accessibility_screen_reader_object_get(void *obj, screen_reader_object_type_e type, const char *part, Evas_Object *parent);
extern Evas_Object *quickpanel_accessibility_ui_get_focus_object(Evas_Object *parent);
extern char *quickpanel_accessibility_info_cb(void *data, Evas_Object *obj);
extern char *quickpanel_accessibility_info_cb_s(void *data, Evas_Object *obj);
extern void quickpanel_accessibility_screen_reader_data_set(Evas_Object *view, const char *part, char *type, char *info);

#endif				/* __ACCESSIBILITY_H__ */
