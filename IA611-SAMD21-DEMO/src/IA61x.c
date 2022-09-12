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
#include "IA61x.h"
#include <asf.h>

static IA61x_instance IA61x;

/*****************************************************************************/
#ifdef IA61x_SAMD21_VQ_UART
# include "IA61x_samd21_VQ_uart.h"


/***************************************************************************
 * @fn          IA61x_init
 *
 * @brief       Initialize IA61x interface port and create IA61x instance handle
 *
 * @param       none
 *
 * @retval      IA61x     Ia61x instance handle for applicaton
 *
 ****************************************************************************/
IA61x_instance *IA61x_init(void)
{
    if (IA61x_samd21_vq_uart_init(&IA61x) == SUCCESS) return (&IA61x);
    return (NULL);
}

/***************************************************************************
 * @fn          IA61x_uninit
 *
 * @brief       Un-initialize IA61x interface port 
 *
 * @param       none
 *
 * @retval      none
 *
 ****************************************************************************/
void IA61x_uninit(void)
{
    IA61x_samd21_vq_uart_uninit();
}


#endif /* ifdef IA61x_SAMD21_VQ_UART */



/*****************************************************************************/
#ifdef IA61x_SAMD21_VQ_I2C

# include "IA61x_samd21_VQ_i2c.h"


IA61x_instance *IA61x_init(void)
{
    if (IA61x_samd21_vq_i2c_init(&IA61x) == SUCCESS) return (&IA61x);
    return (NULL);
}

void IA61x_uninit(void)
{
    IA61x_samd21_vq_i2c_uninit();
}

#endif /*#ifdef IA61x_SAMD21_VQ_I2C*/


/*****************************************************************************/

#ifdef IA61x_SAMD21_VQ_SPI

# include "IA61x_samd21_VQ_spi.h"

IA61x_instance *IA61x_init(void)
{
    if (IA61x_samd21_vq_spi_init(&IA61x) == SUCCESS) return (&IA61x);
    return (NULL);
}

void IA61x_uninit(void)
{
    IA61x_samd21_vq_spi_uninit();
}

#endif
/*****************************************************************************/
/*****************************************************************************/
