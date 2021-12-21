/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-08-10     a       the first version
 */
#include <stdio.h>
#include <string.h>
#include<rtthread.h>
#include "infra_types.h"
#include "at_wrapper.h"
#include "at_parser.h"
#include "infra_list.h"
#include "aty.h"
#include <rtdevice.h>
#include <board.h>
//#include "put.h"
#define OOB_MAX 5
#define RECV    "LockSwitch"
#define Topic   "/sys/a1ehllrd5dK/device1/thing/event/property/post"

#define uart_name  "uart1"
char   at_recv_rsp_buf[400];

typedef struct at_task_s
{
    slist_t   next;
    void *    smpr;
    char *    command;
    char *    rsp;
    char *    rsp_prefix;
    char *    rsp_success_postfix;
    char *    rsp_fail_postfix;
    uint32_t  rsp_prefix_len;
    uint32_t  rsp_success_postfix_len;
    uint32_t  rsp_fail_postfix_len;
    uint32_t  rsp_offset;
    uint32_t  rsp_len;
} at_task_t;


typedef struct oob_s
{
    char *     prefix;
    char *     postfix;
    char *     oobinputdata;
    uint32_t   reallen;
    uint32_t   maxlen;
    at_recv_cb cb;
    void *     arg;
} oob_t;


typedef struct
{
    rt_device_t _pstuart;
    int         _timeout;
    char *      _default_recv_prefix;
    char *      _default_recv_success_postfix;
    char *      _default_recv_fail_postfix;
    char *      _send_delimiter;
    int         _recv_prefix_len;
    int         _recv_success_postfix_len;
    int         _recv_fail_postfix_len;
    int         _send_delim_size;
    oob_t       _oobs[OOB_MAX];
    int         _oobs_num;
     rt_mutex_t       at_uart_recv_mutex ;
     rt_mutex_t       at_uart_send_mutex ;
    rt_mutex_t       task_mutex ;
#if !AT_SINGLE_TASK
    slist_t     task_l;
#endif
} at_parser_t;

#define TASK_DEFAULT_WAIT_TIME 5000

#ifndef AT_WORKER_STACK_SIZE
#define AT_WORKER_STACK_SIZE   1024
#endif

#ifndef AT_UART_TIMEOUT_MS
#define AT_UART_TIMEOUT_MS     1000
#endif


#define AT_CMD_DATA_INTERVAL_MS   0
#ifdef AT_DEBUG_MODE
#define atpsr_err(...)               do{HAL_Printf(__VA_ARGS__);HAL_Printf("\r\n");}while(0)
#define atpsr_warning(...)           do{HAL_Printf(__VA_ARGS__);HAL_Printf("\r\n");}while(0)
#define atpsr_info(...)              do{HAL_Printf(__VA_ARGS__);HAL_Printf("\r\n");}while(0)
#define atpsr_debug(...)             do{HAL_Printf(__VA_ARGS__);HAL_Printf("\r\n");}while(0)
#else
#define atpsr_err(...)
#define atpsr_warning(...)
#define atpsr_info(...)
#define atpsr_debug(...)
#endif

static uint8_t    inited = 1;
static rt_device_t at_uart;

static at_parser_t at;


#define RECV_BUFFER_SIZE 512
static char at_rx_buf[RECV_BUFFER_SIZE];

static rt_sem_t rx_sem;

static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */

    rt_sem_release(rx_sem);

    //rt_kprintf("C\n");
    //at_read(recvdata,sizeof(recvdata));

    //rt_device_write(serial, 0, str1, (sizeof(str1) - 1));

    return RT_EOK;
}


int mutex_init()

{
    if (!at._pstuart)
            {
    at._pstuart = rt_device_find(uart_name);
        if (!at._pstuart)
        {
            rt_kprintf("find %s failed!\n", uart_name);
            return RT_ERROR;
        }
            }
    rt_device_set_rx_indicate(at._pstuart, uart_input);



    if (at.at_uart_recv_mutex == RT_NULL){
     at.at_uart_recv_mutex=  rt_mutex_create("at_uart_recv_mutex", RT_IPC_FLAG_FIFO);
     if (at.at_uart_recv_mutex == RT_NULL)
         {
             rt_kprintf("create dynamic mutex failed.\n");
             return -1;
         }
    }
    if (at.at_uart_send_mutex == RT_NULL){
         at.at_uart_send_mutex=  rt_mutex_create("at_uart_recv_mutex", RT_IPC_FLAG_FIFO);
         if (at.at_uart_send_mutex == RT_NULL)
             {
                 rt_kprintf("create dynamic mutex failed.\n");
                 return -1;
             }
        }
    if (at.task_mutex == RT_NULL){
         at.task_mutex=  rt_mutex_create("at_uart_recv_mutex", RT_IPC_FLAG_FIFO);
         if (at.task_mutex == RT_NULL)
             {
                 rt_kprintf("create dynamic mutex failed.\n");
                 return -1;
             }
        }
    return 0;

}
int at_register_callback(const char *prefix, const char *postfix, char *recvbuf,
                         int bufsize, at_recv_cb cb, void *arg)
{
    oob_t *oob = NULL;
    int    i   = 0;

    if (bufsize < 0 || bufsize >= RECV_BUFFER_SIZE || NULL == prefix) {
        atpsr_err("%s invalid input \r\n", __func__);
        return -1;
    }

    if (NULL != postfix && (NULL == recvbuf || 0 == bufsize)) {
        atpsr_err("%s invalid postfix input \r\n", __func__);
        return -1;
    }


    if (at._oobs_num >= OOB_MAX) {
        atpsr_err("No place left in OOB.\r\n");
        return -1;
    }

    /*check oob exist*/
    for (i = 0; i < at._oobs_num; i++) {
        if (NULL != at._oobs[i].prefix &&
            strcmp(prefix, at._oobs[i].prefix) == 0) {
            atpsr_warning("oob prefix %s is already exist.\r\n", prefix);
            return -1;
        }
    }

    oob = &(at._oobs[at._oobs_num++]);

    oob->oobinputdata = recvbuf;
    if (oob->oobinputdata != NULL) {
        memset(oob->oobinputdata, 0, bufsize);
    }
    oob->maxlen  = bufsize;
    oob->prefix  = (char *)prefix;
    oob->postfix = (char *)postfix;
    oob->cb      = cb;
    oob->arg     = arg;
    oob->reallen = 0;

    atpsr_debug("New oob registered (%s)", oob->prefix);

    return 0;
}


static int at_worker_task_add(at_task_t *tsk)
{
    if (NULL == tsk) {
        atpsr_err("invalid input %s \r\n", __func__);
        return -1;
    }

    HAL_MutexLock(at.task_mutex);
    slist_add_tail(&tsk->next, &at.task_l);
    HAL_MutexUnlock(at.task_mutex);

    return 0;
}
static int at_worker_task_del(at_task_t *tsk)
{
    if (NULL == tsk) {
        atpsr_err("invalid input %s \r\n", __func__);
        return -1;
    }

    HAL_MutexLock(at.task_mutex);
    slist_del(&tsk->next, &at.task_l);
    HAL_MutexUnlock(at.task_mutex);
    if (tsk->smpr) {
        HAL_SemaphoreDestroy(tsk->smpr);
    }
    if (tsk) {
#ifdef PLATFORM_HAS_DYNMEM
        HAL_Free(tsk);
#endif
    }

    return 0;
}

static int at_sendto_lower(rt_device_t uart, void *data, uint32_t size,
        uint32_t timeout, bool ackreq)
{
int ret = -1;

(void) ackreq;
//初始化

ret=rt_device_write(uart, 0, data, size);
if(ret == size)
{
ret =0;
}
return ret;
}

int at_send_wait_reply(const char *cmd, int cmdlen, bool delimiter,
                       const char *data, int datalen,
                       char *replybuf, int bufsize,
                       const atcmd_config_t *atcmdconfig)
{
    mutex_init();
    int ret = 1;
    int intval_ms = AT_CMD_DATA_INTERVAL_MS;
    at_task_t *tsk;

//    if (inited == 0) {
//        atpsr_err("at have not init yet\r\n");
//        return -1;
//    }

    if (NULL == cmd || cmdlen <= 0) {
        atpsr_err("%s invalid input \r\n", __FUNCTION__);
        return -1;
    }

    if (NULL == replybuf || 0 == bufsize) {
        atpsr_err("%s invalid input \r\n", __FUNCTION__);
        return -1;
    }

    HAL_MutexLock(at.at_uart_send_mutex);
#ifdef PLATFORM_HAS_DYNMEM
    tsk = (at_task_t *)HAL_Malloc(sizeof(at_task_t));
#else
    tsk = &g_at_task;
#endif
    if (NULL == tsk) {
        atpsr_err("tsk buffer allocating failed");
        HAL_MutexUnlock(at.at_uart_send_mutex);
        return -1;
    }
    memset(tsk, 0, sizeof(at_task_t));
//不能注释掉
    tsk->smpr = HAL_SemaphoreCreate();
    rx_sem= rt_sem_create("rx_sem", 0, RT_IPC_FLAG_FIFO);
    if (NULL == tsk->smpr) {
        atpsr_err("failed to allocate semaphore");
        goto end;
    }

//    if (atcmdconfig) {
//        if (NULL != atcmdconfig->reply_prefix) {
//            tsk->rsp_prefix     = atcmdconfig->reply_prefix;
//            tsk->rsp_prefix_len = strlen(atcmdconfig->reply_prefix);
//        }
//
//        if (NULL != atcmdconfig->reply_success_postfix) {
//            tsk->rsp_success_postfix     = atcmdconfig->reply_success_postfix;
//            tsk->rsp_success_postfix_len = strlen(atcmdconfig->reply_success_postfix);
//        }
//
//        if (NULL != atcmdconfig->reply_fail_postfix) {
//            tsk->rsp_fail_postfix     = atcmdconfig->reply_fail_postfix;
//            tsk->rsp_fail_postfix_len = strlen(atcmdconfig->reply_fail_postfix);
//        }
//    }

    tsk->command = (char *)cmd;
    tsk->rsp     = replybuf;
    tsk->rsp_len = bufsize;
    //recvdata=replybuf;

    at_worker_task_add(tsk);

    if ((ret = at_sendto_lower(at._pstuart, (void *)cmd, cmdlen,
                               at._timeout, true)) != 0) {
        atpsr_err("uart send command failed");
        goto end;
    }

    if (delimiter) {
        if ((ret = at_sendto_lower(at._pstuart, (void *)at._send_delimiter,
                    strlen(at._send_delimiter), at._timeout, false)) != 0) {
            atpsr_err("uart send delimiter failed");
            goto end;
        }
    }

    if (data && datalen > 0) {
        if (intval_ms > 0)
            HAL_SleepMs(intval_ms);

        if ((ret = at_sendto_lower(at._pstuart, (void *)data, datalen, at._timeout, true)) != 0) {
            atpsr_err("uart send delimiter failed");
            goto end;
        }
    }

    //char ch[6];
  //rt_thread_mdelay(TASK_DEFAULT_WAIT_TIME);
  //rt_device_read(at._pstuart,0,replybuf,bufsize);

  //rt_kprintf("%s",replybuf);




//    if ((ret = HAL_SemaphoreWait(tsk->smpr, TASK_DEFAULT_WAIT_TIME)) != 0) {
//        atpsr_err("sem_wait failed");
//        goto end;
//    }

end:
    at_worker_task_del(tsk);
    HAL_MutexUnlock(at.at_uart_send_mutex);
    if((ret = rt_sem_take(rx_sem,TASK_DEFAULT_WAIT_TIME))==RT_EOK)
             {
        rt_thread_mdelay(1000);
             at_read(replybuf,bufsize);
             }



    //rt_thread_mdelay(1000);
    //at_read(replybuf,bufsize);
    return ret;
 }


int at_send_no_reply(const char *cmd, int cmdlen, bool delimiter,
        const char *data, int datalen,
        char *replybuf, int bufsize,
        const atcmd_config_t *atcmdconfig)
{   mutex_init();
    int ret = 1;
    int intval_ms = AT_CMD_DATA_INTERVAL_MS;
    at_task_t *tsk;

//    if (inited == 0) {
//        atpsr_err("at have not init yet\r\n");
//        return -1;
//    }

    if (NULL == cmd || cmdlen <= 0) {
        atpsr_err("%s invalid input \r\n", __FUNCTION__);
        return -1;
    }

    if (NULL == replybuf || 0 == bufsize) {
        atpsr_err("%s invalid input \r\n", __FUNCTION__);
        return -1;
    }

    HAL_MutexLock(at.at_uart_send_mutex);
#ifdef PLATFORM_HAS_DYNMEM
    tsk = (at_task_t *)HAL_Malloc(sizeof(at_task_t));
#else
    tsk = &g_at_task;
#endif
    if (NULL == tsk) {
        atpsr_err("tsk buffer allocating failed");
        HAL_MutexUnlock(at.at_uart_send_mutex);
        return -1;
    }
    memset(tsk, 0, sizeof(at_task_t));
//不能注释掉
    tsk->smpr = HAL_SemaphoreCreate();
    rx_sem= rt_sem_create("rx_sem", 0, RT_IPC_FLAG_FIFO);
    if (NULL == tsk->smpr) {
        atpsr_err("failed to allocate semaphore");
        goto end;
    }

//    if (atcmdconfig) {
//        if (NULL != atcmdconfig->reply_prefix) {
//            tsk->rsp_prefix     = atcmdconfig->reply_prefix;
//            tsk->rsp_prefix_len = strlen(atcmdconfig->reply_prefix);
//        }
//
//        if (NULL != atcmdconfig->reply_success_postfix) {
//            tsk->rsp_success_postfix     = atcmdconfig->reply_success_postfix;
//            tsk->rsp_success_postfix_len = strlen(atcmdconfig->reply_success_postfix);
//        }
//
//        if (NULL != atcmdconfig->reply_fail_postfix) {
//            tsk->rsp_fail_postfix     = atcmdconfig->reply_fail_postfix;
//            tsk->rsp_fail_postfix_len = strlen(atcmdconfig->reply_fail_postfix);
//        }
//    }

    tsk->command = (char *)cmd;
    tsk->rsp     = replybuf;
    tsk->rsp_len = bufsize;
    //recvdata=replybuf;

    at_worker_task_add(tsk);

    if ((ret = at_sendto_lower(at._pstuart, (void *)cmd, cmdlen,
                               at._timeout, true)) != 0) {
        atpsr_err("uart send command failed");
        goto end;
    }

    if (delimiter) {
        if ((ret = at_sendto_lower(at._pstuart, (void *)at._send_delimiter,
                    strlen(at._send_delimiter), at._timeout, false)) != 0) {
            atpsr_err("uart send delimiter failed");
            goto end;
        }
    }

    if (data && datalen > 0) {
        if (intval_ms > 0)
            HAL_SleepMs(intval_ms);

        if ((ret = at_sendto_lower(at._pstuart, (void *)data, datalen, at._timeout, true)) != 0) {
            atpsr_err("uart send delimiter failed");
            goto end;
        }
    }

    //char ch[6];
  //rt_thread_mdelay(TASK_DEFAULT_WAIT_TIME);
  //rt_device_read(at._pstuart,0,replybuf,bufsize);

  //rt_kprintf("%s",replybuf);




//    if ((ret = HAL_SemaphoreWait(tsk->smpr, TASK_DEFAULT_WAIT_TIME)) != 0) {
//        atpsr_err("sem_wait failed");
//        goto end;
//    }

end:
    at_worker_task_del(tsk);
    HAL_MutexUnlock(at.at_uart_send_mutex);


    return ret;
}


static int at_recvfrom_lower(rt_device_t uart, void *data, uint32_t expect_size,
                             uint32_t *recv_size, uint32_t timeout)
{
    int ret = 0;

    *recv_size = rt_device_read(uart, 0, data, expect_size);
    //rt_thread_mdelay(1000);
    rt_kprintf("%s",data);

    return ret;
}
int at_read(char *outbuf, int readsize)
{
    int      ret = 0;

    uint32_t recv_size, total_read = 0;

    if (inited == 0) {
        atpsr_err("at have not init yet\r\n");
        return -1;
    }
    memset(outbuf,0,readsize);
    HAL_MutexLock(at.at_uart_recv_mutex);
    while (total_read < readsize) {
        ret = at_recvfrom_lower(at._pstuart, (void *)(outbuf + total_read),
                                readsize - total_read, &recv_size, at._timeout);
        if (ret != 0) {
            atpsr_err("at_read failed on uart_recv.");
            break;
        }

        if (recv_size <= 0) {
            break;
        }
        total_read += recv_size;
        if (total_read >= readsize) {
            break;
        }

    }
    HAL_MutexUnlock(at.at_uart_recv_mutex);

    if (ret != 0) {
        return -1;
    }

    return total_read;
}


void recv_callback()
{
    //rt_kprintf("nono:%s",at_recv_rsp_buf);
    //rt_kprintf("%s",at_rsp);
    char     *temp = NULL;
        char     *topic_ptr = NULL;
        char     *msg_ptr = NULL;

        if (NULL == recvdata) {
                return;
            }
        if(strstr(at_recv_rsp_buf,RECV)!=NULL)
        {
            rt_kprintf("callback success!\n");
            temp =strtok(at_recv_rsp_buf,"{");

            temp =strtok(NULL,"{");

            //temp =strtok(NULL,"{");
            temp =strtok(NULL,":");
            //rt_kprintf("temp is :%s",temp);
            temp =strtok(NULL,"}");
            //rt_kprintf("temp is :%s",temp);
            if(temp !=NULL)
            {
                KEY_DEMO = strtol(temp, NULL, 0);
                //rt_kprintf("%d",KEY_DEMO);
                lock_demo(KEY_DEMO);

            }
        }


}


void serial_thread_entry(void*pagram)
{
    rt_thread_mdelay(20000);

    int ret =1;
rt_kprintf("start recieve\n");
    while(1)
    {
    if((ret = rt_sem_take(rx_sem,TASK_DEFAULT_WAIT_TIME))==RT_EOK)
                 {
            rt_thread_mdelay(2000);
                at_read(at_recv_rsp_buf,sizeof(at_recv_rsp_buf));
                 }


    recv_callback();


    }


}


#define ADC_DEV_NAME        "adc0"      /* ADC 设备名称 */
#define ADC_DEV_CHANNEL     7           /* ADC 通道 */
#define REFER_VOLTAGE       330         /* 参考电压 3.3V,数据精度乘以100保留2位小数*/
#define CONVERT_BITS        (1 << 10)   /* 转换位数为10位 */

static rt_uint32_t adc_vol_sample()
{
    rt_adc_device_t adc_dev;
    rt_uint32_t value, vol;
    rt_err_t ret = RT_EOK;

    /* 查找设备 */
    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("adc sample run failed! can't find %s device!\n", ADC_DEV_NAME);
        return RT_ERROR;
    }

    /* 使能设备 */
    ret = rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);

    /* 读取采样值 */
    value = rt_adc_read(adc_dev, ADC_DEV_CHANNEL);

    /* 转换为对应电压值 */
    vol = value * REFER_VOLTAGE / CONVERT_BITS;
    //rt_kprintf("the voltage is :%d.%02d \n", vol / 100, vol % 100);

    /* 关闭通道 */
    ret = rt_adc_disable(adc_dev, ADC_DEV_CHANNEL);

    return vol;
}



void pub_thread_entry(void*pagram)
{
    while(1)
    {
        rt_thread_mdelay(10000);
        rt_uint32_t vol;
        vol = adc_vol_sample();
//        rt_kprintf("the voltage is :%d",vol);
        at_ec20_mqtt_client_publish(Topic, 0, 143,KEY_DEMO,vol);

    }
}

int recv_thread(){
    mutex_init();

    //rt_thread_mdelay(5000);
    int ret =0;
    rx_sem= rt_sem_create("rx_sem", 0, RT_IPC_FLAG_FIFO);
    rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024, 27, 10);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL)
    {

        rt_thread_startup(thread);
    }
    else
    {
        ret = RT_ERROR;
    }

    rt_thread_t thread_pub = rt_thread_create("serial", pub_thread_entry, RT_NULL, 1024, 28, 10);
        /* 创建成功则启动线程 */
        if (thread_pub != RT_NULL)
        {

            rt_thread_startup(thread_pub);
        }
        else
        {
            ret = RT_ERROR;
        }
    return ret;
}

INIT_APP_EXPORT(recv_thread);
