/*
 * bootloader_update.h
 *  Created on: 14 Jun, 2023
 *  Author: abdo daood < abdo454daood@gmail.com >
 */
#ifndef BOOTLOADER_UPDATE_H
#define BOOTLOADER_UPDATE_H

#define BL_SOF 0xAA                         // Start o f Frame
#define BL_EOF 0xBB                         // End of Frame
#define BL_ACK 0x00                         // ACK
#define BL_NACK 0x01                        // NACK

#define BL_MCU_OPEN_CTN_LOOP 600            // Time between two Consecutive attemps to open connection with MCU

#define WAIT_FLASH_ERASE_TIME delay( 1000 * 1000); // 1 second untill mcu earse all apk sectors
#define UART_READ_TIMEOUT_MS 2000           //2sec =2000 msec
#define UART_READ_SLEEP_US 100
#define BL_DATA_MAX_SIZE (1024)             // Maximum data size in one Chunk
#define BL_DATA_OVERHEAD (9)                // Other Bytes in the Chunk, without the Data ( SOF,Packet Type,Len,CRC EOF)
#define BL_PACKET_MAX_SIZE (BL_DATA_MAX_SIZE + BL_DATA_OVERHEAD)

#define MAX_STM32_FW_SIZE  (1 * 1024 * 1024) /* 1MB Maximum Predectable Size for firmware*/


typedef enum {
  BL_PACKET_TYPE_CMD = 0,
  BL_PACKET_TYPE_HEADER = 1,
  BL_PACKET_TYPE_DATA = 2,
  BL_PACKET_TYPE_RESPONSE = 3,
} BL_PACKET_TYPE_;

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

#endif // BOOTLOADER_UPDATE_H
