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

#include <asf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "IA61x.h"
#include "nvm_util.h"

#include "trill_host.h"
#include "provision.h"

#define STR_EXAMPLE        "Wake keyword example"
#define WAKE_KWD_STRING     "Data Detected\r\n"
#define KEYWORD1            "Hello VoiceQ:"

#define MAX_RDB_BLOCK_SIZE (TRILL_BLOCK_HEADER_LEN + TRILL_HOST_MAX_PAYLOAD_SIZE + 4) // + padding

#define COMPILE_TIME_LICENSE "copy-paste license string here."

// Set here or from compiler properties.
#undef USE_COMPILE_TIME_LICENSE
// Set here or from compiler properties.
//#define USE_COMPILE_TIME_LICENSE

#define EOL    "\n"
#define HEADER_STRING \
    "Trillbit Data over Sound Demo with Knowles IA61x SmartMic"EOL \
    "-- Demo Version: 1.1 --"EOL \
	"-- Board: "BOARD_NAME " --"EOL \
	"-- Compiled: "__DATE__ " "__TIME__ " --"EOL


static uint8_t payload_buffer[MAX_RDB_BLOCK_SIZE];
/** UART module for debug. */
static struct usart_module cdc_uart_module;

static trill_host_handle_t trill_host_handle;
static trill_host_init_parameters_t trill_host_init_params;

#ifndef USE_COMPILE_TIME_LICENSE
static prov_parameters_t prov_params;
static char lic_buffer[TRILL_HOST_LICENSE_BUFFER_SIZE];
static int provision_host(void);
#endif
static int start_sdk(const char* license);

/**Initialize EDBG UART port**/
void samd21_vcp_uart_init(void);

/*Infinite loop if any HW API reports error*/
void HW_Error( void );


/***************************************************************************
 * @fn          config_led
 *
 * @brief       Configure LED0, turn it off
 *
 * @param       none
 *
 * @retval      none
 *
 ****************************************************************************/
static void config_led(void)
{
    struct port_config pin_conf;

    port_get_config_defaults(&pin_conf);

    pin_conf.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(LED_0_PIN, &pin_conf);
    port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
}


/***************************************************************************
 * @fn          blink_led
 *
 * @brief       Toggle LED for number of times
 *
 * @param       count   LED0 Blink count
 *
 * @retval      none
 *
 ****************************************************************************/
static void blink_led(uint32_t count)
{
    while (count--)
    {
        port_pin_toggle_output_level(LED_0_PIN);
        delay_ms(150);
        port_pin_toggle_output_level(LED_0_PIN);
        delay_ms(150);
    }
}


/***************************************************************************
 * @fn          samd21_vcp_uart_init
 *
 * @brief       Configure SAMD21 EDBG UART console.
 *
 * @param       none
 *
 * @retval      none
 *
 ****************************************************************************/
void samd21_vcp_uart_init(void)
{
    struct  usart_config usart_conf;

    usart_get_config_defaults(&usart_conf);
    usart_conf.mux_setting  = EDBG_CDC_SERCOM_MUX_SETTING;
    usart_conf.pinmux_pad0  = EDBG_CDC_SERCOM_PINMUX_PAD0;
    usart_conf.pinmux_pad1  = EDBG_CDC_SERCOM_PINMUX_PAD1;
    usart_conf.pinmux_pad2  = EDBG_CDC_SERCOM_PINMUX_PAD2;
    usart_conf.pinmux_pad3  = EDBG_CDC_SERCOM_PINMUX_PAD3;
    usart_conf.baudrate     = 115200;

    //usart_init(&cdc_uart_module, EDBG_CDC_MODULE, &usart_conf);
    stdio_serial_init(&cdc_uart_module, EDBG_CDC_MODULE, &usart_conf);
    usart_enable(&cdc_uart_module);
}

/***************************************************************************
 * @fn          getAlgorithmParamInfo
 *
 * @brief       Read required Algorithm parameters and print
 *
 * @param       IA61x   IA61x interface handle
 *
 * @retval      none
 *
 ****************************************************************************/
#if 0
static void getAlgorithmParamInfo(IA61x_instance *IA61x)
{
    uint16_t pResponse;

#if  IA611_VOICE_ID
    IA61x->cmd(GET_ALGO_PARAM,VID_SENSITIVITY_PARAM,1, &pResponse);
    printf("Voice ID Detection Sensitivity:  %2x\r\n", pResponse);
#elif   IA611_UTK
    IA61x->cmd(GET_ALGO_PARAM,UTK_SENSITIVITY_PARAM,1, &pResponse);
    printf("UTK Keyword Detection Sensitivity:  %2x\r\n", pResponse);
#else
    IA61x->cmd(GET_ALGO_PARAM,OEM_SENSITIVITY_PARAM,1, &pResponse);
    printf("OEM Keyword Detection Sensitivity:  %2x\r\n", pResponse);
#endif

    IA61x->cmd(GET_DIGITAL_GAIN,END_POINT_ID,1, &pResponse);
    printf("Digital Gain:  %d db\r\n", pResponse);


}
#endif
/***************************************************************************
 * @fn          HW_Error
 *
 * @brief       Wait in loop if any HW error occurs. Reset SAMD21 board to recover.
 *
 * @param       none
 *
 * @retval      none
 *
 ****************************************************************************/
void HW_Error( void )
{
    printf("HW Error: Reset the board to recover\r\n");
    while (1) ;
}

/****************************************************************************************************
 * @fn      VersionStringCmd
 *          Helper routine for reading version string (0x8020/0x8021) from IA61x device
 *
 * @param   IA61x    : IA61x Instance handle
 * @param   pBuffer  : Buffer to store Version String
 * @param   bufferSz : Size of Buffer
 *
 * @return  Returns CMD_SUCCESS on success, CMD_FAILED/CMD_TIMEOUT on failed cases.
 ***************************************************************************************************/
static int32_t VersionStringCmd(IA61x_instance *IA61x, uint8_t *pBuffer, uint8_t bufferSz, uint16_t algo_id)
{
    int32_t     result  = 0;
    uint16_t    resp    = 0;
    uint8_t     i       = 0;

    if((pBuffer == NULL) | (bufferSz <= 0))
        return (ERROR);

    /* Clear buffer */
    memset(pBuffer, 0, bufferSz);

    /* Send the build string command to get first character */
    result = IA61x->cmd(BUILD_STRING_CMD1,algo_id,1, &resp);
    if (result != CMD_SUCCESS)
    {
        printf("Version CMD Failed!!\r\n");
        return result;
    }

    pBuffer[i++] = (uint8_t)resp;
    bufferSz--;

    /* Get subsequent characters till response is 0 */
    do
    {
        result = IA61x->cmd(BUILD_STRING_CMD2,EMPTY_DATA,1, &resp);
        pBuffer[i++] = (uint8_t)resp;
        bufferSz--;
    } while ( (resp != 0) && (result == CMD_SUCCESS) && (bufferSz > 0) );

    /* Ensure last character in buffer is null terminator (in case buffer was smaller than version string) */
    if (bufferSz == 0)
    {
        i--;
        pBuffer[i] = 0;
    }

    return result;
}

static int uart_tx_api(void* drv, const char* buf, unsigned int size)
{
    enum status_code status;
    (void) drv;

    status = usart_write_buffer_wait(
                &cdc_uart_module,
                (const uint8_t*) buf,
                (uint16_t) size);

    if (status == STATUS_OK)
        return 0;
    
    return -1;
}

static int uart_rx_api(void* drv, char* buf, unsigned int size)
{
    enum status_code status;
    (void) drv;
    
    do
    {
        status = usart_read_buffer_wait(&cdc_uart_module,
		            (uint8_t*) buf, (uint16_t) size);
    } while (status == STATUS_ERR_TIMEOUT);

    if (status == STATUS_OK)
        return 0;
    
    return -1;
}

// Call this function to start the output clock on PB15
static void config_outClk(void)
{
	struct system_pinmux_config mux_conf;
	system_pinmux_get_config_defaults(&mux_conf);
	mux_conf.mux_position = PINMUX_PB13H_GCLK_IO7;
	mux_conf.direction =  SYSTEM_PINMUX_PIN_DIR_OUTPUT;
	system_pinmux_pin_set_config(PIN_PB13H_GCLK_IO7, &mux_conf);
}

static int start_sdk(const char* license)
{
	int ret;
	
	memset(&trill_host_init_params, 0, sizeof(trill_host_init_parameters_t));
	
	trill_host_init_params.sdk_license = license;
	trill_host_init_params.mem_alloc_fn = malloc;
	trill_host_init_params.mem_free_fn = free;
	
	ret = trill_host_init(&trill_host_init_params,
			&trill_host_handle);

	if (ret < 0)
	{
		printf("trill_host_init failed: %d\n", ret);
		return ret;
	}
	
	return 0;
}

#ifndef USE_COMPILE_TIME_LICENSE
static int provision_host(void)
{
	int ret;

	memset(&prov_params, 0, sizeof(prov_parameters_t));
	
	prov_params.uart_drv_obj = NULL;
	prov_params.app_uart_rx_fn = uart_rx_api;
	prov_params.app_uart_tx_fn = uart_tx_api;

	ret = prov_init(&prov_params);
	if (ret < 0)
	{
		printf("prov_init failed: %d\n", ret);
		return ret;
	}
	
	unsigned int size = TRILL_HOST_LICENSE_BUFFER_SIZE;
	printf("\nHost Provisioning Started. Waiting for PC connection over EDBG UART/USB.\n");
	printf("UART Settings: 115200 8-n-1\n");
	ret = prov_run(&prov_params, lic_buffer, &size);
	if (ret < 0)
	{
		printf("prov_run failed: %d\n", ret);
		return ret;
	}
	
	ret = nvm_util_write_lic(lic_buffer, size);
	if (ret < 0)
	{
		printf("nvm_util_write_lic: %d\n", ret);
		return ret;
	}
	
	printf("Stored license in NVM.\n");
	
	return 0;
}
#endif //#ifndef USE_COMPILE_TIME_LICENSE

/***************************************************************************
 * @fn          main
 *
 * @brief       Main application function.
 *              Initialize system, UART console, IA61x board,
 *              download the IA61x firmware and wake keywords.
 *              Wait in loop for keyword to be detected by IA61x.
 *
 * @param       none
 *
 * @retval      SUCCESS     Return 0 at exit
 *
 ****************************************************************************/
int main (void)
{
    IA61x_instance *IA61x;
    int32_t ret;
    //uint16_t pResponse;
    uint8_t versionstring[50];
	uint32_t rpt_count = 0;
    const char* stored_lic;

    /* Initialize the board. */
    system_init();

    /*Initialize the system clock tick counter for delay*/
    delay_init();

    /*Configure LED0 on SAMD21 Xplained Pro board*/
    config_led();

    /*Initialize Debug UART port to enable the debug prints*/
    samd21_vcp_uart_init();

	nvm_util_init();

    /**printf function uses the Virtual com port of the SAMD21 Xplained pro.
    Debug USB port will be detected as virtual com port on the PC. Baudrate is set to 115200**/
    printf(EOL);
	printf(HEADER_STRING);
    printf(IA61X_HOST_INTERFACE);
	printf(EOL);
    
    /* Insert application code here, after the board has been initialized. */
	printf("MCU/Device ID: %s\n\n", trill_host_get_id());
	
#ifndef USE_COMPILE_TIME_LICENSE
	stored_lic = nvm_util_get_lic();
	if (stored_lic[0] == 0xff)
	{
		printf("Trillbit Host SDK License not found. Starting provisioning...\n");
		ret = provision_host();
		if (ret < 0)
		{
			HW_Error();
		}
	}
#else
	stored_lic = COMPILE_TIME_LICENSE;
#endif
	
	if (stored_lic[0] != 0xff)
	{
		printf("Initializing Trillbit Host SDK\n");
		ret = start_sdk(stored_lic);
		if (ret < 0)
		{
			HW_Error();
		}
	}
	
    /**Initialize SAMD21 USART port and Boot IA61x.
    IA61x auto detects the UART interface.
    Set UART baudrate.  Download the Config file **/
    IA61x = IA61x_init(); /**Returns IA61x interface instance to access API functions**/
	if (IA61x == NULL)
	{
		HW_Error(); //If failed to create the IA61x interface handle then jump to error loop and wait for HW reset.
	}

    
    ret = IA61x->download_config(); /**Download IA61x Firmvare binary**/
    if (ret != CMD_SUCCESS)
    {

        printf("Error: Config Download Failed!!!\r\n");
        HW_Error();
    }
    else
    {
        printf("IA61x Config file Downloaded.\r\n");
    }

    ret = IA61x->download_program(); /**Download IA61x Firmvare binary**/
    if (ret != SYNC_RESP_NORM)
    {
        printf("Error: Firmware Download Failed!!!\r\n");
        HW_Error();
    }
    else printf("IA61x Firmware Downloaded.\r\n");
		
	if(IA61x->VoiceWake())       // Stop --> Set --> Restart the route
        HW_Error();         //if error then jump to HW error loop
    printf("IA61x Route Setup Completed.\r\n");
		
	config_outClk();
	printf("IA61x External Clock Started.\r\n");
		
	//Print IA61x Firmware version information
    printf(EOL);
	ret = VersionStringCmd(IA61x, versionstring, sizeof(versionstring), TRILL_IA61x_ALGO_ID);
	printf("Trillbit IA61x Algorithm Version: %s\r\n",versionstring);
	ret = VersionStringCmd(IA61x, versionstring, sizeof(versionstring), 0);
	printf("Knowles IA61x Firmware Version: %s\r\n",versionstring);
	printf(EOL);
        
    printf("Host is ready for authentication. Waiting for IA61x...\r\n");
    blink_led(1);

	while (1)
    {
        uint32_t buf_size;
		
		int kw = IA61x->wait_keyword(WAIT_KWD_DELAY); 
		
        switch (kw)
        {
            case TRILL_KW_HOST_AUTH_NEEDED:
                printf("Trillbit IA61x Algorithm needs authentication. Responding...\n");
                buf_size = MAX_RDB_BLOCK_SIZE;
                ret = IA61x->rdb(TRILL_IA61x_ALGO_ID, 1, payload_buffer, &buf_size);
                if ((ret == 0) && (buf_size > 1))
                {
                    ret = trill_host_handle_auth(trill_host_handle,
                            payload_buffer,
                            buf_size);
                    if (ret < 0)
                    {
                        printf("trill_host_handle_auth failed = %ld\n", ret);
                        break;
                    }

                    ret = IA61x->download_keyword((uint16_t *)payload_buffer, buf_size);
                    if (ret != 0)
                    {
                        printf("download_keyword failed = %ld\n", ret);
                        break;
                    }
                }
                break;
            case TRILL_KW_HOST_AUTH_PASS:
                printf("Trillbit IA61x Algorithm is ready. Listening for data over sound...\r\n");
                printf(EOL);
				blink_led(4);
                break;
            case TRILL_KW_PAYLOAD_AVAILABLE:
                buf_size = MAX_RDB_BLOCK_SIZE;
                printf("%lu) %s", ++rpt_count, WAKE_KWD_STRING);
                ret = IA61x->rdb(TRILL_IA61x_ALGO_ID, 1, payload_buffer, &buf_size);
                if ((ret == 0) && (buf_size > 1))
                {
                    buf_size = payload_buffer[TRILL_BLOCK_PAYLOAD_LEN_INDEX] + 1;
                    printf("Payload (%lu): %.*s\n", 
                        buf_size, 
                        (int)buf_size, 
                        &payload_buffer[TRILL_BLOCK_PAYLOAD_INDEX]);
                }
                break;
			case NO_KWD_DETECTED:
				break;
            default:
                printf("Unknown keyword: %d\n", kw);
                break;
        }
			
        if (kw != 0)
		{
			IA61x->VoiceWake(); //Reset the route and wait for wake keyword
		}
    } //while (1)
    
    return (SUCCESS);
} //main (void)
