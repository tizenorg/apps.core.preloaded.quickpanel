/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <vconf.h>
#include "common.h"
#include "quickpanel-ui.h"

#define QP_IDLETXT_PART		"qp.noti.swallow.spn"

#define QP_SPN_BASE_PART	"qp.base.spn.swallow"
#define QP_SPN_BOX_PART		"qp.spn.swallow"

#define QP_IDLETXT_MAX_KEY	4
#define QP_IDLETXT_MAX_LEN	1024
#define QP_IDLETXT_SLIDE_LEN	130

#define QP_IDLETXT_LABEL_STRING	\
	"<font_size=30><color=#8C8C8CFF><align=left>%s</align>" \
	"</color></font_size>"

static int quickpanel_idletxt_init(void *data);
static int quickpanel_idletxt_fini(void *data);
static int quickpanel_idletxt_suspend(void *data);
static int quickpanel_idletxt_resume(void *data);

QP_Module idletxt = {
	.name = "idletxt",
	.init = quickpanel_idletxt_init,
	.fini = quickpanel_idletxt_fini,
	.suspend = quickpanel_idletxt_suspend,
	.resume = quickpanel_idletxt_resume,
	.lang_changed = NULL
};

static Evas_Object *_quickpanel_idletxt_create_label(Evas_Object * parent,
						     char *txt)
{
	Evas_Object *obj = NULL;
	char buf[QP_IDLETXT_MAX_LEN] = { 0, };
	int len = 0;

	retif(parent == NULL || txt == NULL, NULL, "Invalid parameter!");

	memset(buf, 0x00, sizeof(buf));

	len = snprintf(buf, sizeof(buf), QP_IDLETXT_LABEL_STRING, txt);

	retif(len < 0, NULL, "len < 0");

	obj = elm_label_add(parent);
	if (obj != NULL) {
		elm_object_text_set(obj, buf);

		evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND,
						 EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, 0.5);

		evas_object_show(obj);
	}

	return obj;
}

static Evas_Object *_quickpanel_idletxt_create_box(Evas_Object * parent)
{
	Evas_Object *box = NULL;

	retif(parent == NULL, NULL, "Invalid parameter!");

	box = elm_box_add(parent);
	if (box != NULL) {
		elm_box_horizontal_set(box, EINA_FALSE);

		evas_object_show(box);
	}

	return box;
}

static int _quickpanel_idletxt_get_txt(const char *key, char *txt, int size)
{
	int len = 0;
	char *str = NULL;
	int i = 0;

	str = vconf_get_str(key);
	if (str == NULL || str[0] == '\0')
		return 0;

	/* check ASCII code */
	for (i = strlen(str) - 1; i >= 0; i--) {
		if (str[i] <= 31 || str[i] >= 127)
			goto failed;
	}

	len = snprintf(txt, size, "%s", str);

 failed:
	if (str)
		free(str);

	return len;
}

static Evas_Object *_quickpanel_idletxt_add_label(Evas_Object * box,
						  char *key[])
{
	char txt[QP_IDLETXT_MAX_LEN] = { 0, };
	char buf[QP_IDLETXT_MAX_LEN] = { 0, };
	int len = 0;
	int i = 0;
	Evas_Object *obj = NULL;

	retif(key == NULL || key[0] == '\0', NULL, "Invalid parameter!");

	memset(txt, 0x00, sizeof(txt));

	for (i = 0; key[i]; i++) {
		memset(buf, 0x00, sizeof(buf));

		/* get next key string */
		if (_quickpanel_idletxt_get_txt(key[i], buf, sizeof(buf))) {
			INFO("VCONFKEY(%s) = %s", key[i], buf);

			len = strlen(txt);

			snprintf(&txt[len], sizeof(txt) - len, "%s%s",
				 len ? " - " : "", buf);
		}
	}

	len = strlen(txt);

	if (len) {
		obj = _quickpanel_idletxt_create_label(box, txt);

		if (obj != NULL) {
			if (len > QP_IDLETXT_SLIDE_LEN)
				elm_label_slide_set(obj, EINA_TRUE);

			return obj;
		}
	}

	return NULL;
}

static Evas_Object *_quickpanel_idletxt_exception_add_label(Evas_Object * box)
{
	int service_type = VCONFKEY_TELEPHONY_SVCTYPE_SEARCH;
	char *text = NULL;
	Evas_Object *obj = NULL;

	if (vconf_get_int(VCONFKEY_TELEPHONY_SVCTYPE, &service_type) != 0) {
		DBG("fail to get VCONFKEY_TELEPHONY_SVCTYPE");
	}

	switch(service_type) {
		case VCONFKEY_TELEPHONY_SVCTYPE_NOSVC:
			text = _S("IDS_COM_BODY_NO_SERVICE");
			break;
		case VCONFKEY_TELEPHONY_SVCTYPE_EMERGENCY:
			text = _NOT_LOCALIZED("Emergency calls only");
			break;
		default:
			if (service_type > VCONFKEY_TELEPHONY_SVCTYPE_SEARCH) {
				text = vconf_get_str(VCONFKEY_TELEPHONY_NWNAME);
			} else {
				text = _S("IDS_COM_BODY_SEARCHING");
			}
			break;
	}

	if (text != NULL) {
		obj = _quickpanel_idletxt_create_label(box, text);

		if (obj != NULL) {
			if (strlen(text) > QP_IDLETXT_SLIDE_LEN)
				elm_label_slide_set(obj, EINA_TRUE);

			return obj;
		}
	}

	return NULL;
}

static Evas_Object *_quickpanel_idletxt_get_spn(Evas_Object * box)
{
	Evas_Object *label = NULL;
	char *keylist[QP_IDLETXT_MAX_KEY] = { 0, };
	int ret = 0;
	int state = 0;
	int i = 0;

	/* make keylist */
	ret = vconf_get_int(VCONFKEY_TELEPHONY_SPN_DISP_CONDITION, &state);
	if (ret == 0) {
		INFO("VCONFKEY(%s) = %d",
		     VCONFKEY_TELEPHONY_SPN_DISP_CONDITION, state);

		if (state != VCONFKEY_TELEPHONY_DISP_INVALID) {
			if (i < QP_IDLETXT_MAX_KEY) {
				if (state & VCONFKEY_TELEPHONY_DISP_SPN) {
					keylist[i++] =
					    strdup(VCONFKEY_TELEPHONY_SPN_NAME);
				}

				if (state & VCONFKEY_TELEPHONY_DISP_PLMN) {
					keylist[i++] =
					    strdup(VCONFKEY_TELEPHONY_NWNAME);
				}
			}

			if (i > 0) {
				/* get string with keylist */
				label = _quickpanel_idletxt_add_label(box, keylist);

				/* free keylist */
				while (i > 0) {
					if (keylist[i])
						free(keylist[i]);

					i--;
				}
			}
		} else {
			label = _quickpanel_idletxt_exception_add_label(box);
		}
	}

	return label;
}

static Evas_Object *_quickpanel_idletxt_get_sat_text(Evas_Object * box)
{
	Evas_Object *label = NULL;
	char *keylist[] = { VCONFKEY_SAT_IDLE_TEXT, 0 };

	/* get string with keylist */
	label = _quickpanel_idletxt_add_label(box, keylist);

	return label;
}

static void quickpanel_idletxt_update(void *data)
{
	struct appdata *ad = NULL;
	Evas_Object *label = NULL;
	Evas_Object *idletxtbox = NULL;
	Evas_Object *spn = NULL;

	retif(!data, , "Invalid parameter!");
	ad = data;

	retif(!ad->ly, , "layout is NULL!");

	spn = elm_object_part_content_get(ad->ly, QP_SPN_BASE_PART);
	retif(!spn, , "spn layout is NULL!");

	idletxtbox = elm_object_part_content_get(spn, QP_SPN_BOX_PART);
	retif(!spn, , "spn layout is NULL!");

	if (idletxtbox == NULL) {
		idletxtbox = _quickpanel_idletxt_create_box(spn);
		retif(idletxtbox == NULL, , "Failed to create box!");
		elm_object_part_content_set(spn,
				QP_SPN_BOX_PART, idletxtbox);
	}

	elm_box_clear(idletxtbox);

	/* get spn */
	label = _quickpanel_idletxt_get_spn(idletxtbox);
	if (label != NULL)
		elm_box_pack_end(idletxtbox, label);

	/* get sat idle text */
	label = _quickpanel_idletxt_get_sat_text(idletxtbox);
	if (label != NULL)
		elm_box_pack_end(idletxtbox, label);
}

static void quickpanel_idletxt_changed_cb(keynode_t *node, void *data)
{
	quickpanel_idletxt_update(data);
}

static int _quickpanel_idletxt_register_event_handler(void *data)
{
	int ret = 0;

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SPN_DISP_CONDITION,
				quickpanel_idletxt_changed_cb, data);
	if (ret != 0)
		ERR("Failed to register [%s]",
			VCONFKEY_TELEPHONY_SPN_DISP_CONDITION);

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SPN_NAME,
				quickpanel_idletxt_changed_cb, data);
	if (ret != 0)
		ERR("Failed to register [%s]",
			VCONFKEY_TELEPHONY_SPN_NAME);


	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_NWNAME,
				quickpanel_idletxt_changed_cb, data);
	if (ret != 0)
		ERR("Failed to register [%s]",
			VCONFKEY_TELEPHONY_NWNAME);

	ret = vconf_notify_key_changed(VCONFKEY_SAT_IDLE_TEXT,
				quickpanel_idletxt_changed_cb, data);
	if (ret != 0)
		ERR("Failed to register [%s]",
			VCONFKEY_SAT_IDLE_TEXT);


	return QP_OK;
}

static int _quickpanel_idletxt_unregister_event_handler(void)
{
	int ret = 0;

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SPN_DISP_CONDITION,
				     quickpanel_idletxt_changed_cb);
	if (ret != 0)
		ERR("Failed to unregister [%s]",
			VCONFKEY_TELEPHONY_SPN_DISP_CONDITION);

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SPN_NAME,
				     quickpanel_idletxt_changed_cb);
	if (ret != 0)
		ERR("Failed to unregister [%s]",
			VCONFKEY_TELEPHONY_SPN_NAME);


	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_NWNAME,
				     quickpanel_idletxt_changed_cb);
	if (ret != 0)
		ERR("Failed to unregister [%s]",
			VCONFKEY_TELEPHONY_NWNAME);

	ret = vconf_ignore_key_changed(VCONFKEY_SAT_IDLE_TEXT,
				     quickpanel_idletxt_changed_cb);
	if (ret != 0)
		ERR("Failed to unregister [%s]",
			VCONFKEY_SAT_IDLE_TEXT);

	return QP_OK;
}

static Evas_Object *_idletxt_load_edj(Evas_Object * parent, const char *file,
				const char *group)
{
	Eina_Bool r;
	Evas_Object *eo = NULL;

	retif(parent == NULL, NULL, "Invalid parameter!");

	eo = elm_layout_add(parent);
	retif(eo == NULL, NULL, "Failed to add layout object!");

	r = elm_layout_file_set(eo, file, group);
	retif(r != EINA_TRUE, NULL, "Failed to set edje object file!");

	evas_object_size_hint_weight_set(eo,
		EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(eo);

	return eo;
}

static int quickpanel_idletxt_init(void *data)
{
	struct appdata *ad = NULL;
	Evas_Object *spn = NULL;

	retif(!data, QP_FAIL, "Invalid parameter!");
	ad = data;

	spn = _idletxt_load_edj(ad->ly, DEFAULT_EDJ, "quickpanel/spn");
	retif(!spn, QP_FAIL, "fail to load spn layout");

	elm_object_part_content_set(ad->ly, QP_SPN_BASE_PART, spn);

	quickpanel_idletxt_update(data);

	_quickpanel_idletxt_register_event_handler(data);

	return QP_OK;
}

static int quickpanel_idletxt_fini(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	Evas_Object *spn = NULL;
	Evas_Object *idletxtbox = NULL;

	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	_quickpanel_idletxt_unregister_event_handler();

	retif(!ad->ly, QP_FAIL, "Invalid parameter!");

	spn = elm_object_part_content_unset(ad->ly, QP_SPN_BASE_PART);
	retif(!spn, QP_OK, "spn is NULL");

	idletxtbox = elm_object_part_content_get(spn, QP_SPN_BOX_PART);
	if (idletxtbox) {
		elm_object_part_content_unset(spn, QP_SPN_BOX_PART);
		evas_object_del(idletxtbox);
	}

	evas_object_del(spn);

	return QP_OK;
}

static int quickpanel_idletxt_suspend(void *data)
{
	return QP_OK;
}

static int quickpanel_idletxt_resume(void *data)
{
	return QP_OK;
}
