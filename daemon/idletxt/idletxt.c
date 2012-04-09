/*                                                                                                                                                    
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All Rights Reserved. 
 *                                                                                 
 * This file is part of quickpanel  
 *
 * Written by Jeonghoon Park <jh1979.park@samsung.com> Youngjoo Park <yjoo93.park@samsung.com>
 *                                                                                 
 * PROPRIETARY/CONFIDENTIAL                                                        
 *                                                                                 
 * This software is the confidential and proprietary information of                
 * SAMSUNG ELECTRONICS ("Confidential Information").                               
 * You shall not disclose such Confidential Information and shall use it only   
 * in accordance with the terms of the license agreement you entered into          
 * with SAMSUNG ELECTRONICS.                                                       
 * SAMSUNG make no representations or warranties about the suitability of          
 * the software, either express or implied, including but not limited to           
 * the implied warranties of merchantability, fitness for a particular purpose, 
 * or non-infringement. SAMSUNG shall not be liable for any damages suffered by 
 * licensee as a result of using, modifying or distributing this software or    
 * its derivatives.                                                                
 *                                                                                 
 */ 

#include <vconf.h>
#include "common.h"
#include "quickpanel-ui.h"

#define QP_IDLETXT_PART			"qp.noti.swallow.spn"

#define QP_IDLETXT_MAX_KEY		4
#define QP_IDLETXT_MAX_LEN		1024
#define QP_IDLETXT_SLIDE_LEN	130

#define QP_IDLETXT_LABEL_STRING	"<font_size=30><color=#8C8C8CFF><align=left>%s</align></color></font_size>"

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
	if (str == NULL || str[0] == '\0') {
		return 0;
	}

	/* check ASCII code */
	for (i = strlen(str) - 1; i >= 0; i--) {
		if (str[i] < 31 || str[i] > 127) {
			goto failed;
		}
	}

	len = snprintf(txt, size, "%s", str);

 failed:
	if (str) {
		free(str);
	}

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
			if (len > QP_IDLETXT_SLIDE_LEN) {
				elm_label_slide_set(obj, EINA_TRUE);
			}

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

	}

	if (i > 0) {
		/* get string with keylist */
		label = _quickpanel_idletxt_add_label(box, keylist);

		/* free keylist */
		while (i > 0) {
			if (keylist[i]) {
				free(keylist[i]);
			}

			i--;
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

static void quickpanel_idletxt_changed_cb(keynode_t * node, void *data)
{
	Evas_Object *label = NULL;
	struct appdata *ad = (struct appdata *)data;

	retif(ad == NULL,, "Invalid parameter!");

	if (ad->idletxtbox == NULL) {
		ad->idletxtbox = _quickpanel_idletxt_create_box(ad->noti.ly);
		retif(ad->idletxtbox == NULL,, "Failed to create box!");

		edje_object_part_swallow(_EDJ(ad->noti.ly), QP_IDLETXT_PART,
					 ad->idletxtbox);
	}

	elm_box_clear(ad->idletxtbox);

	/* get spn */
	label = _quickpanel_idletxt_get_spn(ad->idletxtbox);
	if (label != NULL) {
		elm_box_pack_end(ad->idletxtbox, label);
	}

	/* get sat idle text */
	label = _quickpanel_idletxt_get_sat_text(ad->idletxtbox);
	if (label != NULL) {
		elm_box_pack_end(ad->idletxtbox, label);
	}
}

static int _quickpanel_idletxt_register_event_handler(void *data)
{
	int ret = 0;

	ret =
	    vconf_notify_key_changed(VCONFKEY_TELEPHONY_SPN_DISP_CONDITION,
				     quickpanel_idletxt_changed_cb, data);
	if (ret != 0) {
		ERR("Failed to register VCONFKEY_TELEPHONY_SPN_DISP_CONDITION change callback!");
	}

	ret =
	    vconf_notify_key_changed(VCONFKEY_TELEPHONY_SPN_NAME,
				     quickpanel_idletxt_changed_cb, data);
	if (ret != 0) {
		ERR("Failed to register VCONFKEY_TELEPHONY_SPN_NAME change callback!");
	}

	ret =
	    vconf_notify_key_changed(VCONFKEY_TELEPHONY_NWNAME,
				     quickpanel_idletxt_changed_cb, data);
	if (ret != 0) {
		ERR("Failed to register VCONFKEY_TELEPHONY_NWNAME change callback!");
	}

	ret =
	    vconf_notify_key_changed(VCONFKEY_SAT_IDLE_TEXT,
				     quickpanel_idletxt_changed_cb, data);
	if (ret != 0) {
		ERR("Failed to register VCONFKEY_SAT_IDLE_TEXT change callback!");
	}

	quickpanel_idletxt_changed_cb(NULL, data);

	return QP_OK;
}

static int _quickpanel_idletxt_unregister_event_handler(void)
{
	int ret = 0;

	ret =
	    vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SPN_DISP_CONDITION,
				     quickpanel_idletxt_changed_cb);
	if (ret != 0) {
		ERR("Failed to unregister VCONFKEY_TELEPHONY_SPN_DISP_CONDITION change callback!");
	}

	ret =
	    vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SPN_NAME,
				     quickpanel_idletxt_changed_cb);
	if (ret != 0) {
		ERR("Failed to unregister VCONFKEY_TELEPHONY_SPN_NAME change callback!");
	}

	ret =
	    vconf_ignore_key_changed(VCONFKEY_TELEPHONY_NWNAME,
				     quickpanel_idletxt_changed_cb);
	if (ret != 0) {
		ERR("Failed to unregister VCONFKEY_TELEPHONY_NWNAME change callback!");
	}

	ret =
	    vconf_ignore_key_changed(VCONFKEY_SAT_IDLE_TEXT,
				     quickpanel_idletxt_changed_cb);
	if (ret != 0) {
		ERR("Failed to unregister VCONFKEY_SAT_IDLE_TEXT change callback!");
	}

	return QP_OK;
}

static int quickpanel_idletxt_init(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	ad->idletxtbox = _quickpanel_idletxt_create_box(ad->noti.ly);
	retif(ad->idletxtbox == NULL, QP_FAIL, "Failed to create box!");

	edje_object_part_swallow(_EDJ(ad->noti.ly), QP_IDLETXT_PART,
				 ad->idletxtbox);

	_quickpanel_idletxt_register_event_handler(ad);

	return QP_OK;
}

static int quickpanel_idletxt_fini(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	_quickpanel_idletxt_unregister_event_handler();

	edje_object_part_unswallow(_EDJ(ad->noti.ly), ad->idletxtbox);

	if (ad->idletxtbox != NULL) {
		evas_object_hide(ad->idletxtbox);

		evas_object_del(ad->idletxtbox);

		ad->idletxtbox = NULL;
	}

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
