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

#include "infra_types.h"
#include "at_wrapper.h"
#include "at_parser.h"
#ifndef APPLICATIONS_AT_H_
#define APPLICATIONS_AT_H_

int at_register_callback(const char *prefix, const char *postfix, char *recvbuf,
                         int bufsize, at_recv_cb cb, void *arg);


//int at_send_no_reply(const char *data, int datalen, bool delimiter);
static char recvdata[256];
static char recvdataa[256];
static int KEY_DEMO = 0;
int at_read(char *outbuf, int readsize);
//int at_send_wait_reply(const char *cmd, int cmdlen, bool delimiter,const char *data, int datalen,
//           char *replybuf, int bufsize,
//                       const atcmd_config_t *atcmdconfig);

struct Led_s
{
    uint8_t LED_R;

    uint8_t LED_G;
};
struct IO_s
{
    uint8_t IO_OPEN;

    uint8_t IO_CLOSE;
};
static struct Led_s Led;
    static struct IO_s io;
#endif /* APPLICATIONS_AT_H_ */
