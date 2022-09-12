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

#include "IA61x_config.h"
#ifdef IA61x_SAMD21_VQ_UART

# include <asf.h>
# include <string.h>
# include "IA61x_samd21_VQ_uart.h"

# include "trill_sys_config.h"       /*Trillbit SDK Sys config*/

# include "IA611_FW_Bin_UART.h"      /* Firmware Binary */
/*********************************************************************************/
// private
/*********************************************************************************/
static struct usart_module usart_instance;
volatile uint8_t interrupt_flag = 0;

static uint32_t IA61x_samd21_vq_uart_reg_IRQ(void);
static void recycle_uart(void);



/***************************************************************************
 * @fn      IA61x_uart_get()
 *
 * @brief   Read data from UART port
 *
 * @param   pData    Buffer to receive data
 * @param   size    Size of data to be received
 *
 * @retval  none
 *
 ****************************************************************************/
static int32_t IA61x_uart_get(uint8_t *pData, uint32_t size)
{
    return usart_read_buffer_wait(&usart_instance, pData, size);
    //return (0);
}

/***************************************************************************
 * @fn      IA61x_uart_put()
 *
 * @brief   Write data to UART port
 *
 * @param   pData    Send dat Buffer
 * @param   size    Size of data to be sent
 *
 * @retval  none
 *
 ****************************************************************************/
static int32_t IA61x_uart_put(uint8_t *pData, uint32_t size)
{
    usart_write_buffer_wait(&usart_instance, pData, size);
    return (0);
}

/*******************************************************************************************************
 * @fn      IA61x_uart_cmd()
 *
 * @brief   Send Command word and Data word to IA61x and receive response. If no response is expected 
 *          from IA61x then timeout should be 0.
 *
 * @param   cmdWord     Command word to send to IA61x
 * @param   dataWord    Data word to send to IA61x
 * @param   timeout     Response read retry count. Set to 0 if no response expected
 * @param   pResponse   Response word from IA61x
 *
 * @retval  CMD_FAILED  Command Failed Error
 * @retval  CMD_SUCCESS Command execution successful
 *
 *******************************************************************************************************/
static int32_t IA61x_uart_cmd(uint16_t cmdWord, uint16_t dataWord, uint32_t timeout, uint16_t *pResponse)
{
    uint32_t data = 0;
    union _tmp {
        uint16_t word[2];
        uint8_t byte[4];
    } tmp;

    tmp.word[0] = cmdWord;
    tmp.word[1] = dataWord;

    //Command Word first and then Data word
    data = tmp.byte[1] | tmp.byte[0]<<8 | tmp.byte[3]<<16 | tmp.byte[2] << 24;
    usart_write_buffer_wait(&usart_instance, (uint8_t *)&data, 4);
	
	if ((cmdWord & CMD_NO_RESP_MASK) == CMD_NO_RESP_MASK)
	{
		*pResponse = dataWord;
		return CMD_SUCCESS;
	}

    //if timeout is 0 then no need to read the response else try reading the data multiple times
    while(timeout--)
    {
        //Read response 
        usart_read_buffer_wait(&usart_instance, tmp.byte, 4);

        *pResponse = (tmp.byte[0] << 8) | tmp.byte[1] ;
        //return the second response word if the first response word matches command word
        if(*pResponse == cmdWord)
        {
            *pResponse = (tmp.byte[2] << 8) | tmp.byte[3];
        }
        else
        {
            return (CMD_FAILED);
        }
    }

    return (CMD_SUCCESS);
}

/*******************************************************************************************************
 * @fn      IA61x_download_bin()
 *
 * @brief   Function to download sys config or Firmware binary to IA61x
 *
 * @param   pData       Data buffer to send to IA61x
 * @param   size        Size of data buffer
 *
 * @retval  CMD_FAILED  Command Failed Error
 * @retval  CMD_SUCCESS Command execution successful
 * @retval  i           UART Status code from IA61x. 0x02 indicates successful Firmware download.
 *
 *******************************************************************************************************/
 static int32_t IA61x_download_bin(uint8_t *pData, uint32_t size)
{
    const uint8_t Load01[] = { 0x1 };
    uint8_t cRetVal;
    uint32_t iCount = 0;

    usart_write_buffer_wait(&usart_instance, Load01, 1);
    if (usart_read_buffer_wait(&usart_instance, &cRetVal, 1) != STATUS_OK) return (-1);
    if (cRetVal != 1) return (CMD_FAILED);

    iCount = 0;
    while (iCount < size) 
    {
        usart_write_buffer_wait(&usart_instance, &pData[iCount], (size - iCount >= 0xFFFF) ? 0xFFFF : size - iCount);
        iCount += 0xFFFF;
    }

    delay_us(100);

    if (usart_read_buffer_wait(&usart_instance, &cRetVal, 1) == STATUS_OK) 
    {
        iCount = cRetVal;
        return (iCount);
    }

    return (CMD_SUCCESS);
}

/*******************************************************************************************************
 * @fn      my_usart_init()
 *
 * @brief   Initialize SAMD21 USART port for IA61x interface
 *
 * @param   baud        UART Baudrate vlaue
 *
 * @retval  none
 *
 *******************************************************************************************************/
static void my_usart_init(uint32_t baud)
{
    struct usart_config config_usart;

    usart_get_config_defaults(&config_usart);

    config_usart.baudrate       = baud;
    config_usart.mux_setting    = EXT3_UART_SERCOM_MUX_SETTING; /* EXT3_UART_MODULE */
    config_usart.pinmux_pad0    = EXT3_UART_SERCOM_PINMUX_PAD0;
    config_usart.pinmux_pad1    = EXT3_UART_SERCOM_PINMUX_PAD1;
    config_usart.pinmux_pad2    = EXT3_UART_SERCOM_PINMUX_PAD2;
    config_usart.pinmux_pad3    = EXT3_UART_SERCOM_PINMUX_PAD3;

    while (usart_init(&usart_instance, EXT3_UART_MODULE, &config_usart) != STATUS_OK) ;

    usart_enable(&usart_instance);
}


/*******************************************************************************************************
 * @fn      my_usart_uninit()
 *
 * @brief   Uninitialize SAMD21 USART instance
 *
 * @param   none
 *
 * @retval  none
 *
 *******************************************************************************************************/
static void my_usart_uninit(void)
{
    usart_disable(&usart_instance);
}

/*******************************************************************************************************
 * @fn      IA61x_uart_download_config()
 *
 * @brief   Download Sysconfig file to IA61x
 *
 * @param   none
 *
 * @retval  CMD_FAILED  Command Failed Error
 * @retval  CMD_SUCCESS Command execution successful
 *
 *******************************************************************************************************/
static int32_t IA61x_uart_download_config(void)
{
    uint32_t iRetVal;

    iRetVal = IA61x_download_bin((uint8_t *)SCFG, sizeof(SCFG));
    if (iRetVal != 0)
        return (CMD_FAILED);

    return (CMD_SUCCESS);
}


/*******************************************************************************************************
 * @fn      IA61x_uart_download_firmware()
 *
 * @brief   Download IA61x Firmware binary to IA61x
 *
 * @param   none
 *
 * @retval  pResponse   response to Sync command
 * @retval  CMD_FAILED Command execution failed
 *
 *******************************************************************************************************/
static int32_t IA61x_uart_download_firmware(void)
{
    uint32_t iRetvalue;
    uint16_t pResponse;

    iRetvalue = IA61x_download_bin((uint8_t *)VQ_Bin, sizeof(VQ_Bin));
    delay_ms(35); //wait for firmware to initialize. minimum 30 mS delay

    //if FW download is success then ping firmware and confirm that FW is up and running
    if (iRetvalue == FW_DOWNLOAD_SUCCESS) 
    {
        if(IA61x_uart_cmd(SYNC_CMD,EMPTY_DATA,1, &pResponse))
        {
            return(pResponse);
        }
		
		
		iRetvalue = IA61x_samd21_vq_uart_reg_IRQ();
		if (!iRetvalue)
		{
			return(iRetvalue);	
		}
		
    }
    return (CMD_FAILED);
}


/*******************************************************************************************************
 * @fn      IA61x_uart_download_keyword()
 *
 * @brief   Download Keyword models to IA61x.
 *
 * @param   data    Data buffer to send to IA61x
 * @param   size    Data buffer length
 *
 * @retval  SUCCESS
 *
 *******************************************************************************************************/
uint8_t DownloadNumber  = 0;
#if IA611_VOICE_ID
    uint8_t Ref_model = 0;
#endif

static int32_t IA61x_uart_download_keyword(uint16_t *data, uint16_t size)
{
    uint16_t Response = 0;
    uint8_t inbuf2[4];
	
	//uint8_t zeropad[WDB_SIZE_NO_HEADER];
	int ret;
	
	//memset(zeropad, 0, WDB_SIZE_NO_HEADER);

    //Send Write Data Block command.
	//Do not check WDB response validity here.
    IA61x_uart_cmd(WDB_CMD, size,1, &Response);
	
	ret = usart_write_buffer_wait(&usart_instance, (uint8_t*) data, size);
	if (ret != STATUS_OK)
	{
		return ret;
	}

#if 0 // padding not needed when size is less than 512 bytes. Algo WDB is always less than 256 bytes.
	ret = usart_write_buffer_wait(&usart_instance, zeropad, WDB_SIZE - size);
	if (ret != STATUS_OK)
	{
		return ret;
	}
#endif
	
    ret = usart_read_buffer_wait(&usart_instance, inbuf2, 4);
	if (ret != STATUS_OK)
	{
		return ret;
	}
	
	return inbuf2[3];
}

static void IA61x_irq_callback(void)
{
	interrupt_flag = true;
}

static uint32_t IA61x_samd21_vq_uart_reg_IRQ(void)
{
	uint16_t    pResponse = 0;
	/* Structure for external interrupt channel configuration */
	struct extint_chan_conf eic_conf;

	//set the type of interrupt (Rising Edge) generated on the HOST_IRQ line after an event detected by IA61x
	if(!IA61x_uart_cmd(SET_EVENT_RESP_CMD,IA61x_INT_RISE_EDGE,1, &pResponse))
	{
		if(pResponse != (uint16_t)IA61x_INT_RISE_EDGE)
		return (ERROR);
	}


	/* Configure the external interrupt channel */
	extint_chan_get_config_defaults(&eic_conf);
	eic_conf.gpio_pin           = IA61x_EIC_PIN;
	eic_conf.gpio_pin_mux       = IA61x_EIC_PIN_MUX;
	eic_conf.gpio_pin_pull      = EXTINT_PULL_UP;
	eic_conf.detection_criteria = EXTINT_DETECT_RISING;
	eic_conf.filter_input_signal= true;
	extint_chan_set_config(IA61x_EIC_CHANNEL, &eic_conf);

	/* Register and enable the callback function for External interrupt from IA61x*/
	extint_register_callback(IA61x_irq_callback, IA61x_EIC_CHANNEL, EXTINT_CALLBACK_TYPE_DETECT);
	extint_chan_enable_callback(IA61x_EIC_CHANNEL, EXTINT_CALLBACK_TYPE_DETECT);

	return (SUCCESS);
}


static int32_t IA61x_uart_rdb(uint8_t algo_id, uint8_t block_type, uint8_t *data, uint32_t *size)
{
	uint16_t param = algo_id;
	uint16_t response;
	int32_t ret;
	
	param = (param << 8) | block_type;
	
	ret = IA61x_uart_cmd(RDB_CMD, param, 1, &response);
	if (ret != CMD_SUCCESS)
		return ret;
	
	//printf("RDB Len: %u\n", response);
	if (response == 0)
	{
		*size = 0;
		return 0;
	}
	
	if (response > *size)
	{
		ret = IA61x_uart_get(data, *size);
		if (ret == 0)
		{
			uint32_t wdata;
			uint32_t pending = response - *size;
			while (pending > 0)
			{
				// RDB data size is multiple of 4 bytes.
				IA61x_uart_get((uint8_t*)&wdata, 4);
				pending -= 4;
			}
		}
	}
	else
	{
		ret = IA61x_uart_get(data, response);
		*size = response;
	}
	
#if 0 //swap required only for SPI
	uint32_t* p = (uint32_t*) data;
	for (uint32_t i = 0; i < (*size / 4); i++)
	{
		p[i] = swap_ui32(p[i]);
	}
#endif

	return ret;
}

/*******************************************************************************************************
 * @fn      IA61x_uart_VoiceWake()
 *
 * @brief   Stop the route. Configure algorithm parameters. Select Route 6 and put IA61x in Low Power mode
 *
 * @param   none
 *
 * @retval  error   returns non zero value if any of the command fails
 *
 *******************************************************************************************************/
static int32_t IA61x_uart_VoiceWake(void)
{

    uint32_t error = 0;
    uint16_t pResponse;
	int32_t ret;
	int attempt = 2;
	
	while (attempt > 0)
	{
		attempt--;
		
		if (error)
		{
			error = 0;
			recycle_uart();
		}
	

		//Send Sync command first to make sure that IA61x is awake. Ignore the response.
		ret = IA61x_uart_cmd(SYNC_CMD, EMPTY_DATA, 1, &pResponse);
		if (ret != 0)
		{
			error++;
			continue;
		}
		
		delay_ms(1);

		uint16_t preset = PRESET_VALUE(2);
		
		if(!IA61x_uart_cmd(SET_PRESET_CMD | CMD_NO_RESP_MASK,preset,1, &pResponse))
		{
			if(pResponse != preset)
			{
				error++;	
				continue;
			}
		}
		else
		{
			error++;
			continue;
		}
		
		delay_ms(1);
		break;
	}
	
	return (error);
}

/*Dummy function for future implementation*/
static int32_t IA61x_uart_close(void)
{
    return (0);
}

/*******************************************************************************************************
 * @fn      IA61x_uart_wait_keyword
 *
 * @brief   Wait for Keyword, command or timeout to be detected
 *
 * @param   delay   Delay value to wait for keyword detection
 *
 * @retval  response[3]         Keyword ID for the detected keyword or command word
 * @retval  NO_KWD_DETECTED     0 if no keyword is detected by IA61x
 *
 *******************************************************************************************************/
static int32_t IA61x_uart_wait_keyword(uint32_t delay)
{
    const uint8_t GECmd[] =     { 0x80, 0x6d, 0x00, 0x00 };
    uint8_t response[4];
	
	//Check if IA61x event detection interrupt is generated!!
	//This is the indication that there is either Key word, command or timeout event*/
	while(!interrupt_flag)
	{
		delay_ms(delay); //wait for sometime before checking for interrupt again. Here host can go in sleep mode until the interrupt occurs
	}

	interrupt_flag = 0;
	
	response[3] = 0;
    if (!delay)
    {   /*Check if IA61x has send any data on the UART line. This is the indication that there is either Key word, command or timeout event*/
        if (usart_read_buffer_wait(&usart_instance, response, 1) == STATUS_OK)
        {
            usart_read_buffer_wait(&usart_instance, &response[1], 3); // Just read additional 3 bytes to flush any remaining data from UART port

            usart_write_buffer_wait(&usart_instance, GECmd, 4); //Send Get Event command to check what event has occur
            if (usart_read_buffer_wait(&usart_instance, response, 4) != STATUS_OK) response[3] = 0;
        }
    }
    else
    {
        // flush until uart timeout.
		while (usart_read_buffer_wait(&usart_instance, response, 4) != STATUS_ERR_TIMEOUT);
		
        usart_write_buffer_wait(&usart_instance, GECmd, 4);
        if (usart_read_buffer_wait(&usart_instance, response, 4) != STATUS_OK) response[3] = 0;
    }

    if ((response[0] == GECmd[0]) && (response[1] == GECmd[1]))
    {
        if (response[3])
        {
            return (response[3]);
        }
    }
	
	return (NO_KWD_DETECTED);
}


/*******************************************************************************************************
 * @fn      IA61x_samd21_vq_uart_init
 *
 * @brief   This function Power cycles IA61x and once the Host interface is auto detected
 *          by IA61x, it resets the UART baudrate to 460800 and initialize IA61x instance so 
 *          Host program can access IA61x APIs.
 *
 *              |                                                   |
 *          Host|->>----------------Power OFF/ON---------------->>->| IA61x
 *              |                   Delay 20 mSec                   |
 *              |->>---------Send Auto Baud - 00 00 ------------>>->|
 *              |->>----------Send Sync Byte - B7 -------------->>->|
 *              |-<<------------Ack Sync Byte - B7 -------------<<->|
 *              |->>---------Send Rate Request - 80 10 12 00---->>->|
 *              |     Reset the SAMD21 UART port with new Baud      |
 *              |-<<-------Ack Rate Request -   80 10 12 00-----<<->|
 *              |->>-----------Send Sync Byte - B7 ------------->>->|
 *              |-<<-----------Ack Sync Byte - B7 --------------<<->|
 *
 * @param   IA61x   IA61x interface instance pointer to be initialized
 *
 * @retval  CMD_FAILED      If any command fails
 * @retval  SUCCESS         If IA61x boot process is successful
 *
 *******************************************************************************************************/
int32_t IA61x_samd21_vq_uart_init(IA61x_instance *IA61x)
{
    const uint8_t SetRate460800[]   =       { 0x80, 0x19, 0x12, 0x00 };
    const uint8_t quadzero[]        =       { 0x00, 0x00, 0x00, 0x00 };
    const uint8_t b7[]              =       { 0xb7 };
    uint8_t sRetVal[4];
    uint8_t cRetVal;


    IA61x_samd21_vq_uart_uninit();

    my_usart_uninit();

    /*Power cycle IA61x so Bootloader goes into Auto-detect state to detect the host controller interface*/
    port_pin_set_output_level(IA61x_LDO_ENABLE, 0 ); /* Make sure it's low */
    delay_ms(1);
    port_pin_set_output_level(IA61x_LDO_ENABLE, 1 );  /* Bring LDO Enable High */
    delay_ms(20);

    my_usart_init(115200); /* Configure the USART to talk to the boot loader */
    delay_ms(1);

    /* start by sending a 0x00 0x00 over the UART to tell IA61x the baud you are using */
    usart_write_buffer_wait(&usart_instance, quadzero, 2);

    /*Send Sync Byte to IA61x*/
    delay_ms(1);
    usart_write_buffer_wait(&usart_instance, b7, 1);
    delay_ms(1);
    while (usart_read_buffer_wait(&usart_instance, &cRetVal, 1) == STATUS_OK) ;

    if (cRetVal != 0xb7)
        return (CMD_FAILED);
#if 1
    /*Set UART Baud rate to 460800*/
    usart_write_buffer_wait(&usart_instance, SetRate460800, 4);
    delay_ms(1);

    /*Reinitialize SAMD21 UART port for new Baudrate*/
    my_usart_uninit();
    my_usart_init(460800);
    delay_ms(1);

    usart_write_buffer_wait(&usart_instance, quadzero, 4);
    delay_ms(1);
    usart_read_buffer_wait(&usart_instance, sRetVal, 4);

    if ((sRetVal[0] != SetRate460800[0]) |(sRetVal[1] != SetRate460800[1]) |
            (sRetVal[2] != SetRate460800[2]) | (sRetVal[3] != SetRate460800[3]))
        return (CMD_FAILED);
#endif //disable 460.8K baud

    /*Send Sync Byte to IA61x to confirm new Baud rate*/
    delay_ms(1);
    usart_write_buffer_wait(&usart_instance, b7, 1);
    delay_ms(1);
    while (usart_read_buffer_wait(&usart_instance, &cRetVal, 1) == STATUS_OK) ;
    if (cRetVal != 0xb7)
        return (CMD_FAILED);

    delay_ms(10);

    /*Initialize IA61x API Handle*/
    IA61x->download_config  = IA61x_uart_download_config;
    IA61x->download_program = IA61x_uart_download_firmware;
    IA61x->download_keyword = IA61x_uart_download_keyword;
    IA61x->VoiceWake        = IA61x_uart_VoiceWake;
    IA61x->close            = IA61x_uart_close;
    IA61x->wait_keyword     = IA61x_uart_wait_keyword;
    IA61x->cmd              = IA61x_uart_cmd;
    IA61x->get              = IA61x_uart_get;
    IA61x->put              = IA61x_uart_put;
	IA61x->rdb				= IA61x_uart_rdb;

    return (SUCCESS);
}

/*******************************************************************************************************
 * @fn      IA61x_samd21_vq_uart_uninit
 *
 * @brief   Un-initialize SAMD21 UART port
 *
 * @param   none
 *
 * @retval  SUCCESS     Returns success = 0
 *
 *******************************************************************************************************/
int32_t IA61x_samd21_vq_uart_uninit(void)
{
    struct port_config pin_conf;

    port_get_config_defaults(&pin_conf);

    pin_conf.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(IA61x_LDO_ENABLE, &pin_conf);
    port_pin_set_output_level(IA61x_LDO_ENABLE, 0 ); // hold in reset

    return (SUCCESS);
}

static void recycle_uart(void)
{
	delay_ms(1000);
	
	usart_reset(&usart_instance);
	my_usart_uninit();
	my_usart_init(460800);
	
	delay_ms(1);
}


#endif /* ifdef IA61x_SAMD21_VQ_UART */