#include <aty.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "infra_config.h"
#include "mqtt_api.h"
#include "put.h"
#include "at_wrapper.h"
#include "at_parser.h"
#include "at_api.h"
#include "at_mqtt.h"
#include "aiot_mqtt_sign.h"
#include <rtthread.h>
//#include "aty.h"
#define AT_ec20_MQTT_IPCONN          "AT+SAPBR"
#define AT_ec20_MQTT_MQTTCONF        "AT+QMTCFG"//MQTT配置
#define AT_ec20_MQTT_MQTTOPEN        "AT+QMTOPEN"//打开

#define CLIENT_ID                       "test"
#define PASSWARD                       "08C2F223CD2E3687E89C0470AD5BF8ED03AB28BDB46C1AB51AE6A35FE6A7890D"



#define AT_ec20_MQTT_MQTTCONN        "AT+QMTCONN"
#define AT_ec20_MQTT_MQTTSUB         "AT+QMTSUB"
#define AT_ec20_MQTT_MQTTUNSUB       "AT+SMUNSUB"
#define AT_ec20_MQTT_MQTTPUB         "AT+QMTPUBEX"
#define AT_ec20_MQTT_MQTTSTATE       "AT+QMTCONN?"
#define AT_ec20_MQTT_MQTTDISCONN     "AT+SMDISC"
#define AT_ec20_MQTT_MQTTSSL         "AT+SMSSL"

#define AT_ec20_MQTT_MQTTRCV         "\r\n+QM"
#define AT_ec20_MQTT_MQTTERROR       "\r\nERROR\r\n"
#define AT_ec20_MQTT_MQTTSAPBR       "\r\nOK\r\n\r\n+QMTPUBEX:"
#define AT_ec20_MQTT_MQTTOK          "\r\nOK\r\n"
#define AT_ec20_MQTT_MQTTRCVPUB      "+QMTRECV:"
#define AT_ec20_MQTT_MQTTPUBRSP      "+SMPUB"
#define AT_ec20_MQTT_MQTTSUBRSP      "+QMTSUB"
#define AT_ec20_MQTT_MQTTUNSUBRSP    "+SMUNSUB"
#define AT_ec20_MQTT_MQTTSTATERSP    "\r\n+QMTCONN"

#define AT_ec20_MQTT_POSTFIX         "\r\n"

/* change AT_MQTT_CMD_MAX_LEN from 1024 to 300 */

#define AT_MQTT_CMD_DEF_LEN             256
#define AT_MQTT_CMD_MIN_LEN             256
#define AT_MQTT_CMD_SUCCESS_RSP         "\r\nOK\r\n"
#define AT_MQTT_CMD_FAIL_RSP            "+FAIL"
#define AT_MQTT_CMD_ERROR_RSP           "ERROR"

#define AT_MQTT_SUBSCRIBE_FAIL          128
/* change AT_MQTT_RSP_MAX_LEN from 1500 to 300 */
#define AT_MQTT_RSP_MAX_LEN             (CONFIG_MQTT_MESSAGE_MAXLEN + CONFIG_MQTT_TOPIC_MAXLEN + 20)
#define AT_MQTT_RSP_MIN_LEN             64

#define AT_MQTT_WAIT_MAX_TIMEOUT            60*1000
#define AT_MQTT_WAIT_DEF_TIMEOUT            3*1000
#define AT_MQTT_WAIT_MIN_TIMEOUT            800

#define AT_CMD_SIM_PIN_CHECK            "AT+CPIN?"
#define AT_CMD_SIGNAL_QUALITY_CHECK     "AT+CSQ"
#define AT_CMD_NETWORK_REG_CHECK        "AT+CREG?"
#define AT_CMD_GPRS_ATTACH_CHECK        "AT+CGATT?"

#define AT_CMD_ECHO_OFF   "ATE0\r"
#define AT_CMD_ECHO_OFF_save "AT&W\r"
#define URL                 "iot-as-mqtt.cn-shanghai.aliyuncs.com"

#define ec20_RETRY_MAX          80

#define Topic1  "/sys/a1ehllrd5dK/device1/thing/event/LowBatteryAlarm/post"
#define METHOD "thing.event.property.post"
#define WARNMETHOD "thing.event.LowBatteryAlarm.post"
#define ID      "11"
#define PARAMS  "LockSwitch"
#define PARAMS1  "BatteryPercentage"
#define PARAMS_VAL    1
#define pubback    ">"
#define Identifier "LowBatteryAlarm"

#define ProKey  "a1ehllrd5dK"
#define DevName "device1"
#define DevSecret "21040f3f049c02ebbe07b469000e023b"


#define AT_DEBUG_MODE
#ifdef AT_DEBUG_MODE
#define mal_err(...)                do{HAL_Printf(__VA_ARGS__);HAL_Printf("\r\n");}while(0)
#define mal_info(...)               do{HAL_Printf(__VA_ARGS__);HAL_Printf("\r\n");}while(0)
#else
#define mal_err(...)
#define mal_info(...)
#endif

#ifdef INFRA_MEM_STATS
    #include "infra_mem_stats.h"
    #define AT_MQTT_ec20_MALLOC(size)            LITE_malloc(size, MEM_MAGIC, "mal.ica")
    #define AT_MQTT_ec20_FREE(ptr)               LITE_free(ptr)
#else
    #define AT_MQTT_ec20_MALLOC(size)            HAL_Malloc(size)
    #define AT_MQTT_ec20_FREE(ptr)               {HAL_Free((void *)ptr);ptr = NULL;}
#endif

char   at_max_len_cmd[AT_MQTT_CMD_MAX_LEN];
char   at_def_len_cmd[AT_MQTT_CMD_DEF_LEN];
char   at_min_len_cmd[AT_MQTT_CMD_MIN_LEN];
char    recv_buf[AT_MQTT_CMD_MAX_LEN];
//char   at_recv_rsp_buf[AT_MQTT_CMD_MAX_LEN];

iotx_mqtt_param_t g_pInitParams = {0};
int               g_isInitParamsSet = 0;

typedef enum {
    AT_MQTT_IDLE = 0,
    AT_MQTT_SEND_TYPE_SIMPLE,
    AT_MQTT_AUTH,
    AT_MQTT_SUB,
    AT_MQTT_UNSUB,
    AT_MQTT_PUB,
} at_mqtt_send_type_t;
int at_ec20_mqtt_atsend(char *at_cmd, int timeout_ms);
int at_ec20_mqtt_client_deinit(void);
int at_ec20_mqtt_client_init(iotx_mqtt_param_t * pInitParams);
int at_ec20_mqtt_client_state(void);
int at_ec20_mqtt_client_publish(const char *topic, int qos, int length,int key_demo,rt_uint32_t vol);
int at_ec20_mqtt_client_unsubscribe(const char *topic,
                                   unsigned int *mqtt_packet_id,
                                   int *mqtt_status);
int at_ec20_mqtt_client_subscribe(const char *topic,
                                 int qos,

                                 int timeout_ms);
int at_ec20_mqtt_client_conn(void);
int at_ec20_mqtt_client_ssl(int tlsEnable);
int at_ec20_mqtt_client_disconn(void);
void at_ec20_mqtt_client_reconn(void);
int HAL_AT_MQTT_Init(iotx_mqtt_param_t *pInitParams)
{
    return at_ec20_mqtt_client_init(pInitParams);
}

int HAL_AT_MQTT_Deinit()
{
    return at_ec20_mqtt_client_deinit();
}
int HAL_AT_MQTT_Connect(char *proKey, char *devName, char *devSecret)
{
    /* ec20 module doesn't use Ali ICA MQTT. So don't need to
     use prokey, devName, devSecret. Use username and password instead
     which is assined in the init params. */
    (void) proKey;
    (void) devName;
    (void) devSecret;

    if(g_isInitParamsSet != 0) {
        return at_ec20_mqtt_client_conn();
    }
    else {
        mal_err("HAL_AT_MQTT_Connect failed, because init params are not configured.");
        return -1;
    }
}

struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;  /* 初始化配置参数 */
static rt_device_t serial;



#define SAPBR_STATUS_CONNECTING        0
#define SAPBR_STATUS_CONNECTED         1
#define SAPBR_STATUS_CLOSING           2
#define SAPBR_STATUS_CLOSED            3
#define SAPBR_STATUS_INVALID           4

#ifndef PLATFORM_HAS_OS
char recv_buf[AT_MQTT_RSP_MAX_LEN];
#else
//static char              *recv_buf = NULL;
#endif
static volatile int       g_mqtt_connect_state = IOTX_MC_STATE_INVALID;
static volatile int       g_sapbr_status = SAPBR_STATUS_INVALID;
static volatile at_mqtt_send_type_t   g_ec20_at_response = AT_MQTT_IDLE;
static volatile int       g_at_response_result = 0;
#ifdef PLATFORM_HAS_OS
void* g_sem_response;
#endif
static volatile int       g_response_msg_number = 0;
static int                g_response_packetid = 0;
static int                g_response_status = 0;
static int                g_public_qos = 0;

int at_ec20_mqtt_atsend(char *at_cmd, int timeout_ms);
static struct rt_semaphore rx_sem;
//static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
//{
//    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
//    rt_sem_release(&rx_sem);
//    //rt_device_write(serial, 0, str1, (sizeof(str1) - 1));
//
//    return RT_EOK;
//}

static void serial_thread_entry(void *parameter)
{
    char ch;
    //rt_device_write(serial, 0, str, (sizeof(str) - 1));
        //rt_device_write(serial, 0, str1, (sizeof(str1) - 1));
    while (1)
    {
        /* 从串口读取一个字节的数据，没有读取到则等待接收信号量 */
        while (rt_device_read(serial, -1, &ch, 1) != 1)
        {
            /* 阻塞等待接收信号量，等到信号量后再次读取数据 */
            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        }
        /* 读取到的数据输出 */
        //rt_device_write(serial, 0, &ch, 1);
        rt_kprintf("%c",ch);
    }
}
int thread_serial(void)
{
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX];
    char str[]="AT\r\n";
    char str1[] = "AT+IPR?\r";


    Led.LED_R = rt_pin_get("PE.1");
     Led.LED_G = rt_pin_get("PE.4");
     io.IO_CLOSE = rt_pin_get("PA.5");
     io.IO_OPEN = rt_pin_get("PF.1");

     rt_pin_mode(Led.LED_R, PIN_MODE_OUTPUT);
         rt_pin_mode(Led.LED_G, PIN_MODE_OUTPUT);
         rt_pin_mode(io.IO_CLOSE, PIN_MODE_OUTPUT);
             rt_pin_mode(io.IO_OPEN, PIN_MODE_OUTPUT);


             rt_pin_write(io.IO_OPEN, PIN_HIGH);
                                 rt_pin_write(Led.LED_G, PIN_HIGH);
                                 rt_pin_write(Led.LED_R, PIN_LOW);
                                 rt_pin_write(io.IO_CLOSE, PIN_LOW);


    rt_strncpy(uart_name, SAMPLE_UART_NAME, RT_NAME_MAX);

    /* 查找系统中的串口设备 */
    serial = rt_device_find(uart_name);
    if (!serial)
    {
        rt_kprintf("find %s failed!\n", uart_name);
        return RT_ERROR;
    }
    /* 修改串口配置参数 */
    config.baud_rate = BAUD_RATE_115200;        //修改波特率为 9600
    config.data_bits = DATA_BITS_8;           //数据位 8
    config.stop_bits = STOP_BITS_1;           //停止位 1
    config.bufsz     = 128;                   //修改缓冲区 buff size 为 128
    config.parity    = PARITY_NONE;           //无奇偶校验位

    /* 控制串口设备。通过控制接口传入命令控制字，与控制参数 */
    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);

    /* 初始化信号量 */
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
    /* 以中断接收及轮询发送模式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
    /* 设置接收回调函数 */
//    rt_device_set_rx_indicate(serial, uart_input);
    /* 发送字符串 */
//    rt_device_write(serial, 0, str, (sizeof(str) - 1));
//        rt_thread_mdelay(1000);
//        rt_device_write(serial, 0, str1, (sizeof(str1) - 1));

    //rt_device_write(serial, 0, str2, (sizeof(str2) - 1));
    /* 创建 serial 线程 */
//    rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024, 25, 10);
//    /* 创建成功则启动线程 */
//    if (thread != RT_NULL)
//    {
//        rt_thread_startup(thread);
//    }
//    else
//    {
//        ret = RT_ERROR;
//    }
    //rt_device_write(serial, 0, str, (sizeof(str) - 1));

    return ret;
}



//错误回调
static void recv_sapbr_callback(char *at_rsp)
{
    char *temp;

    g_at_response_result = -1;

    if (NULL == at_rsp) {
        return;
    }

    mal_info("recv sapbr at_rsp =%s", at_rsp);

    temp = strtok(at_rsp, "}");

    if (temp != NULL) {
        temp = strtok(NULL, ":");
        temp = strtok(NULL,",");
        temp = strtok(NULL,",");
        temp = strtok(NULL,"\r");
        if (temp != NULL) {
            int state = strtol(temp, NULL, 0);
            switch(state) {
                case SAPBR_STATUS_CONNECTING:
                rt_kprintf("pub success!\n");
                case SAPBR_STATUS_CONNECTED:
                case SAPBR_STATUS_CLOSING:
                case SAPBR_STATUS_CLOSED:
                    g_sapbr_status = state;
                    mal_info("g_sapbr_status =%d", g_sapbr_status);
                    g_at_response_result = 0;
                    break;
                default:
                    g_sapbr_status = SAPBR_STATUS_INVALID;
                    break;
            }
        }
    }
#ifdef PLATFORM_HAS_OS
    /* notify the sender error; */
    HAL_SemaphorePost(g_sem_response);
#endif
    return ;
}


static void at_err_callback(char *at_rsp)
    {
        char *temp;

        temp            = strtok(at_rsp, ":");
        temp            = strtok(NULL, ":");
        if ((strtol(temp, NULL, 0)) == 3) {
            g_at_response_result = 0;
        } else {
            g_at_response_result = -1;
        }

    #ifdef PLATFORM_HAS_OS
        /* notify the sender error; */
        HAL_SemaphorePost(g_sem_response);
    #endif
        return;
    }
static void at_succ_callback(void)
{
    g_at_response_result = 0;
#ifdef PLATFORM_HAS_OS
    /* notify the sender ok; */
    HAL_SemaphorePost(g_sem_response);
#endif
    return;
}
static void sub_callback(char *at_rsp)
{
    char  *temp;

    if (strstr(at_rsp, AT_MQTT_CMD_ERROR_RSP)) {
        g_at_response_result = -1;

#ifdef PLATFORM_HAS_OS
        /* notify the sender fail; */
        HAL_SemaphorePost(g_sem_response);
#endif
        return;
    } else if (NULL != strstr(at_rsp, AT_ec20_MQTT_MQTTSUBRSP)) {
        /* get status/packet_id */
        if (NULL != strstr(at_rsp, ",")) {
            g_at_response_result = 0;

            temp            = strtok(at_rsp, ":");

            if (temp != NULL) {
                temp            = strtok(NULL, ",");
                temp            = strtok(NULL, ",");
                temp            = strtok(NULL, ",");
                if ((strtol(temp, NULL, 0)) == 0) {
                            g_at_response_result = 0;
                            rt_kprintf("sub success!");
                            g_ec20_at_response =AT_MQTT_IDLE;
                        } else {
                            g_at_response_result = -1;
                        }
            } else {
                mal_err("subscribe rsp param invalid 1");
                g_at_response_result = -1;

#ifdef PLATFORM_HAS_OS
                HAL_SemaphorePost(g_sem_response);
#endif
                return;
            }

            if (temp != NULL) {
                g_response_packetid = strtol(temp, NULL, 0);

                temp            = strtok(NULL, "\r\n");
            } else {
                mal_err("subscribe rsp param invalid 2");
                g_at_response_result = -1;

#ifdef PLATFORM_HAS_OS
                HAL_SemaphorePost(g_sem_response);
#endif
                return;
            }

            if (temp != NULL) {
                g_response_status = strtol(temp, NULL, 0);
            } else {
                mal_err("subscribe rsp param invalid 3");
                g_at_response_result = -1;

#ifdef PLATFORM_HAS_OS
                HAL_SemaphorePost(g_sem_response);
#endif
                return;
            }

#ifdef PLATFORM_HAS_OS
            /* notify the sender ok; */
            HAL_SemaphorePost(g_sem_response);
#endif
        }
    }

    return;
}
static void unsub_callback(char *at_rsp)
{
    char  *temp;
    if (strstr(at_rsp, AT_MQTT_CMD_ERROR_RSP)) {
        g_at_response_result = -1;

#ifdef PLATFORM_HAS_OS
        /* notify the sender fail; */
        HAL_SemaphorePost(g_sem_response);
#endif

        return;
    } else if (NULL != strstr(at_rsp, AT_ec20_MQTT_MQTTUNSUBRSP)) {
        /* get status/packet_id */
        if (NULL != strstr(at_rsp, ",")) {
            g_at_response_result = 0;

            temp            = strtok(at_rsp, ":");

            if (temp != NULL) {
                temp            = strtok(NULL, ",");
            } else {
                mal_err("subscribe rsp param invalid 1");

                return;
            }

            if (temp != NULL) {
                g_response_packetid = strtol(temp, NULL, 0);

                temp            = strtok(NULL, "\r\n");
            } else {
                mal_err("subscribe rsp param invalid 2");

                return;
            }

            if (temp != NULL) {
                g_response_status = strtol(temp, NULL, 0);
            } else {
                mal_err("subscribe rsp param invalid 3");

                return;
            }

            mal_err("unsub: %s", recv_buf);
            mal_err("unsub packetid: %d, sta: %d", g_response_packetid, g_response_status);

#ifdef PLATFORM_HAS_OS
            /* notify the sender ok; */
            HAL_SemaphorePost(g_sem_response);
#endif
        }
    }

    return;
}
static void pub_callback(char *at_rsp)
{
    char  *temp;
    if (strstr(at_rsp, AT_MQTT_CMD_ERROR_RSP)) {
        g_at_response_result = -1;

#ifdef PLATFORM_HAS_OS
        /* notify the sender fail; */
        HAL_SemaphorePost(g_sem_response);
#endif

        return;
    } else if (NULL != strstr(at_rsp, pubback)) {
        /* get status/packet_id */

                   rt_kprintf("pub callback 1\n");
                   g_at_response_result = 0;
                   HAL_SemaphorePost(g_sem_response);
                return;
            }






            g_at_response_result = 0;

#ifdef PLATFORM_HAS_OS
            /* notify the sender ok; */
            HAL_SemaphorePost(g_sem_response);
#endif



    return;
}


static void state_change_callback(char *at_rsp)
{
    char *temp;

    if (NULL == at_rsp) {
        return;
    }
    temp = strtok(at_rsp, ",");

    if (temp != NULL) {
        temp = strtok(NULL, "\r\n");

        if (temp != NULL) {
            int state = strtol(temp, NULL, 0);
            rt_kprintf("%d",state);
            switch(state) {
                /* disconnect */
                case 4:
                    g_mqtt_connect_state = IOTX_MC_STATE_DISCONNECTED;
                    break;
                case 2:
                     g_mqtt_connect_state = IOTX_MC_STATE_DISCONNECTED;
                                    break;
                case 1:
                                    g_mqtt_connect_state = IOTX_MC_STATE_DISCONNECTED;
                                    break;
                /* connected */
                case 3:
                    g_mqtt_connect_state = IOTX_MC_STATE_CONNECTED;
                    g_at_response_result=0;
                    rt_kprintf("connected success!\n");
                    break;
                /* invalid */
                default:
                    g_mqtt_connect_state = IOTX_MC_STATE_INVALID;
                    break;
            }
#ifdef PLATFORM_HAS_OS
        /* notify the sender error; */
        HAL_SemaphorePost(g_sem_response);
    #endif

        }
    }
    return;
}

static void recv_data_callback(char *at_rsp)
{
    char     *temp = NULL;
    char     *topic_ptr = NULL;
    char     *msg_ptr = NULL;
    unsigned int  msg_len = 0;
    struct at_mqtt_input param;

    if (NULL == at_rsp) {
        return;
    }

    /* try to get msg id */
    temp = strtok(recv_buf, ":");

    if (temp != NULL) {
        temp  = strtok(NULL, ",");

        if (temp != NULL) {
            /* packet_id = strtol(temp, NULL, 0); */
        } else {
            mal_err("packet id error");

            return;
        }
    } else {
        mal_err("packet id not found");

        return;
    }

    /* try to get topic string */
    temp = strtok(NULL, "\"");

    if (temp != NULL) {
        temp[strlen(temp)] = '\0';

        topic_ptr = temp;
    } else {
        mal_err("publish topic not found");

        return;
    }

    /* try to get payload string */
    temp = strtok(NULL, ",");

    if (temp != NULL) {
        msg_len = strtol(temp, NULL, 0);

        while (*temp++ != '\"');

        msg_ptr = temp;

        msg_ptr[msg_len] = '\0';

        param.topic = topic_ptr;
        param.topic_len = strlen(topic_ptr);
        param.message = msg_ptr;
        param.msg_len = strlen(msg_ptr);

        if (IOT_ATM_Input(&param) != 0) {
            mal_err("hand data to uplayer fail!\n");
        }

        return;
    } else {
        mal_err("publish data not found");

        return;
    }


}
static void at_ec20_mqtt_client_rsp_callback(void *arg, char *rspinfo, int rsplen)
{

    if (NULL == rspinfo || rsplen == 0) {
        mal_err("invalid input of rsp callback");
        return;
    }

#ifdef PLATFORM_HAS_OS
    if (NULL == recv_buf) {
        mal_err("recv_buf rsp is NULL");
        return;

    }
#endif
    if (rsplen > AT_MQTT_RSP_MAX_LEN) {
                    mal_err("rsp len(%d) exceed max len", rsplen);
                    return;
                }
    memcpy(recv_buf, rspinfo, rsplen);
        recv_buf[rsplen] = '\0';

        mal_err("rsp_buff=%s", recv_buf);

        if (NULL != strstr(recv_buf,
                        AT_ec20_MQTT_MQTTSAPBR
                        )){
            recv_sapbr_callback(recv_buf);
        } else if (0 == memcmp(recv_buf,
                            AT_ec20_MQTT_MQTTERROR,
                            strlen(AT_ec20_MQTT_MQTTERROR))) {

                at_err_callback(recv_buf);
            } else if (NULL != strstr(recv_buf,
                                   AT_ec20_MQTT_MQTTRCVPUB
                                   )) { /* Receive Publish Data */

                recv_data_callback(recv_buf);
            } else if (0 == memcmp(recv_buf,
                                   AT_ec20_MQTT_MQTTSTATERSP,
                                   strlen(AT_ec20_MQTT_MQTTSTATERSP))) {  /* Receive Mqtt Status Change */

                state_change_callback(recv_buf);
            } else {
                switch (g_ec20_at_response) {  /* nothing to process */

                            case AT_MQTT_IDLE:

                                break;

                            case AT_MQTT_SEND_TYPE_SIMPLE:

                                if (0 == memcmp(recv_buf,
                                                AT_MQTT_CMD_SUCCESS_RSP,
                                                strlen(AT_MQTT_CMD_SUCCESS_RSP))) {

                                    at_succ_callback();
                                } else {

                                    mal_err("invalid success type %s", recv_buf);
                                }

                                break;

                            case AT_MQTT_AUTH:
                                /* ec20 is not support Ali ICA MQTT, so should not reach here. */
                                break;

                            case AT_MQTT_SUB:
                                sub_callback(recv_buf);
                                break;

                            case AT_MQTT_UNSUB:
                                unsub_callback(recv_buf);
                                break;

                            case AT_MQTT_PUB:
                                pub_callback(recv_buf);
                                break;

                            default:
                                break;

                        }
                    }

                    return;
                }
int at_ec20_mqtt_client_disconn(void)
{

    memset(at_min_len_cmd, 0, sizeof(at_min_len_cmd));

    /* connect to the network */
    HAL_Snprintf(at_min_len_cmd,
                 sizeof(at_min_len_cmd),
                 "%s\r\n",
                 AT_ec20_MQTT_MQTTDISCONN);

    /* disconnect from server */
    if (0 != at_ec20_mqtt_atsend(at_min_len_cmd, AT_MQTT_WAIT_DEF_TIMEOUT)) {
        mal_err("disconnect at command fail");

        return -1;
    }

    return 0;
}

int at_ec20_mqtt_client_ssl(int tlsEnable)
{
    /* set tls mode before auth */
    if (tlsEnable) {
        memset(at_min_len_cmd, 0, sizeof(at_min_len_cmd));

        /* 1 enable tls, 0 disable tls */
        HAL_Snprintf(at_min_len_cmd,
                     sizeof(at_min_len_cmd) - 1,
                     "%s=%d\r\n",
                     AT_ec20_MQTT_MQTTSSL,
                     1);

        if (0 != at_ec20_mqtt_atsend(at_min_len_cmd, AT_MQTT_WAIT_MIN_TIMEOUT)) {

            mal_err("tls at command fail");

            return -1;
        }
    }

    return 0;
}


int at_ec20_mqtt_client_conn()
{
    int   retry = 0;

    /* not enable ssl */
    int   tlsEnable = 0;

    if(g_isInitParamsSet == 0) {
        mal_err("at_ec20_mqtt_client_conn init parms is not configured.");
        return -1;
    }

    if (0 != at_ec20_mqtt_client_ssl(tlsEnable)) {
        mal_err("tls enable fail");
        return -1;
    }

    /* set mqtt server */
    memset(at_def_len_cmd, 0, sizeof(at_def_len_cmd));

    HAL_Snprintf(at_def_len_cmd,
                 sizeof(at_def_len_cmd),
                 "%s=\"%s\",%d,%d\r\n",
                 AT_ec20_MQTT_MQTTCONF, "KEEPALIVE", 0, 120);

    if (0 != at_ec20_mqtt_atsend(at_def_len_cmd, AT_MQTT_WAIT_DEF_TIMEOUT)) {

        mal_err("conn at command fail");
        at_ec20_mqtt_client_state();

        return -1;
    }

    /* clean mqtt session */
    memset(at_min_len_cmd, 0, sizeof(at_min_len_cmd));

    HAL_Snprintf(at_min_len_cmd,
                 sizeof(at_min_len_cmd),
                 "%s=\"%s\",%d,\"%s\",\"%s\",\"%s\"\r\n",
                 AT_ec20_MQTT_MQTTCONF, "ALIAUTH", 0,ProKey,DevName,DevSecret);

    if (0 != at_ec20_mqtt_atsend(at_min_len_cmd, AT_MQTT_WAIT_MIN_TIMEOUT)) {

        mal_err("conn at command fail");
        at_ec20_mqtt_client_state();
        return -1;
    }

//    do{
//        retry ++;
//        mal_err( "%s %d try to turn off echo: %s %d times\r\n", __func__, __LINE__, at_min_len_cmd, retry);
//        at_send_wait_reply(AT_CMD_ECHO_OFF, strlen(AT_CMD_ECHO_OFF), true, NULL, 0,
//                           at_min_len_cmd, sizeof(at_min_len_cmd), NULL);
//    }while((strstr(at_min_len_cmd, AT_MQTT_CMD_SUCCESS_RSP) == NULL) && (retry < ec20_RETRY_MAX));
//    if(retry == ec20_RETRY_MAX)
//    {
//       mal_err("try to turn off echo failed");
//    }
    /* set username */
    memset(at_def_len_cmd, 0, sizeof(at_def_len_cmd));

    HAL_Snprintf(at_def_len_cmd,
                 sizeof(at_def_len_cmd),
                 "%s=%d,\"%s\",%d\r\n",
                 AT_ec20_MQTT_MQTTOPEN, 0, URL,1883);

    if (0 != at_ec20_mqtt_atsend(at_def_len_cmd, AT_MQTT_WAIT_MIN_TIMEOUT)) {

        mal_err("set username at command fail");
        at_ec20_mqtt_client_state();
        return -1;
    }

    /* set password */
    memset(at_def_len_cmd, 0, sizeof(at_def_len_cmd));

    HAL_Snprintf(at_def_len_cmd,
                 sizeof(at_def_len_cmd),
                 "%s=%d,\"%s\",\"%s\",\"%s\"\r\n",
                 AT_ec20_MQTT_MQTTCONN, 0,CLIENT_ID, g_pInitParams.username,g_pInitParams.password);

    if (0 != at_ec20_mqtt_atsend(at_def_len_cmd, AT_MQTT_WAIT_MIN_TIMEOUT)) {

        mal_err("set password at command fail");
        at_ec20_mqtt_client_state();
        return -1;
    }

    /* set clientid */
//    memset(at_def_len_cmd, 0, sizeof(at_def_len_cmd));
//
//    HAL_Snprintf(at_def_len_cmd,
//                 sizeof(at_def_len_cmd),
//                 "%s=\"%s\",\"%s\"\r\n",
//                 AT_ec20_MQTT_MQTTCONF, "CLIENTID", g_pInitParams.username);

//    if (0 != at_ec20_mqtt_atsend(at_def_len_cmd, AT_MQTT_WAIT_MIN_TIMEOUT)) {
//
//        mal_err("set clientid at command fail");
//
//        return -1;
//    }

    /* set timeout */
//    memset(at_min_len_cmd, 0, sizeof(at_min_len_cmd));
//
//    HAL_Snprintf(at_min_len_cmd,
//                 sizeof(at_min_len_cmd),
//                 "%s=\"%s\",\"%d\"\r\n",
//                 AT_ec20_MQTT_MQTTCONF, "TIMEOUT", AT_MQTT_WAIT_MAX_TIMEOUT/1000);
//
//    if (0 != at_ec20_mqtt_atsend(at_min_len_cmd, AT_MQTT_WAIT_MIN_TIMEOUT)) {
//
//        mal_err("set timeout at command fail");
//        return -1;
//    }

//    /* start to connect mqtt server */
//    memset(at_min_len_cmd, 0, sizeof(at_min_len_cmd));
//    HAL_Snprintf(at_min_len_cmd,
//              sizeof(at_min_len_cmd),
//              "%s\r\n",
//              AT_ec20_MQTT_MQTTCONN);
//
//    if (0 != at_ec20_mqtt_atsend(at_min_len_cmd, AT_MQTT_WAIT_DEF_TIMEOUT)) {
//        mal_err("conn at command fail");
//        return -1;
//    }

    retry = 0;
    while((g_mqtt_connect_state != IOTX_MC_STATE_CONNECTED) && (retry < ec20_RETRY_MAX)) {
        retry ++;
        at_ec20_mqtt_client_state();
        mal_info("try to wait mqtt server ... mstate %d  retry %d", g_mqtt_connect_state, retry);
    }

    if(retry == ec20_RETRY_MAX) {
       mal_err("conn at command wait timeout");
       return -1;
    }

    return 0;
}


//重新连接
void at_ec20_mqtt_client_reconn()
{
    int   ret;

    mal_info( "%s %d \r\n", __func__, __LINE__);

    /* check pdp status */


        /* start to connect mqtt server */
        at_ec20_mqtt_client_conn();


    return ;
}
int g_mqtt_client_count = 0;

//订阅
int at_ec20_mqtt_client_subscribe(const char *topic,
                                 int qos,
                                 int timeout_ms)
{

//    if ((topic == NULL) || (mqtt_packet_id == NULL) || (mqtt_status == NULL)) {
//        mal_err("subscribe param should not be NULL");
//        return -1;
//    }

    memset(at_min_len_cmd, 0, sizeof(at_min_len_cmd));

    HAL_Snprintf(at_min_len_cmd,
                 sizeof(at_min_len_cmd) - 1,
                 "%s=%d,%d,\"%s\",%d\r\n",
                 AT_ec20_MQTT_MQTTSUB,
                 0,
                 1,
                 topic,
                 qos);

    if (0 != at_ec20_mqtt_atsend(at_min_len_cmd, timeout_ms)) {
        mal_err("sub at command fail");

        return -1;
    }

    return 0;
}
//取消订阅
int at_ec20_mqtt_client_unsubscribe(const char *topic,
                                   unsigned int *mqtt_packet_id,
                                   int *mqtt_status)
{

    if ((topic == NULL) || (mqtt_packet_id == NULL) || (mqtt_status == NULL)) {
        mal_err("unsubscribe param should not be NULL");
        return -1;

    }

    memset(at_min_len_cmd, 0, sizeof(at_min_len_cmd));

    HAL_Snprintf(at_min_len_cmd,
                 sizeof(at_min_len_cmd) - 1,
                 "%s=\"%s\"\r\n",
                 AT_ec20_MQTT_MQTTUNSUB,
                 topic);

    if (0 != at_ec20_mqtt_atsend(at_min_len_cmd, AT_MQTT_WAIT_MIN_TIMEOUT)) {
        mal_err("unsub at command fail");
        return -1;
    }

    return 0;
}
//上报
int at_ec20_mqtt_client_publish(const char *topic, int qos, int length,int key_demo,rt_uint32_t vol)
{

    rt_kprintf("let's go\n");
    char   *temp;
    HAL_Snprintf(at_max_len_cmd,
                     sizeof(at_max_len_cmd) - 1,
                     "%s=%d,%d,%d,%d,\"%s\",%d\r\n",

                     AT_ec20_MQTT_MQTTPUB,
                     0,
                     0,
                     qos,
                     1,
                     topic,
                    71+sizeof(ID)+sizeof(PARAMS)+sizeof(KEY_DEMO)+sizeof(METHOD)+sizeof(PARAMS1)+sizeof(vol)
                     );

        g_public_qos = qos;

        if (0 != at_ec20_mqtt_atsend(at_max_len_cmd, AT_MQTT_WAIT_MIN_TIMEOUT)) {
                mal_err("pub at command fail");
                return -1;
            }


        memset(at_min_len_cmd,0,sizeof(at_min_len_cmd));
            HAL_Snprintf(at_min_len_cmd,
                    length,
                           "{\"id\":\"%s\",\"version\":\"1.0.0\",\"params\":{\"%s\":{\"value\":%d},",

                           ID,
                           PARAMS,
                           key_demo
                           );
            rt_kprintf("message:%s",at_min_len_cmd);
                if (0 != at_ec20_mqtt_atsend(at_min_len_cmd, 200)) {
                    mal_err("pub at command fail");

                }

            memset(at_min_len_cmd,0,sizeof(at_min_len_cmd));
                        HAL_Snprintf(at_min_len_cmd,
                                length,
                                "\"%s\":{\"value\":%d.%02d}},\"method\":\"%s\"}",
                                PARAMS1,vol % 100, vol /100,METHOD
                                   );
                        rt_kprintf("%s\n",at_min_len_cmd);


    if (0 != at_ec20_mqtt_atsend(at_min_len_cmd, AT_MQTT_WAIT_MIN_TIMEOUT)) {
        mal_err("pub at command fail");
        return -1;
    }
    if(vol<4)
    {
//        at_ec20_mqtt_client_subscribe(    Topic1,
//                                          qos,
//                                          AT_MQTT_WAIT_MIN_TIMEOUT);
        memset(at_max_len_cmd,0,sizeof(at_max_len_cmd));
        HAL_Snprintf(at_max_len_cmd,
                             sizeof(at_max_len_cmd) - 1,
                             "%s=%d,%d,%d,%d,\"%s\",%d\r\n",

                             AT_ec20_MQTT_MQTTPUB,
                             0,
                             0,
                             qos,
                             1,
                             Topic1,
                            66+sizeof(ID)+sizeof(PARAMS)+sizeof(KEY_DEMO)+sizeof(WARNMETHOD)
                             );//31
        g_public_qos = qos;

                if (0 != at_ec20_mqtt_atsend(at_max_len_cmd, AT_MQTT_WAIT_MIN_TIMEOUT)) {
                        mal_err("pub at command fail");
                        return -1;
                    }
                rt_thread_mdelay(2000);

                memset(at_min_len_cmd,0,sizeof(at_min_len_cmd));
                            HAL_Snprintf(at_min_len_cmd,
                                    length,
                                           "{\"id\":\"%s\",\"version\":\"1.0.0\",\"params\":{\"value\":{\"%s\":%d}},\"method\":\"%s\"}",

                                           //Identifier,\"identifier\":\"%s\",
                                           ID,
                                           PARAMS1,
                                           10,
                                           WARNMETHOD
                                           );
                            rt_kprintf("message:%s",at_min_len_cmd);
                                            if (0 != at_ec20_mqtt_atsend(at_min_len_cmd, 200)) {
                                                mal_err("pub at command fail");

                                            }
    }
    flag_demo++;
    return 0;
}

int at_ec20_mqtt_client_state(void)
{
    int mode;
    int retry = 0;

    if((g_mqtt_connect_state == IOTX_MC_STATE_CONNECTED)) {
        mode = 32;
    } else {
        mode = 1;
    }

    g_mqtt_client_count ++;

    /* avoid sending too many state check request at commands */
    if(g_mqtt_client_count%mode == 0) {
        /* check mqtt state */
        memset(at_min_len_cmd, 0, sizeof(at_min_len_cmd));
        HAL_Snprintf(at_min_len_cmd,
             sizeof(at_min_len_cmd),
             "%s\r\n",
             AT_ec20_MQTT_MQTTSTATE);

        if (0 != at_ec20_mqtt_atsend(at_min_len_cmd, AT_MQTT_WAIT_MIN_TIMEOUT)) {
            mal_err("state at command fail");
            return -1;
        }
    }

    while((g_mqtt_connect_state != IOTX_MC_STATE_CONNECTED) &&( retry < ec20_RETRY_MAX)) {
       at_ec20_mqtt_client_reconn();
       retry ++;

       /* check mqtt state */
        memset(at_min_len_cmd, 0, sizeof(at_min_len_cmd));
        HAL_Snprintf(at_min_len_cmd,
                 sizeof(at_min_len_cmd),
                 "%s?\r\n",
                 AT_ec20_MQTT_MQTTSTATE);

        if (0 != at_ec20_mqtt_atsend(at_min_len_cmd, AT_MQTT_WAIT_DEF_TIMEOUT)) {
            mal_err("reconnect mqtt state at command fail");
        }
    }

    return (int)g_mqtt_connect_state;
}



int at_ec20_mqtt_client_init(iotx_mqtt_param_t * pInitParams)
{
    int ret;
        int retry = 0;
        //recv_buf = AT_MQTT_ec20_MALLOC(AT_MQTT_RSP_MAX_LEN);
            if (NULL == recv_buf) {
                mal_err("at ec20 mqtt client malloc buff failed");
                return -1;
            }

            if (NULL == (g_sem_response = HAL_SemaphoreCreate())) {
                if (NULL != recv_buf) {
                    //AT_MQTT_ec20_FREE(recv_buf);

                    //recv_buf = NULL;
                }
                mal_err("at ec20 mqtt client create sem failed");

                return -1;
            }


            g_mqtt_connect_state = IOTX_MC_STATE_INVALID;

                g_pInitParams.port = pInitParams->port;
                g_pInitParams.host = pInitParams->host;
                g_pInitParams.client_id = pInitParams->client_id;
                g_pInitParams.username = pInitParams->username;
                g_pInitParams.password = pInitParams->password;
                g_pInitParams.handle_event.pcontext = pInitParams->handle_event.pcontext;
                g_pInitParams.handle_event.h_fp = pInitParams->handle_event.h_fp;

                g_isInitParamsSet = 1;
                at_register_callback(AT_ec20_MQTT_MQTTRCV,
                                             AT_ec20_MQTT_POSTFIX,
                                             recv_buf,
                                             AT_MQTT_CMD_MAX_LEN,
                                             at_ec20_mqtt_client_rsp_callback,
                                             NULL);
                at_register_callback(AT_ec20_MQTT_MQTTERROR,
                                             AT_ec20_MQTT_POSTFIX,
                                             recv_buf,
                                             AT_MQTT_CMD_MIN_LEN,
                                             at_ec20_mqtt_client_rsp_callback,
                                             NULL);

                    at_register_callback(AT_ec20_MQTT_MQTTOK,
                                             AT_ec20_MQTT_POSTFIX,
                                             recv_buf,
                                             AT_MQTT_CMD_MIN_LEN,
                                             at_ec20_mqtt_client_rsp_callback,
                                             NULL);

                    at_register_callback(AT_ec20_MQTT_MQTTSAPBR,
                                             AT_ec20_MQTT_POSTFIX,
                                             recv_buf,
                                             AT_MQTT_CMD_DEF_LEN,
                                             at_ec20_mqtt_client_rsp_callback,
                                             NULL);
                    memset(at_min_len_cmd,0,sizeof(at_min_len_cmd));
                    rt_kprintf("ok,%s\n",at_min_len_cmd);
                    do{
                            retry ++;
                            mal_err( "%s %d try to turn off echo: %s %d times\r\n", __func__, __LINE__, at_min_len_cmd, retry);
                        at_send_wait_reply(AT_CMD_ECHO_OFF, strlen(AT_CMD_ECHO_OFF), true, NULL, 0,
                                           at_min_len_cmd, sizeof(at_min_len_cmd), NULL);
                        }while((strstr(at_min_len_cmd, AT_MQTT_CMD_SUCCESS_RSP) == NULL) && (retry < ec20_RETRY_MAX));

                        if(retry == ec20_RETRY_MAX)
                        {
                            mal_err("try to turn off echo failed");
                        }

                        memset(at_min_len_cmd,0,sizeof(at_min_len_cmd));
                        do{
                                                    retry ++;
                                                    mal_err( "%s %d try to turn off echo: %s %d times\r\n", __func__, __LINE__, at_min_len_cmd, retry);
                                                at_send_wait_reply(AT_CMD_ECHO_OFF_save, strlen(AT_CMD_ECHO_OFF_save), true, NULL, 0,
                                                                   at_min_len_cmd, sizeof(at_min_len_cmd), NULL);
                                                }while((strstr(at_min_len_cmd, AT_MQTT_CMD_SUCCESS_RSP) == NULL) && (retry < ec20_RETRY_MAX));

                                                if(retry == ec20_RETRY_MAX)
                                                {
                                                    mal_err("try to turn off echo failed");
                                                }
                        return 0;

}
//去初始化
int at_ec20_mqtt_client_deinit(void)
{
#ifdef PLATFORM_HAS_OS
    if (NULL != recv_buf) {
//        AT_MQTT_ec20_FREE(recv_buf);
//        recv_buf = NULL;
        memset(recv_buf, 0, sizeof(recv_buf));
    }
    HAL_SemaphoreDestroy(g_sem_response);
#else
    memset(recv_buf, 0, sizeof(recv_buf));
#endif

    g_isInitParamsSet = 0;

    g_mqtt_connect_state = IOTX_MC_STATE_INVALID;

    return 0;
}


int at_ec20_mqtt_atsend(char *at_cmd, int timeout_ms)
{
    if (at_cmd == NULL) {
        return -1;
    }
    /* state error */
    if (g_ec20_at_response != AT_MQTT_IDLE) {

        mal_err("at send state is not idle (%d)", g_ec20_at_response);

        return -1;
    }

    mal_err("send: %s", at_cmd);

    if (NULL != strstr(at_cmd, AT_ec20_MQTT_MQTTSUB)) {
        g_ec20_at_response = AT_MQTT_SUB;
    } else if (NULL != strstr(at_cmd, AT_ec20_MQTT_MQTTUNSUB)) {
        g_ec20_at_response = AT_MQTT_UNSUB;
    } else if (NULL != strstr(at_cmd, AT_ec20_MQTT_MQTTPUB)) {
        g_ec20_at_response = AT_MQTT_PUB;
    } else {
        g_ec20_at_response = AT_MQTT_SEND_TYPE_SIMPLE;
    }
            memset(recv_buf,0,sizeof(recv_buf));
            if(flag_demo!=0){if (0 != at_send_no_reply(at_cmd, strlen(at_cmd), false,NULL,0,recv_buf,sizeof(recv_buf),NULL))
            {mal_err("at send raw api fail");}
            g_ec20_at_response = AT_MQTT_IDLE;

                return g_at_response_result;}
            else{
         if (0 != at_send_wait_reply(at_cmd, strlen(at_cmd), false,NULL,0,recv_buf,sizeof(recv_buf),NULL)) {
        mal_err("at send raw api fail");

        g_ec20_at_response = AT_MQTT_IDLE;

        return -1;
    }

//    rt_thread_mdelay(5000);
//    memset(recv_buf,0,sizeof(recv_buf));
//    at_read(recv_buf, sizeof(recv_buf));
        //strcpy(recv_buf,recv_buf);
        //strcpy(at_recv_rsp_buf,recv_buf);

       // rt_kprintf("ec20:  %s",at_recv_rsp_buf);
        at_ec20_mqtt_client_rsp_callback(NULL,recv_buf,sizeof(recv_buf));

#ifdef PLATFORM_HAS_OS
    HAL_SemaphoreWait(g_sem_response, timeout_ms);
#else

    at_yield(NULL, 0, NULL, timeout_ms);

#endif

    g_ec20_at_response = AT_MQTT_IDLE;

    return g_at_response_result;}
}
void lock_demo(int a)
{
    if(a==1){

        rt_pin_write(io.IO_CLOSE, PIN_HIGH);
            rt_pin_write(Led.LED_R, PIN_HIGH);
            rt_pin_write(Led.LED_G, PIN_LOW);
            rt_pin_write(io.IO_OPEN, PIN_LOW);




    }
    else{
        rt_pin_write(io.IO_OPEN, PIN_HIGH);
                    rt_pin_write(Led.LED_G, PIN_HIGH);
                    rt_pin_write(Led.LED_R, PIN_LOW);
                    rt_pin_write(io.IO_CLOSE, PIN_LOW);
    }

}
/* 导出到 msh 命令列表中 */
INIT_APP_EXPORT(thread_serial);


