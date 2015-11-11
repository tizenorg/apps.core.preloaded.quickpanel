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


#ifndef __QUICKPANEL_VI_MANAGER_H__
#define __QUICKPANEL_VI_MANAGER_H__


typedef Eina_Bool (*vi_cb)(void *data);

typedef enum _qp_vim_state_type {
	VIM_STATE_NOT_READY = 0,
	VIM_STATE_READY,
	VIM_STATE_WORKING,
	VIM_STATE_SUSPENDED,
} qp_vim_state_type;

typedef enum _qp_vi_state_type {
	VI_STATE_NOT_READY = 0,
	VI_STATE_READY = 1,
	VI_STATE_RUNNING,
	VI_STATE_DONE,
	VI_STATE_INTERRUPTED,
} qp_vi_state_type;

typedef enum _qp_vi_op_type {
	VI_OP_NONE = -1,
	VI_OP_INSERT = 0,
	VI_OP_UPDATE,
	VI_OP_DELETE,
	VI_OP_DELETE_ALL,
	VI_OP_REORDER,
	VI_OP_ROTATION,
	VI_OP_RESIZE,
} qp_vi_op_type;

typedef struct _QP_VI {
	qp_vi_state_type state;
	qp_vi_op_type op_type;
	qp_item_type_e item_type;
	void *container;
	void *target;
	vi_cb init_cb;
	vi_cb job_cb;
	vi_cb done_cb;
	vi_cb interrupt_cb;
	int disable_interrupt_userevent;
	int disable_freezing;
	void *extra_data_1;
	void *extra_data_2;
	int extra_flag_1;
	int extra_flag_2;
} QP_VI;

typedef  struct _qp_vi_op_table {
	qp_vi_op_type op_type;
	void (*handler)(void *data);
} qp_vi_op_table;

extern QP_VI *quickpanel_vi_new(void);
extern QP_VI *quickpanel_vi_new_with_data(qp_vi_op_type op_type, qp_item_type_e item_type, void *container, void *target, vi_cb init_cb, vi_cb job_cb, vi_cb done_cb, vi_cb interrupt_cb, void *extra_data_1, void *extra_data_2, int extra_flag_1, int extra_flag_2);
extern void quickpanel_vi_start(QP_VI *vi);
extern void quickpanel_vi_interrupt(QP_VI *vi);
extern void quickpanel_vi_done(QP_VI *vi);
extern void quickpanel_vim_set_state_ready(void);
extern void quickpanel_vim_set_state_suspend(void);
extern double quickpanel_vim_get_duration(qp_vi_op_type op_type);
extern void quickpanel_vi_done_cb_for_transit(void *data, Elm_Transit *transit);
extern Elm_Transit_Tween_Mode quickpanel_vim_get_tweenmode(qp_vi_op_type op_type);

extern void quickpanel_vi_user_event_add(QP_VI *vi);
extern void quickpanel_vi_user_event_del(QP_VI *vi);
extern void quickpanel_vi_object_event_freeze_set(Evas_Object *obj, Eina_Bool freeze);

#endif
