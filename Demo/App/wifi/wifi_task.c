#include "json_public.h"
#include "../../Middlewares/Wifi/wifi.h"
#include "../../Middlewares/global.h"
#include "../upgrade/upgrade.h"
#include <string.h>
#include "cmsis_os.h"

#define MAX_TOPIC_LEN 64
#define WIFI_MAX_FRAME_SIZE (1024 - MAX_TOPIC_LEN)

static uint8_t buffer[WIFI_MAX_FRAME_SIZE + MAX_TOPIC_LEN] = {0};
extern osMessageQId wifiQueueHandle;

typedef struct {
    char topic[MAX_TOPIC_LEN];
    char payload[WIFI_MAX_FRAME_SIZE];
} mqtt_message_t;

static bool parse_mqtt(const char* response, mqtt_message_t* msg) {
    if (response == NULL || msg == NULL) return false;
    
    /* 查找 MQTT 响应前缀 */
    const char* prefix = "AT+MQTTSUBRECV=";
    if (strncmp(response, prefix, strlen(prefix)) != 0) {
        return false;
    }
    
    char* ptr = (char*)response + strlen(prefix);
    
    /* 跳过 linkID */
    ptr = strchr(ptr, ',');
    if (ptr == NULL) return false;
    ptr++;
    
    /* 解析 topic */
    if (*ptr == '"') {
        ptr++;
        char* end = strchr(ptr, '"');
        if (end == NULL) return false;
        size_t len = end - ptr;
        len = (len >= MAX_TOPIC_LEN) ? MAX_TOPIC_LEN - 1 : len;
        strncpy(msg->topic, ptr, len);
        msg->topic[len] = '\0';
        ptr = end + 2;  // 跳过 "\"" 和 ","
    }
    
    /* 跳过 payload length */
    ptr = strchr(ptr, ',');
    if (ptr == NULL) return false;
    ptr++;
    
    /* 解析 payload */
    if (*ptr == '"') {
        ptr++;
        char* end = strchr(ptr, '"');
        if (end == NULL) return false;
        size_t len = end - ptr;
        len = (len >= WIFI_MAX_FRAME_SIZE) ? WIFI_MAX_FRAME_SIZE - 1 : len;
        strncpy(msg->payload, ptr, len);
        msg->payload[len] = '\0';
    }
    
    return true;
}

void mqtt_handler(const char* response) {
    mqtt_message_t msg = {0};
    
    if (!parse_mqtt(response, &msg)) {
        LOG_E(MODULE_WIFI, "Failed to parse MQTT message: %s\r\n", msg.topic);
        return;  
    }
    
    LOG_I(MODULE_WIFI, "Topic: %s\r\n", msg.topic);
    
    /* Topic 分发 */
    if (strcmp(msg.topic, "device/ota") == 0) {
        ota_process_chunk(msg.payload);
    } else if (strcmp(msg.topic, "device/config") == 0) {
        // process_config(msg.payload);
    } else if (strcmp(msg.topic, "device/cmd") == 0) {
        // process_command(msg.payload);
    }
}

/* 模拟 OTA 测试数据 */
static const char* ota_test_data = 
    "\"device/ota\",\"{\\\"chunkType\\\":0,\\\"version\\\":\\\"00.00.04\\\",\\\"bagSize\\\":407732,\\\"bagType\\\":0,\\\"sequence\\\":1,\\\"data\\\":\\\"QN==Em==cK==ra==+x==tG==M0==WV==Xd==8n==Mi==WH==v7==od==+y==OD==9B==ts==cd==2K==Uq==nr==Ud==f7==2m==Ue==2S==H3==d5==X/==cB==wv==K5==1n==TR==8g==XG==+A==m5==hS==0O==Zp==qM==/Z==kU==2P==Cr==L9==/Z==CX==8h==0x==sl==oY==nG==Mg==c5==vM==9M==Jh==cx==Jz==mo==VN==v2==g7==tp==tM==BL==21==Oc==ty==jA==8p==0j==d9==wp==Wt==gg==EL==jB==jq==QV==xD==Da==5z==Gx==M0==2h==+C==3a==ld==3Y==tG==oy==GM==N7==DQ==LM==Pg==DS==EA==R4==xa==aT==Od==Lh==Mb==Cm==ec==7O==Tx==3j==Ou==6G==JL==pc==Pq==Ck==t0==wp==as==up==x4==2H==iL==9p==8N==ig==bj==Q9==Vb==I+==NE==F0==y4==An==XQ==0y==OO==J9==Ih==Xt==tg==3M==U+==kc==jX==Ol==Mj==3/==Lu==J+==0d==aa==LS==bU==nv==nk==Tc==fI==SD==c3==g2==ND==CN==o5==Sv==y/==qu==xT==Ht==Fk==R2==LK==cp==aL==ie==mW==3Z==l0==6+==M6==ND==f5==/+==BR==p1==Fy==uU==6y==ax==y6==u8==Cp==6v==Mu==Yv==EL==f+==mp==DN==jv==wV==dk==38==iu==6p==7H==m/==1A==GT==6r==u9==ME==XH==MG==Ee==Fy==Pv==lU==9R==yR==Ll==yj==kE==j8==D+==/e==hg==B1==5k==m3==G5==xx==L9==Fq==BH==5m==U6==Un==Ht==qu==jw==N+==NA==JB==6U==ro==u5==1W==IW==cr==LG==2Y==tW==Tj==Jf==gf==i4==d7==eq==Jv==86==5+==6P==ys==Yr==Og==9R==cv==MV==Sa==kF==On==QL==f9==KG==m8==Mj==t8==68==1l==Jx==De==rA==Ie==X/==zi==nS==Xy==wG==FY==s4==lX==bI==95==hY==BH==2V==if==Ib==da==L4==6L==y1==hg==ro==ob==n7==sJ==nn==2d==nI==Qi==X9==sX==ym==Cd==Vp==/9==WA==fW==Q+==SA==7B==rK==wb==fS==BN==Yn==BK==lR==5r==Gv==ie==Zq==53==VH==BE==y8==hA==\\\"}\"\r\n";

/* 模拟发送 OTA 测试数据 */
void wifi_send_test_data(void) {
    if (wifiQueueHandle == NULL) {
        LOG_E(MODULE_WIFI, "Queue not initialized!\n");
        return;
    }
    
    /* 发送测试数据到队列 */
    uint8_t test_buffer[WIFI_MAX_FRAME_SIZE] = {0};
    snprintf((char*)test_buffer, sizeof(test_buffer), "%s", ota_test_data);
    
    if (xQueueSend(wifiQueueHandle, test_buffer, portMAX_DELAY) == pdTRUE) {
        LOG_I(MODULE_WIFI, "Test OTA data sent to queue successfully\n");
    } else {
        LOG_E(MODULE_WIFI, "Failed to send test data to queue\n");
    }
}

/**
 * @brief WIFI任务函数
 * 
 * @param pvParameters 参数
 */
static void wifi_task(void *pvParameters) {
    LOG_I(MODULE_SYS, "WIFI Task Start.\r\n");

    vTaskDelay(3000);
    wifi_send_test_data();

    while(1) {
        if (xQueueReceive(wifiQueueHandle, buffer, portMAX_DELAY) == pdTRUE) {
            mqtt_handler((char*)buffer);
        }
        vTaskDelay(10);
    }
}
MODULE_TASK(wifi_task, "WIFI_Task", configMINIMAL_STACK_SIZE * 5, 1);
