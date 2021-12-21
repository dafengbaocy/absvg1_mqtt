
#ifndef APPLICATIONS_PUT_H_
#define APPLICATIONS_PUT_H_
/* Standard includes. */
#include <stdio.h>
#define SAMPLE_UART_NAME "uart1"
#define AT_MQTT_CMD_MAX_LEN             400
static int flag_demo =0;

struct at_mqtt_input {
    char     *topic;
    uint32_t  topic_len;
    char     *message;
    uint32_t  msg_len;
};
/* rt-thread includes. */
#include <rtdevice.h>
#include <board.h>
#include <mqtt_api.h>



#define METHOD "thing.event.property.post"
#define ID      "1142523359"

#define PARAMS_VAL    1
#define pubback    ">"





void uart_init();
int HAL_AT_MQTT_Init(iotx_mqtt_param_t *pInitParams);
int HAL_AT_MQTT_Connect(char *proKey, char *devName, char *devSecret);
static int at_sendto_lower(rt_device_t *uart, void *data, uint32_t size,
                           uint32_t timeout, bool ackreq);
int at_sim800_mqtt_client_publish(const char *topic, int qos, int length);
int at_sim800_mqtt_client_subscribe(const char *topic,
                                 int qos,
                                 int timeout_ms);
int at_ec20_mqtt_client_publish(const char *topic, int qos, int length,int key_demo,rt_uint32_t vol);




#endif /* APPLICATIONS_PUT_H_ */
