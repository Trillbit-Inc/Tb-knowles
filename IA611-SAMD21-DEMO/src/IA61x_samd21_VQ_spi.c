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
#ifdef IA61x_SAMD21_VQ_SPI

# include <asf.h>
# include <string.h>
# include "IA61x_samd21_VQ_spi.h"

# include "trill_sys_config.h"       /*Trillbit SDK Sys config*/

# include "IA611_FW_Bin_SPI.h"         /* Firmware Binary for SPI interface */
/*********************************************************************************/
// private
/*********************************************************************************/
struct spi_module spi_master_instance;
volatile uint8_t interrupt_flag = 0;
volatile uint8_t rcv_complete_spi_master = 0;
volatile uint8_t tx_complete_spi_master = 0;

static int32_t IA61x_spi_cmd(uint16_t cmdWord, uint16_t dataWord, uint32_t timeout, uint16_t *pResponse);

/***************************************************************************
 * @fn      IA61x_spi_get()
 *
 * @brief   Read data from UART port
 *
 * @param   pData    Buffer to receive data
 * @param   size    Size of data to be received
 *
 * @retval  none
 *
 ****************************************************************************/
static int32_t IA61x_spi_get(uint8_t *pData, uint32_t size)
{
    uint32_t retVal = STATUS_OK;

    port_pin_set_output_level(SPI_EXT_SS, 0 );
    retVal = spi_read_buffer_job(&spi_master_instance,pData,size,0);

    while (!rcv_complete_spi_master){} //Wait until SPI transfer is complete
    rcv_complete_spi_master = false;
    port_pin_set_output_level(SPI_EXT_SS, 1 );

    return retVal;
}

/***************************************************************************
 * @fn      IA61x_spi_put()
 *
 * @brief   Write data to SPI port
 *
 * @param   pData    Send data Buffer
 * @param   size    Size of data to be sent
 *
 * @retval  none
 *
 ****************************************************************************/
static int32_t IA61x_spi_put(uint8_t *pData, uint32_t size)
{
    uint32_t retVal = STATUS_OK;

    port_pin_set_output_level(SPI_EXT_SS, 0 );
    retVal = spi_write_buffer_job(&spi_master_instance,pData,size);

    while (!tx_complete_spi_master){} //Wait until SPI transfer is complete
    tx_complete_spi_master = false;
    port_pin_set_output_level(SPI_EXT_SS, 1 );

    return retVal;
}

static inline uint32_t swap_ui32(uint32_t v)
{
	union {
		uint32_t w;
		uint8_t b[4];
	} data;
	
	data.w = v;
	return (data.b[0] << 24) | (data.b[1] << 16) | (data.b[2] << 8) | data.b[3];
}

/************************************************************************
	@fn      IA61x_spi_rdb()
	@brief	 Read block from the firmware

************************************************************************/
static int32_t IA61x_spi_rdb(uint8_t algo_id, uint8_t block_type, uint8_t *data, uint32_t *size)
{
	
	uint16_t param = algo_id;
	uint16_t response;
	int32_t ret;
	
	param = (param << 8) | block_type;
	
	ret = IA61x_spi_cmd(RDB_CMD, param, 1, &response);
	if (ret != CMD_SUCCESS)
		return ret;
	
	if (response == 0)
	{
		*size = 0;
		return 0;
	}
	
	if (response > *size) 
	{
		ret = IA61x_spi_get(data, *size);
		if (ret == 0)
		{
			uint32_t wdata;
			uint32_t pending = response - *size;
			while (pending > 0)
			{
				// RDB data size is multiple of 4 bytes.
				IA61x_spi_get((uint8_t*)&wdata, 4);
				pending -= 4;
			}
		}
	}
	else
	{
		ret = IA61x_spi_get(data, response);
		*size = response;
	}
	
	uint32_t* p = (uint32_t*) data;
	for (uint32_t i = 0; i < (*size / 4); i++)
	{
		p[i] = swap_ui32(p[i]);
	}
	
	return ret;
}
/*******************************************************************************************************
 * @fn      IA61x_spi_cmd()
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
static int32_t IA61x_spi_cmd(uint16_t cmdWord, uint16_t dataWord, uint32_t timeout, uint16_t *pResponse)
{
    int32_t cmdResult = CMD_SUCCESS;
    uint32_t data = 0;
    union _tmp {
        uint16_t word[2];
        uint8_t byte[4];
    } tmp;

    tmp.word[0] = cmdWord;
    tmp.word[1] = dataWord;

    //Command Word first and then Data word
    data = tmp.byte[1] | tmp.byte[0]<<8 | tmp.byte[3]<<16 | tmp.byte[2] << 24;
    IA61x_spi_put((uint8_t *)&data, 4);

    tmp.word[0] = 0x0000;
    tmp.word[1] = 0x0000;

    //if timeout is 0 then no need to read the response else try reading the data multiple times
    while(timeout--)
    {
        delay_ms(5);// Delay for Firmware to prepare the response.

        //Read response 
        IA61x_spi_get(tmp.byte, 4);

        *pResponse = (tmp.byte[0] << 8) | tmp.byte[1] ;
        //return the second response word if the first response word matches command word
        if(*pResponse == cmdWord)
        {
            *pResponse = (tmp.byte[2] << 8) | tmp.byte[3];
            cmdResult = CMD_SUCCESS;
            break;
        }
        else
        {
            cmdResult = CMD_FAILED;
        }
    }

    return (cmdResult);
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
 *
 *******************************************************************************************************/
 static int32_t IA61x_download_bin(uint8_t *pData, uint32_t size)
{
    const uint8_t Load01[] = { 0x00, 0x00, 0x00, 0x01 };
    uint8_t cRetVal[4];
    uint32_t iCount = 0;

    //Send Boot command to IA61x and check for the Boot ACK
    if(IA61x_spi_put((uint8_t *)Load01, 4) == STATUS_OK)
    {
        if((IA61x_spi_get((uint8_t *)cRetVal, 4) != STATUS_OK) || (cRetVal[3] != 0x01))
        {
            return (CMD_FAILED);
        }
    }
    else return (CMD_FAILED);

    iCount = 0;
    while (iCount < size) 
    {
        IA61x_spi_put(&pData[iCount], ((size - iCount >= 0xFFFF) ? 0xFFFF : (size - iCount)));
        iCount += 0xFFFF;
    }

    return (CMD_SUCCESS);
}

/*******************************************************************************************************
 * @fn      rcv_callback_spi_master
 *
 * @brief   callback function for SPI receive complete event
 *
 * @param   none
 *
 * @retval  none
 *
 *******************************************************************************************************/
static void rcv_callback_spi_master( struct spi_module *const module)
{
    rcv_complete_spi_master = true;
}

/*******************************************************************************************************
 * @fn      tx_callback_spi_master
 *
 * @brief   callback function for SPI transmit complete event
 *
 * @param   none
 *
 * @retval  none
 *
 *******************************************************************************************************/
static void tx_callback_spi_master( struct spi_module *const module)
{
    tx_complete_spi_master = true;
}



/*******************************************************************************************************
 * @fn      my_spi_init()
 *
 * @brief   Initialize SAMD21 spi port for IA61x interface
 *
 * @param   none
 *
 * @retval  none
 *
 *******************************************************************************************************/
static void my_spi_init(void)
{
    struct spi_config config_spi_master;
    
    //Configure SPI SS pin
    struct port_config pin_conf;
    port_get_config_defaults(&pin_conf);
    pin_conf.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(SPI_EXT_SS, &pin_conf);
    port_pin_set_output_level(SPI_EXT_SS, 1 ); // Keep SS high when no SPI operation.



    /* Configure, initialize and enable SERCOM SPI module */
    spi_get_config_defaults(&config_spi_master);
    config_spi_master.mux_setting = CONF_MASTER_MUX_SETTING;
    config_spi_master.pinmux_pad0 = CONF_MASTER_PINMUX_PAD0;
    config_spi_master.pinmux_pad1 = PINMUX_UNUSED;
    config_spi_master.pinmux_pad2 = CONF_MASTER_PINMUX_PAD2;
    config_spi_master.pinmux_pad3 = CONF_MASTER_PINMUX_PAD3;

    /** Mode 1. Leading edge: rising, setup. Trailing edge: falling, sample */
    config_spi_master.transfer_mode = SPI_TRANSFER_MODE_1;
    //config_spi_master.master_slave_select_enable = false;       /*Use external SS*/
    config_spi_master.mode_specific.master.baudrate = 1000000;

    spi_init(&spi_master_instance, CONF_MASTER_SPI_MODULE, &config_spi_master);
    spi_enable(&spi_master_instance);

    //Register and enable SPI callback for the transmit or receive complete event
    spi_register_callback(&spi_master_instance, rcv_callback_spi_master, SPI_CALLBACK_BUFFER_RECEIVED);
    spi_enable_callback(&spi_master_instance, SPI_CALLBACK_BUFFER_RECEIVED);
    spi_register_callback(&spi_master_instance, tx_callback_spi_master, SPI_CALLBACK_BUFFER_TRANSMITTED);
    spi_enable_callback(&spi_master_instance, SPI_CALLBACK_BUFFER_TRANSMITTED);

}


/*******************************************************************************************************
 * @fn      my_spi_uninit()
 *
 * @brief   Un-initialize SAMD21 SPI instance
 *
 * @param   none
 *
 * @retval  none
 *
 *******************************************************************************************************/
/*static void my_spi_uninit(void)
{
    spi_disable(&spi_master_instance);
}*/

/*******************************************************************************************************
 * @fn      IA61x_spi_download_config()
 *
 * @brief   Download Sysconfig file to IA61x
 *
 * @param   none
 *
 * @retval  CMD_FAILED  Command Failed Error
 * @retval  CMD_SUCCESS Command execution successful
 *
 *******************************************************************************************************/
static int32_t IA61x_spi_download_config(void)
{
    uint32_t iRetVal;

    iRetVal = IA61x_download_bin((uint8_t *)SCFG, sizeof(SCFG));

    return (iRetVal);
}

/*******************************************************************************************************
 * @fn      SPI_Irq_callback
 *
 * @brief   Callback function invoked by the eternal interrupt when IA61x detects the keyword or command.
 *
 * @param   none
 *
 * @retval  none
 *
 *******************************************************************************************************/
static void SPI_Irq_callback(void)
{
    interrupt_flag = true;
}

/*******************************************************************************************************
 * @fn      IA61x_samd21_vq_spi_reg_IRQ
 *
 * @brief   Configure Host External interrupt and register the callback function. Setup rising edge interrupt
            on the IA61x for keyword/command detection event
 *
 * @param   none
 *
 * @retval  ERROR   In case of Set Event Response Command fails 
 * @retval  SUCCESS In case of no error
 *
 *******************************************************************************************************/
static uint32_t IA61x_samd21_vq_spi_reg_IRQ(void)
{
    uint16_t    pResponse = 0;
    /* Structure for external interrupt channel configuration */
    struct extint_chan_conf eic_conf;

    //set the type of interrupt (Rising Edge) generated on the HOST_IRQ line after an event detected by IA61x
    if(!IA61x_spi_cmd(SET_EVENT_RESP_CMD,IA61x_INT_RISE_EDGE,1, &pResponse))
    {
        if(pResponse != (uint16_t)IA61x_INT_RISE_EDGE)
        return (ERROR);
    }


    /* Configure the external interrupt channel */
    extint_chan_get_config_defaults(&eic_conf);
    eic_conf.gpio_pin           = SPI_EIC_PIN;
    eic_conf.gpio_pin_mux       = SPI_EIC_PIN_MUX;
    eic_conf.gpio_pin_pull      = EXTINT_PULL_UP;
    eic_conf.detection_criteria = EXTINT_DETECT_RISING;
    eic_conf.filter_input_signal= true;
    extint_chan_set_config(SPI_EIC_CHANNEL, &eic_conf);

    /* Register and enable the callback function for External interrupt from IA61x*/
    extint_register_callback(SPI_Irq_callback, SPI_EIC_CHANNEL, EXTINT_CALLBACK_TYPE_DETECT);
    extint_chan_enable_callback(SPI_EIC_CHANNEL, EXTINT_CALLBACK_TYPE_DETECT);

    return (SUCCESS);
}

/*******************************************************************************************************
 * @fn      IA61x_spi_download_firmware()
 *
 * @brief   Download IA61x Firmware binary to IA61x
 *
 * @param   none
 *
 * @retval  pResponse   response to Sync command
 * @retval  CMD_FAILED Command execution failed
 *
 *******************************************************************************************************/
static int32_t IA61x_spi_download_firmware(void)
{
    uint32_t iRetvalue;
    uint16_t pResponse;
    uint8_t dummyread[4];

    iRetvalue = IA61x_download_bin((uint8_t *)VQ_Bin, sizeof(VQ_Bin));
    delay_ms(35); //wait for firmware to initialize. minimum 30 mS delay

    IA61x_spi_get(dummyread,4);

    //if FW download is success then ping firmware and confirm that FW is up and running
    if (iRetvalue == CMD_SUCCESS) 
    {
        if(!IA61x_spi_cmd(SYNC_CMD,EMPTY_DATA,1, &pResponse))
        {
            //Firmware is up and running so now set the IRQ for Event detection on Host and IA61x.
            if(!IA61x_samd21_vq_spi_reg_IRQ())
                return(pResponse);
		}
    }

    return (CMD_FAILED);
}


/*******************************************************************************************************
 * @fn      IA61x_spi_download_keyword()
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

static int32_t IA61x_spi_download_keyword(uint16_t *data, uint16_t size)
{

    uint16_t BlockCount = 0;
    uint32_t dataindex = 0;
    uint16_t blockindex = 0;
    uint16_t SEQ = 0;
    uint16_t spacket_g = 0;
    uint8_t inbuf2[4];
    uint8_t *pData;
    uint16_t dataremain = 0;
    uint8_t zeropadding[WDB_SIZE_NO_HEADER];
    uint32_t header = 0;

    union _tmp {
    uint16_t word[2];
    uint8_t byte[4];
    } tmp;

    pData = (uint8_t *)data;

    memset(zeropadding,0x00,WDB_SIZE_NO_HEADER);// zeropadding for the last chunk if it is not 512 bytes aligned.

    /* --------------------------------------------- */
    /* -- BEGIN Creating Header info for STX MODE -- */
    /* --------------------------------------------- */
    /* generate the length */
    spacket_g = (uint16_t)(((size-4) / WDB_SIZE_NO_HEADER));
    spacket_g = (((size-4) % WDB_SIZE_NO_HEADER) > 0) ? (spacket_g + 1) : spacket_g;

    //Send Write Data Block command

    tmp.word[0] = WDB_CMD;
    tmp.word[1] = size;
    //Command Word first and then Data word
    header = tmp.byte[1] | tmp.byte[0]<<8 | tmp.byte[3]<<16 | tmp.byte[2] << 24;
    IA61x_spi_put((uint8_t *)&header, 4);


    delay_ms(5);

#if IA611_VOICE_ID       //No need to add the keyword ID to the Voice ID reference model file.
    if((DownloadNumber == 1) && (Ref_model == 0))
        SEQ = data[1];
    else
#endif
    SEQ = data[1] + (DownloadNumber<<4);/**Set the Keyword sequence number in header bit 4 -7**/

    /* ----------------Send the OEM Model --------------------------------------------*/
    for (dataindex = 4; dataindex < size ; )
    {
        
        delay_us(100);

        port_pin_set_output_level(SPI_EXT_SS, 0 ); //make SS line low. SS needs to be asserted for entire 512 byte chunk

        spi_write_buffer_job(&spi_master_instance,(uint8_t *)&data[0], 2);
        while (!tx_complete_spi_master){} //Wait until SPI transfer is complete
        tx_complete_spi_master = false;
        spi_write_buffer_job(&spi_master_instance,(uint8_t *)&SEQ, 2);
        while (!tx_complete_spi_master){} //Wait until SPI transfer is complete
        tx_complete_spi_master = false;

        //Send 512 Bytes block to IA61x
        for (blockindex = 4; blockindex < WDB_SIZE; )
        {
            if (dataindex < size )
            {
                dataremain = size - dataindex; 
                if (WDB_SIZE_NO_HEADER <= dataremain)
                {
                    spi_write_buffer_job(&spi_master_instance,(uint8_t *)&pData[dataindex], WDB_SIZE_NO_HEADER);
                    while (!tx_complete_spi_master){} //Wait until SPI transfer is complete
                    tx_complete_spi_master = false;

                    dataindex = dataindex + WDB_SIZE_NO_HEADER;
                }
                else //Send last chunk with zero padding. 
                {
                    spi_write_buffer_job(&spi_master_instance,(uint8_t *)&pData[dataindex], dataremain);
                    while (!tx_complete_spi_master){} //Wait until SPI transfer is complete
                    tx_complete_spi_master = false;
                    spi_write_buffer_job(&spi_master_instance,zeropadding,(WDB_SIZE_NO_HEADER - dataremain));
                    while (!tx_complete_spi_master){} //Wait until SPI transfer is complete
                    tx_complete_spi_master = false;
                    
                    dataindex = dataindex + dataremain;
                }
            }
            else //last block data is written then exit from loop
            {
                break;
            }
            blockindex = blockindex + WDB_SIZE_NO_HEADER;
        }

        //Increment block counter and updated the Sequence header
        BlockCount++;
        if ( BlockCount == (spacket_g-1) )//for last block set Sequence number to 0xFF
        {
            BlockCount = 0xFF;
        }
        SEQ = SEQ | (BlockCount << 8);

        port_pin_set_output_level(SPI_EXT_SS, 1 ); // Reset the SS line to high once 512 byte chunk is sent to IA61x

    }

    delay_ms(5); //Wait for sometime for firmware to respond.

    IA61x_spi_get(inbuf2, 4);

#if IA611_VOICE_ID
    if((DownloadNumber == 1) && (Ref_model == 0))
        Ref_model = 1;
    else
#endif
    DownloadNumber++; /**Increment OEM keyword download sequence number for next keyword**/

    return (inbuf2[3]);
}


/*******************************************************************************************************
 * @fn      IA61x_spi_VoiceWake()
 *
 * @brief   Stop the route. Configure algorithm parameters. Select Route 6 and put IA61x in Low Power mode
 *
 * @param   none
 *
 * @retval  error   returns non zero value if any of the command fails
 *
 *******************************************************************************************************/
#if 0 //disable original code. New one will use Presets.
static int32_t IA61x_spi_VoiceWake(void)
{

    uint32_t error = 0;
    uint16_t pResponse;

    //Send Sync command first to make sure that IA61x is awake. Ignore the response.
    IA61x_spi_cmd(SYNC_CMD, EMPTY_DATA, 1, &pResponse);
    delay_ms(1);

    //
    if(!IA61x_spi_cmd(STOP_ROUTE_CMD,EMPTY_DATA,5, &pResponse))
    {
        if(pResponse != (uint16_t)EMPTY_DATA)
            error++;
    }
    else
        error++;
    delay_ms(1);
    
    //Set Digital gain to 20db
    if(!IA61x_spi_cmd(SET_DIGITAL_GAIN_CMD,DIGITAL_GAIN_20,1, &pResponse))
    {
        if(pResponse != (uint16_t)DIGITAL_GAIN_20)
            error++;
    }
    else
        error++;
    delay_ms(1);

    //Set Sample Rate to 16K
    if(!IA61x_spi_cmd(SAMPLE_RATE_CMD,SAMPLE_RATE_16K,1, &pResponse))
    {
        if(pResponse != (uint16_t)SAMPLE_RATE_16K)
        error++;
    }
    else
        error++;
    delay_ms(1);

    //Set Frame Size to 16 mS
    if(!IA61x_spi_cmd(FRAME_SIZE_CMD,FRAME_SIZE_16MS,1, &pResponse))
    {
        if(pResponse != (uint16_t)FRAME_SIZE_16MS)
        error++;
    }
    else
        error++;
    delay_ms(1);

    //Select Route 6
    if(!IA61x_spi_cmd(SELECT_ROUTE_CMD,ROUTE_6,1, &pResponse))
    {
        if(pResponse != (uint16_t)ROUTE_6)
        error++;
    }
    else
        error++;
    delay_ms(1);

    //Set Algorithm Parameter: Sensitivity to 5
    if(!IA61x_spi_cmd(SET_ALGO_PARAM_ID,OEM_SENSITIVITY_PARAM,1, &pResponse))
    {
        if(pResponse == (uint16_t)OEM_SENSITIVITY_PARAM)
        {
            if(!IA61x_spi_cmd(SET_ALGO_PARAM,OEM_SENSITIVITY_5,1, &pResponse))
            {
                if(pResponse != (uint16_t)OEM_SENSITIVITY_5)
                    error++;
            }
            else
                error++;
        }
        else 
            error++;
    }
    else
        error++;
    delay_ms(1);

#if IA611_UTK
    //Set Algorithm Parameter: Sensitivity to 0 for UTK
    if(!IA61x_spi_cmd(SET_ALGO_PARAM_ID,UTK_SENSITIVITY_PARAM,1, &pResponse))
    {
        if(pResponse == (uint16_t)UTK_SENSITIVITY_PARAM)
        {
            if(!IA61x_spi_cmd(SET_ALGO_PARAM,UTK_SENSITIVITY_0,1, &pResponse))
            {
                if(pResponse != (uint16_t)UTK_SENSITIVITY_0)
                    error++;
            }
            else
                error++;
        }
        else 
            error++;
    }
    else
        error++;
    delay_ms(1);
#endif

#if IA611_VOICE_ID
    //Set Algorithm Parameter: Sensitivity to 2 for VID
    if(!IA61x_spi_cmd(SET_ALGO_PARAM_ID,VID_SENSITIVITY_PARAM,1, &pResponse))
    {
        if(pResponse == (uint16_t)VID_SENSITIVITY_PARAM)
        {
            if(!IA61x_spi_cmd(SET_ALGO_PARAM,VID_SENSITIVITY_2,1, &pResponse))
            {
                if(pResponse != (uint16_t)VID_SENSITIVITY_2)
                    error++;
            }
            else
                error++;
        }
        else 
            error++;
    }
    else
        error++;
    delay_ms(1);
#endif

    //Set Algorithm Parameter: VS Processing Mode to Keyword detection
    if(!IA61x_spi_cmd(SET_ALGO_PARAM_ID,VS_PROCESSING_MODE_PARAM,1, &pResponse))
    {
        if(pResponse == (uint16_t)VS_PROCESSING_MODE_PARAM)
        {
            if(!IA61x_spi_cmd(SET_ALGO_PARAM,VS_PROCESSING_MODE_KW,1, &pResponse))
            {
                if(pResponse != (uint16_t)VS_PROCESSING_MODE_KW)
                error++;
            }
            else
            error++;
        }
        else
        error++;
    }
    else
        error++;
    delay_ms(1);

    return (error);
}
#else
static int32_t IA61x_spi_VoiceWake(void)
{

	uint32_t error = 0;
	uint16_t pResponse;

	//Send Sync command first to make sure that IA61x is awake. Ignore the response.
	IA61x_spi_cmd(SYNC_CMD, EMPTY_DATA, 1, &pResponse);
	delay_ms(1);
#if 0
	if(!IA61x_spi_cmd(STOP_ROUTE_CMD,EMPTY_DATA,5, &pResponse))
	{
		if(pResponse != (uint16_t)EMPTY_DATA)
			error++;
	}
	else
	{
		error++;
	}
	
	delay_ms(1);
#endif
	uint16_t preset = PRESET_VALUE(2);
	
	if(!IA61x_spi_cmd(SET_PRESET_CMD,preset,5, &pResponse))
	{
		if(pResponse != preset)
			error++;
	}
	else
	{
		error++;
	}
		
	delay_ms(1);
	
	return error;
}
#endif //#if 0 //disable original code. New one will use Presets.

/*Dummy function for future implementation*/
static int32_t IA61x_spi_close(void)
{
    return (0);
}

/*******************************************************************************************************
 * @fn      IA61x_spi_wait_keyword
 *
 * @brief   Wait for Keyword, command or timeout to be detected
 *
 * @param   delay   Delay value to wait for keyword detection
 *
 * @retval  response[3]         Keyword ID for the detected keyword or command word
 * @retval  NO_KWD_DETECTED     0 if no keyword is detected by IA61x
 *
 *******************************************************************************************************/
static int32_t IA61x_spi_wait_keyword(uint32_t delay)
{
    uint16_t response = 0;

    //Check if IA61x event detection interrupt is generated!! 
    //This is the indication that there is either Key word, command or timeout event*/
    while(!interrupt_flag)
    {
        delay_ms(delay); //wait for sometime before checking for interrupt again. Here host can go in sleep mode until the interrupt occurs
    }

    interrupt_flag = 0;

    //Send Get Event Command to IA61x to check which event has happened!
    if(!IA61x_spi_cmd(GET_EVENT_ID_CMD, EMPTY_DATA, 1, &response))
    {
        if(response)
            return (0x00FF & response); // Mask off other 
    }    
    
    //if no keyword is detected then it could be the invalid interrupt.
    //In this case need to reset the route and put the IA61x into low power mode
    IA61x_spi_VoiceWake(); //Reset the route and wait for wake keyword
    /************************************************************************/
    /* Trill SDK will always operate in active mode.                                                                     */
    /************************************************************************/
	//IA61x_spi_cmd(LOW_POWER_MODE_CMD,LOW_POWER_MODE_RT6,0, &response); //Put IA61x in Low power mode

    return (NO_KWD_DETECTED);
}


/*******************************************************************************************************
 * @fn      IA61x_samd21_vq_spi_init
 *
 * @brief   This function Power cycles IA61x and once the Host interface is auto detected
 *          by IA61x, it configures the SPI interface and initialize IA61x instance so 
 *          Host program can access IA61x APIs.
 *
 *              |                                                   |
 *          Host|->>----------------Power OFF/ON---------------->>->| IA61x
 *              |                   Delay 20 mSec                   |
 *              |->>----------Send Sync Byte - B7B7B7B7--------->>->|
 *              |-<<------------Ack Sync Byte -B7B7B7B7---------<<-<|
 *
 * @param   IA61x   IA61x interface instance pointer to be initialized
 *
 * @retval  CMD_FAILED      If any command fails
 * @retval  SUCCESS         If IA61x boot process is successful
 *
 *******************************************************************************************************/
int32_t IA61x_samd21_vq_spi_init(IA61x_instance *IA61x)
{
    const uint8_t b7[] = {0xb7, 0xb7, 0xb7, 0xb7};
    uint8_t cRetVal[4];

    IA61x_samd21_vq_spi_uninit();

    /*Power cycle IA61x so Boot loader goes into Auto-detect state to detect the host controller interface*/
    port_pin_set_output_level(IA61x_LDO_ENABLE, 0 ); /* Make sure it's low */
    delay_ms(1);

    my_spi_init(); /* Configure the SPI to talk to the boot loader */
    delay_ms(1);

    port_pin_set_output_level(IA61x_LDO_ENABLE, 1 );  /* Bring LDO Enable High */
    delay_ms(20);

    /*Send Sync Byte to IA61x*/
    delay_ms(1);
    IA61x_spi_put((uint8_t *)b7, 4);

    delay_ms(1);
    while (IA61x_spi_get(cRetVal, 4) != STATUS_OK) ;

    if (*cRetVal != 0xb7)
        return (CMD_FAILED);

    delay_ms(10);

    /*Initialize IA61x API Handle*/
    IA61x->download_config  = IA61x_spi_download_config;
    IA61x->download_program = IA61x_spi_download_firmware;
    IA61x->download_keyword = IA61x_spi_download_keyword;
    IA61x->VoiceWake        = IA61x_spi_VoiceWake;
    IA61x->close            = IA61x_spi_close;
    IA61x->wait_keyword     = IA61x_spi_wait_keyword;
    IA61x->cmd              = IA61x_spi_cmd;
    IA61x->get              = IA61x_spi_get;
    IA61x->put              = IA61x_spi_put;
	IA61x->rdb				= IA61x_spi_rdb;

    return (SUCCESS);
}

/*******************************************************************************************************
 * @fn      IA61x_samd21_vq_spi_uninit
 *
 * @brief   Un-initialize SAMD21 SPI port
 *
 * @param   none
 *
 * @retval  SUCCESS     Returns success = 0
 *
 *******************************************************************************************************/
int32_t IA61x_samd21_vq_spi_uninit(void)
{
    struct port_config pin_conf;

    port_get_config_defaults(&pin_conf);

    pin_conf.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(IA61x_LDO_ENABLE, &pin_conf);
    port_pin_set_output_level(IA61x_LDO_ENABLE, 0 ); // hold in reset
    
    return (SUCCESS);
}

#endif /* ifdef IA61x_SAMD21_VQ_UART */