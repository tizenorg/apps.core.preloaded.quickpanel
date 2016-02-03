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
#include <unistd.h>
#include <glib.h>
#include <unistd.h>

#include <Elementary.h>

#include <vconf.h>
#include <metadata_extractor.h>
#include <feedback.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <player.h>
#include <E_DBus.h>

#include "common.h"
#include "quickpanel-ui.h"
#include "media.h"

#define NEED_TO_DEBUG_LOCKUP_ISSUE

static struct info {
	int id;
	int is_feedback_initialized;
	player_h player;
	sound_stream_info_h stream_info;
	Ecore_Timer *playing_timer;
} s_info = {
	.player = NULL,
	.playing_timer = NULL,
	.stream_info = NULL,
	.id = 0,
	.is_feedback_initialized = 0,
};

static void _quickpanel_player_free(player_h *sound_player);

static void _quickpanel_player_del_timeout_timer(void)
{
	if (s_info.playing_timer) {
		ecore_timer_del(s_info.playing_timer);
		s_info.playing_timer = NULL;
	}
}

static Eina_Bool _quickpanel_player_timeout_cb(void *data)
{
	s_info.playing_timer = NULL;

	retif(data == NULL, ECORE_CALLBACK_CANCEL, "invalid parameter");
	player_h *sound_player = data;

	_quickpanel_player_free(sound_player);
	s_info.playing_timer = NULL;

	return ECORE_CALLBACK_CANCEL;
}

static void _quickpanel_player_free_job_cb(void *data)
{
	player_h sound_player = data;
	player_state_e state = PLAYER_STATE_NONE;
	sound_stream_focus_state_e state_for_playback;

	int ret = PLAYER_ERROR_NONE;
	retif(sound_player == NULL, , "invalid parameter");

#ifdef NEED_TO_DEBUG_LOCKUP_ISSUE
	SERR("before stopping media");
#endif
	if (player_get_state(sound_player, &state) == PLAYER_ERROR_NONE) {

		INFO("the state of sound player %d", state);

		if (state == PLAYER_STATE_PLAYING) {
			player_stop(sound_player);
			player_unprepare(sound_player);
		}
		if (state == PLAYER_STATE_READY) {
			player_unprepare(sound_player);
		}
	}
	player_destroy(sound_player);
#ifdef NEED_TO_DEBUG_LOCKUP_ISSUE
	SERR("after stopping media");
#endif

	ret = sound_manager_get_focus_state(s_info.stream_info, &state_for_playback, NULL);

	if (state_for_playback == SOUND_STREAM_FOCUS_STATE_ACQUIRED) {
		ret = sound_manager_release_focus(s_info.stream_info, SOUND_STREAM_FOCUS_FOR_PLAYBACK, NULL);
		if (ret != SOUND_MANAGER_ERROR_NONE) {
			ERR("sound_manager_release_focus() get failed : %d", ret);
		}
	}

	ret = sound_manager_destroy_stream_information(s_info.stream_info);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		ERR("sound_manager_destroy_stream_information() get failed : %d", ret);
	}

	s_info.stream_info = NULL;

	DBG("");
}

static void _quickpanel_player_free(player_h *sound_player)
{
	retif(sound_player == NULL, , "invalid parameter");
	retif(*sound_player == NULL, , "invalid parameter");

	ecore_job_add(_quickpanel_player_free_job_cb, *sound_player);
	*sound_player = NULL;
}

static void _quickpanel_player_start_job_cb(void *data)
{
	int ret = PLAYER_ERROR_NONE;
	player_h *sound_player = data;

#ifdef NEED_TO_DEBUG_LOCKUP_ISSUE
	SERR("before playing media");
#endif
	ret = player_start(*sound_player);
	if (ret != PLAYER_ERROR_NONE) {	/* if directly return retor.. */
		ERR("player_start [%d]", ret);
		_quickpanel_player_free(sound_player);
		return;
	}
	s_info.playing_timer = ecore_timer_add(QP_PLAY_DURATION_LIMIT,
			_quickpanel_player_timeout_cb, sound_player);
#ifdef NEED_TO_DEBUG_LOCKUP_ISSUE
	SERR("after playing media");
#endif
}

static void _quickpanel_player_completed_cb(void *user_data)
{
	retif(user_data == NULL, , "invalid parameter");
	player_h *sound_player = user_data;

	DBG("Media player completed");

	_quickpanel_player_del_timeout_timer();
	_quickpanel_player_free(sound_player);
}

static void _quickpanel_player_error_cb(int error_code, void *user_data)
{
	retif(user_data == NULL, , "invalid parameter");
	player_h *sound_player = user_data;

	ERR("Error code [%d]", (int)error_code);

	_quickpanel_player_del_timeout_timer();
	_quickpanel_player_free(sound_player);
}

HAPI int quickpanel_media_player_is_drm_error(int error_code)
{
	if (error_code == PLAYER_ERROR_DRM_EXPIRED
			|| error_code == PLAYER_ERROR_DRM_NO_LICENSE
			|| error_code == PLAYER_ERROR_DRM_FUTURE_USE
			|| error_code == PLAYER_ERROR_DRM_NOT_PERMITTED) {
		return 1;
	}

	return 0;
}


static void _quickpanel_sound_stream_focus_state_changed_cb(sound_stream_info_h stream_info, sound_stream_focus_change_reason_e reason_for_change, const char *additional_info, void *user_data)
{
	DBG("_quickpanel_sound_stream_focus_state_changed_cb called, reason_for_change [%d], additional_info [%s]", reason_for_change, additional_info);

	retif(user_data == NULL, , "invalid parameter");
	player_h *sound_player = user_data;

	_quickpanel_player_del_timeout_timer();
	_quickpanel_player_free(sound_player);
}

HAPI int quickpanel_media_player_play(sound_type_e sound_type, const char *sound_file)
{
	player_h *sound_player = &s_info.player;
	sound_stream_info_h *stream_info = &s_info.stream_info;

	int ret = PLAYER_ERROR_NONE;
	int sndRet = SOUND_MANAGER_ERROR_NONE;
	player_state_e state = PLAYER_STATE_NONE;

#ifdef NEED_TO_DEBUG_LOCKUP_ISSUE
	SERR("Start player");
#endif
	_quickpanel_player_del_timeout_timer();

	if (*sound_player != NULL) {
		_quickpanel_player_free(sound_player);
	}

#ifdef NEED_TO_DEBUG_LOCKUP_ISSUE
	SERR("setting sound session start");
#endif

	if (*stream_info != NULL) {
		sndRet = sound_manager_destroy_stream_information(*stream_info);
		if (sndRet != SOUND_MANAGER_ERROR_NONE) {
			ERR("sound_manager_destroy_stream_information() get failed : %x", ret);
		}
	}

	if (sound_type == SOUND_TYPE_NOTIFICATION) {
		sndRet = sound_manager_create_stream_information(SOUND_STREAM_TYPE_NOTIFICATION, _quickpanel_sound_stream_focus_state_changed_cb, (void*)sound_player, stream_info);
		if (sndRet != SOUND_MANAGER_ERROR_NONE) {
			ERR("sound_manager_create_stream_information() get failed :%x", sndRet);
			return PLAYER_ERROR_INVALID_PARAMETER;
		}

		sndRet = sound_manager_set_focus_reacquisition(*stream_info, false);
		if (sndRet != SOUND_MANAGER_ERROR_NONE) {
			ERR("sound_manager_set_focus_reacquisition() set failed : %d", ret);
			return sndRet;
		}

		sndRet = sound_manager_acquire_focus(*stream_info, SOUND_STREAM_FOCUS_FOR_PLAYBACK, NULL);
		if (sndRet != SOUND_MANAGER_ERROR_NONE) {
			ERR("sound_manager_acquire_focus() get failed : %d", ret);
			return sndRet;
		}
	}

#ifdef NEED_TO_DEBUG_LOCKUP_ISSUE
	SERR("setting sound session finished");
#endif

#ifdef NEED_TO_DEBUG_LOCKUP_ISSUE
	SERR("player_create start");
#endif
	ret = player_create(sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		ERR("creating the player handle failed[%d]", ret);
		*sound_player = NULL;
		return ret;
	}
#ifdef NEED_TO_DEBUG_LOCKUP_ISSUE
	SERR("player_create finished");
#endif

	player_get_state(*sound_player, &state);
	if (state > PLAYER_STATE_READY) {
		_quickpanel_player_free(sound_player);
		return ret;
	}

	ret = player_set_uri(*sound_player, sound_file);
	if (ret != PLAYER_ERROR_NONE) {
		ERR("set attribute---profile_uri[%d]", ret);
		_quickpanel_player_free(sound_player);
		return ret;
	}

	if (*stream_info != NULL) {
		ret = player_set_audio_policy_info(*sound_player, *stream_info);
		if (ret != PLAYER_ERROR_NONE) {
			ERR("player_set_audio_policy_info failed : %d", ret);
			_quickpanel_player_free(sound_player);
			return ret;
		}
	}

	ret = player_prepare(*sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		ERR("realizing the player handle failed[%d]", ret);
		_quickpanel_player_free(sound_player);
		return ret;
	}

	player_get_state(*sound_player, &state);
	if (state != PLAYER_STATE_READY) {
		ERR("state of player is invalid %d", state);
		_quickpanel_player_free(sound_player);
		return ret;
	}

	/* register callback */
	ret = player_set_completed_cb(*sound_player, _quickpanel_player_completed_cb, sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		ERR("player_set_completed_cb() ERR: %x!!!!", ret);
		_quickpanel_player_free(sound_player);
		return ret;
	}

	ret = player_set_error_cb(*sound_player, _quickpanel_player_error_cb, sound_player);
	if (ret != PLAYER_ERROR_NONE) {
		_quickpanel_player_free(sound_player);
		return ret;
	}

	ecore_job_add(_quickpanel_player_start_job_cb, sound_player);
#ifdef NEED_TO_DEBUG_LOCKUP_ISSUE
	SERR("playing request");
#endif

	return ret;
}

static Eina_Bool _playable_check(const char *file_path)
{
	char *value = NULL;
	int ret_meta =  METADATA_EXTRACTOR_ERROR_NONE;
	metadata_extractor_h metadata = NULL;
	Eina_Bool ret = EINA_FALSE;

	ret_meta = metadata_extractor_create(&metadata);
	if (ret_meta != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("Failed to create metadata extractor:%d", ret_meta);
		return ret;
	}

	if (metadata == NULL) {
		ERR("Failed to create metadata extractor:%d", ret_meta);
		return ret;
	}

	ret_meta = metadata_extractor_set_path(metadata, file_path);
	if (ret_meta != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("Failed to set path to meta extractor:%d", ret_meta);
		metadata_extractor_destroy(metadata);
		return ret;
	}
	ret_meta = metadata_extractor_get_metadata(metadata, METADATA_HAS_AUDIO, &value);
	if (ret_meta != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("Failed to get metadata:%d", ret_meta);
		metadata_extractor_destroy(metadata);
		return ret;
	}

	if(value && g_strcmp0(value, "0")) {
		ret = EINA_TRUE;
	}

	free(value);

	DBG("%s :: playable[%d]", file_path, ret);
	metadata_extractor_destroy(metadata);
	return ret;
}

HAPI Eina_Bool quickpanel_media_playable_check(const char *file_path)
{
	Eina_Bool ret = EINA_FALSE;

	/* Check file exist or not */
	ret = ecore_file_exists(file_path);
	if (ret == EINA_FALSE) {
		ERR("%s file does not exist", file_path);
		return ret;
	}

	/* Check file playable or not */
	ret = _playable_check(file_path);
	if (ret == EINA_FALSE) {
		ERR("%s file does not playable", file_path);
		return ret;
	}

	return ret;
}


HAPI void quickpanel_media_player_stop(void)
{
	_quickpanel_player_del_timeout_timer();

	if (s_info.player != NULL) {
		_quickpanel_player_free(&s_info.player);
	}

	quickpanel_media_player_id_set(0);
}

HAPI void quickpanel_media_player_id_set(int id)
{
	s_info.id = id;
}

HAPI int quickpanel_media_player_id_get(void)
{
	return s_info.id;
}

HAPI int quickpanel_media_is_sound_enabled(void)
{
	int snd_status = 0, ret = -1;

#ifdef VCONFKEY_SETAPPL_ACCESSIBILITY_TURN_OFF_ALL_SOUNDS
	int snd_disabled_status = 0;

	ret = vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TURN_OFF_ALL_SOUNDS, &snd_disabled_status);
	msgif(ret != 0, "failed to get VCONFKEY_SETAPPL_ACCESSIBILITY_TURN_OFF_ALL_SOUNDS");
	ret = vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &snd_status);
	msgif(ret != 0, "failed to get VCONFKEY_SETAPPL_SOUND_STATUS_BOOL");

	if (snd_disabled_status == 0 && snd_status == 1) {
		return 1;
	}
#else
	ret = vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &snd_status);
	msgif(ret != 0, "failed to get VCONFKEY_SETAPPL_SOUND_STATUS_BOOL");

	if (snd_status == 1) {
		return 1;
	}
#endif

	return 0;
}

HAPI int quickpanel_media_is_vib_enabled(void)
{
	int vib_status = 0, ret = -1;

	ret = vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, &vib_status);
	if (ret == 0) {
		if (vib_status == 1)
			return 1;
	} else {
		ERR("failed to get a value of VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL");
	}

	return 0;
}

HAPI void quickpanel_media_play_feedback(void)
{
	int snd_enabled = quickpanel_media_is_sound_enabled();
	int vib_enabled = quickpanel_media_is_vib_enabled();

	quickpanel_media_init();

	if (snd_enabled == 1) {
		feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TAP);
	} else  if (vib_enabled == 1) {
		feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_TAP);
	}
}

HAPI int quickpanel_media_set_mute_toggle(void)
{
	int ret = -1;

	if (quickpanel_media_is_sound_enabled() == 1 ||
			quickpanel_media_is_vib_enabled() == 1) {
		ret = vconf_set_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, 0);
		msgif(ret != 0, "failed to set VCONFKEY_SETAPPL_SOUND_STATUS_BOOL");

		ret = vconf_set_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, 0);
		msgif(ret != 0, "failed to set VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL");

		return 0;
	} else {
		ret = vconf_set_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, 1);
		msgif(ret != 0, "failed to set VCONFKEY_SETAPPL_SOUND_STATUS_BOOL");

		ret = vconf_set_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, 0);
		msgif(ret != 0, "failed to set VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL");

		return 1;
	}
}

HAPI void quickpanel_media_init(void)
{
	if (s_info.is_feedback_initialized == 0) {
		if (feedback_initialize() == FEEDBACK_ERROR_NONE) {
			s_info.is_feedback_initialized = 1;
		} else {
			ERR("failed to init feedback API");
		}
	}
}

HAPI void quickpanel_media_fini(void)
{
	if (s_info.is_feedback_initialized == 1) {
		if (feedback_deinitialize() == FEEDBACK_ERROR_NONE) {
			s_info.is_feedback_initialized = 0;
		} else {
			ERR("failed to deinit feedback API");
		}
	}
}
