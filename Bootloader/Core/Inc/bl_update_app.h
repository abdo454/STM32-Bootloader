/*
 * bl_update_app.h
 *
 *  Created on: Aug 11, 2022
 *      Author: abdo daood < abdo454daood@gmail.com >
 */

#ifndef INC_BL_UPDATE_APP_H_
#define INC_BL_UPDATE_APP_H_

#include <bl_configuration.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "debug.h"

#define BL_SOF 0xAA                         // Start o f Frame
#define BL_EOF 0xBB                         // End of Frame
#define BL_ACK 0x00                         // ACK
#define BL_NACK 0x01                        // NACK


#define BL_MAX_UART_RECEIVE_TIMEOUT 1000 // MAX Timeout of the Function HAL_UART_Receive



typedef enum {
	BL_PACKET_TYPE_CMD = 0,
	BL_PACKET_TYPE_HEADER = 1,
	BL_PACKET_TYPE_DATA = 2,
	BL_PACKET_TYPE_RESPONSE = 3,
} BL_PACKET_TYPE_;

typedef enum
{
	MODE_APK = 0,
	MODE_BOOTLOADER = 1,
} BTLR_MODE_;

/*
 * Commands Cases
 */
typedef enum {
	BL_CMD_RESET = 0,                   // RESET MCU
	BL_CMD_BL_VERSION = 1,              // GET Bootloader Version
	BL_CMD_LUNCH_APK = 2,               // Jumb to application (without reset)
	BL_CMD_VERIFY = 3,                  // End of Downloading process=> verify Binary in FLash
	BL_CMD_STAY_IN_BOOTLOADER_MODE = 6, // Keep bootloader running
	BL_CMD_TIMEOUT = 7,                 // no response from MCU => timout occured
} BL_CMD_CASES_;


/*function*/
/**
 * @brief check if Host pc try to Connect to MCU for firmware update purpuse
 * @param [in] void
 * @retval HAL_OK, HAL_TIMEOUT, HAL_ERROR
 */
void check_need_for_download(void);

#endif /* INC_BL_UPDATE_APP_H_ */
