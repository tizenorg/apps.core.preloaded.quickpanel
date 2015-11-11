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


#ifndef __QP_COMMON_H_
#define __QP_COMMON_H_


#define QP_OK	(0)
#define QP_FAIL	(-1)
#define QP_UTIL_PHONE_NUMBER_MAX_LEN	40
#define EDATA_BACKKEY_CB "bk_cb"

#ifdef _DLOG_USED
#undef LOG_TAG
#define LOG_TAG "QUICKPANEL"
#include <dlog.h>

#define HAPI __attribute__((visibility("hidden")))

#define DBG(fmt , args...) \
	do { \
		LOGD("[%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define INFO(fmt , args...) \
	do { \
		LOGI("[%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define WARN(fmt , args...) \
	do { \
		LOGI("[%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define ERR(fmt , args...) \
	do { \
		LOGE("[%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define SDBG(fmt , args...) \
	do { \
		SECURE_LOGD("[%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define SINFO(fmt , args...) \
	do { \
		SECURE_LOGI("[%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define SERR(fmt , args...) \
	do { \
		SECURE_LOGE("[%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#elif FILE_DEBUG /*_DLOG_USED*/
#define DBG(fmt , args...) \
	do { \
		debug_printf("[D]%s : %d] "fmt"\n", \
				__func__, __LINE__, ##args); \
	} while (0)

#define INFO(fmt , args...) \
	do { \
		debug_printf("[I][%s : %d] "fmt"\n",\
				__func__, __LINE__, ##args); \
	} while (0)

#define WARN(fmt , args...) \
	do { \
		debug_printf("[W][%s : %d] "fmt"\n", \
				__func__, __LINE__, ##args); \
	} while (0)

#define ERR(fmt , args...) \
	do { \
		debug_printf("[E][%s : %d] "fmt"\n", \
				__func__, __LINE__, ##args); \
	} while (0)

#else /*_DLOG_USED*/
#define DBG(fmt , args...) \
	do { \
		fprintf("[D][%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define INFO(fmt , args...) \
	do { \
		fprintf("[I][%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define WARN(fmt , args...) \
	do { \
		fprintf("[W][%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)

#define ERR(fmt , args...) \
	do { \
		fprintf("[E][%s : %d] "fmt"\n", __func__, __LINE__, ##args); \
	} while (0)
#endif /*_DLOG_USED*/

#define msgif(cond, str, args...) do { \
	if (cond) { \
		ERR(str, ##args);\
	} \
} while (0);

#define retif(cond, ret, str, args...) do { \
	if (cond) { \
		ERR(str, ##args);\
		return ret;\
	} \
} while (0);

#define retif_nomsg(cond, ret) do { \
	if (cond) { \
		return ret;\
	} \
} while (0);

#define gotoif(cond, target, str, args...) do { \
	if (cond) { \
		WARN(str, ##args); \
		goto target; \
	} \
} while (0);

extern void quickpanel_common_util_char_trim(char *text);
extern void quickpanel_common_util_char_replace(char *text, char s, char t);
extern void quickpanel_common_util_add_char_to_each_charactor(char *dst, const char *src, char t);
extern int quickpanel_common_util_is_phone_number(const char *address);
extern void quickpanel_common_util_phone_number_tts_make(char *dst, const char *src, int size);
extern void quickpanel_common_ui_set_current_popup(Evas_Object *popup, Evas_Smart_Cb func_close);
extern void quickpanel_common_ui_del_current_popup(void);
extern void *quickpanel_common_ui_get_buffer_from_image(const char *file_path, size_t *memfile_size, char *ext, int ext_size);
extern char *quickpanel_common_ui_get_pkginfo_icon(const char *pkgid);
extern char *quickpanel_common_ui_get_pkginfo_label(const char *pkgid);
extern int quickpanel_common_ui_is_package_exist(const char *pkgid);

#endif				/* __QP_COMMON_H_ */
