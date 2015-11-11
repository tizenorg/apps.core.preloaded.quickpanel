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

	group {
		name: "quickpanel/minictrl/default";
		data.item: "bgcolor" QP_THEME_BANDED_COLOR;
		parts {
			part {
				name: "base";
				type: SPACER;
				repeat_events: 1;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 480 QP_THEME_LIST_ITEM_MINICONTRL_HEIGHT + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT;
					fixed: 0 1;
					rel1 {
						relative: 0.0 0.0;
					}
					rel2 {
						relative: 1.0 1.0;
					}
					align: 0.0 0.0;
				}
			}
			part {
				name: "elm.padding.top";
				type: SPACER;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 0 20 + QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT;
					fixed: 0 1;
					rel1.to:"base";
					rel2.to:"base";
					rel1.relative: 0.0 0.0;
					rel2.relative: 1.0 0.0;
					align: 0.0 0.0;
				}
			}
			part {
				name: "elm.padding.left.bg";
				type: SPACER;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 0 0;
					fixed: 1 0;
					rel1.to:"base";
					rel2.to:"base";
					rel1.relative: 0.0 0.0;
					rel2.relative: 0.0 1.0;
					align: 0.0 0.0;
				}
			}
			part {
				name: "elm.padding.right.bg";
				type: SPACER;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 0 0;
					fixed: 1 0;
					rel1.to:"base";
					rel2.to:"base";
					rel1.relative: 1.0 0.0;
					rel2.relative: 1.0 1.0;
					align: 1.0 0.0;
				}
			}
			part {
				name: "elm.padding.top.bg";
				type: SPACER;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 0 QP_THEME_LIST_ITEM_SEPERATOR_HEIGHT;
					fixed: 0 1;
					rel1.to:"base";
					rel2.to:"base";
					rel1.relative: 0.0 0.0;
					rel2.relative: 1.0 0.0;
					align: 0.0 0.0;
				}
			}
			part {
				name: "elm.content.bg";
				type: RECT;
				scale: 1;
				description {
					state: "default" 0.0;
					rel1 {
						to_x:"elm.padding.left.bg";
						to_y:"elm.padding.top.bg";
						relative: 1.0 1.0;
					}
					rel2 {
						to_x:"elm.padding.right.bg";
						to_y:"elm.padding.bottom";
						relative: 0.0 1.0;
					}
					align: 0.0 0.0;
					visible: 1;
					color: 255 255 255 235;
					color_class: QP_THEME_ITEM_BG_COLOR;
				}
				description {
					state: "show" 0.0;
					inherit: "default" 0.0;
				}
				description {
					state: "hide" 0.0;
					inherit: "default" 0.0;
					color: 0 0 0 0;
				}
			}
			part {
				name: "bgcolor";
				type: "RECT";
				mouse_events: 0;
				description {
					state: "default" 0.0;
					rel1.to:"base";
					rel2.to:"base";
					color_class: QP_THEME_BANDED_COLOR;
					visible: 1;
				}
			}
			QUICKPANEL_FOCUS_OBJECT("focus", "elm.content.bg", "elm.content.bg")
			part {
				name: "elm.padding.left";
				type: SPACER;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 18 0;
					fixed: 1 0;
					rel1.to:"base";
					rel2.to:"base";
					rel1.relative: 0.0 0.0;
					rel2.relative: 0.0 1.0;
					align: 0.0 0.0;
				}
			}
			part {
				name: "elm.padding.right";
				type: SPACER;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 12 0;
					fixed: 1 0;
					rel1.to:"base";
					rel2.to:"base";
					rel1.relative: 1.0 0.0;
					rel2.relative: 1.0 1.0;
					align: 1.0 0.0;
				}
			}
			part {
				name: "elm.icon";
				type: SWALLOW;
				mouse_events: 1;
				scale: 1;
				description {
					state: "default" 0.0;
					align: 0.0 0.0;
					min: 480 140;
					rel1 {
						to: "elm.content.bg";
					}
					rel2 {
						to_x: "elm.content.bg";
						to_y: "elm.padding.bottom";
						relative: 1.0 0.0;
					}
				}
			}
			part {
				name: "elm.padding.bottom";
				type: SPACER;
				scale: 1;
				description {
					state: "default" 0.0;
					min: 0 0;
					fixed: 1 0;
					rel1.relative: 0.0 1.0;
					rel2.relative: 1.0 1.0;
					align: 0.0 1.0;
				}
			}
		}
		programs {
			program{
				name: "bg.show";
				signal: "bg.show";
				source: "prog";
				action: STATE_SET "show" 0.0;
				target: "elm.content.bg";
			}
			program{
				name: "bg.hide";
				signal: "bg.hide";
				source: "prog";
				action: STATE_SET "hide" 0.0;
				target: "elm.content.bg";
			}
		}
	}
