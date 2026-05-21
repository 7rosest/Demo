#include "log.h"
#include <string.h>
#include <ctype.h>

debug_config_t debug_config[MODULE_MAX] = {
    {"sys",      LOG_LEVEL_INFO},    // SYS模块
    {"shell",    LOG_LEVEL_INFO},    // SHELL模块
    {"ecall",    LOG_LEVEL_INFO},    // ECALL模块
    {"gps",      LOG_LEVEL_INFO},    // GPS模块
    {"comm",     LOG_LEVEL_INFO},    // 通信模块
};

/**
 * @brief 设置指定模块的调试等级
 * 
 * @param module 模块ID，取值范围为0到MODULE_MAX-1
 * @param level 调试等级，取值范围为0-5（0=无输出，5=HEX输出）
 * @note 如果传入的模块ID无效，函数将静默返回，不执行任何操作。
 * @return 无
 */
void log_set_level(module_id_t module, log_level_t level) {
    if (module < MODULE_MAX) {
        debug_config[module].level = level;
    }
}

/**
 * @brief 显示所有模块的调试状态
 * 
 * @note 此函数会在SHELL模块中打印所有模块的调试等级，格式为"模块名: L等级"。
 * @return 无
 */
void log_show_status(void) {
    shellPrintf("========== Log Status ==========\r\n");
    for (int i = 0; i < MODULE_MAX; i++) {
        shellPrintf("%-10s: L%c\r\n", debug_config[i].name, debug_config[i].level == LOG_LEVEL_DEBUG ? 'D' :
                                            debug_config[i].level == LOG_LEVEL_INFO ? 'I' :
                                            debug_config[i].level == LOG_LEVEL_WARN ? 'W' :
                                            debug_config[i].level == LOG_LEVEL_ERROR ? 'E' : 'N');
    }
    shellPrintf("===============END==============\r\n");
}