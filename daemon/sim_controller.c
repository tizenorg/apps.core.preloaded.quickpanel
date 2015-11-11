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


#include <Elementary.h>
#include <stdio.h>
#include <stdlib.h>
#include <E_DBus.h>

#include <tapi_common.h>
#include <ITapiSim.h>
#include <TelCall.h>
#include <ITapiCall.h>
#include <TelNetwork.h>

#include <dlog.h>
#include <vconf.h>
#include <tzsh.h>
#include <tzsh_quickpanel_service.h>
#include <E_DBus.h>

#include "setting_utils.h"

#include "list_util.h"
#include "quickpanel-ui.h"
#include "common.h"

#include "handler_controller.h"

#define TAPI_HANDLE_MAX  2

static struct
{
	TapiHandle *handle[TAPI_HANDLE_MAX+1];
	Eina_Bool sim_card_ready[2];

	Evas_Object *layout;

	int call_state;	// 0:none, 1:call
}
sim_state_info =
{
	.handle[0] = NULL,
	.handle[1] = NULL,
	.handle[2] = NULL,
	.sim_card_ready[0] = EINA_FALSE,
	.sim_card_ready[1] = EINA_FALSE,
	.layout = NULL,
	.call_state = 0,
};

static void register_sim_callbacks();
static void unregister_sim_callbacks();

static char *get_sim_plmn(TapiHandle *handle)
{
	int ret;
	char *network_name = NULL;

	/* Reading Network (PLMN) name - ‘string’ type Property */
	ret = tel_get_property_string(handle,
			TAPI_PROP_NETWORK_NETWORK_NAME, &network_name);
	if(ret == TAPI_API_SUCCESS)
	{
		/* ‘network_name’ contains valid Network name based on Display condition */
		return network_name;
	}
	else
	{
		ERR("Sim = %p PLMN = ERROR[%d]", handle, ret);
		/* get property failed */
	}

	return NULL;
}

static char *get_sim_spn(TapiHandle *handle)
{
	int ret;
	char *spn_name = NULL;


	/* Reading SPN name - ‘string’ type Property */
	ret = tel_get_property_string(handle,
			TAPI_PROP_NETWORK_SPN_NAME, &spn_name);
	if(ret == TAPI_API_SUCCESS)
	{
		/* ‘spn_name’ contains valid Service provider name */
		return spn_name;
	}
	else
	{
		ERR("Sim = %p SPN = ERROR[%d]", handle, ret);
		/* get property failed */
		return NULL;
	}
}

static char *get_plmn_spn_network(int handle_num, TapiHandle *handle)
{
	int ret = TAPI_API_SUCCESS;
	int service_type = TAPI_NETWORK_SERVICE_TYPE_UNKNOWN;
	int name_option = TAPI_NETWORK_DISP_INVALID;
	char *plmn = NULL;
	char *spn = NULL;
	char buf[1024] = { 0, };

	// get service type
	ret = tel_get_property_int(handle, TAPI_PROP_NETWORK_SERVICE_TYPE, &service_type);
	if (ret != TAPI_API_SUCCESS) {
		ERR("Failed to get service type[%d]", ret);
	}

	if (service_type >= TAPI_NETWORK_SERVICE_TYPE_2G) {
		// get network name option
		ret = tel_get_property_int(handle, TAPI_PROP_NETWORK_NAME_OPTION, &name_option);
		if (ret != TAPI_API_SUCCESS) {
			ERR("Failed to get name option[%d]", ret);
		}

		switch (name_option) {
		case TAPI_NETWORK_DISP_SPN:
			spn = get_sim_spn(handle);
			if (spn != NULL && spn[0] != 0) {
				INFO("PLMN/SPN - Sim %p using SPN: %s", handle, spn);
				snprintf(buf, sizeof(buf), "%s", spn);
			}
			break;
		case TAPI_NETWORK_DISP_PLMN:
			plmn = get_sim_plmn(handle);
			if (plmn != NULL && plmn[0] != 0) {
				INFO("PLMN/SPN - Sim %p using PLMN: %s", handle, plmn);
				snprintf(buf, sizeof(buf), "%s", plmn);
			}
			break;
		case TAPI_NETWORK_DISP_SPN_PLMN:
			spn = get_sim_spn(handle);
			plmn = get_sim_plmn(handle);
			if (spn != NULL && spn[0] != 0 && plmn != NULL && plmn[0] != 0) {
				INFO("PLMN/SPN - Sim %p using SPN: %s, PLMN: %s", handle, spn, plmn);
				snprintf(buf, sizeof(buf), "%s - %s", plmn, spn);
			} else if (spn != NULL && spn[0] != 0) {
				INFO("PLMN/SPN - Sim %p using SPN: %s", handle, spn);
				snprintf(buf, sizeof(buf), "%s", spn);
			} else if (plmn != NULL && plmn[0] != 0) {
				INFO("PLMN/SPN - Sim %p using PLMN: %s", handle, plmn);
				snprintf(buf, sizeof(buf), "%s", plmn);
			}
			break;
		default:
			ERR("Invalid name option[%d]", name_option);
			plmn = get_sim_plmn(handle);
			if (plmn != NULL && plmn[0] != 0) {
				INFO("PLMN/SPN - Sim %p using PLMN: %s", handle, plmn);
				snprintf(buf, sizeof(buf), "%s", plmn);
			}
			break;
		}
	} else {
		switch (service_type) {
		case TAPI_NETWORK_SERVICE_TYPE_NO_SERVICE:
			snprintf(buf, sizeof(buf), "%s", _("IDS_IDLE_BODY_NO_SERVICE"));
			break;
		case TAPI_NETWORK_SERVICE_TYPE_EMERGENCY:
			snprintf(buf, sizeof(buf), "%s", _("IDS_IDLE_MBODY_EMERGENCY_CALLS_ONLY"));
			break;
		case TAPI_NETWORK_SERVICE_TYPE_SEARCH:
			snprintf(buf, sizeof(buf), "%s", _("IDS_COM_BODY_SEARCHING"));
			break;
		default:
			ERR("invalid service type[%d]", service_type);
			plmn = get_sim_plmn(handle);
			if (plmn != NULL && plmn[0] != 0) {
				INFO("PLMN/SPN - Sim %p using PLMN: %s", handle, plmn);
				snprintf(buf, sizeof(buf), "%s", plmn);
			}
			break;
		}
	}

	DBG("handle[%d][%p] service_type[%d], name_option[%d] >> [%s]", handle_num, handle, service_type, name_option, buf);

	if (strlen(buf) == 0) {
		ERR("Empty string");
		snprintf(buf, sizeof(buf), "%s", _("IDS_IDLE_BODY_NO_SERVICE"));
	} else if (strncasecmp(buf, "No Service", strlen("No Service")) == 0) {
		ERR("USING SPECIAL NETWORK NAME:  %s in handle: %d", _("IDS_IDLE_BODY_NO_SERVICE"), handle_num);
		return strdup(_("IDS_IDLE_BODY_NO_SERVICE"));
	} else if (strncasecmp(buf, "EMERGENCY", strlen("EMERGENCY")) == 0) {
		ERR("USING SPECIAL NETWORK NAME:  %s in handle: %d", _("IDS_IDLE_MBODY_EMERGENCY_CALLS_ONLY"), handle_num);
		return strdup(_("IDS_IDLE_MBODY_EMERGENCY_CALLS_ONLY"));
	} else if (strncasecmp(buf, "Searching", strlen("Searching")) == 0) {
		ERR("USING SPECIAL NETWORK NAME:  %s in handle: %d", _("IDS_COM_BODY_SEARCHING"), handle_num);
		return strdup(_("IDS_COM_BODY_SEARCHING"));
	} else if (strncasecmp(buf, "SIM Error", strlen("SIM Error")) == 0) {
		ERR("USING SPECIAL NETWORK NAME:  %s in handle: %d", _("IDS_IDLE_BODY_INVALID_SIM_CARD"), handle_num);
		return strdup(_("IDS_IDLE_BODY_INVALID_SIM_CARD"));
	} else if (strncasecmp(buf, "NO SIM", strlen("NO SIM")) == 0) {
		ERR("USING SPECIAL NETWORK NAME:  %s in handle: %d", _("IDS_IDLE_MBODY_EMERGENCY_CALLS_ONLY"), handle_num);
		return strdup(_("IDS_IDLE_MBODY_EMERGENCY_CALLS_ONLY"));
	}

	return strdup(buf);
}

// --------------------------------------------------------------------------------------------
static void print_sim_status(TelSimCardStatus_t sim_status, int card_changed)
{
	switch(sim_status) {
	case TAPI_SIM_STATUS_CARD_ERROR:
		INFO("Sim card status: TAPI_SIM_STATUS_CARD_ERROR");
		break;

	case TAPI_SIM_STATUS_CARD_NOT_PRESENT:
		INFO("Sim card status: TAPI_SIM_STATUS_CARD_NOT_PRESENT");
		break;

	case TAPI_SIM_STATUS_SIM_INITIALIZING:
		INFO("Sim card status: TAPI_SIM_STATUS_SIM_INITIALIZING");
		break;

	case TAPI_SIM_STATUS_SIM_INIT_COMPLETED:
		INFO("Sim card status: TAPI_SIM_STATUS_SIM_INIT_COMPLETED");
		break;

	case TAPI_SIM_STATUS_SIM_PIN_REQUIRED:
		INFO("Sim card status: TAPI_SIM_STATUS_SIM_PIN_REQUIRED");
		break;

	case TAPI_SIM_STATUS_SIM_PUK_REQUIRED:
		INFO("Sim card status: TAPI_SIM_STATUS_SIM_PUK_REQUIRED");
		break;

	case TAPI_SIM_STATUS_CARD_BLOCKED:
		INFO("Sim card status: TAPI_SIM_STATUS_CARD_BLOCKED");
		break;

	case TAPI_SIM_STATUS_SIM_NCK_REQUIRED:
		INFO("Sim card status: TAPI_SIM_STATUS_SIM_NCK_REQUIRED");
		break;

	case TAPI_SIM_STATUS_SIM_NSCK_REQUIRED:
		INFO("Sim card status: TAPI_SIM_STATUS_SIM_NSCK_REQUIRED");
		break;

	case TAPI_SIM_STATUS_SIM_SPCK_REQUIRED:
		INFO("Sim card status: TAPI_SIM_STATUS_SIM_SPCK_REQUIRED");
		break;

	case TAPI_SIM_STATUS_SIM_CCK_REQUIRED:
		INFO("Sim card status: TAPI_SIM_STATUS_SIM_CCK_REQUIRED");
		break;

	case TAPI_SIM_STATUS_CARD_REMOVED:
		INFO("Sim card status: TAPI_SIM_STATUS_CARD_REMOVED");
		break;

	case TAPI_SIM_STATUS_SIM_LOCK_REQUIRED:
		INFO("Sim card status: TAPI_SIM_STATUS_SIM_LOCK_REQUIRED");
		break;

	case TAPI_SIM_STATUS_CARD_CRASHED:
		INFO("Sim card status: TAPI_SIM_STATUS_CARD_CRASHED");
		break;

	case TAPI_SIM_STATUS_CARD_POWEROFF:
		INFO("Sim card status: TAPI_SIM_STATUS_CARD_POWEROFF");
		break;

	case TAPI_SIM_STATUS_UNKNOWN:
		INFO("Sim card status: TAPI_SIM_STATUS_UNKNOWN");
		break;
	}

	INFO("Sim_card_changed: %d", card_changed);
}

static void get_sim_status()
{
	int i;
	int ret;
	TelSimCardStatus_t sim_status;
	int card_changed;

	for (i = 0; i < TAPI_HANDLE_MAX + 1; ++i) {
		if (sim_state_info.handle[i]) {
			ret = tel_get_sim_init_info (sim_state_info.handle[i], &sim_status, &card_changed);
			if(ret == 0) {
				print_sim_status(sim_status, card_changed);

				if(sim_status == TAPI_SIM_STATUS_SIM_INIT_COMPLETED || sim_status == TAPI_SIM_STATUS_SIM_PIN_REQUIRED) {
					if (i < TAPI_HANDLE_MAX) {
						sim_state_info.sim_card_ready[i] = EINA_TRUE;
					}
				} else {
					ERR("SIM[%d] is not completed initialization [%d]", i, sim_status);
				}
			} else {
				ERR("Could not get sim[%d] status[%d]", i, ret);
			} // if ret == 0
		} // if sim_state_info
	} // for
}

static void sim_handler_text_set(Eina_Bool flight_mode)
{
	if (flight_mode) {
		// if flight mode, No service
		quickpanel_handler_text_set(_("IDS_IDLE_BODY_NO_SERVICE"));
	} else if (sim_state_info.sim_card_ready[0] && sim_state_info.sim_card_ready[1]) {
		quickpanel_handler_text_set(NULL);
	} else if(sim_state_info.sim_card_ready[0]) {
		char *plmn_spn1 = get_plmn_spn_network(0, sim_state_info.handle[0]);
		quickpanel_handler_text_set(plmn_spn1);
		if (plmn_spn1) {
			free(plmn_spn1);
		}
	} else if(sim_state_info.sim_card_ready[1]) {
		char *plmn_spn1 = get_plmn_spn_network(1, sim_state_info.handle[1]);
		quickpanel_handler_text_set(plmn_spn1);
		if (plmn_spn1) {
			free(plmn_spn1);
		}
	} else {
		quickpanel_handler_text_set(_("IDS_IDLE_MBODY_EMERGENCY_CALLS_ONLY"));
	}
}

static void init_view()
{
	struct appdata *ad = NULL;

	ad = quickpanel_get_app_data();
	if (ad == NULL) {
		ERR("invalid data");
		return;
	}

	int flight_mode_state = EINA_FALSE;
	int ret = vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE, &flight_mode_state);
	if(ret != 0) {
		ERR("Could not get 'VCONFKEY_TELEPHONY_FLIGHT_MODE' value");
	}

	sim_handler_text_set(flight_mode_state);
}

/* Initialize TAPI */
static void _init_tel()
{
	char **cp_list = NULL;
	unsigned int modem_num = 0;

	/* Get CP name list – cp_list */
	cp_list = tel_get_cp_name_list();

	if(cp_list == NULL) {
		ERR("Could not get the cp_name_list");
		return;
	}

	while (cp_list[modem_num]) {
		/* Initialize TAPI handle */
		sim_state_info.handle[modem_num] = tel_init(cp_list[modem_num]);

		if (cp_list[modem_num]) {
			ERR("sim_state_info.handle[%d] = %s; ptr = %p", modem_num, cp_list[modem_num], sim_state_info.handle[modem_num]);
		}

		/* Move to next CP Name in cp_list */
		modem_num++;
	}

	sim_state_info.handle[modem_num] = NULL;

	/* free cp_list */
	free(cp_list);
}

/* De-initialize TAPI */
static void _deinit_tel()
{
	int i = 0;
	while (sim_state_info.handle[i]) {
		/* De-initialize TAPI handle */
		tel_deinit(sim_state_info.handle[i]);
		sim_state_info.handle[i] = NULL;

		/* Move to next handle */
		i++;
	}
}

/* Telephony state change callback */
void tel_ready_cb(keynode_t *key, void *data)
{
	Eina_Bool status = EINA_FALSE;

	status = vconf_keynode_get_bool(key);

	if (status == TRUE) {	/* Telephony State - READY */
		DBG("tel status[%d]", status);
		_init_tel();
		register_sim_callbacks();
		get_sim_status();

		init_view();
	} else {   /* Telephony State – NOT READY */
		/* De-initialization is optional here (ONLY if required) */
		ERR("tel status[%d]", status);
		_deinit_tel();
		sim_state_info.sim_card_ready[0] = EINA_FALSE;
		sim_state_info.sim_card_ready[1] = EINA_FALSE;

		unregister_sim_callbacks();
	}
}

static void tel_flight_mode_cb(keynode_t *key, void *data)
{
	Eina_Bool flight_mode_state = EINA_FALSE;

	flight_mode_state = vconf_keynode_get_bool(key);
	sim_handler_text_set(flight_mode_state);
}

// --------------------------------------------------------------------------------------------
static void on_sim_card_status_changed(TapiHandle *handle, const char *noti_id, void *data, void *user_data)
{
	int handle_num;
	int *sim_status = data;

	/**
	 * @note
	 * Casting the pointer to "long" first for 64 bits architecture.
	 * And then convert it to "int"
	 */
	handle_num = (int)((long)user_data);

	ERR("SIM[%p][%d] status[%d], [%d][%d]", handle, handle_num, *sim_status, sim_state_info.sim_card_ready[0], sim_state_info.sim_card_ready[1]);

	if(*sim_status == TAPI_SIM_STATUS_SIM_INIT_COMPLETED || *sim_status == TAPI_SIM_STATUS_SIM_PIN_REQUIRED) {
		sim_state_info.sim_card_ready[handle_num] = EINA_TRUE;
	} else {
		sim_state_info.sim_card_ready[handle_num] = EINA_FALSE;
	}

	init_view();
}

static void on_plmn_spn_changed(TapiHandle *handle, const char *noti_id,
		void *data, void *user_data)
{
	if (!handle) {
		ERR("handle == NULL");
		return;
	}

	int flight_mode_state = EINA_FALSE;
	int ret = vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE, &flight_mode_state);
	if (ret != 0) {
		ERR("Could not get the 'VCONFKEY_TELEPHONY_FLIGHT_MODE' value");
	}
	sim_handler_text_set(flight_mode_state);
}

static void register_sim_callbacks()
{
	long i;
	int ret;

	for (i = 0; i < TAPI_HANDLE_MAX; ++i) {
		if (sim_state_info.handle[i]) {
			ret = tel_register_noti_event(sim_state_info.handle[i], TAPI_NOTI_SIM_STATUS, on_sim_card_status_changed, (void *)i);
			if (ret != TAPI_API_SUCCESS) {
				ERR("Failed to register 'on_sim_card_status_changed' callback to handle[%d][%d]", i, ret);
			} else {
				ERR("SIM card status changed event registered");
			}

			ret = tel_register_noti_event(sim_state_info.handle[i], TAPI_PROP_NETWORK_SPN_NAME, on_plmn_spn_changed, (void *)i);
			if (ret != TAPI_API_SUCCESS) {
				ERR("Failed to register 'on_plmn_spn_changed' callback to handle[%d][%d]", i, ret);
			}

			ret = tel_register_noti_event(sim_state_info.handle[i], TAPI_PROP_NETWORK_NETWORK_NAME, on_plmn_spn_changed, (void *)i);
			if (ret != TAPI_API_SUCCESS) {
				ERR("Failed to register 'on_plmn_spn_changed' callback to handle: %i", i);
			}

			ret = tel_register_noti_event(sim_state_info.handle[i], TAPI_PROP_NETWORK_SERVICE_TYPE, on_plmn_spn_changed, (void *)i);
			if (ret != TAPI_API_SUCCESS) {
				ERR("Failed to register network service type[%d][%d]", ret, i);
			}

			ret = tel_register_noti_event(sim_state_info.handle[i], TAPI_PROP_NETWORK_NAME_OPTION, on_plmn_spn_changed, (void *)i);
			if (ret != TAPI_API_SUCCESS) {
				ERR("Failed to register network name option[%d][%d]", ret, i);
			}
		} else {
			ERR("No handle [%d]", i);
		}
	}
}

static void unregister_sim_callbacks()
{
	int i;
	int ret;
	for(i = 0; i < TAPI_HANDLE_MAX; ++i) {
		if(sim_state_info.handle[i]) {
			ret = tel_deregister_noti_event(sim_state_info.handle[i], TAPI_NOTI_SIM_STATUS);
			if (ret != TAPI_API_SUCCESS) {
				ERR("Failed to dereregister TAPI_NOTI_SIM_STATUS callback from handle: %i", i);
			} else {
				DBG("SIM status changed event deregistered");
			}

			ret = tel_deregister_noti_event(sim_state_info.handle[i], TAPI_PROP_NETWORK_NETWORK_NAME);
			if (ret != TAPI_API_SUCCESS) {
				ERR("Failed to dereregister TAPI_PROP_NETWORK_PLMN callback from handle: %i", i);
			}

			ret = tel_deregister_noti_event(sim_state_info.handle[i], TAPI_PROP_NETWORK_SPN_NAME);
			if (ret != TAPI_API_SUCCESS) {
				ERR("Failed to dereregister TAPI_PROP_NETWORK_SPN_NAME callback from handle: %i", i);
			}

			ret = tel_deregister_noti_event(sim_state_info.handle[i], TAPI_PROP_NETWORK_SERVICE_TYPE);
			if (ret != TAPI_API_SUCCESS) {
				ERR("Failed to deregister network service type[%d][%d]", ret, i);
			}

			ret = tel_deregister_noti_event(sim_state_info.handle[i], TAPI_PROP_NETWORK_NAME_OPTION);
			if (ret != TAPI_API_SUCCESS) {
				ERR("Failed to deregister network name option[%d][%d]", ret, i);
			}

			if(i == 0) {
				ret = tel_deregister_noti_event(sim_state_info.handle[i], TAPI_NOTI_CALL_PREFERRED_VOICE_SUBSCRIPTION);
				if (ret != TAPI_API_SUCCESS) {
					ERR("Failed to dereregister  callback to handle: %d", i);
				}
			}
		}
	} // for
}

void sim_controller_init(Evas_Object *master_layout)
{
	int state = EINA_FALSE;
	int ret;
	/* Check if Telephony state - READY */
	ret = vconf_get_bool(VCONFKEY_TELEPHONY_READY, &state);

	DBG("VCONFKEY_TELEPHONY_READY == %d", state);

	if (ret != -1 && state == TRUE) {
		/* Telephony State - READY */
		/* Initialize TAPI handles */

		_init_tel();
		register_sim_callbacks();
		get_sim_status();

		init_view();
	} else {	/* Telephony State – NOT READY, register for change in state */
		DBG("Telephony state: [NOT Ready]");
	}

	/* Register for Telephony state change */
	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_READY, tel_ready_cb, master_layout);
	if(ret != 0) {
		ERR("Failed to register VCONFKEY_TELEPHONY_READY key changed callback");
	}

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE, tel_flight_mode_cb, master_layout);
	if(ret != 0) {
		ERR("Failed to register VCONFKEY_TELEPHONY_FLIGHT_MODE key changed callback");
	}
}

void sim_controller_resume()
{
	int state = FALSE;
	int ret = 0;
	int i = 0;
	TelSimCardStatus_t sim_status;
	int card_changed;

	ret = vconf_get_bool(VCONFKEY_TELEPHONY_READY, &state);
	if (ret != 0 || state == FALSE) {
		ERR("Failed to get telephony state[%d][%d]", state, ret);
		return;
	}

	for (i = 0; i < TAPI_HANDLE_MAX; ++i) {
		if (sim_state_info.handle[i]) {
			ret = tel_get_sim_init_info(sim_state_info.handle[i], &sim_status, &card_changed);
			DBG("SIM[%d] info[%d][%d][%d]", i, ret, sim_status, card_changed);
			if (sim_status == TAPI_SIM_STATUS_SIM_INIT_COMPLETED || sim_status == TAPI_SIM_STATUS_SIM_PIN_REQUIRED) {
				if (sim_state_info.sim_card_ready[i] != EINA_TRUE) {
					ERR("SIM[%d] is init completed but local value is not ture", i);
				}
			}
		} else {
			ERR("No handle[%d]", i);
		}
	}
}

void sim_controller_on_language_change()
{
	on_plmn_spn_changed(sim_state_info.handle[0], "SELF", NULL, (void*) 0);
	on_plmn_spn_changed(sim_state_info.handle[1], "SELF", NULL, (void*) 1);

	if (sim_state_info.handle[0] == NULL && sim_state_info.handle[1] == NULL) {
		int flight_mode = EINA_FALSE;
		int ret = vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE, &flight_mode);
		if (ret != 0) {
			ERR("Failed to get flight mode[%d]", ret);
		}

		sim_handler_text_set(flight_mode);
	}
}
