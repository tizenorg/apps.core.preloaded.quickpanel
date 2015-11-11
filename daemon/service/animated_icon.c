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
#include "quickpanel_def.h"
#include "animated_icon.h"

#define E_DATA_ANI_ICON_TYPE "ANI_ICON_TYPE"
#define PATH_DOWNLOAD "reserved://quickpanel/ani/downloading"
#define PATH_UPLOAD "reserved://quickpanel/ani/uploading"
#define PATH_INSTALL "reserved://quickpanel/ani/install"

static qp_animated_icon_type _animated_type_get(const char *path)
{
	retif_nomsg(path == NULL, QP_ANIMATED_ICON_NONE);

	if (strncasecmp(path, PATH_DOWNLOAD, MIN(strlen(PATH_DOWNLOAD), strlen(path))) == 0) {
		return QP_ANIMATED_ICON_DOWNLOAD;
	} else if (strncasecmp(path, PATH_UPLOAD, MIN(strlen(PATH_UPLOAD), strlen(path))) == 0) {
		return QP_ANIMATED_ICON_UPLOAD;
	} else if (strncasecmp(path, PATH_INSTALL, MIN(strlen(PATH_INSTALL), strlen(path))) == 0) {
		return QP_ANIMATED_ICON_INSTALL;
	}

	return QP_ANIMATED_ICON_NONE;
}

HAPI Evas_Object *quickpanel_animated_icon_get(Evas_Object *parent, const char *path)
{
	qp_animated_icon_type type = QP_ANIMATED_ICON_NONE;
	const char *layout_icon = NULL;
	Evas_Object *layout = NULL;
	retif_nomsg(parent == NULL, NULL);
	retif_nomsg(path == NULL, NULL);

	type = _animated_type_get(path);

	if (type == QP_ANIMATED_ICON_DOWNLOAD) {
		layout_icon = "quickpanel/animated_icon_download";
	} else if (type == QP_ANIMATED_ICON_UPLOAD) {
		layout_icon = "quickpanel/animated_icon_upload";
	} else if (type == QP_ANIMATED_ICON_INSTALL) {
		layout_icon = "quickpanel/animated_icon_install";
	} else {
		return NULL;
	}

	layout = elm_layout_add(parent);
	if (layout != NULL) {
		elm_layout_file_set(layout, DEFAULT_EDJ, layout_icon);
		evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_data_set(layout, E_DATA_ANI_ICON_TYPE, (void *)type);
		evas_object_show(layout);
	}

	return layout;
}

HAPI int quickpanel_animated_icon_is_same_icon(Evas_Object *view, const char *path)
{
	qp_animated_icon_type type = QP_ANIMATED_ICON_NONE;
	qp_animated_icon_type type_old = QP_ANIMATED_ICON_NONE;
	retif_nomsg(view == NULL, 0);
	retif_nomsg(path == NULL, 0);

	type = _animated_type_get(path);
	type_old = (qp_animated_icon_type)evas_object_data_get(view,
			E_DATA_ANI_ICON_TYPE);

	if (type == type_old) {
		return 1;
	}

	return 0;
}
