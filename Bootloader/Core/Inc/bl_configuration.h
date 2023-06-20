/*
 * bl_configuration.h
 *
 *  Created on: Aug 17, 2022
 *      Author: abdo
 */

#ifndef INC_BL_CONFIGURATION_H_
#define INC_BL_CONFIGURATION_H_


#define MAJOR 0		/*BootLoader Major Version Number*/
#define MINOR 2		/*BootLoader Minor Version Number*/

#define FLASH_APP_ADDER ((uint32_t) 0x08020000)	//128KB for Bootloader => 896KB for APK

#define BL_LED_Pin GPIO_PIN_0 	// LED Connected to PE0 as Pull-Up LED
#define BL_LED_GPIO_Port GPIOE


#endif /* INC_BL_CONFIGURATION_H_ */
