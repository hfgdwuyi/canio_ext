/*!
 * Copyright Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Error defines
 */
/*----------------------------------------------------------------------------*/
#ifndef ERROR_DEFINES_H
#define ERROR_DEFINES_H

/*----------------------------------------------------------------------------*/
/*!
 @name           Error bits in Manufacturer Status Register (0x1002)
 @{
*/
/*----------------------------------------------------------------------------*/
#define ERR_MAN_CANOPEN 1UL
#define ERR_MAN_STORAGE (1UL << 1)
#define ERR_MAN_HW_INIT (1UL << 2)
#define ERR_MAN_APP_ERR (1U << 3)

/*!
@}
*/

/*----------------------------------------------------------------------------*/
/*!
 @name           Subindexes in Manufacturer Error Description (0x2200)
 @{
*/
/*----------------------------------------------------------------------------*/
#define ERR_DESC_SUBINDEX_CANOPEN 1
#define ERR_DESC_SUBINDEX_STORAGE 2
#define ERR_DESC_SUBINDEX_HW_INIT 3
#define ERR_DESC_SUBINDEX_APP_ERR 4

/*!
@}
*/

/*----------------------------------------------------------------------------*/
/*!
 @name           CANOpen error defines
 @{
*/
/*----------------------------------------------------------------------------*/
#define ERR_CANOPEN_PASSIVE          1UL
#define ERR_CANOPEN_BUS_OFF          (1UL << 1)
#define ERR_CANOPEN_OVERFLOW         (1UL << 2)
#define ERR_CANOPEN_TX_OVERFLOW      (1UL << 3)
#define ERR_CANOPEN_RX_OVERFLOW      (1UL << 4)
#define ERR_CANOPEN_CHGSTATE_STOPPED (1UL << 5)
/*!
@}
*/

/*----------------------------------------------------------------------------*/
/*!
 @name           Storage error defines
 @{
*/
/*----------------------------------------------------------------------------*/
#define ERR_STORAGE_SECTOR1 1UL
#define ERR_STORAGE_SECTOR2 (1UL << 1)
#define ERR_STORAGE_SECTOR3 (1UL << 2)
#define ERR_STORAGE_SECTOR4 (1UL << 3)
#define ERR_STORAGE_SECTOR5 (1UL << 4)
#define ERR_STORAGE_ASSET   (1UL << 5)
/*!
@}
*/


/*----------------------------------------------------------------------------*/
/*!
 @name           Hardware error defines
 @{
*/
/*----------------------------------------------------------------------------*/
#define ERR_INIT_UART          1UL
#define ERR_INIT_I2C           (1UL << 1)
#define ERR_INIT_SPI           (1UL << 2)
#define ERR_INIT_ADC           (1UL << 3)
#define ERR_INIT_DAC           (1UL << 4)
#define ERR_INIT_PWM           (1UL << 5)
#define ERR_INIT_QSPI          (1UL << 6)
#define ERR_I2C_AF_CONFIG      (1UL << 7)
#define ERR_ADC_CALIBRSTART    (1UL << 8)
#define ERR_PWM_CHANNEL_CONFIG (1UL << 9)
#define ERR_PWM_STARTSTOP      (1UL << 10)
#define ERR_DAC_CHANNEL_CONFIG (1UL << 11)
#define ERR_TPM75_STATUS_RW    (1UL << 12)
#define ERR_ISM330_STATUS_RW   (1UL << 13)
#define ERR_INIT_WTDG          (1Ul << 14)

/*!
@}
*/
/*----------------------------------------------------------------------------*/
/*!
 @name           Application error defines
 @{
*/
/*----------------------------------------------------------------------------*/
#define ERR_NO_APPLICATION  1UL
#define ERR_APPLICATION_CRC (1UL << 1)
#define ERR_FLASH_ERASE     (1UL << 2)
#define ERR_FLASH_PROGRAM   (1UL << 3)


/*!
@}
*/
/*----------------------------------------------------------------------------*/
/*!
 @name           Emergency codes
 @{
*/
/*----------------------------------------------------------------------------*/
#define ERRCODE_UART_INIT            (0x5002)
#define ERRCODE_I2C_INIT             (0x5003)
#define ERRCODE_SPI_INIT             (0x5004)
#define ERRCODE_ADC_INIT             (0x5005)
#define ERRCODE_DAC_INIT             (0x5006)
#define ERRCODE_PWM_INIT             (0x5007)
#define ERRCODE_QSPI_INIT            (0x5008)
#define ERRCODE_I2C_AF_CONFIG        (0x5009)
#define ERRCODE_ADC_CALIBRSTART      (0x5010)
#define ERRCODE_PWM_CHANNEL_CONFIG   (0x5011)
#define ERRCODE_PWM_STARTSTOP        (0x5012)
#define ERRCODE_DAC_CHANNEL_CONFIG   (0x5013)
#define ERRCODE_WTDG_INIT            (0x5014)
#define ERRCODE_CAN_OVERRUN_TX       (0x8111)
#define ERRCODE_CAN_OVERRUN_RX       (0x8112)
#define ERRCODE_CAN_CHGSTATE_STOPPED (0x8230)
#define ERRCODE_STORAGE_SECTOR       (0x5014)
#define ERRCODE_STORAGE_ASSET        (0x5015)
#define ERRCODE_TMP75                (0x7120)    // TMP75 error code
#define ERRCODE_ISM330               (0x7110)    // ISM330 error code
#define ERRCODE_NO_APP               (0x5201)
#define ERRCODE_APP_CRC              (0x5202)
#define ERRCODE_FLASH_ERASE          (0x5203)
#define ERRCODE_FLASH_PROGRAM        (0x5204)

#endif
