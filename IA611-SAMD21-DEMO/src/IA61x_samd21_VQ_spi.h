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


#ifndef IA61x_SAMD21_VQ_SPI_H_
#define IA61x_SAMD21_VQ_SPI_H_

#include "IA61x.h"
extern int32_t IA61x_samd21_vq_spi_init(IA61x_instance *IA61x);
extern int32_t IA61x_samd21_vq_spi_uninit(void);

#define SPI_EXT_INT_PIN     PIN_PB12
#define SPI_EIC_PIN         PIN_PB12A_EIC_EXTINT12
#define SPI_EIC_PIN_MUX     PINMUX_PB12A_EIC_EXTINT12
#define SPI_EIC_CHANNEL     12

#define SPI_EXT_SS          PIN_PA17        //Use external SS instead of peripheral driven.

/* Number of times to try to send packet if failed. */
#define VQ_SPI_TIMEOUT 1000

//[definition_master]
#define CONF_MASTER_SPI_MODULE  EXT2_SPI_MODULE                 //SERCOM0
#define CONF_MASTER_MUX_SETTING EXT2_SPI_SERCOM_MUX_SETTING     //DIPO= 0, DOPO=0x1
#define CONF_MASTER_PINMUX_PAD0 EXT2_SPI_SERCOM_PINMUX_PAD0     //MISO
#define CONF_MASTER_PINMUX_PAD1 EXT2_SPI_SERCOM_PINMUX_PAD1     //SS
#define CONF_MASTER_PINMUX_PAD2 EXT2_SPI_SERCOM_PINMUX_PAD2     //MOSI
#define CONF_MASTER_PINMUX_PAD3 EXT2_SPI_SERCOM_PINMUX_PAD3     //SCK
//[definition_master]


#endif