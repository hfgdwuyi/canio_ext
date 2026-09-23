/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for can_cfg.c
 */
/*----------------------------------------------------------------------------*/
#ifndef CAN_CFG_H
#define CAN_CFG_H

extern UNSIGNED8  nodeId;
extern UNSIGNED16 bitRate;

bool canCfgGetConfig(void);
void canCfgSetCode(void);

#endif    // CAN_CFG_H