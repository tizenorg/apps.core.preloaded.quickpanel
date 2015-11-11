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

#ifndef __QUICKPANEL_MEDIA_H__
#define __QUICKPANEL_MEDIA_H__

#define QP_PLAY_DURATION_LIMIT 15

extern int quickpanel_media_player_play(sound_type_e sound_type, const char *sound_file);
extern void quickpanel_media_player_stop(void);
extern int quickpanel_media_is_sound_enabled(void);
extern int quickpanel_media_is_vib_enabled(void);
extern void quickpanel_media_play_feedback(void);
extern int quickpanel_media_set_mute_toggle(void);
extern void quickpanel_media_player_id_set(int id);
extern int quickpanel_media_player_id_get(void);
extern Eina_Bool quickpanel_media_playable_check(const char *file_path);
extern int quickpanel_media_player_is_drm_error(int error_code);
extern void quickpanel_media_init(void);
extern void quickpanel_media_fini(void);

#endif
