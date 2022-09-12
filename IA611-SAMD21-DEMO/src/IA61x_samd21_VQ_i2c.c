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
#ifdef IA61x_SAMD21_VQ_I2C

# include <asf.h>
# include <string.h>
# include "IA61x_samd21_VQ_i2c.h"

#if IA611_VOICE_ID
    #include "SysConfig6secTO_vid.h"    /*Sysconfig with 1 Voice ID + 3 OEM commands*/
#elif IA611_UTK
    #include "SysConfig6secTO_utk.h"    /*Sysconfig with 1 UTK + 3 OEM commands*/
#else
    # include "SysConfig6secTO.h"       /*4 Command Sys Config with 6 Sec TIMEOUT*/
#endif

# include "IA611_FW_Bin_I2C.h"         /* Firmware Binary for I2C interface */
/*********************************************************************************/
// private
/*********************************************************************************/
struct i2c_master_module i2c_master_instance;
volatile uint8_t interrupt_flag = 0;

/***************************************************************************
 * @fn      IA61x_i2c_get()
 *
 * @brief   Read data from UART port
 *
 * @param   pData    Buffer to receive data
 * @param   size    Size of data to be received
 *
 * @retval  none
 *
 ****************************************************************************/
static int32_t IA61x_i2c_get(uint8_t *pData, uint32_t size)
{
    uint16_t timeout = 0;
    uint32_t retVal = STATUS_OK;
    struct i2c_master_packet packet = {
        .address     = SLAVE_ADDRESS,
        .data_length = size,
        .data        = pData,
        .ten_bit_address = false,
        .high_speed      = false,
        .hs_master_code  = 0x0,
    };

    while (i2c_master_read_packet_wait(&i2c_master_instance, &packet) != STATUS_OK) 
    {
        /* Increment timeout counter and check if timed out. */
        if (timeout++ == I2C_TIMEOUT) 
        {
            retVal = STATUS_ERR_TIMEOUT;
            break;
        }
    }
    
    return retVal;
}

/***************************************************************************
 * @fn      IA61x_i2c_put()
 *
 * @brief   Write data to I2C port
 *
 * @param   pData    Send dat Buffer
 * @param   size    Size of data to be sent
 *
 * @retval  none
 *
 ****************************************************************************/
static int32_t IA61x_i2c_put(uint8_t *pData, uint32_t size)
{
    uint16_t timeout = 0;
    uint32_t retVal = STATUS_OK;
    struct i2c_master_packet packet = {
        .address     = SLAVE_ADDRESS,
        .data_length = size,
        .data        = pData,
        .ten_bit_address = false,
        .high_speed      = false,
        .hs_master_code  = 0x0,
    };

    while (i2c_master_write_packet_wait(&i2c_master_instance, &packet) != STATUS_OK) 
    {
        /* Increment timeout counter and check if timed out. */
        if (timeout++ == I2C_TIMEOUT) 
        {
            retVal = STATUS_ERR_TIMEOUT;
            break;
        }
    }

    return retVal;
}

/*******************************************************************************************************
 * @fn      IA61x_i2c_cmd()
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
static int32_t IA61x_i2c_cmd(uint16_t cmdWord, uint16_t dataWord, uint32_t timeout, uint16_t *pResponse)
{
    uint32_t data = 0;
    int32_t cmdResult = CMD_SUCCESS;
    union _tmp {
        uint16_t word[2];
        uint8_t byte[4];
    } tmp;

    tmp.word[0] = cmdWord;
    tmp.word[1] = dataWord;

    //Command Word first and then Data word
    data = tmp.byte[1] | tmp.byte[0]<<8 | tmp.byte[3]<<16 | tmp.byte[2] << 24;
    IA61x_i2c_put((uint8_t *)&data, 4);

    

    //if timeout is 0 then no need to read the response else try reading the data multiple times
    while(timeout--)
    {
        delay_ms(5);// Delay for Firmware to prepare the response.

        //Read response 
        IA61x_i2c_get(tmp.byte, 4);

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
 * @retval  i           UART Status code from IA61x. 0x02 indicates successful Firmware download.
 *
 *******************************************************************************************************/
 static int32_t IA61x_download_bin(uint8_t *pData, uint32_t size)
{
    const uint8_t Load01[] = { 0x1 };
    uint8_t cRetVal = 0;
    uint32_t iCount = 0;

    //Send Boot command to IA61x and check for the Boot ACK
    if(IA61x_i2c_put((uint8_t *)Load01, 1) == STATUS_OK)
    {
        if((IA61x_i2c_get(&cRetVal, 1) != STATUS_OK) || (cRetVal != 1))
        {
            return (CMD_FAILED);
        }
    }
    else return (CMD_FAILED);

    iCount = 0;
    while (iCount < size) 
    {
        IA61x_i2c_put(&pData[iCount], ((size - iCount >= 0xFFFF) ? 0xFFFF : (size - iCount)));
        iCount += 0xFFFF;
    }

    return (CMD_SUCCESS);
}

/*******************************************************************************************************
 * @fn      my_i2c_init()
 *
 * @brief   Initialize SAMD21 i2c port for IA61x interface
 *
 * @param   none
 *
 * @retval  none
 *
 *******************************************************************************************************/
static void my_i2c_init(void)
{

    /* Initialize config structure and software module. */
    struct i2c_master_config config_i2c_master;

    i2c_master_get_config_defaults(&config_i2c_master);

    /* Change buffer timeout to something longer. */
    config_i2c_master.buffer_timeout = 10000;
    config_i2c_master.baud_rate = I2C_MASTER_BAUD_RATE_400KHZ; 
    //config_i2c_master.transfer_speed = I2C_MASTER_SPEED_FAST_MODE_PLUS;

    /* Initialize and enable device with config. */
    i2c_master_init(&i2c_master_instance, CONF_I2C_MASTER_MODULE, &config_i2c_master);
    
    i2c_master_enable(&i2c_master_instance);
}


/*******************************************************************************************************
 * @fn      my_i2c_uninit()
 *
 * @brief   Uninitialize SAMD21 I2C instance
 *
 * @param   none
 *
 * @retval  none
 *
 *******************************************************************************************************/
/*static void my_i2c_uninit(void)
{
    i2c_master_disable(&i2c_master_instance);
}*/

/*******************************************************************************************************
 * @fn      IA61x_i2c_download_config()
 *
 * @brief   Download Sysconfig file to IA61x
 *
 * @param   none
 *
 * @retval  CMD_FAILED  Command Failed Error
 * @retval  CMD_SUCCESS Command execution successful
 *
 *******************************************************************************************************/
static int32_t IA61x_i2c_download_config(void)
{
    uint32_t iRetVal;

    iRetVal = IA61x_download_bin((uint8_t *)SCFG, sizeof(SCFG));

    return (iRetVal);
}

/*******************************************************************************************************
 * @fn      I2C_Irq_callback
 *
 * @brief   Callback function invoked by the eternal interrupt when IA61x detects the keyword or command.
 *
 * @param   none
 *
 * @retval  none
 *
 *******************************************************************************************************/
static void I2C_Irq_callback(void)
{
    interrupt_flag = true;
    //extint_chan_clear_detected(I2C_EIC_CHANNEL); 
}

/*******************************************************************************************************
 * @fn      IA61x_samd21_vq_i2c_reg_IRQ
 *
 * @brief   Configure Host External interrupt and register the callback function. Setup rising edge interrupt
            on the IA61x for keyword/cmmand detection event
 *
 * @param   none
 *
 * @retval  ERROR   In case of Set Event Response Command fails 
 * @retval  SUCCESS In case of no error
 *
 *******************************************************************************************************/
static uint32_t IA61x_samd21_vq_i2c_reg_IRQ(void)
{
    uint16_t    pResponse = 0;
    /* Structure for external interrupt channel configuration */
    struct extint_chan_conf eic_conf;

    //set the type of interrupt (Rising Edge) generated on the HOST_IRQ line after an event detected by IA61x
    if(!IA61x_i2c_cmd(SET_EVENT_RESP_CMD,IA61x_INT_RISE_EDGE,1, &pResponse))
    {
        if(pResponse != (uint16_t)IA61x_INT_RISE_EDGE)
        return (ERROR);
    }


    /* Configure the external interrupt channel */
    extint_chan_get_config_defaults(&eic_conf);
    eic_conf.gpio_pin           = I2C_EIC_PIN;
    eic_conf.gpio_pin_mux       = I2C_EIC_PIN_MUX;
    eic_conf.gpio_pin_pull      = EXTINT_PULL_UP;
    eic_conf.detection_criteria = EXTINT_DETECT_RISING;
    eic_conf.filter_input_signal= true;
    extint_chan_set_config(I2C_EIC_CHANNEL, &eic_conf);
    /* Register and enable the callback function */
    extint_register_callback(I2C_Irq_callback, I2C_EIC_CHANNEL, EXTINT_CALLBACK_TYPE_DETECT);
    extint_chan_enable_callback(I2C_EIC_CHANNEL, EXTINT_CALLBACK_TYPE_DETECT);

    return (SUCCESS);
}

/*******************************************************************************************************
 * @fn      IA61x_i2c_download_firmware()
 *
 * @brief   Download IA61x Firmware binary to IA61x
 *
 * @param   none
 *
 * @retval  pResponse   response to Sync command
 * @retval  CMD_FAILED Command execution failed
 *
 *******************************************************************************************************/
static int32_t IA61x_i2c_download_firmware(void)
{
    uint32_t iRetvalue;
    uint16_t pResponse;

    iRetvalue = IA61x_download_bin((uint8_t *)VQ_Bin, sizeof(VQ_Bin));
    delay_ms(35); //wait for firmware to initialize. minimum 30 mS delay

    //if FW download is success then ping firmware and confirm that FW is up and running
    if (iRetvalue == CMD_SUCCESS) 
    {
        if(!IA61x_i2c_cmd(SYNC_CMD,EMPTY_DATA,1, &pResponse))
        {
            //Firmware is up and running so now set the IRQ for Event detection on Host and IA61x.
            if(!IA61x_samd21_vq_i2c_reg_IRQ())
                return(pResponse);
        }
    }

    return (CMD_FAILED);
}


/*******************************************************************************************************
 * @fn      IA61x_i2c_download_keyword()
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

static int32_t IA61x_i2c_download_keyword(uint16_t *data, uint16_t size)
{
    uint16_t Response = 0;
    uint16_t BlockCount = 0;
    uint32_t dataindex = 0;
    uint16_t blockindex = 0;
    uint16_t SEQ = 0;
    uint16_t spacket_g = 0;
    uint8_t inbuf2[4];

    /* --------------------------------------------- */
    /* -- BEGIN Creating Header info for STX MODE -- */
    /* --------------------------------------------- */
    /* generate the length */
    spacket_g = (uint16_t)((size / WDB_SIZE_NO_HEADER));
    spacket_g = ((size % WDB_SIZE_NO_HEADER) > 0) ? (spacket_g + 1) : spacket_g;

    //Send Write Data Block command
    IA61x_i2c_cmd(WDB_CMD,size,1, &Response);

    delay_us(100);

#if IA611_VOICE_ID       //No need to add the keyword ID to the Voice ID reference model file.
    if((DownloadNumber == 1) && (Ref_model == 0))
        SEQ = data[1];
    else
#endif
    SEQ = data[1] + (DownloadNumber<<4); /**Set the Keyword sequence number in header bit 4 -7**/

    /* ----------------Send the OEM Model --------------------------------------------*/
    for (dataindex = 2; dataindex < (size / 2); )
    {
        
        delay_us(100);

        //Send Block Header including updated block sequence number
        IA61x_i2c_put((uint8_t *)&data[0], 2);
        IA61x_i2c_put((uint8_t *)&SEQ, 2);

        //Send 512 Bytes block to IA61x
        for (blockindex = 2; blockindex < (WDB_SIZE/2); blockindex++)
        {
            if (dataindex < (size / 2))
            {
                IA61x_i2c_put((uint8_t *)&data[dataindex++], 2);
            }
            else //Send extra 0x0000 to Pad the data if it's not 512 byte aligned
            {
                dataindex++;
                IA61x_i2c_put(EMPTY_DATA, 2);
            }
        }

        //Increment block counter and updated the Sequence header
        BlockCount++;
        if ( BlockCount == spacket_g )//for last block set Sequence number to 0xFF
        {
            BlockCount = 0xFF;
        }
        SEQ = SEQ | (BlockCount << 8);

    }

    delay_ms(5); //Wait for sometime for firmware to respond.

    IA61x_i2c_get(inbuf2, 4);

#if IA611_VOICE_ID
    if((DownloadNumber == 1) && (Ref_model == 0))
        Ref_model = 1;
    else
#endif
    DownloadNumber++; /**Increment OEM keyword download sequence number for next keyword**/

    return (inbuf2[3]);
}


/*******************************************************************************************************
 * @fn      IA61x_i2c_VoiceWake()
 *
 * @brief   Stop the route. Configure algorithm parameters. Select Route 6 and put IA61x in Low Power mode
 *
 * @param   none
 *
 * @retval  error   returns non zero value if any of the command fails
 *
 *******************************************************************************************************/
static int32_t IA61x_i2c_VoiceWake(void)
{

    uint32_t error = 0;
    uint16_t pResponse;

    //Send Sync command first to make sure that IA61x is awake. Ignore the response.
    IA61x_i2c_cmd(SYNC_CMD, EMPTY_DATA, 1, &pResponse);
    delay_ms(1);

    //Send Stop Rout command first
    if(!IA61x_i2c_cmd(STOP_ROUTE_CMD,EMPTY_DATA,5, &pResponse))
    {
        if(pResponse != (uint16_t)EMPTY_DATA)
            error++;
    }
    else
        error++;
    delay_ms(1);
    
    //Set Digital gain to 20db
    if(!IA61x_i2c_cmd(SET_DIGITAL_GAIN_CMD,DIGITAL_GAIN_20,1, &pResponse))
    {
        if(pResponse != (uint16_t)DIGITAL_GAIN_20)
            error++;
    }
    else
        error++;
    delay_ms(1);

    //Set Sample Rate to 16K
    if(!IA61x_i2c_cmd(SAMPLE_RATE_CMD,SAMPLE_RATE_16K,1, &pResponse))
    {
        if(pResponse != (uint16_t)SAMPLE_RATE_16K)
        error++;
    }
    else
        error++;
    delay_ms(1);

    //Set Frame Size to 16 mS
    if(!IA61x_i2c_cmd(FRAME_SIZE_CMD,FRAME_SIZE_16MS,1, &pResponse))
    {
        if(pResponse != (uint16_t)FRAME_SIZE_16MS)
        error++;
    }
    else
        error++;
    delay_ms(1);

    //Select Route 6
    if(!IA61x_i2c_cmd(SELECT_ROUTE_CMD,ROUTE_6,1, &pResponse))
    {
        if(pResponse != (uint16_t)ROUTE_6)
        error++;
    }
    else
        error++;
    delay_ms(1);

    //Set Algorithm Parameter: Sensitivity to 5
    if(!IA61x_i2c_cmd(SET_ALGO_PARAM_ID,OEM_SENSITIVITY_PARAM,1, &pResponse))
    {
        if(pResponse == (uint16_t)OEM_SENSITIVITY_PARAM)
        {
            if(!IA61x_i2c_cmd(SET_ALGO_PARAM,OEM_SENSITIVITY_5,1, &pResponse))
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
    if(!IA61x_i2c_cmd(SET_ALGO_PARAM_ID,UTK_SENSITIVITY_PARAM,1, &pResponse))
    {
        if(pResponse == (uint16_t)UTK_SENSITIVITY_PARAM)
        {
            if(!IA61x_i2c_cmd(SET_ALGO_PARAM,UTK_SENSITIVITY_0,1, &pResponse))
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
    if(!IA61x_i2c_cmd(SET_ALGO_PARAM_ID,VID_SENSITIVITY_PARAM,1, &pResponse))
    {
        if(pResponse == (uint16_t)VID_SENSITIVITY_PARAM)
        {
            if(!IA61x_i2c_cmd(SET_ALGO_PARAM,VID_SENSITIVITY_2,1, &pResponse))
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
    if(!IA61x_i2c_cmd(SET_ALGO_PARAM_ID,VS_PROCESSING_MODE_PARAM,1, &pResponse))
    {
        if(pResponse == (uint16_t)VS_PROCESSING_MODE_PARAM)
        {
            if(!IA61x_i2c_cmd(SET_ALGO_PARAM,VS_PROCESSING_MODE_KW,1, &pResponse))
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

/*Dummy function for future implementation*/
static int32_t IA61x_i2c_close(void)
{
    return (0);
}

/*******************************************************************************************************
 * @fn      IA61x_i2c_wait_keyword
 *
 * @brief   Wait for Keyword, command or timeout to be detected
 *
 * @param   delay   Delay value to wait for keyword detection
 *
 * @retval  response[3]         Keyword ID for the detected keyword or command word
 * @retval  NO_KWD_DETECTED     0 if no keyword is detected by IA61x
 *
 *******************************************************************************************************/
static int32_t IA61x_i2c_wait_keyword(uint32_t delay)
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
    if(!IA61x_i2c_cmd(GET_EVENT_ID_CMD, EMPTY_DATA, 2, &response))
    {
        if(response)
            return (0x00FF & response); // Mask off other 
    }    
    
    return (NO_KWD_DETECTED);
}


/*******************************************************************************************************
 * @fn      IA61x_samd21_vq_i2c_init
 *
 * @brief   This function Power cycles IA61x and once the Host interface is auto detected
 *          by IA61x, it sets the I2C interface and initialize IA61x instance so Host program 
 *          can access IA61x APIs.
 *
 *              |                                                   |
 *          Host|->>----------------Power OFF/ON---------------->>->| IA61x
 *              |                   Delay 20 mSec                   |
 *              |->>----------Send Sync Byte - B7 -------------->>->|
 *              |-<<------------Ack Sync Byte - B7 -------------<<->|
 *
 * @param   IA61x   IA61x interface instance pointer to be initialized
 *
 * @retval  CMD_FAILED      If any command fails
 * @retval  SUCCESS         If IA61x boot process is successful
 *
 *******************************************************************************************************/
int32_t IA61x_samd21_vq_i2c_init(IA61x_instance *IA61x)
{
    const uint8_t b7[]              =       { 0xb7 };
    uint8_t cRetVal;
    struct port_config pin_conf;

    IA61x_samd21_vq_i2c_uninit();

    /*Set IA61x I2C Slave Address. 0 0 ==> Sets 0X3E Slave address*/
    port_get_config_defaults(&pin_conf);
    pin_conf.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(I2C_SLAVE_ADD1_PIN, &pin_conf);
    port_pin_set_output_level(I2C_SLAVE_ADD1_PIN, false);
    port_pin_set_config(I2C_SLAVE_ADD2_PIN, &pin_conf);
    port_pin_set_output_level(I2C_SLAVE_ADD2_PIN, false);

    /*Power cycle IA61x so Bootloader goes into Auto-detect state to detect the host controller interface*/
    port_pin_set_output_level(IA61x_LDO_ENABLE, 0 ); /* Make sure it's low */
    delay_ms(1);

    port_pin_set_output_level(IA61x_LDO_ENABLE, 1 );  /* Bring LDO Enable High */
    delay_ms(20);

    my_i2c_init(); /* Configure the I2C to talk to the boot loader */
    delay_ms(1);

    /*Power cycle IA61x so Bootloader goes into Auto-detect state to detect the host controller interface*/
    port_pin_set_output_level(IA61x_LDO_ENABLE, 0 ); /* Make sure it's low */
    delay_ms(1);

    port_pin_set_output_level(IA61x_LDO_ENABLE, 1 );  /* Bring LDO Enable High */
    delay_ms(20);

    /*Send Sync Byte to IA61x*/
    delay_ms(1);
    IA61x_i2c_put((uint8_t *)b7, 1);

    delay_ms(1);
    while (IA61x_i2c_get(&cRetVal, 1) != STATUS_OK) ;

    if (cRetVal != 0xb7)
        return (CMD_FAILED);

    delay_ms(10);

    /*Initialize IA61x API Handle*/
    IA61x->download_config  = IA61x_i2c_download_config;
    IA61x->download_program = IA61x_i2c_download_firmware;
    IA61x->download_keyword = IA61x_i2c_download_keyword;
    IA61x->VoiceWake        = IA61x_i2c_VoiceWake;
    IA61x->close            = IA61x_i2c_close;
    IA61x->wait_keyword     = IA61x_i2c_wait_keyword;
    IA61x->cmd              = IA61x_i2c_cmd;
    IA61x->get              = IA61x_i2c_get;
    IA61x->put              = IA61x_i2c_put;

    return (SUCCESS);
}

/*******************************************************************************************************
 * @fn      IA61x_samd21_vq_i2c_uninit
 *
 * @brief   Un-initialize SAMD21 UART port
 *
 * @param   none
 *
 * @retval  SUCCESS     Returns success = 0
 *
 *******************************************************************************************************/
int32_t IA61x_samd21_vq_i2c_uninit(void)
{
    struct port_config pin_conf;

    port_get_config_defaults(&pin_conf);

    pin_conf.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(IA61x_LDO_ENABLE, &pin_conf);
    port_pin_set_output_level(IA61x_LDO_ENABLE, 0 ); // hold in reset

    return (SUCCESS);
}

#endif /* ifdef IA61x_SAMD21_VQ_UART */