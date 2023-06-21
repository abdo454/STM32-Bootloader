# STM32-Bootloader
Customizable Bootloader for STM32 microcontrollers using UART.

# Introudtion 
A bootloader is an application whose primary objective is to upgrade/modify system software without the intervention of specialized firmware upgrade tools. Bootloaders can have many functionalities, but it's mainly used to manage the application. They can also utilize different protocols such as UART, CAN, I2C, I2S, Ethernet, or USB to establish communication and initial a firmware upgrade.

## Software Requirments:
In this Project, I have used the folowing Tools :
- OS: Linux, "ubuntu 22.04"
- STM32CubeIDE Version: 1.11.2
- STM32CubeProgrammer
- Qt Creator 5.0.2

## Harware Requerments :
the below component are used :
- STM32F412ZGT6 MCU.
- ST-LINK/V2  , for flash the bootloader  in MCU.
- 
# Memory of STM32F412ZGT6

STM32 MCU comes with a memory that is divided into multiple sectors with different sizes.  
The Flash memory of **STM32412ZGT6** has the following main features:
- Capacity up to 1 Mbyte.
- 128 bits wide data read.
- Byte, half-word, word and double word write.
- Sector and mass erase.
- Memory organization: As Given in the Table Bellow, The Flash memory is organized as follows:
  * A main memory block divided into 4 sectors of 16 Kbyte, plus 1 sector of 64 Kbyte  and plus 7 sector of 128 Kbyte.
  * System memory from which the device boots in System memory boot mode  "*when Boot0 pins is set"*.

  ![alt text](https://github.com/abdo454/STM32-Bootloader/blob/[branch]/image.jpg?raw=true)

the programers have to devied the Flash into two sections "at least" : one application, one bootloader section. The application section contains end-costumer code, and the bootloader section contains the customizable bootloader.

i have devided the Memory into two sections as folow :

|   Block|  Start Adresss   |  End Adress |  Size |
| ------------ | ------------ | ------------ |------------ |
| Bootloader Section    | 0x0800 0000   |  0x0801 FFFF |128KB   |
| Application Section    | 0x0802 0000   |  0x080F FFFF |896KB   |

So, when the MCU is Powerd-on or the RESET button is pressed,  (and accroding to the BOOT0 pin which is 0 in our circuit) the MCU will boots from the flash memory. We have placed the bootloader in the starting position. So, it will read the stack address  from 0x08000000 and store that in the MSP register. Then it takes the reset handler address from 0x08000004 and jumps to it which inside does  all memory initialization of data segments (.bss , .data ..), after that,  it calls the bootloader's  main function.

# Flowchart Diagram

The flowchart  below shows the Flow states of the Bootloader program:



The above flow chart shows what happens when MCU enters into the bootloader. First, it initialaize all the requred peripherals,  then Wait (for a predefined time) for reqests from Host. if MCU recevied a Valid request, it process it depending on the type of the packet.
* **Cmd Packet**:  (Get Bootloader Version ,  Reset MCU or Lanuch APP).
* **Header Packet**: has the inforamtion of the new App binary file ( Size and his CRC).
* **Data Packet**: Contains a single Chunk of the new App binary file.


-------------------

## Modify   FLASH.ld 
To applay the previous steps on the flash and define the start adress of each programs(Bootloader, Application), we must change :
#### Bootloader Linker Script 

 In the linker script  *STM32F412ZGTX_FLASH.ld *  file of Bootloader project,, change Length of  Flash Memory  according to the bootloader section size (128KB) wihout modifing Orgin Adress of it, Like the snippet of code below :

			/* Memories definition */
			MEMORY
			{
			  RAM    (xrw)    : ORIGIN = 0x20000000,   LENGTH = 256K
			  FLASH    (rx)   : ORIGIN = 0x8000000,      LENGTH = 128K  /* 1024K*/
			}
			
#### Application Linker Script 

 In the linker script  *STM32F412ZGTX_FLASH.ld *  file of Application project, change Length of  Flash Memory  according to the application section size (896KB) , and Modify Orgin Adress of it to 0x8020000 , Like the snippet of code below :

			/* Memories definition */
			MEMORY
			{
			  RAM    (xrw)    : ORIGIN = 0x20000000,   LENGTH = 256K
			  FLASH    (rx)   : ORIGIN = 0x8020000,     LENGTH = 896K  /*1024K*/
			}
##Change Application  Vector Table address

In the previous step,  the ORIGIN address of Application Flash  Memory had changed to 0x8020000 , So Vector Table Start adress will have to be changed too. this is done by uncommenting the USER_VECT_TAB_ADDRESS macro in file (core -> Src ->system_stm32f4xx.c) and also Modify the VECT_TAB_OFFSET macro to 0x00020000 , as the below code.( line 1 , 15 have been changed).


    #define USER_VECT_TAB_ADDRESS
    
    #if defined(USER_VECT_TAB_ADDRESS)
    /*!< Uncomment the following line if you need to relocate your vector Table
         in Sram else user remap will be done in Flash. */
    /* #define VECT_TAB_SRAM */
    #if defined(VECT_TAB_SRAM)
    #define VECT_TAB_BASE_ADDRESS   SRAM_BASE       /*!< Vector Table base address field.
                                                         This value must be a multiple of 0x200. */
    #define VECT_TAB_OFFSET         0x00000000U     /*!< Vector Table base offset field.
                                                         This value must be a multiple of 0x200. */
    #else
    #define VECT_TAB_BASE_ADDRESS   FLASH_BASE      /*!< Vector Table base address field.
                                                         This value must be a multiple of 0x200. */
    #define VECT_TAB_OFFSET         0x00020000     /*!< Vector Table base offset field.
                                                         This value must be a multiple of 0x200. */
    #endif /* VECT_TAB_SRAM */
    #endif /* USER_VECT_TAB_ADDRESS */
    /******************************************************************************/

------------
# Comunication 
 The following settings are used in Host and MCU Serial Port   :
-     Baud Rate: 115200 bps
-     Data: 8 bit
-     Parity: None
-     Stop bits: 1 bit
-     Flow Control: none


 Format of the frame is defineded as :

|   SOF |  Packet Type  |  Len |  Data |CRC |  EOF |
| :------------: | :------------: | :------------: | :------------: | :------------: | :------------: |
| 1B     | 1B    |  2B  |N*B   |4B |1B   |

where:
* SOF : Start Of Frame, fixed Byte value =0xAA .
* Packet Type : determine packet type { CMD, HEADER  or DATA }.
* len :  N  count of data bytes in the frame.  the Maximum allowable value is 1024 Bytes of Data.
* Data : data in the frame with a maximum count of N*Byte .
* CRC : Checkum Sum.
* EOF : End Of Frame , Fixed Byte value 0xBB.


For more details just explore the project files.
