/************************************************************************//**
 * File: FileName
 *
 * Description: Sample File Description
 *
 * Copyright 2018 Knowles Corporation. All rights reserved.
 *
 * All information, including software, contained herein is and remains
 * the property of Knowles Corporation. The intellectual and technical
 * concepts contained herein are proprietary to Knowles Corporation
 * and may be covered by U.S. and foreign patents, patents in process,
 * and/or are protected by trade secret and/or copyright law.
 * This information may only be used in accordance with the applicable
 * Knowles SDK License. Dissemination of this information or distribution
 * of this material is strictly forbidden unless in accordance with the
 * applicable Knowles SDK License.
 *
 *
 * KNOWLES SOURCE CODE IS STRICTLY PROVIDED "AS IS" WITHOUT ANY WARRANTY
 * WHATSOEVER, AND KNOWLES EXPRESSLY DISCLAIMS ALL WARRANTIES,
 * EXPRESS, IMPLIED OR STATUTORY WITH REGARD THERETO, INCLUDING THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE OR NON-INFRINGEMENT OF THIRD PARTY RIGHTS. KNOWLES
 * SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY YOU AS A RESULT OF
 * USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 * IN CERTAIN STATES, THE LAW MAY NOT ALLOW KNOWLES TO DISCLAIM OR EXCLUDE
 * WARRANTIES OR DISCLAIM DAMAGES, SO THE ABOVE DISCLAIMERS MAY NOT APPLY.
 * IN SUCH EVENT, KNOWLES' AGGREGATE LIABILITY SHALL NOT EXCEED
 * FIFTY DOLLARS ($50.00).
 *
 ****************************************************************************/


#ifndef IA61x_SAMD21_VQ_I2C_H_
#define IA61x_SAMD21_VQ_I2C_H_

#include "IA61x.h"
extern int32_t IA61x_samd21_vq_i2c_init(IA61x_instance *IA61x);
extern int32_t IA61x_samd21_vq_i2c_uninit(void);

#define CONF_I2C_MASTER_MODULE    SERCOM2
#define SLAVE_ADDRESS 0x3E //for IA61x ADD0 = 0  and ADD1 = 0 ==> slave Address is 0x3E

#define I2C_SLAVE_ADD1_PIN  PIN_PA07
#define I2C_SLAVE_ADD2_PIN  PIN_PA04

#define I2C_EXT_INT_PIN     I2C_SLAVE_ADD2_PIN
#define I2C_EIC_PIN         PIN_PA04A_EIC_EXTINT4
#define I2C_EIC_PIN_MUX     PINMUX_PA04A_EIC_EXTINT4
#define I2C_EIC_CHANNEL     4


/* Number of times to try to send packet if failed. */
#define I2C_TIMEOUT 1000


#endif