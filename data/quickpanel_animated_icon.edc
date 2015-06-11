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

#define FRAME_TIME 0.3

images {
	image: "noti_download_01.png" COMP;
	image: "noti_download_02.png" COMP;
	image: "noti_download_03.png" COMP;
	image: "noti_download_04.png" COMP;
	image: "noti_download_05.png" COMP;
	image: "noti_download_complete.png" COMP;

	image: "noti_upload_01.png" COMP;
	image: "noti_upload_02.png" COMP;
	image: "noti_upload_03.png" COMP;
	image: "noti_upload_04.png" COMP;
	image: "noti_upload_05.png" COMP;
	image: "noti_upload_complete.png" COMP;

	image: "noti_install_01.png" COMP;
	image: "noti_install_02.png" COMP;
	image: "noti_install_03.png" COMP;
	image: "noti_install_04.png" COMP;
	image: "noti_install_05.png" COMP;
	image: "noti_install_complete.png" COMP;
}

group {
	name: "quickpanel/animated_icon_download";
	parts {
		part { name: "background";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 47 47;
				max: 47 47;
			}
		}
		part{
			name: "obj.image";
			type:IMAGE;
			scale:1;
			description {
				state: "default" 0.0;
				fixed: 1 1;
				state: "default" 0.0;
				min: 47 47;
				max: 47 47;
				rel1 {
					to: "background";
				}
				rel2 {
					to: "background";
				}
				image {
					normal:"noti_download_01.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
				color_class: QP_THEME_LIST_ITEM_ICON_COLOR;
				visible:1;
			}
			description {
				state: "state.0" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_download_01.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.1" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_download_02.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.2" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_download_03.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.3" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_download_04.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.4" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_download_05.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.5" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_download_complete.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
		}
	}

	programs {
		program {
			name: "init.layout";
			signal: "load";
			source: "";
			in: 0.0 0.0;
			action: SIGNAL_EMIT "icon.state.0" "prog";
		}
		program{
			name: "icon.state.0";
			signal: "icon.state.0";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.0" 0.0;
			target: "obj.image";
			after: "icon.state.1";
		}
		program{
			name: "icon.state.1";
			signal: "icon.state.1";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.1" 0.0;
			target: "obj.image";
			after: "icon.state.2";
		}
		program{
			name: "icon.state.2";
			signal: "icon.state.2";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.2" 0.0;
			target: "obj.image";
			after: "icon.state.3";
		}
		program{
			name: "icon.state.3";
			signal: "icon.state.3";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.3" 0.0;
			target: "obj.image";
			after: "icon.state.4";
		}
		program{
			name: "icon.state.4";
			signal: "icon.state.4";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.4" 0.0;
			target: "obj.image";
			after: "icon.state.5";
		}
		program{
			name: "icon.state.5";
			signal: "icon.state.5";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.5" 0.0;
			target: "obj.image";
			after: "icon.state.0";
		}
	}
}

group {
	name: "quickpanel/animated_icon_upload";
	parts {
		part { name: "background";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 47 47;
				max: 47 47;
			}
		}
		part{
			name: "obj.image";
			type:IMAGE;
			scale:1;
			description {
				state: "default" 0.0;
				fixed: 1 1;
				state: "default" 0.0;
				min: 47 47;
				max: 47 47;
				rel1 {
					to: "background";
				}
				rel2 {
					to: "background";
				}
				image {
					normal:"noti_upload_01.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
				color_class: QP_THEME_LIST_ITEM_ICON_COLOR;
				visible:1;
			}
			description {
				state: "state.0" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_upload_01.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.1" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_upload_02.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.2" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_upload_03.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.3" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_upload_04.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.4" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_upload_05.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.5" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_upload_complete.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
		}
	}

	programs {
		program {
			name: "init.layout";
			signal: "load";
			source: "";
			in: 0.0 0.0;
			action: SIGNAL_EMIT "icon.state.0" "prog";
		}
		program{
			name: "icon.state.0";
			signal: "icon.state.0";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.0" 0.0;
			target: "obj.image";
			after: "icon.state.1";
		}
		program{
			name: "icon.state.1";
			signal: "icon.state.1";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.1" 0.0;
			target: "obj.image";
			after: "icon.state.2";
		}
		program{
			name: "icon.state.2";
			signal: "icon.state.2";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.2" 0.0;
			target: "obj.image";
			after: "icon.state.3";
		}
		program{
			name: "icon.state.3";
			signal: "icon.state.3";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.3" 0.0;
			target: "obj.image";
			after: "icon.state.4";
		}
		program{
			name: "icon.state.4";
			signal: "icon.state.4";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.4" 0.0;
			target: "obj.image";
			after: "icon.state.5";
		}
		program{
			name: "icon.state.5";
			signal: "icon.state.5";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.5" 0.0;
			target: "obj.image";
			after: "icon.state.0";
		}
	}
}

group {
	name: "quickpanel/animated_icon_install";
	parts {
		part { name: "background";
			type: SPACER;
			scale: 1;
			description {
				state: "default" 0.0;
				min: 47 47;
				max: 47 47;
			}
		}
		part{
			name: "obj.image";
			type:IMAGE;
			scale:1;
			description {
				state: "default" 0.0;
				fixed: 1 1;
				state: "default" 0.0;
				min: 47 47;
				max: 47 47;
				rel1 {
					to: "background";
				}
				rel2 {
					to: "background";
				}
				image {
					normal:"noti_install_01.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
				color_class: QP_THEME_LIST_ITEM_ICON_COLOR;
				visible:1;
			}
			description {
				state: "state.0" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_install_01.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.1" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_install_02.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.2" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_install_03.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.3" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_install_04.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.4" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_install_05.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
			description {
				state: "state.5" 0.0;
				inherit: "default" 0.0;
				image {
					normal:"noti_install_complete.png";
					border: 0 0 0 0;
					border_scale: 1;
				}
			}
		}
	}

	programs {
		program {
			name: "init.layout";
			signal: "load";
			source: "";
			in: 0.0 0.0;
			action: SIGNAL_EMIT "icon.state.0" "prog";
		}
		program{
			name: "icon.state.0";
			signal: "icon.state.0";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.0" 0.0;
			target: "obj.image";
			after: "icon.state.1";
		}
		program{
			name: "icon.state.1";
			signal: "icon.state.1";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.1" 0.0;
			target: "obj.image";
			after: "icon.state.2";
		}
		program{
			name: "icon.state.2";
			signal: "icon.state.2";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.2" 0.0;
			target: "obj.image";
			after: "icon.state.3";
		}
		program{
			name: "icon.state.3";
			signal: "icon.state.3";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.3" 0.0;
			target: "obj.image";
			after: "icon.state.4";
		}
		program{
			name: "icon.state.4";
			signal: "icon.state.4";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.4" 0.0;
			target: "obj.image";
			after: "icon.state.5";
		}
		program{
			name: "icon.state.5";
			signal: "icon.state.5";
			source: "prog";
			in: FRAME_TIME 0.0;
			action: STATE_SET "state.5" 0.0;
			target: "obj.image";
			after: "icon.state.0";
		}
	}
}