/**
 ******************************************************************************
 * @file    USB_Device/CDC_Standalone/Inc/usbd_cdc_interface.h
 * @author  MCD Application Team
 * @brief   Header for usbd_cdc_interface.c file.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2017 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_CDC_IF_H
#define __USBD_CDC_IF_H


#define APP_DATA_SIZE 64

extern USBD_CDC_ItfTypeDef USBD_CDC_fops;
int8_t                     CDC_Itf_Transmit(char *Buf, uint32_t Len);

#endif /* __USBD_CDC_IF_H */
