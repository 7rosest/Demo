#include "json_public.h"
#include "../Middlewares/global.h"
#include <string.h>

void parse_json(const char* json_string);

void generate_state_json(void) {
    cJSON* root = cJSON_CreateObject();
    if (root == NULL) {
        LOG_E(MODULE_OTA, "Failed to create JSON object!\r\n");
        return;
    }

    cJSON_AddStringToObject(root, "bootVersion", "0.0.3");
    cJSON_AddStringToObject(root, "appVersion", ECU_SW_VERSION);
    cJSON_AddStringToObject(root, "status", "success");
 
    char* json_string = cJSON_Print(root);
    // LOG_I(MODULE_OTA, "Generated JSON: %s\n", json_string);
    parse_ota_json(json_string);
 
    cJSON_free(json_string);
    cJSON_Delete(root);
}

void parse_ota_json(const char* json_string) {
    if (json_string == NULL) {
        LOG_E(MODULE_OTA, "Invalid JSON string!\r\n");
        return;
    }

    if (strlen(json_string) > 1024) {
        LOG_E(MODULE_OTA, "JSON string is too long!\r\n");
        return;
    }

    cJSON* root = cJSON_Parse(json_string);
    if (root == NULL) {
        LOG_E(MODULE_OTA, "JSON parse error!\r\n");
        return;
    }
 
    cJSON* name = cJSON_GetObjectItem(root, "bootVersion");
    if (name != NULL && cJSON_IsString(name) && name->valuestring != NULL) {
        LOG_I(MODULE_OTA, "BootVersion: %s\r\n", name->valuestring);
    } else {
        LOG_E(MODULE_OTA, "BootVersion not found or is not a string!\r\n");
    }
 
    cJSON* age = cJSON_GetObjectItem(root, "appVersion");
    if (age != NULL && cJSON_IsString(age) && age->valuestring != NULL) {
        LOG_I(MODULE_OTA, "AppVersion: %s\r\n", age->valuestring);
    } else {
        LOG_E(MODULE_OTA, "AppVersion not found or is not a string!\r\n");
    }
 
    cJSON* status = cJSON_GetObjectItem(root, "status");
    if (status != NULL && cJSON_IsString(status) && status->valuestring != NULL) {
        LOG_I(MODULE_OTA, "Status: %s\r\n", status->valuestring);
    } else {
        LOG_E(MODULE_OTA, "Status not found or is not a string!\r\n");
    }
        
    cJSON_Delete(root);
}

void json_init_with_os(void)
{
    cJSON_Hooks hooks;
    hooks.malloc_fn = pvPortMalloc; 
    hooks.free_fn = vPortFree;       
    cJSON_InitHooks(&hooks);         
}

