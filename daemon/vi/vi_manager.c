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
#include "common_uic.h"
#include "common.h"
#include "list_util.h"
#include "vi_manager.h"
#include "pager.h"

static int _init(void *data);
static int _fini(void *data);
static int _resume(void *data);
static void _qp_opened(void *data);

QP_Module vi_manager = {
	.name = "vi_manager",
	.init = _init,
	.fini = _fini,
	.resume = _resume,
	.qp_opened = _qp_opened,
	.lang_changed = NULL,
	.refresh = NULL
};

static struct info {
	Eina_List *vi_list;
	Eina_List *vi_user_event_list;
	QP_VI *current;
	qp_vim_state_type state;
} s_info = {
	.vi_list = NULL,
	.vi_user_event_list = NULL,
	.current = NULL,
	.state = VIM_STATE_NOT_READY,
};

static QP_VI *_vi_list_get_first(void);
static QP_VI *_vi_user_event_list_get_first(void);
static void _vi_list_del(QP_VI *vi);

static inline void _vim_set_state(qp_vim_state_type state)
{
	s_info.state = state;
}

static inline qp_vim_state_type _vim_get_state(void)
{
	return s_info.state;
}

static void _vi_freeze_start(void)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid parameter");
	retif(ad->list == NULL, , "invalid parameter");

	if (!evas_object_freeze_events_get(ad->scroller)) {
		INFO("VIM freezing");
		evas_object_freeze_events_set(ad->scroller, EINA_TRUE);
	}
}

static void _vi_freeze_stop(void)
{
	struct appdata *ad = quickpanel_get_app_data();
	retif(ad == NULL, , "invalid parameter");
	retif(ad->list == NULL, , "invalid parameter");

	if (evas_object_freeze_events_get(ad->scroller)) {
		INFO("VIM unfreezing");
		evas_object_freeze_events_set(ad->scroller, EINA_FALSE);
	}
}

static void _vi_restart_job_cb(void *data)
{
	QP_VI *next_vi = NULL;
	QP_VI *user_vi = NULL;

	next_vi = _vi_list_get_first();

	if (next_vi != NULL && next_vi->disable_interrupt_userevent == 0) {
		while ((user_vi = _vi_user_event_list_get_first()) != NULL) {
			if (user_vi->interrupt_cb != NULL) {
				user_vi->interrupt_cb(user_vi);
				user_vi->state = VI_STATE_INTERRUPTED;
			}
			quickpanel_vi_user_event_del(user_vi);
		}
	}

	if (_vim_get_state() == VIM_STATE_READY) {
		next_vi = _vi_list_get_first();

		if (next_vi) {
			if (next_vi->job_cb != NULL) {
				_vim_set_state(VIM_STATE_WORKING);
				next_vi->state = VI_STATE_RUNNING;
				next_vi->job_cb(next_vi);
			}
		}
	} else if (_vim_get_state() == VIM_STATE_SUSPENDED
			|| _vim_get_state() == VIM_STATE_NOT_READY){
		while ((next_vi = _vi_list_get_first()) != NULL) {
			quickpanel_vi_done(next_vi);
		}
	}
}

static void _vi_list_add(QP_VI *new_vi)
{
	retif(new_vi == NULL, ,"invalid parameter");

	s_info.vi_list = eina_list_append(s_info.vi_list, new_vi);
}

static int _vi_list_count(void)
{
	retif(s_info.vi_list == NULL, 0, "list null");

	return eina_list_count(s_info.vi_list);
}

static void _vi_list_del(QP_VI *vi)
{
	retif(vi == NULL, ,"invalid parameter");

	s_info.vi_list = eina_list_remove(s_info.vi_list, vi);
}

static int _vi_list_is_data_valid(void *node)
{
	if (eina_list_data_find(s_info.vi_list, node) != NULL) {
		return 1;
	}

	return 0;
}

static QP_VI *_vi_list_get_first(void)
{
	QP_VI *vi = eina_list_nth(s_info.vi_list, 0);

	return vi;
}

static QP_VI *_vi_user_event_list_get_first(void)
{
	QP_VI *vi = eina_list_nth(s_info.vi_user_event_list, 0);

	return vi;
}

HAPI QP_VI *quickpanel_vi_new(void)
{
	QP_VI *vi = (QP_VI *)calloc(1, sizeof(QP_VI));

	retif(vi == NULL, NULL, "failed to memory allocation");

	return vi;
}

HAPI QP_VI *quickpanel_vi_new_with_data(qp_vi_op_type op_type, qp_item_type_e item_type, void *container, void *target, vi_cb init_cb, vi_cb job_cb, vi_cb done_cb, vi_cb interrupt_cb, void *extra_data_1, void *extra_data_2, int extra_flag_1, int extra_flag_2)
{
	QP_VI *vi = (QP_VI *)calloc(1, sizeof(QP_VI));

	retif(vi == NULL, NULL, "failed to memory allocation");

	DBG("");

	vi->state = VI_STATE_NOT_READY;
	vi->op_type = op_type;
	vi->item_type = item_type;
	vi->container = container;
	vi->target = target;
	vi->init_cb = init_cb;
	vi->job_cb = job_cb;
	vi->done_cb = done_cb;
	vi->interrupt_cb = interrupt_cb;
	vi->extra_data_1 = extra_data_1;
	vi->extra_data_2 = extra_data_2;
	vi->extra_flag_1 = extra_flag_1;
	vi->extra_flag_2 = extra_flag_2;

	return vi;
}

HAPI void quickpanel_vi_start(QP_VI *vi)
{
	retif(vi == NULL, , "vi is NULL");

	/*
	 * workaround - turn off mapbuf
	 * if mapbuf is enabled, geometry information from object become invalid
	 */
	quickpanel_pager_mapbuf_set(0);

	if (vi->init_cb != NULL) {
		vi->init_cb(vi);
	}

	vi->state = VI_STATE_READY;

	if (vi->disable_freezing == 0) {
		_vi_freeze_start();
	}
	_vi_list_add(vi);
	if (vi->target != NULL) {
		evas_object_ref(vi->target);
		if (vi->op_type == VI_OP_DELETE) {
			evas_object_freeze_events_set(vi->target, EINA_TRUE);
		}
	}
	if (_vim_get_state() == VIM_STATE_NOT_READY) {
		_vi_restart_job_cb(NULL);
	} else {
		ecore_job_add(_vi_restart_job_cb, NULL);
	}
}

HAPI void quickpanel_vi_interrupt(QP_VI *vi)
{
	retif(vi == NULL, , "vi is NULL");

	if (!_vi_list_is_data_valid(vi)) {
		return;
	}

	_vi_list_del(vi);

	if (vi->interrupt_cb != NULL) {
		vi->interrupt_cb(vi);
		vi->state = VI_STATE_INTERRUPTED;
	}

	if (s_info.current == vi) {
		s_info.current = NULL;
	}
	if (_vim_get_state() == VIM_STATE_WORKING) {
		if (!quickpanel_uic_is_opened()) {
			_vim_set_state(VIM_STATE_SUSPENDED);
		} else {
			_vim_set_state(VIM_STATE_READY);
		}
	}

	evas_object_unref(vi->target);
	free(vi);

	if (_vi_list_count() <= 0) {
		_vi_freeze_stop();
	}

	ecore_job_add(_vi_restart_job_cb, NULL);
}

HAPI void quickpanel_vi_done(QP_VI *vi)
{
	retif(vi == NULL, , "vi is NULL");

	if (!_vi_list_is_data_valid(vi)) {
		return;
	}

	_vi_list_del(vi);

	if (vi->done_cb != NULL) {
		vi->done_cb(vi);
		vi->state = VI_STATE_DONE;
	}

	if (s_info.current == vi) {
		s_info.current = NULL;
	}
	if (_vim_get_state() == VIM_STATE_WORKING) {
		if (!quickpanel_uic_is_opened()) {
			_vim_set_state(VIM_STATE_SUSPENDED);
		} else {
			_vim_set_state(VIM_STATE_READY);
		}
	}

	evas_object_unref(vi->target);
	free(vi);

	if (_vi_list_count() <= 0) {
		_vi_freeze_stop();
	}

	ecore_job_add(_vi_restart_job_cb, NULL);
}

HAPI void quickpanel_vi_done_cb_for_transit(void *data, Elm_Transit *transit)
{
	retif(data == NULL, , "data is NULL");

	quickpanel_vi_done(data);
}

HAPI void quickpanel_vim_set_state_ready(void)
{
	if (!quickpanel_uic_is_opened()) {
		_vim_set_state(VIM_STATE_SUSPENDED);
	} else {
		_vim_set_state(VIM_STATE_READY);
	}
}

HAPI void quickpanel_vim_set_state_suspend(void)
{
	_vim_set_state(VIM_STATE_SUSPENDED);
}

#define VIM_DURATION_INSERT 0.17
#define VIM_DURATION_UPDATE 0.17
#define VIM_DURATION_DELETE 0.17
#define VIM_DURATION_REORDER 0.25
#define VIM_THROTTLE_THRESHOLD 5

HAPI double quickpanel_vim_get_duration(qp_vi_op_type op_type)
{
	int count = 0;
	double duration = 0.0;

	switch (op_type) {
	case VI_OP_INSERT:
		duration =  VIM_DURATION_INSERT;
		break;
	case VI_OP_UPDATE:
		duration =  VIM_DURATION_UPDATE;
		break;
	case VI_OP_DELETE:
		duration =  VIM_DURATION_DELETE;
		break;
	case VI_OP_REORDER:
		duration =  VIM_DURATION_REORDER;
		break;
	default :
		duration = VIM_DURATION_INSERT;
		break;
	}

	if ((count = _vi_list_count()) > VIM_THROTTLE_THRESHOLD) {
		duration = duration * (1.0 / (double)count);
	}

	return duration;
}

HAPI Elm_Transit_Tween_Mode quickpanel_vim_get_tweenmode(qp_vi_op_type op_type)
{
	Elm_Transit_Tween_Mode mode = ELM_TRANSIT_TWEEN_MODE_LINEAR;

	switch (op_type) {
		case VI_OP_INSERT:
			mode =  ELM_TRANSIT_TWEEN_MODE_DECELERATE;
			break;
		case VI_OP_UPDATE:
			mode =  ELM_TRANSIT_TWEEN_MODE_DECELERATE;
			break;
		case VI_OP_DELETE:
			mode =  ELM_TRANSIT_TWEEN_MODE_ACCELERATE;
			break;
		case VI_OP_REORDER:
			mode =  ELM_TRANSIT_TWEEN_MODE_SINUSOIDAL;
			break;
		default :
			mode = ELM_TRANSIT_TWEEN_MODE_LINEAR;
			break;
	}

	return mode;
}

/*****************************************************************************
 *
 * Util functions
 *
 *****************************************************************************/
static int _init(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	_vim_set_state(VIM_STATE_NOT_READY);

	return QP_OK;
}

static int _fini(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	_vim_set_state(VIM_STATE_NOT_READY);

	return QP_OK;
}

static int _resume(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, QP_FAIL, "Invalid parameter!");

	if (_vim_get_state() == VIM_STATE_SUSPENDED) {
		_vim_set_state(VIM_STATE_READY);
	}

	return QP_OK;
}

static void _qp_opened(void *data)
{
	struct appdata *ad = data;
	retif(ad == NULL, , "Invalid parameter!");

	if (_vim_get_state() == VIM_STATE_SUSPENDED) {
		_vim_set_state(VIM_STATE_READY);
	}
}

HAPI void quickpanel_vi_user_event_add(QP_VI *new_vi)
{
	retif(new_vi == NULL, ,"invalid parameter");

	s_info.vi_user_event_list = eina_list_append(s_info.vi_user_event_list, new_vi);
}

HAPI void quickpanel_vi_user_event_del(QP_VI *vi)
{
	retif(vi == NULL, ,"invalid parameter");

	if (eina_list_data_find(s_info.vi_user_event_list, vi) != NULL) {
		s_info.vi_user_event_list = eina_list_remove(s_info.vi_user_event_list, vi);
		free(vi);
	}
}

HAPI void quickpanel_vi_object_event_freeze_set(Evas_Object *obj, Eina_Bool freeze)
{
	retif(obj == NULL, ,"invalid parameter");

	evas_object_freeze_events_set(obj, freeze);
}
