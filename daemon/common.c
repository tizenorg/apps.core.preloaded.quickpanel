/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common.h"
#include "quickpanel-ui.h"

HAPI void quickpanel_util_char_replace(char *text, char s, char t) {
	retif(text == NULL, , "invalid argument");

	int i = 0, text_len = 0;

	text_len = strlen(text);

	for (i = 0; i < text_len; i++) {
		if (*(text + i) == s) {
			*(text + i) = t;
		}
	}
}
