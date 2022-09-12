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

/*-------------------------------------------------------------------------------------------------*\
 |    I N C L U D E   F I L E S
\*-------------------------------------------------------------------------------------------------*/

#ifndef IA61x_H_
#define IA61x_H_

#include <asf.h>
#include "IA61x_config.h"


/*-------------------------------------------------------------------------------------------------*\
 |    C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/

#define SW_VERSION  "-- IA611 SW Release V 2.0.3 --\r\n"

#define CMD_SUCCESS                     (0)
#define CMD_TIMEOUT                     (-1)
#define CMD_FAILED                      (-2)
#define FW_DOWNLOAD_SUCCESS             (2)
#define NO_KWD_DETECTED                 (0)
#define SUCCESS                         (0)
#define ERROR                           (-1)
#define CMD_TIMEOUT_ERROR               0xFF    //IA61x returns 0xFF when Command word timeout occurs.

/* API Commands & Data for IA61x */
#define BOOT_BYTE                       0x01
#define SYNC_CMD                        0x8000  //Sync Command is 8000 0000
#define WDB_CMD                         0x802F
#define RDB_CMD                         0x802E
#define BOOT_CMD                        0x0001
#define BUILD_STRING_CMD1               0x8020  //To get first character
#define BUILD_STRING_CMD2               0x8021  //To get subsequent characters
#define EMPTY_DATA                      0x0000
#define SYNC_RESP_SBL                   0xFFFF
#define SYNC_RESP_NORM                  0x0000
#define CMD_NO_RESP_MASK                0x9000

#define STOP_ROUTE_CMD                  0x8033  //Stop Rout command
#define SAMPLE_RATE_CMD                 0x8030
#define SAMPLE_RATE_16K                 0x0001  // 16k Sample Rate
#define FRAME_SIZE_CMD                  0x8035
#define FRAME_SIZE_16MS                 0x0010  //For Voice Q 16 mS Frame Size
#define BUFF_DATA_FMT_CMD               0x8034
#define BUFF_DATA_FMT_16BIT             0x0002  //16 bit Buffer Data Format
#define SELECT_ROUTE_CMD                0x8032
#define ROUTE_6                         0x0006  //Select Route 6
#define SET_ALGO_PARAM_ID               0x8017
#define SET_ALGO_PARAM                  0x8018
#define GET_ALGO_PARAM                  0x8016
#define OEM_SENSITIVITY_PARAM           0x5008
#define OEM_SENSITIVITY_5               0x0005  //Set the OEM Detection Sensitivity to 5
#define UTK_SENSITIVITY_PARAM           0x5009
#define UTK_SENSITIVITY_0               0x0000  //Set the UTK Detection Sensitivity to 0
#define VID_SENSITIVITY_PARAM           0x500D
#define VID_SENSITIVITY_2               0x0002  //Set the Voice ID Detection Sensitivity to 0
#define VS_PROCESSING_MODE_PARAM        0x5003
#define VS_PROCESSING_MODE_KW           0x0000  //Keyword Detection mode
#define SET_DIGITAL_GAIN_CMD            0x8015
#define DIGITAL_GAIN_20                 0x0C14  //20 db Gain and End point ID = 12
#define GET_DIGITAL_GAIN                0x801D
#define GET_EVENT_ID_CMD                0x806d  //Command to check which event has occurred
#define END_POINT_ID                    0x0C00  //Endpoint ID for Route-6 Stream manager-0
#define LOW_POWER_MODE_CMD              0x9010
#define LOW_POWER_MODE_RT6              0x0002  //Lowe Power mode for Route 6
#define BUILD_STRING_CMD1               0x8020  //Firmware build string
#define BUILD_STRING_CMD2               0x8021  //Retrieves next character from build string
#define SET_EVENT_RESP_CMD              0x801A  //set the type of interrupt generated on the HOST_IRQ line after an event
#define SET_PRESET_CMD					0x8031
#define PRESET_VALUE(n)					((n) & 0xffff)

#define IA61x_INT_RISE_EDGE             0x04    //Generate Rising Edge interrupt
#define IA61x_INT_FALL_EDGE             0x03    //Generate Rising Edge interrupt
#define IA61x_INT_LOW_LEVEL             0x01    //Generate Rising Edge interrupt

#define IA61x_LDO_ENABLE                PIN_PA21 //LDO Enable pin of IA61x Xplained pro board to power cycle Low keeps LDO in reset.

#define WDB_SIZE_NO_HEADER              508     //Data block size without 4 byte Header
#define WDB_SIZE                        512     //Data block size with 4 byte Header

/*-------------------------------------------------------------------------------------------------*\
 |    T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/


typedef struct
{
    int32_t (*download_config)(void);
    int32_t (*download_program)(void);
    int32_t (*download_keyword)(uint16_t *data, uint16_t size);
    int32_t (*VoiceWake)(void);
    int32_t (*close)(void);
    int32_t (*wait_keyword)(uint32_t ms);
	int32_t (*rdb)(uint8_t algo_id, uint8_t block_type, uint8_t *data, uint32_t *size);

    int32_t (*cmd)(uint16_t cmdWord, uint16_t dataWord, uint32_t timeout, uint16_t *pResponse);
    int32_t (*get)(uint8_t *data, uint32_t size);
    int32_t (*put)(uint8_t *data, uint32_t size);
} IA61x_instance;

IA61x_instance *IA61x_init(void);
void IA61x_uninit(void);

#endif /* IA61x_H_ */