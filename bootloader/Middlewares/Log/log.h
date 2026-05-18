#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "../shell/shell.h"

typedef enum {
    LOG_LEVEL_NONE = 0,   
    LOG_LEVEL_ERROR,       
    LOG_LEVEL_WARN,        
    LOG_LEVEL_INFO,        
    LOG_LEVEL_DEBUG,       
    LOG_LEVEL_HEX          
} log_level_t;

typedef enum {
    MODULE_SHELL = 0,      
    MODULE_ECALL,            
    MODULE_GPS,           
    MODULE_COMM,           
    MODULE_MAX            
} module_id_t;

typedef struct {
    const char* name;      
    log_level_t level;     
} debug_config_t;

extern debug_config_t debug_config[MODULE_MAX];

#define LOG_E(module, fmt, ...) do { if(debug_config[module].level >= LOG_LEVEL_INFO) shellPrintf("I/%s: " fmt, debug_config[module].name, ##__VA_ARGS__); } while(0)
#define LOG_W(module, fmt, ...) do { if(debug_config[module].level >= LOG_LEVEL_INFO) shellPrintf("I/%s: " fmt, debug_config[module].name, ##__VA_ARGS__); } while(0)
#define LOG_I(module, fmt, ...) do { if(debug_config[module].level >= LOG_LEVEL_INFO) shellPrintf("I/%s: " fmt, debug_config[module].name, ##__VA_ARGS__); } while(0)
#define LOG_D(module, fmt, ...) do { if(debug_config[module].level >= LOG_LEVEL_INFO) shellPrintf("I/%s: " fmt, debug_config[module].name, ##__VA_ARGS__); } while(0)

#define LOG_HEX(module, data, len) \
    if(debug_config[module].level >= LOG_LEVEL_HEX) { \
        shellPrintf("H/%s: ", debug_config[module].name); \
        for(uint32_t i=0; i<len; i++) shellPrintf("%02X ", ((uint8_t*)data)[i]); \
    }

void log_init(void);
void log_set_level(module_id_t module, log_level_t level);
void log_show_status(void);

#endif 