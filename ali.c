/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-08-10     a       the first version
 */
#include "board.h"
#include <rtthread.h>
#include "dev_sign_api.h"
#include "mqtt_api.h"
#include <infra_defs.h>
#include "aiot_mqtt_sign.h"
#include "put.h"
#include "at_wrapper.h"
#include "at_parser.h"
#include "at_api.h"
#include "at_mqtt.h"

void example_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    HAL_Printf("msg->event_type : %d\n", msg->event_type);
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "infra_config.h"
#include "mqtt_api.h"
#include "dev_sign_api.h"
#define Topic "/sys/a1ehllrd5dK/device1/thing/event/property/post"
#define ToPic   "/sys/a1ehllrd5dK/device1/thing/event/property/post"
#ifdef ATM_ENABLED

#endif

static char g_topic_name[CONFIG_MQTT_TOPIC_MAXLEN];

void HAL_Printf(const char *fmt, ...);
int HAL_GetProductKey(char product_key[IOTX_PRODUCT_KEY_LEN]);
int HAL_GetDeviceName(char device_name[IOTX_DEVICE_NAME_LEN]);
int HAL_GetDeviceSecret(char device_secret[IOTX_DEVICE_SECRET_LEN]);
uint64_t HAL_UptimeMs(void);
int HAL_Snprintf(char *str, const int len, const char *fmt, ...);

#define EXAMPLE_TRACE(fmt, ...)  \
    do { \
        HAL_Printf("%s|%03d :: ", __func__, __LINE__); \
        HAL_Printf(fmt, ##__VA_ARGS__); \
        HAL_Printf("%s", "\r\n"); \
    } while(0)

//void example_message_arrive(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
//{
//    iotx_mqtt_topic_info_t     *topic_info = (iotx_mqtt_topic_info_pt) msg->msg;
//
//    switch (msg->event_type) {
//        case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
//            /* print topic name and topic message */
//            HAL_Printf("Message Arrived: \n");
//            HAL_Printf("Topic  : %.*s\n", topic_info->topic_len, topic_info->ptopic);
//            HAL_Printf("Payload: %.*s\n", topic_info->payload_len, topic_info->payload);
//            HAL_Printf("\n");
//            break;
//        default:
//            break;
//    }
//}
//
//int example_subscribe(void *handle)
//{
//    int res = 0;
//    char product_key[IOTX_PRODUCT_KEY_LEN] = {0};
//    char device_name[IOTX_DEVICE_NAME_LEN] = {0};
//    const char *fmt = "/%s/%s/user/get";
//    char *topic = NULL;
//    int topic_len = 0;
//
//    HAL_GetProductKey(product_key);
//    HAL_GetDeviceName(device_name);
//
//    topic_len = strlen(fmt) + strlen(product_key) + strlen(device_name) + 1;
//    if (topic_len > CONFIG_MQTT_TOPIC_MAXLEN) {
//        HAL_Printf("topic too long\n");
//        return -1;
//    }
//    topic = g_topic_name;
//    memset(topic, 0, CONFIG_MQTT_TOPIC_MAXLEN);
//    HAL_Snprintf(topic, topic_len, fmt, product_key, device_name);
//
//    //res = IOT_MQTT_Subscribe(handle, topic, IOTX_MQTT_QOS0, example_message_arrive, NULL);
//    if (res < 0) {
//        HAL_Printf("subscribe failed\n");
//        return -1;
//    }
//
//    return 0;
//}
//
//int example_publish(void *handle)
//{
//    int res = 0;
//    iotx_mqtt_topic_info_t topic_msg;
//    char product_key[IOTX_PRODUCT_KEY_LEN] = {"gb675vsnWMt"};
//    char device_name[IOTX_DEVICE_NAME_LEN] = {"ebytetest"};
//    const char *fmt = "/%s/%s/user/get";
//    char *topic = NULL;
//    int topic_len = 0;
//    char *payload = "hello,world";
//
//    HAL_GetProductKey(product_key);
//    HAL_GetDeviceName(device_name);
//
//    topic_len = strlen(fmt) + strlen(product_key) + strlen(device_name) + 1;
//    if (topic_len > CONFIG_MQTT_TOPIC_MAXLEN) {
//        HAL_Printf("topic too long\n");
//        return -1;
//    }
//    topic = g_topic_name;
//    memset(topic, 0, CONFIG_MQTT_TOPIC_MAXLEN);
//    HAL_Snprintf(topic, topic_len, fmt, product_key, device_name);
//
//
//    memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));
//    topic_msg.qos = IOTX_MQTT_QOS0;
//    topic_msg.retain = 0;
//    topic_msg.dup = 0;
//    topic_msg.payload = (void *)payload;
//    topic_msg.payload_len = strlen(payload);
//
//    //res = IOT_MQTT_Publish(handle, topic, &topic_msg);
//    if (res < 0) {
//        HAL_Printf("publish failed\n");
//        return -1;
//    }
//
//    return 0;
//}
//
//void example_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
//{
//    HAL_Printf("msg->event_type : %d\n", msg->event_type);
//}

/*
 *  NOTE: About demo topic of /${productKey}/${deviceName}/user/get
 *
 *  The demo device has been configured in IoT console (https://iot.console.aliyun.com)
 *  so that its /${productKey}/${deviceName}/user/get can both be subscribed and published
 *
 *  We design this to completely demostrate publish & subscribe process, in this way
 *  MQTT client can receive original packet sent by itself
 *
 *  For new devices created by yourself, pub/sub privilege also required to be granted
 *  to its /${productKey}/${deviceName}/user/get to run whole example
 */




























static void ali_thread_entry(void *parameter)
{
    rt_thread_mdelay(2000);
    //uart_init();
    void *      pclient = NULL;
        int         res = 0;
        int         loop_cnt = 0;
        iotx_mqtt_region_types_t    region = IOTX_CLOUD_REGION_SHANGHAI;
        iotx_sign_mqtt_t            sign_mqtt;
        iotx_dev_meta_info_t        meta;
        iotx_mqtt_param_t           mqtt_params;
    #ifdef ATM_ENABLED
        if (IOT_ATM_Init() < 0) {
            rt_kprintf("IOT ATM init failed!\n");
            return ;
        }
    #endif
        rt_kprintf("mqtt example\n");
        memset(&meta, 0, sizeof(iotx_dev_meta_info_t));
        HAL_GetProductKey(meta.product_key);
        HAL_GetDeviceName(meta.device_name);
        HAL_GetDeviceSecret(meta.device_secret);
        sign_mqtt.port = 1883;

        *(sign_mqtt.hostname) = "gb675vsnWMt.iot-as-mqtt.cn-shanghai.aliyuncs.com";
        aiotMqttSign(meta.product_key, meta.device_name, meta.product_secret, sign_mqtt.clientid, sign_mqtt.username, sign_mqtt.password);
        /* Initialize MQTT parameter */

            memset(&mqtt_params, 0x0, sizeof(mqtt_params));
            mqtt_params.port = sign_mqtt.port;
            mqtt_params.host = sign_mqtt.hostname;
            mqtt_params.client_id = sign_mqtt.clientid;
            mqtt_params.username = sign_mqtt.username;
            mqtt_params.password = sign_mqtt.password;
            mqtt_params.request_timeout_ms = 2000;
            mqtt_params.clean_session = 0;
            mqtt_params.keepalive_interval_ms = 60000;
            mqtt_params.read_buf_size = 1024;
            mqtt_params.write_buf_size = 1024;
            mqtt_params.handle_event.h_fp = &example_event_handle;
            mqtt_params.handle_event.pcontext = NULL;
            HAL_AT_MQTT_Init(&mqtt_params);
           HAL_AT_MQTT_Connect("gb675vsnWMt","testEC600S","e223516e990e46d39d84cac5f7ef16ad");
           at_ec20_mqtt_client_subscribe(Topic,2,5000);
           at_ec20_mqtt_client_publish(ToPic, 0, 143,1,7);


//            while (1) {
//                    if (0 == loop_cnt % 20) {
//                        example_publish(pclient);
//                    }
//
//                    IOT_MQTT_Yield(pclient, 200);
//
//                    loop_cnt += 1;
//                }

    }

int ali_serial(void)
{
    int ret =0;
    rt_thread_t thread = rt_thread_create("ali", ali_thread_entry, RT_NULL, 2048, 26, 10);

    if (thread != RT_NULL)
        {
            rt_thread_startup(thread);
        }
        else
        {
            ret = RT_ERROR;
        }
        //rt_device_write(serial, 0, str, (sizeof(str) - 1));

        return ret;
}

INIT_APP_EXPORT(ali_serial);
