/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 *  @file
 *  @brief     Hanldes the UART queue interrupt procedure
 */
/*----------------------------------------------------------------------------*/
// Standard includes
#include <stdint.h>
#include <string.h>
// Project includes
#include "message_queue.h"

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Adds data from UART queue
 * @param  queue - pointer to UART array
 * @param  data  - pointer to received character from UART
 * @retval Status of operation
 */
/*----------------------------------------------------------------------------*/
msgQueueStatus msgQueuePush(messageQueue *queue, const void *data)
{
    // Check pointers
    if ((queue == NULL) || (data == NULL))
    {
        return MSGQ_ARGS;
    }
    // Check if there is room in the queue
    if (queue->wrIdx - queue->rdIdx < queue->elNum)
    {
        // Push data to the queue

        uint32_t offset = (queue->wrIdx % queue->elNum) * queue->elSize;
        queue->wrIdx++;
        memcpy((queue->data + offset), data, queue->elSize);
    }
    else
    {
        return MSGQ_FULL;
    }
    return MSGQ_OK;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief  Deletes oone character from UART queue
 * @param  queue - pointer to UART array
 * @param  data  - pointer to received character from UART
 * @retval Status of operation
 */
/*----------------------------------------------------------------------------*/
msgQueueStatus msgQueuePop(messageQueue *queue, void *data)
{
    // Check pointers
    if ((queue == NULL) || (data == NULL))
    {
        return MSGQ_ARGS;
    }
    if (queue->wrIdx > queue->rdIdx)
    {
        // Pop data from the queue
        uint32_t offset = (queue->rdIdx % queue->elNum) * queue->elSize;
        queue->rdIdx++;
        memcpy(data, (queue->data + offset), queue->elSize);
    }
    else
    {
        return MSGQ_EMPTY;
    }
    return MSGQ_OK;
}
