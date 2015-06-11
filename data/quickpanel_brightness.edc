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



//#define DEBUG
//#define DEBUG_TEXT

#define BRIGHTNESS_ICON_COLOR "AO004"
#define BRIGHTNESS_TEXT_COLOR "AO014"

	styles {
		style {
			name: "checkbox_text";
			base: "font=Tizen:style=Regular text_class=tizen font_size=24 color=#FFFFFF color_class="BRIGHTNESS_TEXT_COLOR" ellipsis=0.0 wrap=mixed";
		}
	}

	group {
		name: "quickpanel/brightness_controller/wrapper";
		parts {
			part { name: "base";
				type: SPACER;
				repeat_events: 1;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 0 QP_THEME_LIST_ITEM_BRIGHTNESS_HEIGHT;
					max: 9999 QP_THEME_LIST_ITEM_BRIGHTNESS_HEIGHT;
					fixed: 1 1;
					rel1 {
						relative: 0.0 0.0;
					}
					rel2 {
						relative: 1.0 0.0;
					}
					align: 0.0 0.0;
				}
			}
			part { name: "elm.swallow.controller";
				type: SWALLOW;
				mouse_events: 1;
				scale: 1;
				description {
					state: "default" 0.0;
					fixed: 1 1;
					rel1 {
						to: "base";
					}
					rel2 {
						to: "base";
					}
				}
			}
		}
	}

	group {
		name: "quickpanel/brightness_controller/default";

		images {
			image: "quick_icon_brightness.png" COMP;
			image: "quick_icon_brightness_00.png" COMP;
			image: "quick_icon_brightness_01.png" COMP;
			image: "quick_icon_brightness_02.png" COMP;
			image: "quick_icon_brightness_03.png" COMP;
			image: "quick_icon_brightness_04.png" COMP;
			image: "quick_icon_brightness_05.png" COMP;
			image: "quick_icon_brightness_06.png" COMP;
			image: "quick_icon_brightness_07.png" COMP;
			image: "quick_icon_brightness_08.png" COMP;
			image: "quick_icon_brightness_09.png" COMP;
			image: "quick_icon_brightness_10.png" COMP;
			image: "quick_icon_brightness_11.png" COMP;
			image: "core_theme_bg_01.png" COMP;
		}

		script {
			public ctnt_shown;
			public time_shown;
		}

		parts {
			part { name: "base";
				type: RECT;
				repeat_events: 1;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 0 QP_THEME_LIST_ITEM_BRIGHTNESS_HEIGHT;
					max: 9999 QP_THEME_LIST_ITEM_BRIGHTNESS_HEIGHT;
					fixed: 1 1;
					rel1 {
						relative: 0.0 0.0;
					}
					rel2 {
						relative: 1.0 0.0;
					}
					align: 0.0 0.0;
					color_class: QP_BACKGROUND_COLOR;
					visible: QP_THEME_BG_VISIBILITY;
				}
			}

			part {
				name: "background";
				type: IMAGE;
				scale: 1;
				description {
					state: "default" 0.0;
					fixed: 1 1;
					align: 0.5 0.5;
					rel1 {
						to: "base";
					}
					rel2 {
						to: "base";
					}
					image {
						normal: "core_theme_bg_01.png";
					}
					visible: 0;
				}
				description {
					state: "show.bg" 0.0;
					inherit: "default" 0.0;
					visible: 1;
				}
				description {
					state: "hide.bg" 0.0;
					inherit: "default" 0.0;
					visible: 0;
				}
			}

			part { name: "disabler";
				type: RECT;
				mouse_events: 0;
				description {
					state: "default" 0.0;
					rel1 {
						to: "base";
					}
					rel2 {
						to: "base";
					}
					color_class: AO004;
 				}
				description {
					state: "disabled" 0.0;
					inherit: "default" 0.0;
					color_class: AO004D;
				}
			}
			part { name: "bg_image";
				type: RECT;
				mouse_events: 0;
				description {
					state: "default" 0.0;
					rel1.to:"base";
					rel2.to:"base";
					color_class: QP_THEME_BRIGHTNESS_BG_COLOR;
					visible: QP_THEME_BG_VISIBILITY;
				}
				description {
					state: "selected" 0.0;
					inherit: "default" 0.0;
					color_class: QP_THEME_BRIGHTNESS_BG_COLOR;
				}
			}
			part { name: "elm.content.bg";
				type: RECT;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 9 0;
					fixed: 1 0;
					rel1 {
						to_x:"base";
						to_y:"base";
						relative: 1.0 0.0;
					}
					rel2 {
						to_x:"base";
						to_y:"base";
						relative: 0.0 1.0;
					}
					align: 0.0 0.0;
					visible: 0;
					//color_class: QP_BACKGROUND_COLOR;
				}
			}
			QUICKPANEL_FOCUS_OBJECT("focus", "elm.content.bg", "elm.content.bg")
			part { name: "elm.padding.left";
				type: RECT;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 15 0;
					max: 15 0;
					fixed: 1 0;
					rel1.to:"base";
					rel2.to:"base";
					rel2.relative: 0.0 0.0;
					rel2.relative: 0.0 1.0;
					align: 0.0 0.0;
					visible: 0;
				}
			}
			part { name: "elm.rect.thumbnail";
				type: RECT;
				mouse_events: 0;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 42 42;
					max: 42 42;
					fixed: 1 1;
					rel1 {
						to_x:"elm.padding.left";
						to_y:"base";
						relative: 1.0 0.0;
					}
					rel2 {
						to_x:"elm.padding.left";
						to_y:"base";
						relative: 1.0 1.0;
					}
					align: 0.0 0.5;
					visible:0;
				}
			}
			part { name: "elm.image.thumbnail";
				type:IMAGE;
				clip_to: "disabler";
				scale:1;
				description {
					state: "default" 0.0;
					fixed: 0 0;
					rel1 {
						to: "elm.rect.thumbnail";
					}
					rel2 {
						to: "elm.rect.thumbnail";
					}
					image {
						normal:"quick_icon_brightness_00.png";
					}
				}
				description {
					state: "state.0" 0.0;
					inherit: "default" 0.0;
					image {
						normal:"quick_icon_brightness_00.png";
					}
				}
				description {
					state: "state.1" 0.0;
					inherit: "default" 0.0;
					image {
						normal:"quick_icon_brightness_01.png";
					}
				}
				description {
					state: "state.2" 0.0;
					inherit: "default" 0.0;
					image {
						normal:"quick_icon_brightness_02.png";
					}
				}
				description {
					state: "state.3" 0.0;
					inherit: "default" 0.0;
					image {
						normal:"quick_icon_brightness_03.png";
					}
				}
				description {
					state: "state.4" 0.0;
					inherit: "default" 0.0;
					image {
						normal:"quick_icon_brightness_04.png";
					}
				}
				description {
					state: "state.5" 0.0;
					inherit: "default" 0.0;
					image {
						normal:"quick_icon_brightness_05.png";
					}
				}
				description {
					state: "state.6" 0.0;
					inherit: "default" 0.0;
					image {
						normal:"quick_icon_brightness_06.png";
					}
				}
				description {
					state: "state.7" 0.0;
					inherit: "default" 0.0;
					image {
						normal:"quick_icon_brightness_07.png";
					}
				}
				description {
					state: "state.8" 0.0;
					inherit: "default" 0.0;
					image {
						normal:"quick_icon_brightness_08.png";
					}
				}
				description {
					state: "state.9" 0.0;
					inherit: "default" 0.0;
					image {
						normal:"quick_icon_brightness_09.png";
					}
				}
				description {
					state: "state.10" 0.0;
					inherit: "default" 0.0;
					image {
						normal:"quick_icon_brightness_10.png";
					}
				}
				description {
					state: "state.11" 0.0;
					inherit: "default" 0.0;
					image {
						normal:"quick_icon_brightness_11.png";
					}
				}
				description {
					state: "state.auto" 0.0;
					inherit: "default" 0.0;
					image {
						normal:"quick_icon_brightness.png";
					}
				}
			}
			part { name: "elm.padding.thumbnail.x";
				type: RECT;
				mouse_events: 0;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 0 0;
					fixed: 1 0;
					rel1 {
						to:"elm.rect.thumbnail";
						relative: 1.0 0.0;
					}
					rel2 {
						to:"elm.rect.thumbnail";
						relative: 1.0 1.0;
					}
					align: 0.0 0.0;
					visible: 0;
				}
			}
			part { name: "elm.swallow.slider";
				type: SWALLOW;
				//clip_to: "disabler";
				mouse_events: 1;
				scale: 1;
				description {
					state: "default" 0.0;
					fixed: 1 1;
					rel1 {
						relative: 1.0 0.0;
						to: "elm.padding.thumbnail.x";
					}
					rel2 {
						relative: 0.0 1.0;
						to: "elm.padding.bf.check";
					}
				}
			}
			part { name: "elm.rect.slider";
				type: RECT;
				mouse_events: 1;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 0 48;
					fixed: 0 1;
					rel1 {
						relative: 1.0 0.0;
						to_x: "elm.padding.thumbnail.x";
						to_y: "base";
					}
					rel2 {
						relative: 0.0 1.0;
						to_x: "elm.padding.bf.check";
						to_y: "base";
					}
					align: 0.0 0.5;
					color: 0 0 0 0;
					visible: 0;
				}
				description {
					state: "disabled" 0.0;
					inherit: "default" 0.0;
					visible: 1;
				}
			}
			part { name: "elm.padding.bf.check";
				type: SPACER;
				scale: 1;
				description {
					state: "default" 0.0;
					rel1 {
						relative: 0.0 0.0;
						to: "elm.check.swallow";
					}
					rel2 {
						relative: 0.0 1.0;
						to: "elm.check.swallow";
					}
					align: 1.0 0.5;
				}
			}

			part { name: "elm.check.swallow";
				type: SWALLOW;
				scale: 1;
				description {
					state: "default" 0.0;
					rel1 {
						relative: 0.0 0.0;
						to: "elm.padding.bf.auto";
					}
					rel2 {
						relative: 0.0 1.0;
						to: "elm.padding.bf.auto";
					}
					align: 1.0 0.5;
					min: 0 40;
					fixed: 1 1;
				}
			}

			part { name: "elm.padding.bf.auto";
				type: SPACER;
				scale: 1;
				description {
					state: "default" 0.0;
					rel1 {
						relative: 0.0 0.0;
						to: "elm.rect.auto";
					}
					rel2 {
						relative: 0.0 1.0;
						to: "elm.rect.auto";
					}
					align: 1.0 0.5;
					min: 0 0;
					fixed: 1 0;
				}
			}
			part { name: "elm.rect.auto";
				type: SPACER;
				scale: 1;
				description {
					state: "default" 0.0;
					rel1 {
						relative : 0.0 0.0;
						to : "elm.check.text";
					}
					rel2 {
						relative : 1.0 1.0;
						to : "elm.check.text";
					}
				}
			}
			part { name: "elm.check.text";
				type: TEXT;
				scale: 1;
				description {
					state: "default" 0.0;
					align : 1.0 0.5;
					fixed : 1 1;
					rel1 {
						relative : 0.0 0.5;
						to : "elm.padding.right";
					}
					rel2 {
						relative : 0.0 0.5;
						to : "elm.padding.right";
					}
					text {
						font : "Tizen:style=Regular";
						size : 22;
						text_class : "Tizen";
						align : 1.0 0.5;
						min : 1 1;
					}
					color_class: ATO019;
				}
			}
			
			part { name: "elm.padding.right";
				type: SPACER;
				scale: 1;
				description {
					state: "default" 0.0;
					rel1 {
						relative: 1.0 0.0;
						to: "base";
					}
					rel2 {
						relative: 1.0 1.0;
						to: "base";
					}
					align: 1.0 0.5;
					min: 15 0;
					fixed: 1 0;
				}
			}
		}

		programs {
			program { name: "show_content";
				signal: "elm,state,elm.swallow.slider,active";
				source: "elm";
				script {
					set_state(PART:"elm.swallow.slider", "show", 0.0);
					set_int(ctnt_shown, 1);
				}
			}
			program { name: "go_active";
				signal: "elm,state,selected";
				source: "elm";
				script {
					set_state(PART:"bg_image", "selected", 0.0);
					if (get_int(ctnt_shown) == 1)
					set_state(PART:"elm.swallow.slider", "selected", 0.0);
				}
			}
			program { name: "go_passive";
				signal: "elm,state,unselected";
				source: "elm";
				script {
					set_state(PART:"bg_image", "default", 0.0);
					if (get_int(ctnt_shown) == 1)
					set_state(PART:"elm.swallow.slider", "show", 0.0);
				}
			}
			program{
				name: "show.bg";
				signal: "show.bg";
				source: "prog.bg";
				action: STATE_SET "show.bg" 0.0;
				target: "background";
			}
			program{
				name: "hide.bg";
				signal: "hide.bg";
				source: "prog.bg";
				action: STATE_SET "hide.bg" 0.0;
				target: "background";
			}
			program{
				name: "icon.state.0";
				signal: "icon.state.0";
				source: "prog";
				action: STATE_SET "state.0" 0.0;
				target: "elm.image.thumbnail";
			}
			program{
				name: "icon.state.1";
				signal: "icon.state.1";
				source: "prog";
				action: STATE_SET "state.1" 0.0;
				target: "elm.image.thumbnail";
			}
			program{
				name: "icon.state.2";
				signal: "icon.state.2";
				source: "prog";
				action: STATE_SET "state.2" 0.0;
				target: "elm.image.thumbnail";
			}
			program{
				name: "icon.state.3";
				signal: "icon.state.3";
				source: "prog";
				action: STATE_SET "state.3" 0.0;
				target: "elm.image.thumbnail";
			}
			program{
				name: "icon.state.4";
				signal: "icon.state.4";
				source: "prog";
				action: STATE_SET "state.4" 0.0;
				target: "elm.image.thumbnail";
			}
			program{
				name: "icon.state.5";
				signal: "icon.state.5";
				source: "prog";
				action: STATE_SET "state.5" 0.0;
				target: "elm.image.thumbnail";
			}
			program{
				name: "icon.state.6";
				signal: "icon.state.6";
				source: "prog";
				action: STATE_SET "state.6" 0.0;
				target: "elm.image.thumbnail";
			}
			program{
				name: "icon.state.7";
				signal: "icon.state.7";
				source: "prog";
				action: STATE_SET "state.7" 0.0;
				target: "elm.image.thumbnail";
			}
			program{
				name: "icon.state.8";
				signal: "icon.state.8";
				source: "prog";
				action: STATE_SET "state.8" 0.0;
				target: "elm.image.thumbnail";
			}
			program{
				name: "icon.state.9";
				signal: "icon.state.9";
				source: "prog";
				action: STATE_SET "state.9" 0.0;
				target: "elm.image.thumbnail";
			}
			program{
				name: "icon.state.10";
				signal: "icon.state.10";
				source: "prog";
				action: STATE_SET "state.10" 0.0;
				target: "elm.image.thumbnail";
			}
			program{
				name: "icon.state.11";
				signal: "icon.state.11";
				source: "prog";
				action: STATE_SET "state.11" 0.0;
				target: "elm.image.thumbnail";
			}
			program{
				name: "icon.state.auto";
				signal: "icon.state.auto";
				source: "prog";
				action: STATE_SET "state.auto" 0.0;
				target: "elm.image.thumbnail";
			}
			program{
				signal: "disable";
				source: "disabler";
				action: STATE_SET "default" 0.0;
				target: "disabler";
				target: "elm.rect.slider";
			}
			program{
				signal: "enable";
				source: "disabler";
				action: STATE_SET "disabled" 0.0;
				target: "disabler";
				target: "elm.rect.slider";
			}
		}
	}
