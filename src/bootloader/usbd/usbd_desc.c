/**
 ******************************************************************************
 * @file    USB_Device/CDC_Standalone/Src/usbd_desc.c
 * @author  MCD Application Team
 * @brief   This file provides the USBD descriptors and string formatting method.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------ */
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_conf.h"

/* Private typedef ----------------------------------------------------------- */
/* Private define ------------------------------------------------------------ */
#define USBD_VID                     0x0483
#define USBD_PID                     0x5740
#define USBD_LANGID_STRING           0x409
#define USBD_MANUFACTURER_STRING     "BB_LE Virtual COM"
#define USBD_PRODUCT_HS_STRING       "BB_LE Bootloader VCP"
#define USBD_PRODUCT_FS_STRING       "BB_LE Bootloader VCP"
#define USBD_CONFIGURATION_HS_STRING "VCP HS BB_LE Config"
#define USBD_INTERFACE_HS_STRING     "VCP HS BB_LE Interface"
#define USBD_CONFIGURATION_FS_STRING "VCP FS BB_LE Config"
#define USBD_INTERFACE_FS_STRING     "VCP FS BB_LE Interface"

/* Private macro ------------------------------------------------------------- */
/* Private function prototypes ----------------------------------------------- */
uint8_t *USBD_VCP_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_VCP_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_VCP_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_VCP_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_VCP_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_VCP_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_VCP_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);

/* Private variables --------------------------------------------------------- */
USBD_DescriptorsTypeDef VCP_Desc = {
    USBD_VCP_DeviceDescriptor,    USBD_VCP_LangIDStrDescriptor, USBD_VCP_ManufacturerStrDescriptor, USBD_VCP_ProductStrDescriptor,
    USBD_VCP_SerialStrDescriptor, USBD_VCP_ConfigStrDescriptor, USBD_VCP_InterfaceStrDescriptor,
};

/* USB Standard Device Descriptor */
#if defined(__ICCARM__) /* !< IAR Compiler */
#pragma data_alignment = 4
#endif
__ALIGN_BEGIN uint8_t USBD_DeviceDesc[USB_LEN_DEV_DESC] __ALIGN_END = {
    0x12,                 /* bLength */
    USB_DESC_TYPE_DEVICE, /* bDescriptorType */
    0x00,                 /* bcdUSB */
    0x01,
    0x00,             /* bDeviceClass */
    0x00,             /* bDeviceSubClass */
    0x00,             /* bDeviceProtocol */
    USB_MAX_EP0_SIZE, /* bMaxPacketSize */
    LOBYTE(USBD_VID), /* idVendor */
    HIBYTE(USBD_VID), /* idVendor */
    LOBYTE(USBD_PID), /* idVendor */
    HIBYTE(USBD_PID), /* idVendor */
    0x00,             /* bcdDevice rel. 1.00 */
    0x01,
    USBD_IDX_MFC_STR,          /* Index of manufacturer string */
    USBD_IDX_PRODUCT_STR,      /* Index of product string */
    USBD_IDX_SERIAL_STR,       /* Index of serial number string */
    USBD_MAX_NUM_CONFIGURATION /* bNumConfigurations */
}; /* USB_DeviceDescriptor */

/* USB Standard Device Descriptor */
#if defined(__ICCARM__) /* !< IAR Compiler */
#pragma data_alignment = 4
#endif
__ALIGN_BEGIN uint8_t USBD_LangIDDesc[USB_LEN_LANGID_STR_DESC] __ALIGN_END = {
    USB_LEN_LANGID_STR_DESC,
    USB_DESC_TYPE_STRING,
    LOBYTE(USBD_LANGID_STRING),
    HIBYTE(USBD_LANGID_STRING),
};

#if defined(__ICCARM__) /* !< IAR Compiler */
#pragma data_alignment = 4
#endif
__ALIGN_BEGIN uint8_t USBD_StringSerial[USB_SIZ_STRING_SERIAL] __ALIGN_END = {
    USB_SIZ_STRING_SERIAL,
    USB_DESC_TYPE_STRING,
};

#if defined(__ICCARM__) /* !< IAR Compiler */
#pragma data_alignment = 4
#endif
__ALIGN_BEGIN uint8_t USBD_StrDesc[USBD_MAX_STR_DESC_SIZ] __ALIGN_END;

/* Private functions --------------------------------------------------------- */
static void Get_SerialNum(void);

/**
 * @brief  Returns the device descriptor.
 * @param  speed: Current device speed
 * @param  length: Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t *USBD_VCP_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(USBD_DeviceDesc);
    return (uint8_t *)USBD_DeviceDesc;
}

/**
 * @brief  Returns the LangID string descriptor.
 * @param  speed: Current device speed
 * @param  length: Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t *USBD_VCP_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(USBD_LangIDDesc);
    return (uint8_t *)USBD_LangIDDesc;
}

/**
 * @brief  Returns the product string descriptor.
 * @param  speed: Current device speed
 * @param  length: Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t *USBD_VCP_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    if (speed == 0)
    {
        USBD_GetString((uint8_t *)USBD_PRODUCT_HS_STRING, USBD_StrDesc, length);
    }
    else
    {
        USBD_GetString((uint8_t *)USBD_PRODUCT_FS_STRING, USBD_StrDesc, length);
    }
    return USBD_StrDesc;
}

/**
 * @brief  Returns the manufacturer string descriptor.
 * @param  speed: Current device speed
 * @param  length: Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t *USBD_VCP_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
    return USBD_StrDesc;
}

/**
 * @brief  Returns the serial number string descriptor.
 * @param  speed: Current device speed
 * @param  length: Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t *USBD_VCP_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = USB_SIZ_STRING_SERIAL;

    /* Update the serial number string descriptor with the data from the unique
     * ID */
    Get_SerialNum();

    return (uint8_t *)USBD_StringSerial;
}

/**
 * @brief  Returns the configuration string descriptor.
 * @param  speed: Current device speed
 * @param  length: Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t *USBD_VCP_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    if (speed == USBD_SPEED_HIGH)
    {
        USBD_GetString((uint8_t *)USBD_CONFIGURATION_HS_STRING, USBD_StrDesc, length);
    }
    else
    {
        USBD_GetString((uint8_t *)USBD_CONFIGURATION_FS_STRING, USBD_StrDesc, length);
    }
    return USBD_StrDesc;
}

/**
 * @brief  Returns the interface string descriptor.
 * @param  speed: Current device speed
 * @param  length: Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t *USBD_VCP_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    if (speed == 0)
    {
        USBD_GetString((uint8_t *)USBD_INTERFACE_HS_STRING, USBD_StrDesc, length);
    }
    else
    {
        USBD_GetString((uint8_t *)USBD_INTERFACE_FS_STRING, USBD_StrDesc, length);
    }
    return USBD_StrDesc;
}

/**
 * @brief  Create the serial number string descriptor
 * @param  None
 * @retval None
 */
static void Get_SerialNum(void)
{
    uint8_t deviceserial0[] = "12345678";
    for (uint8_t i = 0; i < 8; i++)
    {
        USBD_StringSerial[2 + (i * 2)] = deviceserial0[i];
    }
}
