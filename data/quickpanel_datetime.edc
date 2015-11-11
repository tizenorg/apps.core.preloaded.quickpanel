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
	image: "icon_indicator_setting.png" COMP;
	image: "line_indicator_divider.png" COMP;
	image: "bg_press.#.png" COMP;
}

styles {
	style {
		name: "font_time_am_pm_style";
		base: "font=Tizen:style=Regular  font_size=22 valign=middle  ellipsis=1.0 wrap=none";
		tag: "time" "+ color=#FFFFFF ";
		tag: "ampm" "+ color=#FFFFFF ";
	}

	style {
		name: "font_date_style";
		base: "font=Tizen:style=Regular  font_size=22 color=#FFFFFF valign=middle color_class=tizen  ellipsis=1.0 wrap=none";
	}
}

group {
	name: "quickpanel/datetime";

	script {
		public g_is_timedate_clickable;
	}

	parts {
		part { name: "base";
			type: RECT;
			repeat_events: 1;
			scale: 1;
			description {
				state: "default" 0.0;
				color_class: QP_BACKGROUND_COLOR;
				visible: 0;
			}
		}
		QUICKPANEL_FOCUS_OBJECT("focus", "base", "base")

		part { name: "space.button.setting";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 60 0;
				fixed: 1 0;
				align: 1.0 0.5;
				rel1 {
					relative : 1.0 0.0;
					to: "base";
				}
				rel2 {
					relative : 1.0 1.0;
					to: "base";
				}
			}
		}
		QUICKPANEL_FOCUS_OBJECT("focus.setting", "space.button.setting", "space.button.setting")

		part { name: "space.divider";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 1 0;
				fixed: 1 0;
				align: 1.0 0.5;
				rel1 {
					relative : 0.0 0.0;
					to: "space.button.setting";
				}
				rel2 {
					relative : 0.0 1.0;
					to: "space.button.setting";
				}
			}
		}

		part {
			name: "space.datetime";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				rel1 {
					relative : 0.0 0.0;
					to: "base";
				}
				rel2 {
					relative : 0.0 1.0;
					to: "space.divider";
				}
			}
		}
		QUICKPANEL_FOCUS_OBJECT("focus.datetime", "space.datetime", "space.datetime")

		part { name: "image.button.setting.bg";
			type: IMAGE;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 36;
				fixed: 0 1;
				align: 0.0 0.5;
				rel1 {
					relative: 0.0 0.5;
					to: "space.button.setting";
				}
				rel2 {
					relative: 1.0 0.5;
					to: "space.button.setting";
				}
				image {
					normal: "bg_press.#.png";
				}
				color_class: "F043P";
				visible: 0;
			}
			description {
				state: "pressed" 0.0;
				inherit: "default";
				visible: 1;
			}
		}

		part { name: "image.datetime.bg";
			type: IMAGE;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 36;
				fixed: 0 1;
				align: 0.0 0.5;
				rel1 {
					relative : 0.0 0.5;
					to: "space.datetime";
				}
				rel2 {
					relative : 1.0 0.5;
					to: "space.datetime";
				}
				image {
					normal: "bg_press.#.png";
				}
				color_class: "F043P";
				visible: 0;
			}
			description {
				state: "pressed" 0.0;
				inherit: "default";
				visible: 1;
			}
		}

		part { name: "image.button.setting";
			type: IMAGE;
			mouse_events: 0;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 42 42;
				fixed: 1 1;
				align: 0.5 0.5;
				rel1 {
					relative: 0.5 0.5;
					to: "space.button.setting";
				}
				rel2 {
					relative: 0.5 0.5;
					to: "space.button.setting";
				}
				image {
					normal:"icon_indicator_setting.png";
				}
				color_class: "AO025";
			}
		}

		part { name: "image.divider";
			type: IMAGE;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 1 24;
				fixed: 1 1;
				align: 0.5 0.5;
				rel1 {
					relative: 0.5 0.5;
					to: "space.divider";
				}
				rel2 {
					relative: 1.0 0.5;
					to: "space.divider";
				}
				image {
					normal: "line_indicator_divider.png";
				}
				color_class: "AO003E1";
			}
		}

		part { name: "divider.padding.left";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 15 0;
				fixed: 1 0;
				align: 1.0 0.5;
				rel1 {
					relative: 0.0 0.0;
					to: "space.divider";
				}
				rel2 {
					relative: 0.0 1.0;
					to: "space.divider";
				}
			}
		}

		part { name: "text.time";
			type: TEXTBLOCK;
			mouse_events: 0;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 (3 + 30 + 3);
				fixed: 0 1;
				align: 1.0 0.5;
				rel1 {
					relative : 0.0 0.5;
					to: "divider.padding.left";
				}
				rel2 {
					relative : 0.0 0.5;
					to: "divider.padding.left";
				}
				text {
					style: "font_time_am_pm_style";
					min: 1 0;
					max: 1 0;
					align: 1.0 0.5;
				}
			}
		}

		part { name: "date.left.padding";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 15 0;
				fixed: 1 0;
				align: 0.0 0.0;
				rel1 {
					relative : 0.0 0.0;
					to: "space.datetime";
				}
				rel2 {
					relative : 0.0 1.0;
					to: "space.datetime";
				}
			}
		}

		part {
			name: "date.top.padding";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 0;
				fixed: 0 1;
				align: 0.0 0.0;
				rel1 {
					relative: 0.0 0.0;
					to: "space.datetime";
				}
				rel2 {
					relative: 1.0 0.0;
					to: "space.datetime";
				}
			}
		}

		part { name: "text.time.left.padding";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 15 0;
				fixed: 1 0;
				align: 1.0 0.0;
				rel1 {
					//relative : 0.0 0.0;
					to: "text.time";
				}
				rel2 {
					relative : 0.0 1.0;
					to: "text.time";
				}
			}
		}

		part { name: "text.date";
			type: TEXTBLOCK;
			mouse_events: 0;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 0 (8 + 20 + 8);
				fixed: 0 1;
				align: 0.0 0.5;
				rel1 {
					relative : 1.0 1.0;
					to_x: "date.left.padding";
					to_y: "date.top.padding";
				}
				rel2 {
					relative : 0.0 1.0;
					to_x : "text.time.left.padding";
					to_y: "space.datetime";
				}
				text {
					style: "font_date_style";
					min: 0 0;
					max: 1 0;
					align: 0.0 0.5;
				}
			}
		}
	}

	programs {
		program {
			name: "timedate.init";
			signal: "load";
			script{
				set_int(g_is_timedate_clickable, 1);
			}
		}
		program{
			name: "button.setting.down";
			signal: "mouse,down,*";
			source: "focus.setting";
			action: STATE_SET "pressed" 0.0;
			target: "image.button.setting.bg";
		}
		program{
			name: "button.setting.up";
			signal: "mouse,up,*";
			source: "focus.setting";
			action: STATE_SET "default" 0.0;
			target: "image.button.setting.bg";
		}
		program {
			name: "timendate.click.enable";
			signal: "timendate.click.enable";
			source: "prog";
			script {
				set_int(g_is_timedate_clickable, 1);
			}
		}
		program {
			name: "timendate.click.disable";
			signal: "timendate.click.disable";
			source: "prog";
			script {
				set_int(g_is_timedate_clickable, 0);
			}
		}
		program{
			name: "date_time.down";
			signal: "mouse,down,*";
			source: "focus.datetime";
			script {
				if (get_int(g_is_timedate_clickable) == 1) {
					set_state(PART:"image.datetime.bg", "pressed", 0.0);
				}
			}
		}
		program{
			name: "date_time.up";
			signal: "mouse,up,*";
			source: "focus.datetime";
			action: STATE_SET "default" 0.0;
			target: "image.datetime.bg";
		}
	}
}

