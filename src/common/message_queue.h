/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Header file for message_queue.c
 */
/*----------------------------------------------------------------------------*/
#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

typedef enum
{
    MSGQ_OK = 0,
    MSGQ_FULL,
    MSGQ_EMPTY,
    MSGQ_ARGS,
} msgQueueStatus;

/*! Message queue tyoe definition */
typedef struct
{
    uint32_t rdIdx;
    uint32_t wrIdx;
    uint32_t elSize;
    uint32_t elNum;
    void    *data;
} messageQueue;

msgQueueStatus msgQueuePush(messageQueue *queue, const void *data);
msgQueueStatus msgQueuePop(messageQueue *queue, void *data);

#endif
