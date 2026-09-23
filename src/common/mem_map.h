/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Memory definitions
 */
/*----------------------------------------------------------------------------*/
#ifndef MEM_MAP_H
#define MEM_MAP_H

/*----------------------------------------------------------------------------*/
/*!
@name           Definitions for flash
@{
*/
/*----------------------------------------------------------------------------*/
/*! Size of microcontroller's flash */
#define FLASH_FULL_SIZE 0x00020000
/*! Base address of Bootloader */
#define BOOT_FLASH_ADDRESS 0x08000000

/*! Base address for Application */
#ifdef INTERNAL_FLASH
#define APP_FLASH_ADDRESS 0x08000000
/*! Application max size */
#define APP_MAX_SIZE 0x00020000
#else
#define APP_FLASH_ADDRESS RAM_AXI_ADDRESS
/*! Application max size */
#define APP_MAX_SIZE      RAM_AXI_SIZE
#endif
/*!
@}
*/
#define APPLICATION_STORED_ADDRESS 0x00000000

#define DEFAULT_APP_ADDRESS        0x00090000
/*----------------------------------------------------------------------------*/
/*!
@name           Definitions for RAM
@{
*/
/*----------------------------------------------------------------------------*/
/*! RAM DTCM (data tightly coupled) start address */
#define RAM_DTCM_ADDRESS 0x20000000
/*! RAM DTCM (data tightly coupled) size */
#define RAM_DTCM_SIZE 0x00020000

/*! RAM AXI (advanced extensible interface) start address */
#define RAM_AXI_ADDRESS 0x24000000
/*! RAM AXI (advanced extensible interface) size */
#define RAM_AXI_SIZE 0x00080000

/*! RAM SRAM123 start address */
#define RAM_SRAM123_ADDRESS 0x30000000
/*! RAM AXI (advanced extensible interface) size */
#define RAM_SRAM123_SIZE 0x00048000

/*! RAM SRAM123 start address */
#define RAM_SRAM4_ADDRESS 0x38000000
/*! RAM AXI (advanced extensible interface) size */
#define RAM_SRAM4_SIZE 0x00010000

/*!
@}
*/

/*----------------------------------------------------------------------------*/
/*!
@name           Stack and heap definitions
@{
*/
/*----------------------------------------------------------------------------*/
/*! Base address of system stack */
#define STACK_BASE_ADDR (RAM_DTCM_ADDRESS + RAM_DTCM_SIZE)
/*! Size of system stack */
#define STACK_SIZE 0x4000
/*! Size of system heap */
#define HEAP_SIZE 0x1000

/*!
@}
*/

/*! Offset where image size is stored */
#define IMAGE_SIZE_OFFSET 0x1C

#define APP_START_CODE    0x20


#endif    // MEM_MAP_H
