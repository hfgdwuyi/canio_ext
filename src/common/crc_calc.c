/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Cyclic Redundancy Check calculation functions
 */
/*----------------------------------------------------------------------------*/
// HAL includes
#include <hal.h>
// Project includes
#include "crc_calc.h"

// Setup calculation settings
static CRC_HandleTypeDef hcrc = {
    .Instance                     = CRC,
    .Init.DefaultPolynomialUse    = DEFAULT_POLYNOMIAL_ENABLE,
    .Init.DefaultInitValueUse     = DEFAULT_INIT_VALUE_ENABLE,
    .Init.InputDataInversionMode  = CRC_INPUTDATA_INVERSION_NONE,
    .Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE,
    .InputDataFormat              = CRC_INPUTDATA_FORMAT_BYTES,
};

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Calculates CRC for 4 bytes of data
 * @param[in]     data pointer to array to calculate CRC from
 * @param[in]     length daat length
 *
 */
/* ---------------------------------------------------------------------------- */
uint32_t crcCalc32(void *data, uint32_t length)
{
    hcrc.Init.CRCLength = CRC_POLYLENGTH_32B;
    // CRC calculation
    return HAL_CRC_Accumulate(&hcrc, data, length);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Calculates CRC for 1 byte of data
 * @param[in]     data pointer to array to calculate CRC from
 * @param[in]     length data length
 * @return        calculated CRC value
 */
/* ---------------------------------------------------------------------------- */
uint8_t crcCalc8(void *data, uint32_t length)
{
    hcrc.Init.CRCLength = CRC_POLYLENGTH_8B;
    // CRC calculation
    return (uint8_t)HAL_CRC_Accumulate(&hcrc, data, length);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Initialization of CRC module
 *
 */
/* ---------------------------------------------------------------------------- */
void crcInit(void)
{
    // Enable CRC peripheral
    __HAL_RCC_CRC_CLK_ENABLE();

    // CRC init
    (void)HAL_CRC_Init(&hcrc);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Resets the CRC module
 *
 */
/* ---------------------------------------------------------------------------- */
void crcReset(void)
{
    __HAL_CRC_DR_RESET(&hcrc);
}
