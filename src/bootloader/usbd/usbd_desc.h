/*!
 * Copyright Siemens Healthcare GmbH 2024, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header for usbd_desc.c module
 */
/*----------------------------------------------------------------------------*/

#ifndef __USBD_DESC_H
#define __USBD_DESC_H
// Project includes
#include "usbd_def.h"

#define DEVICE_ID1            (0x5C001000)
#define DEVICE_ID2            (0x5C001004)
#define DEVICE_ID3            (0x5C001008)

#define USB_SIZ_STRING_SERIAL 0x12

extern USBD_DescriptorsTypeDef VCP_Desc;

#endif /* __USBD_DESC_H */
