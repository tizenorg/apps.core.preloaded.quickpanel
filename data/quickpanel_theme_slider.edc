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

#include "color_classes.edc"

#define SLIDER_BASE_HEIGHT_INC  6
#define SLIDER_INDICATOR_SIZE_INC 42
#define SLIDER_SWALLOWBAR_HEIGHT_INC  42
#define SLIDER_LEFT_RIGHT_PADDING_SIZE_INC 20 0
#define SLIDER_ICON_PADDING_SIZE_INC 5 0
#define SLIDER_CENTER_POINT_SIZE_INC 10 24

#define SLIDER_POPUP_WIDTH_INC 80
#define SLIDER_POPUP_HEIGHT_INC 92

group {
	name: "elm/slider/horizontal/indicator/quickpanel_style";
	images {
		image: "core_slider_handle_normal.png" COMP;
		image: "core_slider_handle_press.png" COMP;
	}
	parts {
		part {
			name: "access";
			type: RECT;
			description { state: "default" 0.0;
				fixed: 1 1;
				color: 0 0 0 0;
			}
		}
		part {
			name: "button_events";
			type: RECT;
			mouse_events: 1;
			scale: 1;
			description {
				state: "default" 0.0;
				fixed: 1 1;
				min: 2*SLIDER_INDICATOR_SIZE_INC 1.5*SLIDER_INDICATOR_SIZE_INC;
				aspect: 1.0 1.0;
				aspect_preference: VERTICAL;
				color: 0 0 0 0;
			}
			description {
				state: "disabled" 0.0;
				inherit: "default" 0.0;
				visible: 0;
			}
		}
		part {
			name: "button0";
			mouse_events: 0;
			scale: 1;
			description {
				state: "default" 0.0;
				fixed: 1 1;
				min: SLIDER_INDICATOR_SIZE_INC SLIDER_INDICATOR_SIZE_INC;
				max: SLIDER_INDICATOR_SIZE_INC SLIDER_INDICATOR_SIZE_INC;
				image {
					normal: "core_slider_handle_normal.png";
				}
				color_class: "AO005L3";
			}
			description {
				state: "disabled" 0.0;
				inherit: "default" 0.0;
				color_class: "AO004D";
				visible: 1;
			}
		}
		part {
			name: "button0_press";
			mouse_events: 0;
			scale: 1;
			description {
				state: "default" 0.0;
				fixed: 1 1;
				min: SLIDER_INDICATOR_SIZE_INC SLIDER_INDICATOR_SIZE_INC;
				max: SLIDER_INDICATOR_SIZE_INC SLIDER_INDICATOR_SIZE_INC;
				image {
					normal: "core_slider_handle_press.png";
				}
				color_class: "AO005L4";
				visible: 0;
				}
			description {
				state: "pressed" 0.0;
				inherit: "default" 0.0;
				visible: 1;
			}
		}
	}
	programs {
		program { name: "mouse_down";
			signal: "mouse,down,*";
			source: "button_events";
			action:  STATE_SET "pressed" 0.0;
			target: "button0_press";
		}
		program { name: "mouse_up";
			signal: "mouse,up,*";
			source: "button_events";
			action:  STATE_SET "default" 0.0;
			target: "button0_press";
		}
		program { name: "slider_disable";
			signal: "elm,state,disabled";
			source: "elm";
			action:  STATE_SET "disabled" 0.0;
			target: "button0";
			target: "button_events";
		}
		program { name: "slider_enable";
			signal: "elm,state,enabled";
			source: "elm";
			action:  STATE_SET "default" 0.0;
			target: "button0";
			target: "button_events";
		}
	}
}

group {
	name: "elm/slider/horizontal/quickpanel_style";
		images {
			image: "core_progressbar_bg.#.png" COMP;
			image: "core_progress_bar.#.png" COMP;
			image: "core_slider_center_point_bg.png" COMP;
		}
		script {
			public invert_on = 0;
			public popup_show = 1;
			public show = 0;
			public set_popup_show() {
				set_int(popup_show, 1);
			}
			public set_popup_hide() {
				set_int(popup_show, 0);
			}
			public set_invert_on() {
				set_state(PART:"level", "inverted", 0.0);
				set_state(PART:"level2", "inverted", 0.0);
				set_int(invert_on, 1);
			}
			public set_invert_off() {
				set_state(PART:"level", "default", 0.0);
				set_state(PART:"level2", "default", 0.0);
				set_int(invert_on, 0);
			}
			public thumb_down() {
				if (get_int(invert_on) == 0) {
					set_state(PART:"level", "pressed", 0.0);
					set_state(PART:"level2", "pressed", 0.0);
				}
				if (get_int(popup_show) == 1) {
					emit("popup,show", "elm");
				}
			}
			public thumb_up() {
				if (get_int(invert_on) == 0) {
					set_state(PART:"level", "default", 0.0);
					set_state(PART:"level2", "default", 0.0);
				}
				if (get_int(popup_show) == 1) {
					emit("popup,hide", "elm");
				}
			}
			public drag_state() {
				new Float:dx, Float:dy;
				get_drag(PART:"elm.dragable.slider", dx, dy);
				if (dx > 0.5) {
					set_state(PART:"level", "default", 0.0);
					set_state(PART:"level2", "default", 0.0);
					set_int(invert_on, 0);
				}
				else {
					set_state(PART:"level", "inverted", 0.0);
					set_state(PART:"level2", "inverted", 0.0);
					set_int(invert_on, 1);
				}
			}
			public message(Msg_Type:type, id, ...) {
				if (type == MSG_FLOAT_SET) {
					new Float:ratio;
					ratio = getfarg(2);
					if (id == 1) {
						custom_state(PART:"left_restrict", "default", 0.0);
						set_state_val(PART:"left_restrict", STATE_REL2, ratio, 0.0);
						set_state(PART:"left_restrict", "custom", 0.0);
					} else if (id == 2) {
						custom_state(PART:"right_restrict", "default", 0.0);
						set_state_val(PART:"right_restrict", STATE_REL1, 1.0 - ratio, 0.0);
						set_state(PART:"right_restrict", "custom", 0.0);
					}
				}
			}
		}
	parts {
		part {
			name: "access";
			type: RECT;
			description {
				state: "default" 0.0;
				fixed: 1 1;
				color: 0 0 0 0;
			}
		}
		part {
			name: "bg";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 SLIDER_SWALLOWBAR_HEIGHT_INC;
			}
		}
		part {
			name: "elm.swallow.icon";
			type: SWALLOW;
			clip_to: "icon_clipper";
			scale: 1;
			description {
				state: "default" 0.0;
				visible: 0;
				align: 0.0 0.5;
				max: SLIDER_SWALLOWBAR_HEIGHT_INC SLIDER_SWALLOWBAR_HEIGHT_INC;
				rel1 {
					relative: 0.0 0.0;
					to: "bg";
				}
				rel2 {
					relative: 0.0 1.0;
					to: "bg";
				}
			}
			description {
				state: "visible" 0.0;
				inherit: "default" 0.0;
				visible: 1;
			}
		}
		part {
			name: "bar_icon_right_padding_1";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				align: 0.0 0.5;
				min: 0 0;
				max: 0 0;
				rel1 {
					relative: 1.0  0.0;
					to: "elm.swallow.icon";
				}
				rel2 {
					relative: 1.0  1.0;
					to: "elm.swallow.icon";
				}
				fixed: 1 1;
			}
			description { state: "visible";
				inherit: "default" 0.0;
				min: SLIDER_ICON_PADDING_SIZE_INC;
				max: SLIDER_ICON_PADDING_SIZE_INC;
			}
		}
		part {
			name: "bar_icon_right_padding_2";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				align: 0.0 0.5;
				min: SLIDER_LEFT_RIGHT_PADDING_SIZE_INC;
				max: SLIDER_LEFT_RIGHT_PADDING_SIZE_INC;
				rel1 {
				relative: 1.0  0.0;
					to: "bar_icon_right_padding_1";
				}
				rel2 {
					relative: 1.0  1.0;
					to: "bar_icon_right_padding_1";
				}
				fixed: 1 1;
			}
		}
		part {
			name: "left_restrict";
			type: RECT;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 SLIDER_BASE_HEIGHT_INC;
				max: -1 SLIDER_BASE_HEIGHT_INC;
				fixed: 1 1;
				rel1 {
					to: "base";
					relative: 0.0 0.0;
				}
				rel2 {
					to: "base";
					relative: 0.0 1.0;
				}
				align: 0.0 0.5;
			}
		}
		part {
			name: "right_restrict";
			type: RECT;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 SLIDER_BASE_HEIGHT_INC;
				max: -1 SLIDER_BASE_HEIGHT_INC;
				rel1 {
					to: "base";
					relative: 1.0 0.0;
				}
				rel2 {
					to: "base";
					relative: 1.0 1.0;
				}
			}
		}
		part {
			name: "base";
			mouse_events: 0;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 SLIDER_BASE_HEIGHT_INC;
				max: -1 SLIDER_BASE_HEIGHT_INC;
				rel1 {
					to: "bar_icon_right_padding_2";
					relative: 1.0 0.0;
				}
				rel2 {
					to: "bar_image_right_padding";
					relative: 0.0 1.0;
				}
				fixed: 0 1;
				image.normal: "core_progressbar_bg.#.png";
				color: 255 255 255 255;
				color_class: "AO005L1";
			}
			description {
				state: "disabled" 0.0;
				inherit: "default" 0.0;
				color_class: "AO005L1D";
			}
		}
		part {
			name: "drag_base";
			type: "SPACER";
			description {
				state: "default" 0.0;
				min: 0 SLIDER_SWALLOWBAR_HEIGHT_INC;
				rel1 {
					to_x: "left_restrict";
					to_y: "bg";
					relative: 1.0 0.0;
					offset: -SLIDER_LEFT_RIGHT_PADDING_SIZE_INC;
				}
				rel2 {
					to_x: "right_restrict";
					to_y: "bg";
					relative: 0.0 1.0;
					offset: SLIDER_LEFT_RIGHT_PADDING_SIZE_INC;
				}
				fixed: 1 1;
			}
		}
		part {
			name: "level";
			mouse_events: 0;
			scale: 1;
			clip_to: "clipper";
			description {
				state: "default" 0.0;
				rel1 {
					to_x: "bar_icon_right_padding_2";
					relative: 1.0 0.0;
				}
				rel2 {
					to_x: "elm.dragable.slider";
					relative: 0.5 1.0;
				}
				fixed: 1 1;
				min: 0 SLIDER_BASE_HEIGHT_INC;
				max: -1 SLIDER_BASE_HEIGHT_INC;
				image.normal: "core_progressbar_bg.#.png";
			}
			description {
				state: "inverted" 0.0;
				inherit: "default" 0.0;
				visible: 0;
			}
		}
		part { 
			name: "level2";
			mouse_events: 0;
			scale: 1;
			clip_to: "clipper";
			description {
				state: "default" 0.0;
				fixed: 1 1;
				visible: 0;
				rel1 {
					to_y: "base";
					to_x: "elm.dragable.slider";
					relative: 0.5 0.0;
				}
				rel2.to: "base";
					image.normal: "core_progressbar_bg.#.png";
				}
				description {
					state: "inverted" 0.0;
					inherit: "default" 0.0;
					visible: 1;
			}
		}
		part {
			name: "bar_image_right_padding";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: SLIDER_LEFT_RIGHT_PADDING_SIZE_INC;
				max: SLIDER_LEFT_RIGHT_PADDING_SIZE_INC;
				fixed: 1 0;
				rel1 {
					relative: 1.0  0.0;
					to: "bg";
				}
				rel2 {
					relative: 1.0  1.0;
					to: "bg";
				}
				align: 1.0 0.5;
			}
		}
		part { name: "icon_clipper";
			scale: 1;
			type: RECT;
			description {
				state: "default" 0.0;
				color_class: "W0661";
			}
			description {
				state: "disabled" 0.0;
				color_class: "W0661D";
			}
		}
		part {
			name: "elm.swallow.bar";
			type: SWALLOW;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0  SLIDER_SWALLOWBAR_HEIGHT_INC;
				max: -1 SLIDER_SWALLOWBAR_HEIGHT_INC;
				fixed: 0 1;
				rel1 {
					to: "bar_icon_right_padding_2";
					relative: 1.0 0.0;
				}
				rel2 {
					to: "bar_image_right_padding";
					relative: 0.0 1.0;
				}
			}
		}
		part {
			name:"center_point";
			scale: 1;
			description {
				state: "default" 0.0;
				min: SLIDER_CENTER_POINT_SIZE_INC;
				max: SLIDER_CENTER_POINT_SIZE_INC;
				visible: 0;
				image.normal: "core_slider_center_point_bg.png";
				rel1.to: "base";
				rel2.to: "base";
				fixed: 0 1;
				color_class: "W062L1";
			}
			description {
				state: "visible" 0.0;
				inherit: "default" 0.0;
				visible: 1;
			}
			description {
				state: "disabled" 0.0;
				inherit: "default" 0.0;
				visible: 1;
				color_class: "W062L1D";
			}
		}
		part {
			name: "elm.track.slider";
			type: SWALLOW;
			mouse_events: 0;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 SLIDER_POPUP_HEIGHT_INC;
				max: -1 SLIDER_POPUP_HEIGHT_INC;
				align: 0.5 1.0;
				fixed: 0 1;
				rel1.to: "elm.dragable.slider";
				rel1.offset: 0 8;
				rel2 {
					relative: 1.0 0.0;
					to: "elm.dragable.slider";
					offset: 0 8;
				}
			}
		}
		part {
			name: "elm.dragable.slider";
			type: GROUP;
			source: "elm/slider/horizontal/indicator/quickpanel_style";
			mouse_events: 1;
			scale: 1;
			dragable {
				x: 1 1 0;
				y: 0 0 0;
				confine: "drag_base";
			}
			description {
				state: "default" 0.0;
				min: SLIDER_INDICATOR_SIZE_INC SLIDER_INDICATOR_SIZE_INC;
				fixed: 1 1;
				align: 0.5 0.5;
				rel1.to: "drag_base";
				rel1.relative: 0.5 0.5;
				rel2.to: "drag_base";
				rel2.relative: 0.5 0.5;
				color: 0 0 0 0;
			}
		}
		part {
			name: "disabler";
			type: RECT;
			mouse_events: 1;
			repeat_events: 0;
			scale: 1;
			description {
				state: "default" 0.0;
				visible: 0;
				color: 0 0 0 0;
			}
			description {
				state: "disabled" 0.0;
				inherit: "default" 0.0;
				visible: 1;
			}
		}
		part {
			name: "clipper";
			type: RECT;
			description {
				state: "default" 0.0;
					color_class: "AO005L2";
				}
				description {
					state: "pressed" 0.0;
					color_class: "AO005L2";
				}
				description {
					state: "disabled" 0.0;
					color_class: "AO004D";
				}
			}
		}
		programs {
			program {
			name: "invert_on";
			signal: "elm,state,inverted,on";
			source: "elm";
			script {
				set_invert_on();
			}
		}
		program {
			name: "invert_off";
			signal: "elm,state,inverted,off";
			source: "elm";
			script {
				set_invert_off();
			}
		}
		program {
			name: "val_show";
			signal: "mouse,down,*";
			source: "elm.dragable.slider";
			script {
				thumb_down();
			}
		}
		program {
			name: "val_hide";
			signal: "mouse,up,*";
			source: "elm.dragable.slider";
			script {
				thumb_up();
			}
		}
		program {
			name: "popup_show";
			signal: "elm,state,val,show";
			source: "elm";
			script {
				set_popup_show();
			}
		}
		program {
			name: "popup_hide";
			signal: "elm,state,val,hide";
			source: "elm";
			script {
				set_popup_hide();
			}
		}
		program {
			name: "icon_show";
			signal: "elm,state,icon,visible";
			source: "elm";
			action:  STATE_SET "visible" 0.0;
			target: "elm.swallow.icon";
			target: "bar_icon_right_padding_1";
		}
		program {
			name: "center_point_enable";
			signal: "slider,center,point,show";
			source: "elm";
			script {
				set_state(PART:"center_point", "visible", 0.0);
				set_int(show, 1);
			}
		}
		program {
			name: "center_point_disable";
			signal: "slider,center,point,hide";
			source: "elm";
			script {
				set_state(PART:"center_point", "default", 0.0);
				set_int(show, 0);
			}
		}
		program {
			name: "slider_disable";
			signal: "elm,state,disabled";
			source: "elm";
			script {
				if (get_int(show)) {
					set_state(PART:"center_point", "disabled", 0.0);
				}
				set_state(PART:"disabler", "disabled", 0.0);
				set_state(PART:"icon_clipper", "disabled", 0.0);
				set_state(PART:"clipper", "disabled", 0.0);
				set_state(PART:"base", "disabled", 0.0);
			}
		}
		program {
			name: "slider_enable";
			signal: "elm,state,enabled";
			source: "elm";
			script {
				if (get_int(show)) {
					set_state(PART:"center_point", "default", 0.0);
				}
				set_state(PART:"disabler", "default", 0.0);
				set_state(PART:"icon_clipper", "default", 0.0);
				set_state(PART:"clipper", "default", 0.0);
				set_state(PART:"base", "default", 0.0);
			}
		}
	}
}
