#ifndef JSON_PUBLIC_H
#define JSON_PUBLIC_H

#include <stdint.h>
#include "../../Middlewares/CJson/cJSON.h"
#include "../../Middlewares/Log/log.h"

void generate_state_json(void);
void parse_ota_json(const char* json_string);
void json_init_with_os(void);

#endif 
