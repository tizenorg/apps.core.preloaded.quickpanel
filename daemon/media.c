/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <vconf.h>
#include "common.h"
#include "quickpanel-ui.h"

static player_h g_sound_player;
static Ecore_Timer *g_sound_player_timer;

static void _quickpanel_player_free_job_cb(void *data)
{
	player_h sound_player = data;
	player_state_e state = PLAYER_STATE_NONE;

	retif(sound_player == NULL, , "invalid parameter");

	if (player_get_state(sound_player, &state) == PLAYER_ERROR_NONE) {

		DBG("state of player %d", state);

		if (state == PLAYER_STATE_PLAYING) {
			player_stop(sound_player);
			player_unprepare(sound_player);
		}
		if (state == PLAYER_STATE_READY) {
			player_unprepare(sound_player);
		}
	}
	player_destroy(sound_player);
}

static void _quickpanel_player_free(player_h *sound_player)
{
	retif(sound_player == NULL, , "invalid parameter");
	retif(*sound_player == NULL, , "invalid parameter");

	ecore_job_add(_quickpanel_player_free_job_cb, *sound_player);
	*sound_player = NULL;
}

static void
_quickpanel_player_del_timeout_timer(void)
{
	if (g_sound_player_timer) {
		ecore_timer_del(g_sound_player_timer);
		g_sound_player_timer = NULL;
	}
}

static Eina_Bool _quickpanel_player_timeout_cb(void *data)
{
	g_sound_player_timer = NULL;

	retif(data == NULL, ECORE_CALLBACK_CANCEL, "invalid parameter");
	player_h *sound_player = data;

	_quickpanel_player_free(sound_player);
	g_sound_player_timer = NULL;

	return ECORE_CALLBACK_CANCEL;
}

static void
_quickpanel_player_completed_cb(void *user_data)
{
	retif(user_data == NULL, , "invalid parameter");
	player_h *sound_player = user_data;

	_quickpanel_player_del_timeout_timer();
	_quickpanel_player_free(sound_player);
}

static void
_quickpanel_player_interrupted_cb(player_interrupted_code_e code, void *user_data)
{
	retif(user_data == NULL, , "invalid parameter");
	player_h *sound_player = user_data;

	_quickpanel_player_del_timeout_timer();
	_quickpanel_player_free(sound_player);
}

static void
_quickpanel_player_error_cb(int error_code, void *user_data)
{
	retif(user_data == NULL, , "invalid parameter");
	player_h *sound_player = user_data;

	_quickpanel_player_del_timeout_timer();
	_quickpanel_player_free(sound_player);
}

HAPI void quickpanel_player_play(sound_type_e sound_type, const char *sound_file)
{
	player_h *sound_player = &g_sound_player;

	int ret = PLAYER_ERROR_NONE;
	player_state_e state = PLAYER_STATE_NONE;

	_quickpanel_player_del_timeout_timer();

	if (*sound_player != NULL) {
		_quickpanel_player_free(sound_player);
	}

	ret = player_create(sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		ERR("creating the player handle failed[%d]", ret);
		player_destroy(*sound_player);
	}

	ret = player_set_sound_type(*sound_player, sound_type);
	if (ret != PLAYER_ERROR_NONE) {
		ERR("player_set_sound_type() ERR: %x!!!!", ret);
		_quickpanel_player_free(sound_player);
		return ;
	}

	player_get_state(*sound_player, &state);
	if (state > PLAYER_STATE_READY) {
		_quickpanel_player_free(sound_player);
		return;
	}

	ret = player_set_uri(*sound_player, sound_file);
	if (ret != PLAYER_ERROR_NONE) {
		DBG("set attribute---profile_uri[%d]", ret);
		_quickpanel_player_free(sound_player);
		return;
	}

	ret = player_prepare(*sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		DBG("realizing the player handle failed[%d]", ret);
		_quickpanel_player_free(sound_player);
		return;
	}

	player_get_state(*sound_player, &state);
	if (state != PLAYER_STATE_READY) {
		DBG("state of player is invalid %d", state);
		_quickpanel_player_free(sound_player);
		return;
	}

	/* register callback */
	ret = player_set_completed_cb(*sound_player, _quickpanel_player_completed_cb, sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		DBG("player_set_completed_cb() ERR: %x!!!!", ret);
		_quickpanel_player_free(sound_player);
		return;
	}

	ret = player_set_interrupted_cb(*sound_player, _quickpanel_player_interrupted_cb, sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		_quickpanel_player_free(sound_player);
		return;
	}

	ret = player_set_error_cb(*sound_player, _quickpanel_player_error_cb, sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		_quickpanel_player_free(sound_player);
		return;
	}

	ret = player_start(*sound_player);
	if (ret != PLAYER_ERROR_NONE) {	/* if directly return retor.. */
		DBG("player_start [%d]", ret);
		_quickpanel_player_free(sound_player);
		return;
	}

	g_sound_player_timer = ecore_timer_add(QP_PLAY_DURATION_LIMIT,
			_quickpanel_player_timeout_cb, sound_player);
}

HAPI void quickpanel_player_stop(void)
{
	_quickpanel_player_del_timeout_timer();

	if (g_sound_player != NULL) {
		_quickpanel_player_free(&g_sound_player);
	}
}

HAPI int quickpanel_is_sound_enabled(void)
{
	int snd_status = 0;

#ifdef VCONFKEY_SETAPPL_ACCESSIBILITY_TURN_OFF_ALL_SOUNDS
	int snd_disabled_status = 0;

	vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TURN_OFF_ALL_SOUNDS, &snd_disabled_status);
	vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &snd_status);

	if (snd_disabled_status == 0 && snd_status == 1) {
		return 1;
	}
#else
	vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &snd_status);

	if (snd_status == 1) {
		return 1;
	}
#endif

	return 0;
}

HAPI int quickpanel_is_vib_enabled(void)
{
	int vib_status = 0;

	vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, &vib_status);

	if (vib_status == 1)
		return 1;
	else
		return 0;
}

HAPI void quickpanel_play_feedback(void)
{
	int snd_enabled = quickpanel_is_sound_enabled();
	int vib_enabled = quickpanel_is_vib_enabled();

	if (snd_enabled == 1) {
		feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TOUCH_TAP);
	} else  if (vib_enabled == 1) {
		feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_TOUCH_TAP);
	}
}
