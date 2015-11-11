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



#ifndef _QP_SERVICE_ANIMATED_ICON_DEF_
#define _QP_SERVICE_ANIMATED_ICON_DEF_


typedef enum _qp_animated_icon_type {
	QP_ANIMATED_ICON_NONE = -1,
	QP_ANIMATED_ICON_DOWNLOAD = 1,
	QP_ANIMATED_ICON_UPLOAD,
	QP_ANIMATED_ICON_INSTALL,
} qp_animated_icon_type;

extern Evas_Object *quickpanel_animated_icon_get(Evas_Object *parent, const char *path);
extern int quickpanel_animated_icon_is_same_icon(Evas_Object *view, const char *path);

#endif
