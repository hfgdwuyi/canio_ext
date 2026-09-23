/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 * @file
 * @brief Header for error.c
 */
/*----------------------------------------------------------------------------*/
#include <stdint.h>
#include "error_defines.h"
#include <objects.h>


void set_error(uint32_t const ERR_HANDLE, uint32_t const ERR_TYPE);
void clear_error(uint32_t const ERR_HANDLE, uint32_t const ERR_TYPE);