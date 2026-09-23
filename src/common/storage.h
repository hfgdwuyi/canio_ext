/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for storage.c
 */
/*----------------------------------------------------------------------------*/
#ifndef STORAGE_H
#define STORAGE_H

extern const uint16_t storageSegmentsCount;

#define STORAGE_SEG_COMM  0
#define STORAGE_SEG_APPL  1
#define STORAGE_SEG_ASSET 2
#define STORAGE_SEG_CAN   3

void storageSegmentInit(uint8_t segment);
bool storageSaveSegment(uint8_t segment);
bool storageLoadSegment(uint8_t segment);
bool storageEraseSegment(uint8_t segment);

#endif
