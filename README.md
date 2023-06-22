# STM32-Bootloader
Customizable Bootloader for STM32 microcontrollers using UART.

# Introduction 
A bootloader is an application whose primary objective is to upgrade or modify system software without the intervention of specialized firmware upgrade tools. Bootloaders can have many functionalities, but they're mainly used to manage the application. They can also utilize different protocols such as UART, CAN, I2C, Ethernet, or USB to establish communication and initiate a firmware upgrade.

## Software Requirements:
In this Project, I have used the following Tools :
- OS: Linux, "Ubuntu 22.04"
- STM32CubeIDE Version: 1.11.2
- STM32CubeProgrammer
- Qt Creator 5.0.2

## Hardware Requirements:
The below components are used:
- STM32F412ZGT6 MCU.
- ST-LINK/V2, for flashing the bootloader in the MCU.
- 
# Memory of STM32F412ZGT6

The STM32 MCU comes with memory that is divided into multiple sectors of different sizes.
The Flash memory of **STM32412ZGT6** has the following main features:
- Capacity up to 1 Mbyte.
- 128 bits wide data read
- Byte, half-word, word, and double-word write
- Sector and mass erase.
- Memory organization: As Given in the Table below, The Flash memory is organized as follows:
  * A main memory block is divided into 4 sectors of 16 kbytes, plus 1 sector of 64 kbytes, and 7 sectors of 128 kbytes.
  * System memory from which the device boots in System memory boot mode  "*when Boot0 pin is set"*.

  ![Flash module organization](https://github.com/abdo454/STM32-Bootloader/blob/main/img/Screenshot%20from%202023-06-21%2011-42-11.png?raw=true)

The programmers have to divide the Flash into two sections "at least" : one application, one bootloader section. The application section contains end-customer code, and the bootloader section contains the customizable bootloader.

I have divided the Memory into two sections, as follows:

|   Block|  Start Address   |  End Address |  Size |
| ------------ | ------------ | ------------ |------------ |
| Bootloader Section    | 0x0800 0000   |  0x0801 FFFF |128 KB   |
| Application Section    | 0x0802 0000   |  0x080F FFFF |896 KB   |

  ![Memory Section](https://github.com/abdo454/STM32-Bootloader/blob/main/img/Flash%20Memory%20Sections.png?raw=true)


So, when the MCU is powered on or the RESET button is pressed (according to the BOOT0 pin, which is 0 in our circuit), the MCU will boot from the flash memory. We have placed the bootloader in the starting position. So, it will read the stack address from 0x08000000 and store that in the MSP register. Then it takes the reset handler address from 0x08000004 and jumps to it, which inside does all memory initialization of data segments (.BSS, .Data, etc.). After that, it calls the bootloader's main function.

# Flowchart Diagram

The flowchart below shows the Flow states of the Bootloader program:





The above flow chart shows what happens when the MCU enters the bootloader. First, it initializes all the required peripherals, then waits (for a predefined time) for requests from the host. If the MCU receives a Valid request, it processes it depending on the type of packet.
* **Cmd Packet**:  (Get Bootloader Version, Reset MCU, or Launch APP)
* **Header Packet**: has the information of the new App binary file (size and his CRC).
* **Data Packet**: Contains a single Chunk of the new App binary file.

  ![FlowChart Diagram](https://github.com/abdo454/STM32-Bootloader/blob/main/img/Bootloader%20Gui.png?raw=true)
 


-------------------

## Modify   FLASH.ld 
To apply the previous steps to the flash and define the start address of each program (bootloader, application), we must change:
#### Bootloader Linker Script 

 In the linker script  *STM32F412ZGTX_FLASH.ld *  file of the bootloader project, change the length of flash memory according to the bootloader section size (128 KB) without changing Origin Address of it. Like the snippet of code below:

```c
/* Memories definition */
MEMORY
{
  RAM    (xrw)    : ORIGIN = 0x20000000,   LENGTH = 256K
  FLASH    (rx)    : ORIGIN = 0x8000000,   LENGTH = 128K /*1024K*/
}
```

     
#### Application Linker Script 

 In the linker script  *STM32F412ZGTX_FLASH.ld *  file of the application project, change the length of the Flash Memory  according to the application section size (896 KB) and Modify Origin Address of it to 0x8020000, like in the snippet of code below:

```c
/* Memories definition */
MEMORY
{
  RAM    (xrw)    : ORIGIN = 0x20000000,   LENGTH = 256K
  FLASH    (rx)    : ORIGIN = 0x8020000,   LENGTH = 896K /*1024K*/
}
```


## Change Application Vector Table Address

In the previous step, the ORIGIN address of Application Flash memory was changed to 0x8020000, So Vector Table Start address will have to be changed too. This is done by uncommenting the USER_VECT_TAB_ADDRESS macro in the file (core -> Src ->system_stm32f4xx.c) and also changing the VECT_TAB_OFFSET macro to 0x00020000, as shown in the below code. (Lines 1 and 15 have been changed.)

```c
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
```

------------

# Graphical user interface (GUI):
The following screen is designed and implemented by QT Creator with C++ on Ubuntu OS for Flashing MCU and doing some commands:

  ![Memory Section](https://github.com/abdo454/STM32-Bootloader/blob/main/img/Bootloader%Gui.png?raw=true)
 


-------------------


# Communication 
 The following settings are used on the host and MCU Serial ports:

* Baud Rate: 115200 bps
* Data: 8 bits
* Parity: None
* Stop bits: 1 bit
* Flow Control: None


 Format of the frame is defined as:

|   SOF |  Packet Type  |  Len |  Data |CRC |  EOF |
| :------------: | :------------: | :------------: | :------------: | :------------: | :------------: |
| 1B     | 1B    |  2B  |N*B   |4B |1B   |

where:
* SOF: Start Of Frame, fixed Byte value =0xAA
* Packet Type: determine the packet type (CMD, HEADER or DATA).
* len: N counts of data bytes in the frame. The Maximum allowable value is 1024 Bytes of Data.
* Data: data in the frame with a maximum count of N*Byte.
* CRC: Checksum Sum.
* EOF: End Of Frame, Fixed Byte Value 0xBB.


For more details, just explore the project files.

