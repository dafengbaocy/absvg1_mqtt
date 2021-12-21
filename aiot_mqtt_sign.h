/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-08-11     a       the first version
 */
#include "infra_sha256.h"
#ifndef APPLICATIONS_AIOT_MQTT_SIGN_H_
#define APPLICATIONS_AIOT_MQTT_SIGN_H_
int aiotMqttSign(const char *productKey, const char *deviceName, const char *deviceSecret,
                     char clientId[150], char username[64], char password[65]);


#endif /* APPLICATIONS_AIOT_MQTT_SIGN_H_ */
