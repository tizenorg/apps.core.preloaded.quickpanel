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


images {
	image: "quick_menu_divide.png" COMP;
	image: "quick_menu_touch.png" COMP;
	image: "quick_button_each_bg.png" COMP;
	image: "quick_button_each_bg_land.png" COMP;
	image: "quick_button_each_ef.png" COMP;
	image: "quick_button_each_ef_land.png" COMP;
	image: "quick_btn_del_nor.png" COMP;
	image: "quick_btn_del_press.png" COMP;
	image: "quick_button_over_land.png" COMP;
	image: "quick_button_over.png" COMP;
	image: "quick_menu_divide_highlight.png" COMP;
	image: "quick_button_icon_bg.png" COMP;
}

#define ENABLE_COLOR_CLIP

#define QP_SETTINGS_HEIGHT	221

styles
{
	style
	{
		name: "icon_text_style";
		base: "font=Tizen:style=Regular font_size=19 text_class=tizen align=center color=#FFFFFF color_class=ATO017 ellipsis=1.0 wrap=mixed";
	}
}

group {
	name: "quickpanel/setting_container_wvga";

	parts {
		part {
			name: "background";
			type: RECT;
			scale: 1;
			description
			{
				state: "default" 0.0;
				min: 0 QP_SETTINGS_HEIGHT;
				max: -1 QP_SETTINGS_HEIGHT;

				align: 0.0 0.0;
				visible: 0;
			}
			description {
				state: "landscape" 0.0;
				inherit: "default" 0.0;
			}
		}
		part {
			name: "setting.margin.left";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 0;
				fixed: 1 0;
				rel1 {
					relative: 0.0 0.0;
					to: "background";
				}
				rel2 {
					relative: 0.0 1.0;
					to: "background";
				}
				align: 0.0 0.0;
			}
			description {
				state: "portrait" 0.0;
				inherit: "default" 0.0;
			}
			description {
				state: "landscape" 0.0;
				inherit: "default" 0.0;
				min: 0 0;
			}
		}
		part {
			name: "setting.margin.right";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 0;
				fixed: 1 0;
				rel1 {
					relative: 1.0 0.0;
					to: "background";
				}
				rel2 {
					relative: 1.0 1.0;
					to: "background";
				}
				align: 1.0 0.0;
			}
			description {
				state: "portrait" 0.0;
				inherit: "default" 0.0;
			}
			description {
				state: "landscape" 0.0;
				inherit: "default" 0.0;
				min: 0 0;
			}
		}
		part{
			name: "setting.container.swallow.wvga";
			type:SWALLOW;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 QP_SETTING_BG_H;
				fixed: 0 1;
				rel1 {
					to: "background";
					relative: 0.0 0.0;
				}
				rel2 {
					to: "background";
					relative: 1.0 0.0;
				}
				align: 0.5 0.0;
			}
		}
		part {
			name: "brightness.container.swallow.wvga";
			type: SWALLOW;
			scale:1;
			description {
				state: "default" 0.0;
				min: 0 QP_THEME_LIST_ITEM_BRIGHTNESS_HEIGHT;
				fixed: 0 1;
				rel1 {
					to: "background";
					relative: 0.0 1.0;

				}
				rel2 {
					to: "background";
					relative: 1.0 1.0;
				}

				align: 0.5 1.0;
			}
		}
	}

	programs {
		program{
			name: "container.portrait";
			signal: "portrait";
			source: "background";
			action: STATE_SET "portrait" 0.0;
			target: "setting.margin.left";
			target: "setting.margin.right";
			target: "setting.container.swallow.wvga";
			after: "background.default";
		}
		program {
			name : "background.default";
			action: STATE_SET "default" 0.0;
			target: "background";
		}
		program{
			name: "container.landscape";
			signal: "landscape";
			source: "background";
			action: STATE_SET "landscape" 0.0;
			target: "setting.margin.left";
			target: "setting.margin.right";
			target: "setting.container.swallow.wvga";
			target: "background";
		}
	}
}

group {
	name: "quickpanel/setting_icon_divider_wvga";

	script {
		public g_is_hided;
	}

	parts {
		part {
			name: "background";
			type: RECT;
			scale: 1;
			description {
				state: "default" 0.0;
				min: QP_SETTING_DIVIDER_W_P QP_SETTING_ICON_MIN_WH_WVGA;
				fixed: 1 1;
				align: 0.0 0.0;
				color_class: QP_BACKGROUND_COLOR;
				visible: 1;
			}
			description {
				state: "normal" 0.0;
				inherit: "default" 0.0;
			}
			description {
				state: "blank" 0.0;
				inherit: "default" 0.0;
				color_class: QP_BACKGROUND_COLOR;
			}
			description {
				state: "hide" 0.0;
				inherit: "default" 0.0;
				visible: 0;
			}
			description {
				state: "show" 0.0;
				inherit: "default" 0.0;
			}
		}
		part {
			name: "object.image.divider";
			type:IMAGE;
			scale:1;
			description {
				state: "default" 0.0;
				fixed: 1 1;
				min: QP_SETTING_DIVIDER_W_P 99;
				rel1 {
					to: "background";
					relative:0.0 0.5;
				}
				rel2 {
					to: "background";
					relative:1.0 0.5;
				}
				image {
					normal:"quick_menu_divide.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
				align:0.0 0.5;
				visible: 0;
			}
			description {
				state: "highlight.show" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"quick_menu_divide_highlight.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
				visible: 1;
			}
			description {
				state: "highlight.hide" 0.0;
				inherit: "default" 0.0;
			}
			description {
				state: "hide" 0.0;
				inherit: "default" 0.0;
				visible: 0;
			}
			description {
				state: "show" 0.0;
				inherit: "default" 0.0;
				visible: 0;
			}
		}
	}

	programs {
		program {
			name: "init_divider";
			signal: "load";
			script{
				set_int(g_is_hided, 0);
			}
		}
		program{
			name: "divider.highlight.show";
			signal: "highlight.show";
			source: "quickpanl.prog";
			script {
				if (get_int(g_is_hided) == 0) {
					set_state(PART:"object.image.divider", "highlight.show", 0.0);
				}
			}
		}
		program{
			name: "divider.highlight.hide";
			signal: "highlight.hide";
			source: "quickpanl.prog";
			script {
				if (get_int(g_is_hided) == 0) {
					set_state(PART:"object.image.divider", "highlight.hide", 0.0);
				}
			}
		}
		program{
			name: "blank.mode.enable";
			signal: "blank.enable";
			source: "quickpanl.prog";
			script {
				set_state(PART:"background", "blank", 0.0);
				set_state(PART:"object.image.divider", "show", 0.0);
				set_int(g_is_hided, 0);
			}
		}
		program{
			name: "blank.mode.disable";
			signal: "blank.disable";
			source: "quickpanl.prog";
			script {
				set_state(PART:"background", "normal", 0.0);
				set_state(PART:"object.image.divider", "show", 0.0);
				set_int(g_is_hided, 0);
			}
		}
		program{
			name: "divider.hide";
			signal: "divider.hide";
			source: "quickpanl.prog";
			script {
				set_state(PART:"background", "hide", 0.0);
				set_state(PART:"object.image.divider", "hide", 0.0);
				set_int(g_is_hided, 1);
			}
		}
	}
}

#define QP_SETTING_ICON_COLOR_ON "AO002"
#define QP_SETTING_ICON_COLOR_OFF "AO003"
#define QP_SETTING_ICON_COLOR_DIM "AO003D"
#define QP_SETTING_ICON_COLOR_LOADING "AO003E1"

group {
	name: "quickpanel/setting_icon_wvga";

	parts {
		part {
			name: "background.super";
			type: RECT;
			scale: 1;
			mouse_events: 1;
			description {
				state: "default" 0.0;
				min : QP_SETTING_ICON_BG_MIN_W_P_WVGA QP_SETTING_BG_H;
				color: 0 0 0 0;
			}
			description {
				state: "portrait" 0.0;
				inherit: "default" 0.0;
			}
			description {
				state: "landscape" 0.0;
				inherit: "default" 0.0;
				min : QP_SETTING_ICON_BG_MIN_W_L_WVGA QP_SETTING_BG_H;
			}
			description {
				state: "dragging" 0.0;
				inherit: "default" 0.0;
				min : QP_SETTING_ICON_DRAGGING_BG_MIN_W_WVGA QP_SETTING_ICON_DRAGGING_BG_MIN_H;
			}
		}
		part {
			name: "padding.top";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 17;
				fixed: 0 1;
				align: 0.5 0.0;
				rel1 {
					relative: 0.0 0.0;
					to: "background.super";
				}
				rel2 {
					relative: 1.0 0.0;
					to: "background.super";
				}
			}
		}
		part {
			name: "background";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min : QP_SETTING_ICON_BG_MIN_W_P_WVGA QP_SETTING_ICON_MIN_WH_WVGA;
				max : QP_SETTING_ICON_BG_MIN_W_P_WVGA QP_SETTING_ICON_MIN_WH_WVGA;
				rel1 {
					relative : 0.0 1.0;
					to: "padding.top";
				}
				rel2 {
					relative : 1.0 1.0;
					to: "padding.top";
				}
				align: 0.5 0.0;
			}
			description {
				state: "portrait" 0.0;
				inherit: "default" 0.0;
			}
			description {
				state: "landscape" 0.0;
				inherit: "default" 0.0;
				min : QP_SETTING_ICON_BG_MIN_W_L_WVGA QP_SETTING_ICON_MIN_WH_WVGA;
				max : QP_SETTING_ICON_BG_MIN_W_L_WVGA QP_SETTING_ICON_MIN_WH_WVGA;
			}
		}

		part {
			name: "bg.image";
			type : IMAGE;
			scale : 1;
			repeat_events: 1;
			description	{
				state: "default" 0.0;

				min: QP_SETTING_ICON_MIN_WH_WVGA QP_SETTING_ICON_MIN_WH_WVGA;
				max: QP_SETTING_ICON_MIN_WH_WVGA QP_SETTING_ICON_MIN_WH_WVGA;

				image {
					normal:"quick_button_icon_bg.png";
				}

				rel1 {
					relative : 0.5 0.5;
					to: "background";
				}
				rel2 {
					relative : 0.5 0.5;
					to: "background";
				}
				color_class: "AO001L1";
			}
			description {
				state: "pressed" 0.0;
				inherit: "default" 0.0;
				color_class: "AO001L1P";
			}
			description {
				state: "dimmed" 0.0;
				inherit: "default" 0.0;
				color_class: "AO001L1D";
			}
		}

		part {
			name: "pad.top";
			type: SPACER;
			scale : 1;
			mouse_events: 0;
			description {
				state: "default" 0.0;
				min: 0 0;
				max: 0 0;
				rel1 {
					relative: 0.0 0.0;
					to: "background";
				}
				rel2 {
					relative: 1.0 0.0;
					to: "background";
				}
				align: 0.0 0.0;
			}
		}
		part {
			name: "icon.bg";
			type: SPACER;
			scale: 1;
			mouse_events: 0;
			description {
				state: "default" 0.0;
				min: 66 66;
				fixed: 0 1;
				rel1 {
					relative: 0.0 1.0;
					to_x: "background.super";
					to_y: "pad.top";
				}
				rel2 {
					relative: 1.0 1.0;
					to_x: "background.super";
					to_y: "pad.top";
				}
				align: 0.0 0.0;
			}
		}
#ifdef ENABLE_COLOR_CLIP
		part {
			name : "color_clip";
			type : RECT;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 66 66;
				rel1 {
					relative: 0.5 0.5;
					to: "icon.bg";
				}
				rel2 {
					relative: 0.5 0.5;
					to: "icon.bg";
				}
				align: 0.5 0.5;
				color: 255 255 255 255;
				color_class: QP_SETTING_ICON_COLOR_OFF;
			}
			description {
				state: "on" 0.0;
				inherit: "default" 0.0;
				color: 255 255 255 255;
				color_class: QP_SETTING_ICON_COLOR_ON;
			}
			description {
				state: "off" 0.0;
				inherit: "default" 0.0;
				color: 255 255 255 255;
				color_class: QP_SETTING_ICON_COLOR_OFF;
			}
			description {
				state: "dim" 0.0;
				inherit: "default" 0.0;
				color: 255 255 255 255;
				color_class: QP_SETTING_ICON_COLOR_DIM;
			}
			description {
				state: "loading" 0.0;
				inherit: "default" 0.0;
				color: 255 255 255 255;
				color_class: QP_SETTING_ICON_COLOR_LOADING;
			}
		}
#endif
		part {
			name: "icon.swallow.wvga";
			type: SWALLOW;
			scale: 1;
			mouse_events: 0;
#ifdef ENABLE_COLOR_CLIP
			clip_to: "color_clip";
#endif
			description {
				state: "default" 0.0;
				min: 46 46;
				max: 46 46;
				fixed: 1 1;
				rel1 {
					relative: 0.5 0.5;
					to: "background";
				}
				rel2 {
					relative: 0.5 0.5;
					to: "background";
				}
				align: 0.5 0.5;
				visible: 1;
			}
		}

		part {
			name: "spacer.text";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: (96 - 10) (24 + 2 + 24);
				max: (96 - 10) (24 + 2 + 24);
				fixed: 1 1;
				align: 0.5 0.0;
				rel1 {
					relative: 0.0 1.0;
					to: "background";
					offset: 0 3;
				}
				rel2 {
					relative: 1.0 1.0;
					to: "background";
				}
			}
		}

		part
		{
			name: "icon.text";
			type: TEXTBLOCK;
			scale: 1;
			mouse_events: 0;
			multiline: 1;

			description {
				state: "default" 0.0;
				rel1 {
					relative: 0.0 0.0;
					to: "spacer.text";
				}
				rel2 {
					relative: 1.0 1.0;
					to: "spacer.text";
				}
				align: 0.5 0.0;
				text
				{
					style: "icon_text_style";
					align: 0.5 0.0;
					max: 0 1;
				}
			}
		}

		QUICKPANEL_FOCUS_OBJECT("focus", "background.super", "background.super")
	}
	script {
		public icon_rotation;
		public icon_state;
		public icon_text_state;
	}
	programs {
		program{
			name: "icon.portrait";
			signal: "icon.portrait";
			source: "quickpanl.prog";
			script {
				set_int(icon_rotation, 0);
				set_state(PART:"background.super", "portrait", 0.0);
				set_state(PART:"background", "portrait", 0.0);
			}
		}
		program{
			name: "icon.landscape";
			signal: "icon.landscape";
			source: "quickpanl.prog";
			script {
				set_int(icon_rotation, 1);
				set_state(PART:"background.super", "landscape", 0.0);
				set_state(PART:"background", "landscape", 0.0);
			}
		}
		program{
			name: "icon.dragging.on";
			signal: "dragging.on";
			source: "quickpanl.prog";
			script {
				set_state(PART:"background.super", "dragging", 0.0);
				if (get_int(icon_rotation) == 0) {
				} else {
				}
			}
		}
		program{
			name: "icon.dragging.off";
			signal: "dragging.off";
			source: "quickpanl.prog";
			script {
				if (get_int(icon_rotation) == 0) {
					set_state(PART:"background.super", "portrait", 0.0);
				} else {
					set_state(PART:"background.super", "landscape", 0.0);
				}
			}
		}
		program {
			name: "color.progress";
			signal: "icon.progress";
			source: "quickpanl.prog";
			script{
#ifdef ENABLE_COLOR_CLIP
				set_state(PART:"color_clip", "loading", 0.0);
#endif
			}

		}
		program {
			name: "color.off";
			signal: "icon.off";
			source: "quickpanl.prog";
			script{
				set_int(icon_state, 0);
#ifdef ENABLE_COLOR_CLIP
				set_state(PART:"color_clip", "off", 0.0);
#endif
			}

		}
		program {
			name: "color.on";
			signal: "icon.on";
			source: "quickpanl.prog";
			script{
				set_int(icon_state, 1);
#ifdef ENABLE_COLOR_CLIP
				set_state(PART:"color_clip", "on", 0.0);
#endif
			}
		}
		program {
			name: "color.dim";
			signal: "icon.dim";
			source: "quickpanl.prog";
			script{
				set_int(icon_state, 2);
#ifdef ENABLE_COLOR_CLIP
				set_state(PART:"color_clip", "dim", 0.0);
#endif
			}
		}

		program {
			name: "color.release";
			action: STATE_SET "press" 0.0;
			signal: "mouse,up,1";
			source: "background.super";
			script{
				if (get_int(icon_state) == 1) {
					set_state(PART:"bg.image", "default", 0.0);
#ifdef ENABLE_COLOR_CLIP
					set_state(PART:"color_clip", "on", 0.0);
#endif
				} else if (get_int(icon_state) == 2) {
					set_state(PART:"bg.image", "dimmed", 0.0);
#ifdef ENABLE_COLOR_CLIP
					set_state(PART:"color_clip", "dim", 0.0);
#endif
				} else {
					set_state(PART:"bg.image", "default", 0.0);
#ifdef ENABLE_COLOR_CLIP
					set_state(PART:"color_clip", "off", 0.0);
#endif
				}
			}
		}

		program{
			name: "bg.image.press";
			signal: "mouse,down,1";
			source: "background.super";
			action: STATE_SET "pressed" 0.0;
			target: "bg.image";
		}
		program{
			name: "bg.image.release";
			signal: "mouse,up,1";
			source: "background.super";
			action: STATE_SET "default" 0.0;
			target: "bg.image";
		}
	}
}
