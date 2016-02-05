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

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <Elementary.h>

#include <package_manager.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "common.h"
#include "quickpanel-ui.h"

static inline int _is_space(char in)
{
	if ((in == _SPACE)) {
		return 1;
	} else {
		return 0;
	}
}

static inline int _l_trim(char *in)
{
	int i, j;
	short int done;

	i = 0;
	done = 0;

	while (!done && in[i] != '\0') {
		if (_is_space(in[i])) {
			i++;
		} else {
			done = 1;
		}
	}

	j = 0;
	while (in[i] != '\0') {
		in[j++] = in[i++];
	}

	in[j] = '\0';

	return 0;
}

static inline int _r_trim(char *in)
{
	int i;
	short int done;

	i = strlen(in) - 1;
	done = 0;

	while (!done && !(i < 0)) {
		if (_is_space(in[i])) {
			in[i--] = '\0';
		} else {
			done = 1;
		}
	}

	return(0);
}

HAPI void quickpanel_common_util_char_trim(char *text)
{
	retif(text == NULL, , "invalid argument");

	_l_trim(text);
	_r_trim(text);
}

HAPI void quickpanel_common_util_char_replace(char *text, char s, char t)
{
	retif(text == NULL, , "invalid argument");

	int i = 0, text_len = 0;

	text_len = strlen(text);

	for (i = 0; i < text_len; i++) {
		if (*(text + i) == s) {
			*(text + i) = t;
		}
	}
}

HAPI void quickpanel_common_util_add_char_to_each_charactor(char *dst, const char *src, char t)
{
	retif(dst == NULL, , "invalid argument");
	retif(src == NULL, , "invalid argument");

	int i = 0, text_len = 0;

	text_len = strlen(src);

	for (i = 0; i < text_len; i++) {
		*(dst + (i * 2)) = *(src + i);
		*(dst + ((i * 2) + 1)) = t;
	}
}

static void _char_set(char *dst, char s, int index, int size)
{
	if (index < size) {
		*(dst + index) = s;
	}
}

HAPI void quickpanel_common_util_phone_number_tts_make(char *dst, const char *src, int size)
{
	retif(dst == NULL, , "invalid argument");
	retif(src == NULL, , "invalid argument");

	int no_op = 0;
	int i = 0, j = 0, text_len = 0;

	text_len = strlen(src);

	for (i = 0, j= 0; i < text_len; i++) {
		if (no_op == 1) {
			_char_set(dst, *(src + i), j++, size);
		} else {
			if (isdigit(*(src + i))) {
				if (i + 1 < text_len) {
					if (*(src + i + 1) == '-' || *(src + i + 1) == _SPACE) {
						_char_set(dst, *(src + i), j++, size);
					} else {
						_char_set(dst, *(src + i), j++, size);
						_char_set(dst, _SPACE, j++, size);
					}
				} else {
					_char_set(dst, *(src + i), j++, size);
					_char_set(dst, _SPACE, j++, size);
				}
			} else if (*(src + i) == '-') {
				no_op = 1;
				_char_set(dst, *(src + i), j++, size);
			} else {
				_char_set(dst, *(src + i), j++, size);
			}
		}
	}
}

HAPI int quickpanel_common_util_is_phone_number(const char *address)
{
	int digit_count = 0;
	retif(address == NULL, 0, "address is NULL");

	int addr_len = 0;
	addr_len = strlen(address);

	if (addr_len == 0) {
		return 0;
	}

	/*  length check phone address should be longer than 2 and shorter than 40 */
	if (addr_len > 2 && addr_len <= QP_UTIL_PHONE_NUMBER_MAX_LEN) {
		const char *pszOneChar = address;

		while (*pszOneChar) {
			if (isdigit(*pszOneChar)) {
				digit_count++;
			}

			++pszOneChar;
		}

		pszOneChar = address;

		if (*pszOneChar == '+') {
			++pszOneChar;
		}

		while (*pszOneChar) {
			if (!isdigit(*pszOneChar)
					&& (*pszOneChar != '*') && (*pszOneChar != '#')
					&& (*pszOneChar != ' ')
					&& !((*pszOneChar == '-') && digit_count >= 7)) {
				return 0;
			}

			++pszOneChar;
		}

		return 1;
	} else {
		DBG("invalid address length [%d]", addr_len);
		return 0;
	}
}

static void _current_popup_default_backkey_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *popup = data;
	retif(popup == NULL, , "invalid argument");

	if (popup!= NULL) {
		evas_object_del(popup);
		popup = NULL;
	}
}

static void _current_popup_deleted_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	retif(obj == NULL, , "obj is NULL");

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid argument");

	if (ad->popup == obj) {
		ad->popup = NULL;
	} else {
		ERR("popup is created over the popup");
	}
}

HAPI void quickpanel_common_ui_set_current_popup(Evas_Object *popup, Evas_Smart_Cb func_back)
{
	retif(popup == NULL, , "invalid argument");

	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid argument");

	ad->popup = popup;
	evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL, _current_popup_deleted_cb, NULL);

	if (func_back != NULL) {
		evas_object_data_set(popup, EDATA_BACKKEY_CB, func_back);
	} else {
		evas_object_data_set(popup, EDATA_BACKKEY_CB, _current_popup_default_backkey_cb);
	}
}

HAPI void quickpanel_common_ui_del_current_popup(void)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid argument");

	if (ad->popup != NULL) {
		evas_object_del(ad->popup);
		ad->popup = NULL;
	}
}

HAPI void *quickpanel_common_ui_get_buffer_from_image(const char *file_path, size_t *memfile_size, char *ext, int ext_size)
{
	FILE *fp = NULL;
	void *buffer = NULL;
	char *buf_ext = NULL;

	retif(file_path == NULL, NULL, "invalid data");

	if (ext != NULL) {
		buf_ext = ecore_file_strip_ext(file_path);
		if (buf_ext != NULL) {
			strncpy(ext, buf_ext, ext_size);
			free(buf_ext);
		}
	}

	fp = fopen(file_path, "r");
	if (fp) {
		struct stat stat_buf;
		if (stat(file_path, &stat_buf) != 0) {
			ERR("Getting file information Error");
			goto err;
		}

		if (stat_buf.st_size > 0) {
			buffer = (void *)calloc(1, (size_t)stat_buf.st_size + 1);
			if (buffer == NULL) {
				ERR("failed to alloc a buffer");
				goto err;
			}
			int result = fread(buffer, sizeof(char), stat_buf.st_size, fp);
			if (result != stat_buf.st_size) {
				ERR("failed to read a file");
				free(buffer);
				buffer = NULL;
				goto err;
			}
			if (memfile_size != NULL) {
				*memfile_size = result;
			}
		} else {
			if (memfile_size != NULL) {
				*memfile_size = 0;
			}
		}
	}

err:
	if (fp) {
		fclose(fp);
	}
	return buffer;
}

HAPI char *quickpanel_common_ui_get_pkginfo_icon(const char *pkgid)
{
	int ret = 0;
	char *icon_path = NULL;
	char *icon_ret = NULL;
	package_info_h package_info = NULL;

	ret = package_manager_get_package_info(pkgid, &package_info);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		ERR("package_manager_get_package_info is failed id : %s %d", pkgid, ret);
		return NULL;
	}

	ret = package_info_get_icon(package_info, &icon_path);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		ERR("package_info_get_icon is failed %d", ret);
		return NULL;
	}

	if (icon_path) {
		icon_ret = (char*)strdup(icon_path);
	}

	package_info_destroy(package_info);

	return icon_ret;
}

HAPI char *quickpanel_common_ui_get_pkginfo_label(const char *pkgid)
{
	int ret = 0;
	char *label = NULL;
	char *label_ret = NULL;
	package_info_h package_info = NULL;

	ret = package_manager_get_package_info(pkgid, &package_info);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		ERR("package_manager_get_package_info is failed %d", ret);
		return NULL;
	}

	ret = package_info_get_label(package_info, &label);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		ERR("package_info_get_label is failed %d", ret);
		return NULL;
	}

	if (label) {
		label_ret = (char*)strdup(label);
	}

	package_info_destroy(package_info);

	return label_ret;

}

HAPI int quickpanel_common_ui_is_package_exist(const char *pkgid)
{
	int ret = 0;
	retif(pkgid == NULL, 0, "invalid parameter");

	package_info_h package_info = NULL;

	ret = package_manager_get_package_info(pkgid, &package_info);
	if (ret == PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE) {
		DBG("package %s isn't exist", pkgid);
		return 0;
	}

	if (package_info) {
		package_info_destroy(package_info);
	}

	return 1;
}
