/*
 * bl_update_app.c
 *
 *  Created on: Aug 11, 2022
 *      Author: abdo daood < abdo454daood@gmail.com >
 */

#include <bl_update_app.h>

#define BL_DATA_MAX_SIZE (1024)             // Maximum data size in one Chunk
#define BL_DATA_OVERHEAD (9)                // Other Bytes in the Chunk, without the Data ( SOF,Packet Type,Len,CRC EOF)
#define BL_PACKET_MAX_SIZE (BL_DATA_MAX_SIZE + BL_DATA_OVERHEAD)

#define MAX_STM32_FW_SIZE  (1 * 1024 * 1024) /* 1MB Maximum Predectable Size for firmware*/

#define BL_PACKET_MAX_SIZE (BL_DATA_MAX_SIZE + BL_DATA_OVERHEAD)

uint8_t Buffer[BL_PACKET_MAX_SIZE]; // buffer for Receive from UART

static uint16_t receive_chunk(uint8_t *, uint16_t );
static void goto_application();
static void send_resp(uint8_t);
static void process_chunk(uint8_t * );
static uint8_t Bootloader_Earse_Flash();
static uint16_t write_data_to_flash(uint8_t * ,uint16_t);
static bool stay_in_Boooloader=true;
static uint32_t flash_ptr=FLASH_APP_ADDER;
static uint32_t CrcCCITTBytes(const uint8_t * data, uint32_t size);


extern UART_HandleTypeDef huart2;
extern CRC_HandleTypeDef hcrc;
/*
 * Response Frame format
 *  ________________________________________________________________
 *  | SOF	|	Packet Type	|	Len		|	type	|	CRC	|	EOF	|
 *  |_______|_______________|___________|___________|_______|_______|
 *  | 1B	|		1B		|	2B		|	 1B		|	4B	 	1B
 *  ^----------------------CRC----------------------^
 */
typedef struct
{
	uint8_t sof;
	uint8_t packet_type;
	uint16_t data_len;
	uint8_t status;
	uint32_t crc;
	uint8_t eof;
} __attribute__((packed)) BL_STM32_RESPONSE_;





/*
 *  Standard & General Frame format
 *  ----------------------------------------------------------------
 *  | SOF	|	Packet Type	|	Len		|	Data	|	CRC	|	EOF	|
 *  |		|	 			|			|			|		|  		|
 *  | 1B	|		1B		|	2B		|	n*B		|	4B	| 	1B	|
 *  ----------------------------------------------------------------
 *  ^----------------------CRC----------------------^
 *  */

typedef struct
{
	uint8_t sof;
	uint8_t packet_type;
	uint16_t data_len;
	uint8_t data;

} __attribute__((packed)) BL_FRAME_TEMPLATE_;




typedef struct
{
	uint32_t firmware_size;
	uint32_t firmware_crc;
	uint32_t reserved1;
	uint32_t reserved2;
} __attribute__((packed)) BL_FIRMWARE_INFO_;

static BL_FIRMWARE_INFO_ fw_total_info = {0, 0, 0, 0};	   // the size the received with header packet
static BL_FIRMWARE_INFO_ fw_received_info = {0, 0, 0, 0}; // the incremental size of packets which receiving from Host

/**
 * @brief check if Host pc try to Connect to MCU for firmware update purpuse
 * @param [in] void
 * @retval HAL_OK, HAL_TIMEOUT, HAL_ERROR
 */
void check_need_for_download(void){
	while(1)
	{

		uint16_t ret=receive_chunk(Buffer,BL_PACKET_MAX_SIZE);
		HAL_GPIO_TogglePin(BL_LED_GPIO_Port, BL_LED_Pin);
		switch(ret){
		case HAL_OK :
			process_chunk(Buffer);
			break;
		case HAL_TIMEOUT :
			// TO_DO check configuration Bytes
			if(!stay_in_Boooloader)
				goto_application(); // if it return from this function=> dos't go to application
			// Reapeat the Previous Sequences
			break;
		case HAL_ERROR:
			break;
		default :
			// impossible case
			break;
		}
	}
}



static void goto_application() {

	// Check the content of FLASH_APP_ADDER address in flash if it's empty (should contain MSP )
	// if Valaid , then check configuration Bytes
	DEBUG_PRINT("MSP Adress : 0x%02x",(*(volatile unsigned int *)FLASH_APP_ADDER));
	if(((*(volatile uint32_t*)FLASH_APP_ADDER)&0xFFF00000)==0x20000000)
	{
		DEBUG_PRINT("GO TO Application ..");
		__set_MSP(*(volatile uint32_t*)FLASH_APP_ADDER);

		HAL_UART_DeInit(&huart2);
		HAL_GPIO_DeInit(BL_LED_GPIO_Port, BL_LED_Pin);
		HAL_RCC_DeInit();
		//TO_DO
		//app_reset_handler is :Function Pointer to the Function of App Reset Handler
		void (*app_reset_handler)(void)= (void*)(*((volatile uint32_t*) (FLASH_APP_ADDER + 4U)));
		app_reset_handler();
		DEBUG_ERR_PRINT("NO Valid Apk to Run !!! Waiting Receiving Valid APK from Host");
	}
}


/**
 * @brief Receive a one chunk of data.
 * @param buf buffer pointer to store the received data
 * @param max_len maximum allowed length of Packet
 * @retval HAL_OK, HAL_TIMEOUT, HAL_ERROR
 */
static uint16_t receive_chunk(uint8_t *buf, uint16_t max_len)
{
	uint16_t index = 1;
	uint16_t data_len = 0;
	uint32_t cal_data_crc = 0u;
	uint32_t rec_data_crc = 0u;
	// receive SOF byte (1byte) with 3 attemps
	for (int i = 0; i <= 2; i++)
	{
		if (HAL_UART_Receive(&huart2, &buf[0], 1, BL_MAX_UART_RECEIVE_TIMEOUT) == HAL_OK && buf[0] == BL_SOF)
			break;
		if (i == 2)
			return HAL_TIMEOUT; // 3sec without response & Not received SOF
	}
	// Receive the packet type (1byte).
	if (HAL_UART_Receive(&huart2, &buf[index++], 1, BL_MAX_UART_RECEIVE_TIMEOUT) != HAL_OK)
		return HAL_ERROR;
	// Receive the data length (2byte).
	if (HAL_UART_Receive(&huart2, &buf[index], 2, BL_MAX_UART_RECEIVE_TIMEOUT) != HAL_OK)
		return HAL_ERROR;
	data_len = *(uint16_t *)&buf[index];
	index += 2u;

	// Receive the data (len*Byte)
	for (uint16_t i = 0u; i < data_len; i++)
	{
		if (HAL_UART_Receive(&huart2, &buf[index++], 1, BL_MAX_UART_RECEIVE_TIMEOUT) != HAL_OK)
			return HAL_ERROR;
	}
	// Get the CRC.
	if (HAL_UART_Receive(&huart2, &buf[index], 4, BL_MAX_UART_RECEIVE_TIMEOUT) != HAL_OK)
		return HAL_ERROR;

	rec_data_crc = *(uint32_t *)&buf[index];
	index += 4u;
	// receive EOF byte (1byte)
	if (HAL_UART_Receive(&huart2, &buf[index], 1, HAL_MAX_DELAY) != HAL_OK || buf[index] != BL_EOF)
		return HAL_ERROR;
	if(buf[index]!=BL_EOF)
		return HAL_ERROR; /*Not received end of frame*/
	cal_data_crc = CrcCCITTBytes( buf, 4+data_len); /*4= sof,packet_type, data length*/

	//Verify the CRC
	if(cal_data_crc!=rec_data_crc)
		return HAL_ERROR;

	if (max_len < index)
	{
		DEBUG_ERR_PRINT("Received more data than expected. Expected = %d, Received = %d",
				max_len, index);
		return HAL_ERROR;
	}
	return HAL_OK;
}

static void process_chunk(uint8_t *buf)
{
	if (buf == NULL)
		return ;
	BL_FRAME_TEMPLATE_ *frame = (BL_FRAME_TEMPLATE_ *)buf;
	DEBUG_PRINT("Packet Type : %d",frame->packet_type);
	switch (frame->packet_type)
	{
	case BL_PACKET_TYPE_HEADER:
		DEBUG_PRINT("BL_PACKET_TYPE_HEADER");
		fw_total_info.firmware_size = *(uint32_t *)&(frame->data);
		fw_total_info.firmware_crc = *(uint32_t *)((&(frame->data)) + 4u);
		fw_received_info.firmware_size=0;
		fw_received_info.firmware_crc=0;
		flash_ptr=FLASH_APP_ADDER;      /* Reset Flash Destination Adress */
		DEBUG_PRINT("Received HEADER. FW Size = %ld", fw_total_info.firmware_size);
		send_resp(BL_ACK);
		break;
	case BL_PACKET_TYPE_DATA:
		DEBUG_PRINT("BL_PACKET_TYPE_DATA");
		if (write_data_to_flash(&(frame->data), frame->data_len) == HAL_OK)
		{
			fw_received_info.firmware_size += frame->data_len;
			DEBUG_PRINT("Data Writen : [%ld/%ld]", fw_received_info.firmware_size / BL_DATA_MAX_SIZE + (fw_received_info.firmware_size % BL_DATA_MAX_SIZE != 0),
					fw_total_info.firmware_size / BL_DATA_MAX_SIZE + (fw_total_info.firmware_size % BL_DATA_MAX_SIZE != 0));
			send_resp(BL_ACK);
		}
		else
		{
			DEBUG_ERR_PRINT("Error writing Data packet[%ld/%ld] into Flash", fw_received_info.firmware_size / BL_DATA_MAX_SIZE + (fw_received_info.firmware_size % BL_DATA_MAX_SIZE != 0) + 1,
					fw_total_info.firmware_size / BL_DATA_MAX_SIZE + (fw_total_info.firmware_size % BL_DATA_MAX_SIZE != 0));
			send_resp(BL_NACK);
		}
		break;
	case BL_PACKET_TYPE_CMD:
		switch(frame->data) // = CMD
		{
		case BL_CMD_STAY_IN_BOOTLOADER_MODE:
			DEBUG_PRINT("BL_CMD_STAY_IN_BOOTLOADER_MODE");
			stay_in_Boooloader=true;
			send_resp(BL_ACK);
			break;
		case BL_CMD_LUNCH_APK:
			DEBUG_PRINT("BL_CMD_LUNCH_APK");
			stay_in_Boooloader=false;
			goto_application();
			send_resp(BL_NACK);
			break;
		case BL_CMD_RESET:
			DEBUG_PRINT("BL_CMD_RESET");
			send_resp(BL_ACK);
			HAL_NVIC_SystemReset();
			break;
		case BL_CMD_BL_VERSION:
			DEBUG_PRINT("BL_CMD_BL_VERSION");
			send_resp((uint8_t)(((MAJOR&0x0F)<<4)|(MINOR&0x0F)));
			break;
		case BL_CMD_VERIFY:
			DEBUG_PRINT("BL_CMD_VERIFY");
			DEBUG_PRINT("Validating the received Binary...");
			if(fw_received_info.firmware_size==	fw_total_info.firmware_size)
				fw_received_info.firmware_crc=CrcCCITTBytes((uint32_t *)FLASH_APP_ADDER, fw_received_info.firmware_size);
			if(fw_received_info.firmware_size!=	fw_total_info.firmware_size || 	fw_total_info.firmware_crc !=fw_received_info.firmware_crc){
				send_resp(BL_NACK);
				DEBUG_ERR_PRINT("FW Downloading Error ...");
			}else{
				//TO_DO set Configuation Bytes for success of Downloading
				send_resp(BL_ACK);
				DEBUG_PRINT("firmware downloaded and flashed Successfully.");
			}
			break;
		default:
			DEBUG_PRINT("UNkown CMD !!!");
			break;
		}// switch CMD
		break;
		default :
			break;
	}
}

static uint16_t write_data_to_flash(uint8_t * buf, uint16_t len)
{
	if (buf == NULL || len == 0u)
		return HAL_ERROR;


	// 1- Earse Full Application Sectors for Once Before Re-Flashing
	if(flash_ptr==FLASH_APP_ADDER)
	{
		DEBUG_PRINT("  Earse FLash sectors ...");
		if(Bootloader_Earse_Flash()!=HAL_OK)
		{
			DEBUG_ERR_PRINT("Bootloader Earse Flash Error");
			return HAL_ERROR;
		}
	}
	// 2- Unlock the FLASH control register access
	if(HAL_FLASH_Unlock()!= HAL_OK)
		return HAL_ERROR;
	// 3- Check if the FLASH_FLAG_BSY.
	FLASH_WaitForLastOperation( HAL_MAX_DELAY );

	// 4- Clear pending flags (if any)
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
			FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

	// 3- Program Flash
	for(uint16_t i=0;i<len;)
	{
		uint32_t data=*(uint32_t *)&buf[i];
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_ptr, data)== HAL_OK)
		{
			/* Increment Flash destination address */
			flash_ptr += 4;
			/* Increment index destination of Data */
			i+=4;
		}
		else
		{
			/* Error occurred while writing data into Flash */
			HAL_FLASH_Lock();
			return HAL_ERROR;
		}
	}
	//Check if the FLASH_FLAG_BSY.
	FLASH_WaitForLastOperation( HAL_MAX_DELAY );
	// Locks the FLASH control register access
	HAL_FLASH_Lock();
	return HAL_OK;
}

static uint8_t Bootloader_Earse_Flash()
{
	// Unlock the FLASH control register access
	if(HAL_FLASH_Unlock()!= HAL_OK)
		return HAL_ERROR;
	//Check if the FLASH_FLAG_BSY.
	FLASH_WaitForLastOperation( HAL_MAX_DELAY );
	// Clear pending flags (if any)
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
			FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

	// Erase the sector(s)
	uint32_t SectorError=0;
	FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_SECTORS ;
	EraseInitStruct.Sector   = FLASH_SECTOR_5;
	EraseInitStruct.NbSectors=7;
	EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3; // voltage range needs to be 2.7V to 3.6V

	uint8_t status= HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);

	// Locks the FLASH control register access
	HAL_FLASH_Lock();
	return status;

}
static void send_resp(uint8_t type)
{
	BL_STM32_RESPONSE_ resp =
	{
			.sof = BL_SOF,
			.packet_type = BL_PACKET_TYPE_RESPONSE,
			.data_len = 0x01,
			.status = type,
			.crc = 0,
			.eof = BL_EOF};
	resp.crc= CrcCCITTBytes( (uint8_t *)&resp, 4+resp.data_len);
	DEBUG_PRINT("Response : %d",resp.status);

	// send response
	HAL_UART_Transmit(&huart2, (uint8_t *)&resp, sizeof(BL_STM32_RESPONSE_), HAL_MAX_DELAY);
}
/*
 * The HW CRC works on the part operates on 32-bit at a time.
 *  To handle odd bytes, we must either use consistent padding bytes
 */

uint32_t CrcCCITTBytes(const uint8_t * data, uint32_t size) {
	uint32_t i;
	CRC->CR = CRC_CR_RESET;
	while(CRC->CR & CRC_CR_RESET);  // avoiding the not-reset-fast-enough bug
	i = size % 4;
	switch(i) {
	case 0:
		break;
	case 1:
		CRC->DR = 0xFFFFFFB9;
		CRC->DR = 0xAF644900
				| (__RBIT(*(uint32_t*)(uintptr_t)&data[0]) >> 24)
				;
		break;
	case 2:
		CRC->DR = 0xFFFFB950;
		CRC->DR = 0x64490000
				| (__RBIT(*(uint32_t*)(uintptr_t)&data[0]) >> 16)
				;
		break;
	case 3:
		CRC->DR = 0xFFB9509B;
		CRC->DR = 0x49000000
				| (__RBIT(*(uint32_t*)(uintptr_t)&data[0]) >> 8)
				;
		break;
	}

	for (; i < size; i += 4) {
		CRC->DR = __RBIT(*(uint32_t*)(uintptr_t)&data[i]);
	}
	return __RBIT(CRC->DR) ^ 0xFFFFFFFF;
}
