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



group {
	name: "elm/progressbar/horizontal/quickpanel_style";

	images {
		image: "core_activity_Indicator_medium_00.png" COMP;
		image: "core_activity_Indicator_medium_01.png" COMP;
		image: "core_activity_Indicator_medium_02.png" COMP;
		image: "core_activity_Indicator_medium_03.png" COMP;
		image: "core_activity_Indicator_medium_04.png" COMP;
		image: "core_activity_Indicator_medium_05.png" COMP;
		image: "core_activity_Indicator_medium_06.png" COMP;
		image: "core_activity_Indicator_medium_07.png" COMP;
		image: "core_activity_Indicator_medium_08.png" COMP;
		image: "core_activity_Indicator_medium_09.png" COMP;
		image: "core_activity_Indicator_medium_10.png" COMP;
		image: "core_activity_Indicator_medium_11.png" COMP;
		image: "core_activity_Indicator_medium_12.png" COMP;
		image: "core_activity_Indicator_medium_13.png" COMP;
		image: "core_activity_Indicator_medium_14.png" COMP;
		image: "core_activity_Indicator_medium_15.png" COMP;
		image: "core_activity_Indicator_medium_16.png" COMP;
		image: "core_activity_Indicator_medium_17.png" COMP;
		image: "core_activity_Indicator_medium_18.png" COMP;
		image: "core_activity_Indicator_medium_19.png" COMP;
		image: "core_activity_Indicator_medium_20.png" COMP;
		image: "core_activity_Indicator_medium_21.png" COMP;
		image: "core_activity_Indicator_medium_22.png" COMP;
		image: "core_activity_Indicator_medium_23.png" COMP;
		image: "core_activity_Indicator_medium_24.png" COMP;
		image: "core_activity_Indicator_medium_25.png" COMP;
		image: "core_activity_Indicator_medium_26.png" COMP;
		image: "core_activity_Indicator_medium_27.png" COMP;
		image: "core_activity_Indicator_medium_28.png" COMP;
		image: "core_activity_Indicator_medium_29.png" COMP;
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
			name: "elm.background.progressbar";
			type: RECT;
			mouse_events: 0;
			scale: 1;
			description {
				state: "default" 0.0;
			}
		}
		part {
			name: "background";
			mouse_events: 0;
			scale: 1;
			clip_to: "elm.background.progressbar";
			description {
				state: "default" 0.0;
				min: PROCESS_MEDIUM_IMAGE_WIDTH_INC PROCESS_MEDIUM_IMAGE_HEIGHT_INC;
				max: PROCESS_MEDIUM_IMAGE_WIDTH_INC PROCESS_MEDIUM_IMAGE_HEIGHT_INC;
				aspect: 1.0 1.0;
				aspect_preference: BOTH;
				color_class: PROCESS_MEDIUM_IMAGE_COLOR;
				image.normal: "core_activity_Indicator_medium_00.png";
				image.tween: "core_activity_Indicator_medium_01.png";
				image.tween: "core_activity_Indicator_medium_02.png";
				image.tween: "core_activity_Indicator_medium_03.png";
				image.tween: "core_activity_Indicator_medium_04.png";
				image.tween: "core_activity_Indicator_medium_05.png";
				image.tween: "core_activity_Indicator_medium_06.png";
				image.tween: "core_activity_Indicator_medium_07.png";
				image.tween: "core_activity_Indicator_medium_08.png";
				image.tween: "core_activity_Indicator_medium_09.png";
				image.tween: "core_activity_Indicator_medium_10.png";
				image.tween: "core_activity_Indicator_medium_11.png";
				image.tween: "core_activity_Indicator_medium_12.png";
				image.tween: "core_activity_Indicator_medium_13.png";
				image.tween: "core_activity_Indicator_medium_14.png";
				image.tween: "core_activity_Indicator_medium_15.png";
				image.tween: "core_activity_Indicator_medium_16.png";
				image.tween: "core_activity_Indicator_medium_17.png";
				image.tween: "core_activity_Indicator_medium_18.png";
				image.tween: "core_activity_Indicator_medium_19.png";
				image.tween: "core_activity_Indicator_medium_20.png";
				image.tween: "core_activity_Indicator_medium_21.png";
				image.tween: "core_activity_Indicator_medium_22.png";
				image.tween: "core_activity_Indicator_medium_23.png";
				image.tween: "core_activity_Indicator_medium_24.png";
				image.tween: "core_activity_Indicator_medium_25.png";
				image.tween: "core_activity_Indicator_medium_26.png";
				image.tween: "core_activity_Indicator_medium_27.png";
				image.tween: "core_activity_Indicator_medium_28.png";
				image.tween: "core_activity_Indicator_medium_29.png";
			}
		}
	}

	programs {
		program {
			name: "start_pulse";
			signal: "elm,state,pulse,start";
			source: "elm";
			action: STATE_SET "default" 0.0;
			target: "background";
			transition: LINEAR 1.8;
			after: "start_pulse";
		}
		program {
			name: "stop_pulse";
			signal: "elm,state,pulse,stop";
			source: "elm";
			action: ACTION_STOP;
			target: "start_pulse";
			after: "init_pulse";
		}
		program {
			name: "init_pulse";
			action: STATE_SET "default" 0.0;
			target: "background";
		}
	}
}
