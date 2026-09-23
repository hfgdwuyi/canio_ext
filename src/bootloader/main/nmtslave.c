/*!
 * Copyright © Siemens Healthcare GmbH 2022, All Rights Reserved
 *
 * Project: Building Block Low End MCU
 *
 * @file
 * @brief Callback functions for NMT commands
 *
 * This module contains callback functions for NMT commands
 * specified in the CiA standard CiA-301.
 * These callback functions are called by the CANopen Library
 * when a NMT command was received.
 *
 */

// HAL headers
#include <hal.h>
/* headers of the CANopen Library  */
#include <cal_conf.h>
#include <co_usr.h>
#include <co_nmt.h>
#include <co_drv.h>
// Project headers
#include "prog_ctrl.h"


#ifdef CONFIG_RESET_APPL_PRE_CMD
/*******************************************************************/
/**
 * \brief resetApplPreInd - Reset Application commanded
 *
 * This function is called immediately after the receipt of
 * of the  NMT command Reset Node.
 * Here the application can be change to a safety state
 * before the Library resets all objects to their default values.
 * After the resetApplication was finished the indication function
 * resetApplInd()
 * is called.
 *
 * The user can enable this functionality by the CANopen Design Tool about:
 * General Settings / Advanced Configuration. In Advanced Configuration
 * the user has to define the compiler define CONFIG_RESET_APPL_PRE_CMD
 * in C syntax.
 *
 * \return
 * nothing
 *
 */
void resetApplPreInd(CO_REDCY_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    /* users_function() */
}
#endif /* CONFIG_RESET_APPL_PRE_CMD */


/*******************************************************************/
/**
 * \brief resetApplInd - Reset Application executed
 *
 * All manufacturer-specific and device-profile-specific CANopen objects
 * are set to the default values before this function is called.
 * This function can be used to reset the application and
 * load values from the nonvolatile memory.
 *
 * \return
 * nothing
 *
 */
void resetApplInd(CO_REDCY_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    // Set signature to start bootloader again
    progCtrlSetSignature(PROG_CTRL_START_BOOTLOADER);
    // Stop CAN
    Stop_CAN();
    // Perform Softreset
    needToReset = true;
}


#ifdef CO_CONFIG_RESET_COMM_PRE_CMD
/************************************************************************/
/**
 * \brief resetCommPreInd - Reset Communication commanded
 *
 * This function is called immediately after the receipt of
 * of the  NMT command Reset Communication.
 * Here the application can be change to a safety state
 * before the Library resets all communication objects to their default values.
 * After the Reset Communication handling was finished the indication function
 * resetCommInd() is called.
 *
 * Alternativly, the indication function resetCommPostInd() is called after the reset
 * handling and re-initilization. The function resetCommPostInd() needs to be enabled.
 *
 * The user can enable this functionality by the CANopen Design Tool about:
 * General Settings / Advanced Configuration. In Advanced Configuration
 * the user has to define the compiler define as follow:
 * define CO_CONFIG_RESET_COMM_PRE_CMD 1
 *
 * \return
 * nothing
 */
void resetCommPreInd(CO_REDCY_PARA_DECL /**< in: number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
}
#endif /* CO_CONFIG_RESET_COMM_PRE_CMD */


/*******************************************************************/
/**
 * \brief resetCommInd - Reset Communication executed
 *
 * This function is called after NMT command Reset Communication was received and
 * all CANopen communication objects are reset to the default values but before
 * any CANopen services are re-initialized. In this function the default values
 * can be altered and thus the re-initialization can be controlled.
 *
 * The node-ID is updated by calling the function getNodeId().
 * All node-id depending settings are reset to the
 * pre-defined connection set.
 *
 * After exiting this function the library will re-initialize all services using
 * the (possibly) altered values. After the re-initialization the indication function
 * resetCommPostInd() will be called (if it is enabled).
 *
 * Note: CANopen objects, which are overloaded here by values from
 * the nonvolatile memory for instance must be activated by calling
 * of the function setCommPar():
 *
 * \return
 * nothing
 *
 */
void resetCommInd(CO_REDCY_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    /* users_function() */
}

#ifdef CO_CONFIG_RESET_COMM_POST_CMD
/************************************************************************/
/**
 * \brief resetCommPostInd - Reset Communication commanded
 *
 * This function is called at the end of the execution
 * of the  NMT command Reset Communication and re-initialization of all
 * services.
 *
 * The user can enable this functionality by the CANopen Design Tool about:
 * General Settings / Advanced Configuration. In Advanced Configuration
 * the user has to define the compiler define as follow:
 * define CO_CONFIG_RESET_COMM_POST_CMD 1
 *
 * \return
 * nothing
 */
void resetCommPostInd(CO_REDCY_PARA_DECL /**< in: number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
}
#endif /* CO_CONFIG_RESET_COMM_POST_CMD */

/*******************************************************************/
/**
 * \brief newStateInd -  NMT state changed
 *
 * This function is called when a NMT command was received and
 * a state transition shall be executed.
 * The application has the possibility to do application-specific
 * actions before the NMT state transition is executed by the Library.
 * The NMT state transition into the NMT state OPERATIONAL can be rejected
 * by the application here.
 *
 * \retval CO_TRUE
 * NMT state transition can be executed
 * \retval CO_FALSE
 * do not change into the NMT state OPERATIONAL
 */
BOOL_T newStateInd(NODE_STATE_T newState        /**< new NMT state */
                       CO_COMMA_REDCY_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    switch (newState)
    {
        case STOPPED: break;
        case PRE_OPERATIONAL: break;
        case OPERATIONAL: break;
        default: break;
    }

    return CO_TRUE;
}

#ifdef CO_CONFIG_USER_NMT_MSG_IND
/*******************************************************************/
/**
 * \brief coUserNmtMsgInd - application-specific NMT state transitions
 *
 * This function makes it possible that a NMT state transtion can be
 * reject by the application.
 *
 * This functionality can be enabled by the CANopen Design Tool about:
 * General Settings / Non-standard Extensions /
 * Activate application-specific NMT message handling.
 *
 * ATTENTION: It is possible that the device does not pass the CiA
 * Conformance Test.
 *
 * \return
 * CANopen return value
 */
RET_T coUserNmtMsgInd(UNSIGNED8 newState          /**< new NMT state */
                          CO_COMMA_LINE_PARA_DECL /**< number of CAN line 0..CO_MAX_CAN_LINES-1 */
)
{
    RET_T retVal = CO_OK;

    switch (newState)
    {
        case OPERATIONAL: break;
        case STOPPED: break;
        default: break;
    }

    return retVal;
}
#endif /* CO_CONFIG_USER_NMT_MSG_IND */

/*______________________________________________________________________EOF_*/
