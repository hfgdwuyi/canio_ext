/*!
 * Copyright Siemens Healthcare GmbH 2024, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief General low level driver configuration
 */
/*----------------------------------------------------------------------------*/

#ifndef __USBD_CONF_H
#define __USBD_CONF_H

#include <hal.h>

/* Common Config */
#define USBD_MAX_NUM_INTERFACES    1
#define USBD_MAX_NUM_CONFIGURATION 1
#define USBD_MAX_STR_DESC_SIZ      64
#define USBD_SELF_POWERED          1

/* Memory management macros make sure to use static memory allocation */
/** Alias for memory allocation. */

#define USBD_malloc (void *)USBD_static_malloc

/** Alias for memory set. */
#define USBD_memset memset

/** Alias for memory copy. */
#define USBD_memcpy memcpy

/** Alias for delay. */
#define USBD_Delay HAL_Delay

void *USBD_static_malloc(uint32_t size);
void  USBD_static_free(void *p);


#endif /* __USBD_CONF_H */
