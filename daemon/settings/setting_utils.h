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


#ifndef __SETTING_UTILS_H__
#define __SETTING_UTILS_H__


#define TIMER_CONTINUE	1
#define TIMER_STOP	0
#define TIMER_COUNT	10

#define QP_SETTING_BASE_PART "qp.base.setting.swallow"
#define QP_SETTING_SCROLLER_PART_HD "setting.container.swallow.hd"
#define QP_SETTING_SCROLLER_PART_WVGA "setting.container.swallow.wvga"
#define QP_SETTING_CONTAINER_ICON_PART "setting.icon.swallow"
#define QP_SETTING_BRIGHTNESS_PART_HD "brightness.container.swallow.hd"
#define QP_SETTING_BRIGHTNESS_PART_WVGA "brightness.container.swallow.wvga"

extern int quickpanel_setting_start(Evas_Object *base);
extern int quickpanel_setting_stop(Evas_Object *base, int is_bring_in);

extern Evas_Object *quickpanel_setting_scroller_get(Evas_Object *base);
extern int quickpanel_setting_set_scroll_page_width(void *data);
extern int quickpanel_setting_layout_set(Evas_Object *base, Evas_Object *setting);
extern Evas_Object *quickpanel_setting_layout_get(Evas_Object *base, const char *setting_part);

extern int quickpanel_setting_layout_remove(Evas_Object *base);
extern int quickpanel_setting_icon_text_set(Evas_Object *icon, const char *text, int state);
extern void quickpanel_setting_icon_access_text_set(Evas_Object *icon, const char *text);
extern int quickpanel_setting_icon_content_set(Evas_Object *icon, Evas_Object *content);

extern Evas_Object *quickpanel_setting_box_get(Evas_Object *base);
extern Evas_Object *quickpanel_setting_icon_new(Evas_Object *parent);
extern Evas_Object *quickpanel_setting_icon_image_new(Evas_Object *parent, const char *img_path);
extern int quickpanel_setting_icon_pack(Evas_Object *box, Evas_Object *icon, int is_attach_divider);
extern void quickpanel_setting_icon_unpack_all(Evas_Object *box);
extern int quickpanel_setting_container_rotation_set(Evas_Object *base, int angle);
extern int quickpanel_setting_icons_rotation_set(Evas_Object *base, int angle);
extern int quickpanel_setting_icons_dragging_set(Evas_Object *icon, int is_on);
extern int quickpanel_setting_icons_screen_mode_set(Evas_Object *icon, int screen_mode);
extern void quickpanel_setting_icons_emit_sig(Evas_Object *icon, const char *signal);
extern Evas_Object *quickpanel_setting_icon_content_get(Evas_Object *icon);
extern int quickpanel_setting_icon_content_set(Evas_Object *icon, Evas_Object *content);

extern int quickpanel_setting_icon_state_set(Evas_Object *icon, int is_on);
extern int quickpanel_setting_icon_state_progress_set(Evas_Object *icon);

// Do not use full window popup in quickpanel
extern void quickpanel_setting_create_confirm_popup(Evas_Object *parent, char *title, char *text, Evas_Smart_Cb func);
extern void quickpanel_setting_create_2button_confirm_popup(Evas_Object *parent, char *title, char *text, char *btn1_text, Evas_Smart_Cb btn1_func, char *btn2_text, Evas_Smart_Cb btn2_func);
extern void quickpanel_setting_create_timeout_popup(Evas_Object *parent, char *msg);

extern int quickpanel_setting_scroll_page_get(void *data);

#endif /* __SETTING_UTILS_H__ */
