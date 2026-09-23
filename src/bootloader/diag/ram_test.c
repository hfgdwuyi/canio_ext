/*!
 * Copyright Siemens Healthcare GmbH 2021, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief RAM check
 */
/*----------------------------------------------------------------------------*/
// Standard includes
#include <stdint.h>
#include <stdbool.h>

// Project includes
#include <hal.h>
#include "mem_map.h"
#include "selftest.h"
#include "prog_ctrl.h"

/*! Array containing patterns to be used in RAM test */
static const uint32_t testPatterns[] = {
    0x00000000,
    0x55555555,
    0xAAAAAAAA,
    0xFFFFFFFF,
};

static const struct
{
    uint32_t address;
    uint32_t size;
} memRegions[] = {
    { RAM_AXI_ADDRESS, RAM_AXI_SIZE },
};

/*! RAM test return point */
extern void RAM_Test_Return(void);

/*----------------------------------------------------------------------------*/
/*!
@brief          Write to each RAM address specified number. Read
                out the complete RAM and check if all prior written
                values are kept.
                On failure, run the internal error state.
                On success, continue with the startup code

@return         None
*/
/*----------------------------------------------------------------------------*/
void RAM_Test(void)
{
    __HAL_RCC_RTC_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    if(READ_REG(RTC->BKP1R) == 0)
    {
        for (register unsigned pattern = 0; pattern < sizeof(testPatterns) / sizeof(testPatterns[0]); pattern++)
        {
            for (register unsigned region = 0; region < sizeof(memRegions) / sizeof(memRegions[0]); region++)
            {
                uint32_t *pAddress = (uint32_t *)memRegions[region].address;
                // Fill memory with current pattern value
                while ((uint32_t)pAddress < (memRegions[region].address + memRegions[region].size))
                {
                    *pAddress++ = testPatterns[pattern];
                }

                // Check local memory by reading existing values and comparing with pattern value
                pAddress = (uint32_t *)memRegions[region].address;
                while ((uint32_t)pAddress < (memRegions[region].address + memRegions[region].size))
                {
                    if (*pAddress++ != testPatterns[pattern])
                    {
                        // Unexpected value found - test failed
                        selftestErrorState();
                    }
                }
            }
        }
    }
    RAM_Test_Return();
}
