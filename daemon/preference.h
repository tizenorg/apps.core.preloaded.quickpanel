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


#ifndef __QP_PREFERENCE_H_
#define __QP_PREFERENCE_H_

#define PREF_LEN_VALUE_MAX 256

#define PREF_SECTION "quickpanel"
#define PREF_BRIGHTNESS_KEY "brightness"
#define PREF_QUICKSETTING_ORDER_KEY "quicksetting_order"
#define PREF_QUICKSETTING_FEATURED_NUM_KEY "quicksetting_featured_num"
#define PREF_SHORTCUT_ENABLE_KEY "shortcut_enable"
#define PREF_SHORTCUT_EARPHONE_ORDER_KEY "shortcut_earphone"

#define PREF_BRIGHTNESS PREF_SECTION":"PREF_BRIGHTNESS_KEY
#define PREF_QUICKSETTING_ORDER PREF_SECTION":"PREF_QUICKSETTING_ORDER_KEY
#define PREF_QUICKSETTING_FEATURED_NUM PREF_SECTION":"PREF_QUICKSETTING_FEATURED_NUM_KEY
#define PREF_SHORTCUT_ENABLE PREF_SECTION":"PREF_SHORTCUT_ENABLE_KEY
#define PREF_SHORTCUT_EARPHONE_ORDER PREF_SECTION":"PREF_SHORTCUT_EARPHONE_ORDER_KEY

extern int quickpanel_preference_get(const char *key, char *value);
extern const char *quickpanel_preference_default_get(const char *key);
extern int quickpanel_preference_set(const char *key, char *value);

#endif
