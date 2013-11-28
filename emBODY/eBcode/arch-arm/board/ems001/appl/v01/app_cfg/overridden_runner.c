
/*
 * Copyright (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
 * Author:  Marco Accame
 * email:   marco.accame@iit.it
 * website: www.robotcub.org
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

#include "stdio.h"

//general
#include "stdlib.h"
#include "string.h"
#include "EoCommon.h"
#include "EOarray.h"
#include "EOtheErrorManager.h"

//appl
#include "EOMtheEMSappl.h"
#include "EOtheEMSApplBody.h"

//embobj-cfg-icub
#include "EoMotionControl.h"
#include "EoSensors.h"
#include "EoManagement.h"

#include "EOemsController_hid.h" 
#include "OPCprotocolManager_Cfg.h" 
#include "EOtheEMSapplDiagnostics.h"

#include "EOtheEMSapplDiagnostics.h"

#include "eventviewer.h"


// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "EOMtheEMSrunner.h"


// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------

#include "EOMtheEMSrunner_hid.h" 


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------
//if defined SET_DESIRED_CURR_IN_ONLY_ONE_MSG, the appl sends all desered current to 2fon in only one msg
#define SET_DESIRED_CURR_IN_ONLY_ONE_MSG
#define runner_timeout_send_diagnostics         1000
#define runner_countmax_check_ethlink_status    5000 //every one second

#define COUNT_WATCHDOG_VIRTUALSTRAIN_MAX        50



#if defined(EVIEWER_ENABLED)
#define EVIEWER_userDef_IDbase             (ev_ID_first_usrdef+1)
//#define EVIEWER_userDef_RUNRecRopframe     (EVIEWER_userDef_IDbase +1) see definition in EOMtheEMSrunner.c
#define EVIEWER_userDef_hwErrCntr     (EVIEWER_userDef_IDbase +3)
#endif

#if defined(EVIEWER_ENABLED)
void userDef_hwErrCntr(void){}
#endif


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables, but better using _get(), _set() 
// --------------------------------------------------------------------------------------------------------------------
// empty-section

#ifdef MC_CAN_DEBUG
//extern int32_t encoder_can_pos;
//extern int32_t encoder_can_vel;
extern int16_t torque_debug_can[4];
#endif

//uint16_t hysto_error[4]={0,0,0,0};

// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section


    
// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------

//RX
static void s_eom_emsrunner_hid_taskRX_act_afterdgramrec_skinOnly_mode(EOtheEMSapplBody *p);
static void s_eom_emsrunner_hid_taskRX_act_afterdgramrec_skinAndMc4_mode(EOtheEMSapplBody *p);
static void s_eom_emsrunner_hid_taskRX_act_afterdgramrec_2foc_mode(EOtheEMSapplBody *p);

//DO
static void s_eom_emsrunner_hid_userdef_taskDO_activity_2foc(EOMtheEMSrunner *p);
static void s_eom_emsrunner_hid_userdef_taskDO_activity_mc4(EOMtheEMSrunner *p);



//utils
static void s_eom_emsrunner_hid_readSkin(EOtheEMSapplBody *p);
static eOresult_t s_eom_emsrunner_hid_SetCurrentsetpoint(EOtheEMSapplBody *p, int16_t *pwmList, uint8_t size);
static void s_eom_emsrunner_hid_readMc4andMais(EOtheEMSapplBody *p);
static void s_eom_emsrunner_hid_UpdateJointstatus(EOMtheEMSrunner *p);

#ifdef SET_DESIRED_CURR_IN_ONLY_ONE_MSG
static eOresult_t s_eom_emsrunner_hid_SetCurrentsetpoint_inOneMsgOnly(EOtheEMSapplBody *p, int16_t *pwmList, uint8_t size);
#else
static eOresult_t s_eom_emsrunner_hid_SetCurrentsetpoint_with4msg(EOtheEMSapplBody *p, int16_t *pwmList, uint8_t size);
#endif
EO_static_inline  eOresult_t s_eom_emsrunner_hid_SetCurrentsetpoint(EOtheEMSapplBody *p, int16_t *pwmList, uint8_t size);
static void s_checkEthLinks(void);


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------

static uint16_t motionDoneJoin2Use = 0;
static uint16_t count_ethlink_status = 0;
static uint8_t count_watchdog_virtaulStrain = 0;
#if defined(EVIEWER_ENABLED) 
static uint8_t event_view = 0;
#endif 
// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------

extern void eom_emsrunner_hid_userdef_taskRX_activity_beforedatagramreception(EOMtheEMSrunner *p)
{
    //char str[100];
    eOmn_appl_runMode_t runmode =  eo_emsapplBody_GetAppRunMode(eo_emsapplBody_GetHandle());
    if(applrunMode__2foc == runmode)
    {
         eo_appEncReader_StartRead(eo_emsapplBody_GetEncoderReaderHandle(eo_emsapplBody_GetHandle()));
    }
    
    #if defined(EVIEWER_ENABLED)   
    if(0 == event_view)
    {   
        eventviewer_load(EVIEWER_userDef_hwErrCntr, userDef_hwErrCntr);
        event_view = 1;
    }
    #endif    


}

extern void eom_emsrunner_hid_userdef_taskRX_activity_afterdatagramreception(EOMtheEMSrunner *p)
{
    EOtheEMSapplBody* emsappbody_ptr = eo_emsapplBody_GetHandle();
    eOmn_appl_runMode_t runmode  =  eo_emsapplBody_GetAppRunMode(emsappbody_ptr);

    switch(runmode)
    {
        case applrunMode__skinOnly:
        {
            s_eom_emsrunner_hid_taskRX_act_afterdgramrec_skinOnly_mode(emsappbody_ptr);
        }break;
        
        case applrunMode__2foc:
        {
            s_eom_emsrunner_hid_taskRX_act_afterdgramrec_2foc_mode(emsappbody_ptr);
        }break;
        
        case applrunMode__skinAndMc4:
        {
            s_eom_emsrunner_hid_taskRX_act_afterdgramrec_skinAndMc4_mode(emsappbody_ptr);
        }break;
        
        case applrunMode__mc4Only:
        {
            ;
        }break;

        default:
        {
            ;
        };
    };
    
}



extern void eom_emsrunner_hid_userdef_taskDO_activity(EOMtheEMSrunner *p)
{
    EOtheEMSapplBody* emsappbody_ptr = eo_emsapplBody_GetHandle();
    eOmn_appl_runMode_t runmode =  eo_emsapplBody_GetAppRunMode(emsappbody_ptr);

    /* TAG_ALE */
//     if(applrunMode__skinAndMc4 == runmode)
//     {
//         #warning VALE--> remove this code after test on semaphore-can
//         s_eom_emsrunner_hid_SetCurrentsetpoint(appTheSP, pwm, 0);
//         s_eom_emsrunner_hid_SetCurrentsetpoint(appTheSP, pwm, 0);
//         s_eom_emsrunner_hid_SetCurrentsetpoint(appTheSP, pwm, 0);
//         s_eom_emsrunner_hid_SetCurrentsetpoint(appTheSP, pwm, 0);
//         s_eom_emsrunner_hid_SetCurrentsetpoint(appTheSP, pwm, 0);
//         s_eom_emsrunner_hid_SetCurrentsetpoint(appTheSP, pwm, 0);
//     }
    

    switch(runmode)
    {
        case applrunMode__2foc:
        {
            s_eom_emsrunner_hid_userdef_taskDO_activity_2foc(p);
        }break;
        
        case applrunMode__mc4Only:
        case applrunMode__skinAndMc4:
        {
            s_eom_emsrunner_hid_userdef_taskDO_activity_mc4(p);
        }break;
        
        case applrunMode__skinOnly:
        {
            return; //currently nothing to do 
        }//break;
        
        default:
        {
            return;
        }
    };
  
}


extern void eom_emsrunner_hid_userdef_taskTX_activity_beforedatagramtransmission(EOMtheEMSrunner *p)
{

    EOtheEMSapplBody* emsappbody_ptr = eo_emsapplBody_GetHandle();
    uint8_t numoftxframe_p1, numoftxframe_p2;
    //eOresult_t res;
    
#ifdef _USE_PROTO_TEST_
    eOmc_setpoint_t     mySetPoint_current = 
    {
        EO_INIT(.type)       eomc_setpoint_current,
        EO_INIT(.to)
        {
            EO_INIT(.current)
            {
                EO_INIT(.value)     0
            }   
        }
    };
#endif
   
//following activities are independent on runmode
    
    #ifdef MC_CAN_DEBUG
    //uint8_t payload[8];
    //((int32_t*)payload)[0]=encoder_can_pos;
    //((int32_t*)payload)[1]=encoder_can_vel;
    //eo_appCanSP_SendMessage_TEST(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), payload);
    static uint16_t cnt = 0;
    
    if (++cnt >= 1000)
    {
        cnt = 0;
        
        eo_appCanSP_SendMessage_TEST(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), (uint8_t*)torque_debug_can);
    }
    #endif
    
    /*
    static uint16_t hysto_error_timer = 0;
    
    if (++hysto_error_timer >= 1000)
    {
        uint8_t payload[8];
        ((uint16_t*)payload)[0]=hysto_error[1];
        ((uint16_t*)payload)[1]=hysto_error[2];
        ((uint16_t*)payload)[2]=hysto_error[3];
        eo_appCanSP_SendMessage_TEST(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), payload);

        hysto_error_timer = 0;
        
        hysto_error[1] = 0;
        hysto_error[2] = 0;
        hysto_error[3] = 0;
    }
    */
    
    
 

 
    
//     res = eo_appCanSP_StartTransmitCanFrames(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), eOcanport1, eobool_true);
//     if(eores_OK != res)
//     {
//         return;
//     }        

//     res = eo_appCanSP_StartTransmitCanFrames(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), eOcanport2, eobool_true);
//     if(eores_OK != res)
//     {
//         return;
//     }        
    
    eo_appCanSP_starttransmit_XXX(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), eOcanport1, &numoftxframe_p1);
    eo_appCanSP_starttransmit_XXX(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), eOcanport2, &numoftxframe_p2);
    
#ifdef _GET_CANQUEUE_STATISTICS_
    eo_theEMSdgn_updateCanTXqueueStatisticsOnRunMode(eOcanport1, numoftxframe_p1);
    eo_theEMSdgn_updateCanTXqueueStatisticsOnRunMode(eOcanport2, numoftxframe_p2);
#endif

}



extern void eom_emsrunner_hid_userdef_taskTX_activity_afterdatagramtransmission(EOMtheEMSrunner *p)
{
    EOtheEMSapplBody* emsappbody_ptr = eo_emsapplBody_GetHandle();
    eOresult_t res[2];
    eOmn_appl_runMode_t runmode =  eo_emsapplBody_GetAppRunMode(emsappbody_ptr);
    
/* METODO 1 */
//     res[0] = eo_appCanSP_WaitTransmitCanFrames(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), eOcanport1);
//     res[1] = eo_appCanSP_WaitTransmitCanFrames(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), eOcanport2);

 /* METODO 2 */   
//     eo_appCanSP_GetNumOfTxCanframe(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), eOcanport1, &numofframe_tx);
//     while(1)
//     {
//        eo_appCanSP_GetNumOfTxCanframe(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), eOcanport1, &numofframe_tx);
//        if(numofframe_tx == 0)
//        {
//            break;
//        }
//     }

/* METODO 3 */    
//     eo_appCanSP_StartTransmitAndWait(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), eOcanport1);
//     eo_appCanSP_StartTransmitAndWait(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), eOcanport2);
    
 
 
 //before wait can, check link status!!
    count_ethlink_status ++;
    if(runner_countmax_check_ethlink_status == count_ethlink_status)
    {
        //uint8_t link1_isup;
        //uint8_t link2_isup;
		//this func chacks if one of link change state and notify it.
		//the pkt arrived on pc104 backdoor when one link change down->up.
        //eo_theEMSdgn_checkEthLinksStatus_quickly(eo_theEMSdgn_GetHandle(), &link1_isup, &link2_isup);
        if(eo_dgn_cmds.signalCanStatistics)
        {
            eo_theEMSdgn_Signalerror(eo_theEMSdgn_GetHandle(), eodgn_nvidbdoor_canQueueStatistics , 0);
        }
        count_ethlink_status = 0;
    }
/* METODO 4*/

    res[0] = eo_appCanSP_wait_XXX(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), eOcanport1);
    res[1] = eo_appCanSP_wait_XXX(eo_emsapplBody_GetCanServiceHandle(emsappbody_ptr), eOcanport2);
    if((eores_NOK_timeout ==  res[0]) || (eores_NOK_timeout ==  res[1]))
    {
        eo_dgn_emsapplcore.core.runst.cantxfailuretimeoutsemaphore++;
        eo_theEMSdgn_Signalerror(eo_theEMSdgn_GetHandle(), eodgn_nvidbdoor_emsapplcommon , runner_timeout_send_diagnostics);

    }
    
}


extern void eom_emsrunner_hid_userdef_onfailedtransmission(EOMtheEMSrunner *p)
{
    eOemsrunner_diagnosticsinfo_t *dgn_ptr = eom_emsrunner_GetDiagnosticsInfoHandle(p);
    if(NULL == dgn_ptr)
    {
        return;
    }
    eo_theEMSdgn_UpdateApplCore(eo_theEMSdgn_GetHandle());
    eo_theEMSdgn_Signalerror(eo_theEMSdgn_GetHandle(), eodgn_nvidbdoor_emsapplcommon , runner_timeout_send_diagnostics);

}

extern void eom_emsrunner_hid_userdef_onemstransceivererror(EOMtheEMStransceiver *p)
{
    eOemstransceiver_diagnosticsinfo_t* dgn_ptr = eom_emstransceiver_GetDiagnosticsInfoHandle(p);
    
    if(NULL == dgn_ptr)
    {
        return;
    }
    eo_theEMSdgn_UpdateApplCore(eo_theEMSdgn_GetHandle());
    eo_theEMSdgn_Signalerror(eo_theEMSdgn_GetHandle(), eodgn_nvidbdoor_emsapplcommon , runner_timeout_send_diagnostics);
    
}
// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------
static void s_eom_emsrunner_hid_taskRX_act_afterdgramrec_skinOnly_mode(EOtheEMSapplBody *p)
{
    s_eom_emsrunner_hid_readSkin(p); 
}

static void s_eom_emsrunner_hid_taskRX_act_afterdgramrec_2foc_mode(EOtheEMSapplBody *p)
{
    uint8_t     numofRXcanframe = 0;
    eOresult_t  res;

    //read can msg from port 1
    res = eo_appCanSP_GetNumOfRecCanframe(eo_emsapplBody_GetCanServiceHandle(p), eOcanport1, &numofRXcanframe);
    eo_errman_Assert(eo_errman_GetHandle(), (eores_OK == res), "emsrunner_hid", "err in GetNumOfRecCanframe");
    
#ifdef _GET_CANQUEUE_STATISTICS_
   eo_theEMSdgn_updateCanRXqueueStatisticsOnRunMode(eOcanport1, numofRXcanframe);
#endif

    eo_appCanSP_read(eo_emsapplBody_GetCanServiceHandle(p), eOcanport1, numofRXcanframe, NULL);

    
    
    //read msg from port 2
    res = eo_appCanSP_GetNumOfRecCanframe(eo_emsapplBody_GetCanServiceHandle(p), eOcanport2, &numofRXcanframe);
    eo_errman_Assert(eo_errman_GetHandle(), (eores_OK == res), "emsrunner_hid", "err in GetNumOfRecCanframe");
    
#ifdef _GET_CANQUEUE_STATISTICS_
   eo_theEMSdgn_updateCanRXqueueStatisticsOnRunMode(eOcanport2, numofRXcanframe);
#endif
    
    eo_appCanSP_read(eo_emsapplBody_GetCanServiceHandle(p), eOcanport2, numofRXcanframe, NULL);
}
static void s_eom_emsrunner_hid_taskRX_act_afterdgramrec_skinAndMc4_mode(EOtheEMSapplBody *p)
{
        
    /* 1) get can frames about skin and process them*/
    s_eom_emsrunner_hid_readSkin(p);
    
    /* 2) get can frame from mais and mc4 board and process them */
    s_eom_emsrunner_hid_readMc4andMais(p);
    
    //something else about mc4 ??????

}

static void s_eom_emsrunner_hid_readSkin(EOtheEMSapplBody *p)
{
    eOsk_skinId_t     sId = 0;//only one skin. (evenif skin is composed by more skin board, here we see the skin like a unique identity)
//    eOskin_status_t     *skstatus_ptr;
    EOarray_of_10canframes *arrayof10canframes_ptr = NULL;
    eOresult_t res;
    

//uncomment this code when status and config nvvar are used.   
//     res = eo_appTheDB_GetSkinStatusPtr(eo_appTheDB_GetHandle(), sId,  &skstatus_ptr);
//     eo_errman_Assert(eo_errman_GetHandle(), (eores_OK == res), "emsrunner_hid", "err in GetSkinNVMemoryRef");

//     eo_array_Reset((EOarray*)(&skstatus_ptr->arrayof10canframes));

    
    //1) get pointer to nv array and reset it

    res = eo_appTheDB_GetSkinStArray10CanFramesPtr(eo_appTheDB_GetHandle(), sId,  &arrayof10canframes_ptr);
    eo_errman_Assert(eo_errman_GetHandle(), (eores_OK == res), "emsrunner_hid", "err in GetSkinStArray10CanFramesPtr");
    
    eo_array_Reset((EOarray*)arrayof10canframes_ptr);

#ifdef _GET_CANQUEUE_STATISTICS_
    uint8_t numofRXcanframe=0;
    eo_appCanSP_GetNumOfRecCanframe(eo_emsapplBody_GetCanServiceHandle(p), eOcanport2, &numofRXcanframe);
    eo_theEMSdgn_updateCanRXqueueStatisticsOnRunMode(eOcanport2, numofRXcanframe);
#endif
    eo_appCanSP_read(eo_emsapplBody_GetCanServiceHandle(p), eOcanport2, 10, NULL); //10 is the max size of sk_array

}

static void s_eom_emsrunner_hid_readMc4andMais(EOtheEMSapplBody *p)
{
    eOsnsr_maisId_t                 sId = 0; //only one mais per ems
    eOsnsr_mais_status_t            *sstatus_ptr;
    eOresult_t                      res;
    uint8_t                         numofRXcanframe = 10; //default num
    
    //1) reset nv array
    res = eo_appTheDB_GetSnrMaisStatusPtr(eo_appTheDB_GetHandle(), sId,  &sstatus_ptr);
    eo_errman_Assert(eo_errman_GetHandle(), (eores_OK == res), "emsrunner_hid", "err in GetSnrMaisStatusPtr");

    //reset array
    sstatus_ptr->the15values.head.size = 0;
    sstatus_ptr->the15values.data[0] = 0;

    res = eo_appCanSP_GetNumOfRecCanframe(eo_emsapplBody_GetCanServiceHandle(p), eOcanport1, &numofRXcanframe);
    eo_errman_Assert(eo_errman_GetHandle(), (eores_OK == res), "emsrunner_hid", "err in GetNumOfRecCanframe");

#ifdef _GET_CANQUEUE_STATISTICS_
   eo_theEMSdgn_updateCanRXqueueStatisticsOnRunMode(eOcanport1, numofRXcanframe);
#endif
    eo_appCanSP_read(eo_emsapplBody_GetCanServiceHandle(p), eOcanport1, numofRXcanframe, NULL);

}



EO_static_inline  eOresult_t s_eom_emsrunner_hid_SetCurrentsetpoint(EOtheEMSapplBody *p, int16_t *pwmList, uint8_t size)
{
    
#ifdef SET_DESIRED_CURR_IN_ONLY_ONE_MSG
    return(s_eom_emsrunner_hid_SetCurrentsetpoint_inOneMsgOnly(p, pwmList, size));
#else	
    return(s_eom_emsrunner_hid_SetCurrentsetpoint_with4msg(p, pwmList,size));
#endif
}

#ifndef SET_DESIRED_CURR_IN_ONLY_ONE_MSG
static eOresult_t s_eom_emsrunner_hid_SetCurrentsetpoint_with4msg(EOtheEMSapplBody *p, int16_t *pwmList, uint8_t size)
{
    eOresult_t 				err;
	eOmeas_current_t            value;
    eOresult_t 				    res = eores_OK;
    eOicubCanProto_msgCommand_t msgCmd = 
    {
        EO_INIT(.class) eo_icubCanProto_msgCmdClass_pollingMotorBoard,
        EO_INIT(.cmdId) ICUBCANPROTO_POL_MB_CMD__SET_DISIRED_CURRENT
    };	

#warning VALE --> solo per test
    pwmList[0] = 0x11AA;
    pwmList[1] = 0x12AA;
    pwmList[2] = 0x13AA;
    pwmList[3] = 0x14AA;
    uint16_t numofjoint = eo_appTheDB_GetNumeberOfConnectedJoints(eo_appTheDB_GetHandle());

    for (uint8_t jid = 0; jid <numofjoint; jid++)
    {
        value = pwmList[jid];
    
        /*Since in run mode the frame are sent on demnad...here i can punt in tx queue frame to send.
        they will be sent by transmitter */

        err = eo_appCanSP_SendCmd2Joint(eo_emsapplBody_GetCanServiceHandle(p), jid, msgCmd, (void*)&value);
        
        if (err != eores_OK)
        {
            res = err;
        }
    }
    
    return(res);
   
}
#endif

#ifdef SET_DESIRED_CURR_IN_ONLY_ONE_MSG
static eOresult_t s_eom_emsrunner_hid_SetCurrentsetpoint_inOneMsgOnly(EOtheEMSapplBody *p, int16_t *pwmList, uint8_t size)
{
    eOresult_t 				                res = eores_OK;
    int16_t                                 pwm_aux[4] = {0, 0, 0, 0};
    eOappTheDB_jointOrMotorCanLocation_t    canLoc;
    eOicubCanProto_msgDestination_t         dest;
    eOicubCanProto_msgCommand_t             msgCmd = 
    {
        EO_INIT(.class) eo_icubCanProto_msgCmdClass_periodicMotorBoard,
        EO_INIT(.cmdId) ICUBCANPROTO_PER_MB_CMD_EMSTO2FOC_DESIRED_CURRENT
    };


    uint16_t numofjoint = eo_appTheDB_GetNumeberOfConnectedJoints(eo_appTheDB_GetHandle());
    
    for (uint8_t jid = 0; jid <numofjoint; ++jid)
    {
        res = eo_appTheDB_GetJointCanLocation(eo_appTheDB_GetHandle(), jid,  &canLoc, NULL);
        if(eores_OK != res)
        {
            return(res); //i think i'll be never here
        }
        
        pwm_aux[canLoc.addr-1] = pwmList[jid];    
    }
    //since msg is periodic, all 2foc received it so dest is useless.
    dest.dest = 0;
    eo_appCanSP_SendCmd(eo_emsapplBody_GetCanServiceHandle(p), canLoc.emscanport, dest, msgCmd, (void*)pwm_aux);
    
    return(res);    
}
#endif


static void s_eom_emsrunner_hid_userdef_taskDO_activity_2foc(EOMtheEMSrunner *p)
{
    eOresult_t          res;
    EOtheEMSapplBody    *emsappbody_ptr = eo_emsapplBody_GetHandle();
    uint32_t            encvalue[4] = {(uint32_t)ENC_INVALID, (uint32_t)ENC_INVALID, (uint32_t)ENC_INVALID, (uint32_t)ENC_INVALID};
    int16_t             pwm[4];
    static uint8_t             mask = 0;
//     static uint32_t             count_mask = 0;
//     #define     COUNT_MASK_MAX      2000

    uint16_t numofjoint = eo_appTheDB_GetNumeberOfConnectedJoints(eo_appTheDB_GetHandle());

    if (eo_appEncReader_isReady(eo_emsapplBody_GetEncoderReaderHandle(emsappbody_ptr)))
    {    
        for (uint8_t enc = 0; enc < numofjoint; ++enc)
        {
            res = eo_appEncReader_GetValue(eo_emsapplBody_GetEncoderReaderHandle(emsappbody_ptr), (eOappEncReader_encoder_t)enc, &(encvalue[enc]));
            
            if (res != eores_OK)
            {
                //if (enc == 3) ++hysto_error[encvalue[3]];
                
                encvalue[enc] = (uint32_t)ENC_INVALID;
            }
        }        
    }

    eo_emsController_ReadEncoders((int32_t*)encvalue);
        
    /* 2) pid calc */
    eo_emsController_PWM(pwm);

    /* 3) prepare and punt in rx queue new setpoint */
    s_eom_emsrunner_hid_SetCurrentsetpoint(emsappbody_ptr, pwm, 4); //4: eo_emsController_PWM fills an arry of 4 item 
                                                                    //because max num of mortor for each ems is 4 
 
    /* 4) update joint status */
    s_eom_emsrunner_hid_UpdateJointstatus(p);
    /*Note: motor status is updated with data sent by 2foc by can */
    
    /* 5) update */
    if(eo_emsController_GetLimitedCurrentMask(&mask))
    {
        eo_theEMSdgn_UpdateDummyFieldOfApplWithMc(eo_theEMSdgn_GetHandle(), (uint32_t)mask);
        eo_theEMSdgn_Signalerror(eo_theEMSdgn_GetHandle(), eodgn_nvidbdoor_emsapplmc , 0);
    }


    s_checkEthLinks();

}


static void s_eom_emsrunner_hid_UpdateJointstatus(EOMtheEMSrunner *p)
{
    eOmc_joint_status_t             *jstatus_ptr;
    eOmc_jointId_t                  jId;
    eOresult_t                      res;
    uint16_t                        numofjoint;
    
    numofjoint = eo_appTheDB_GetNumeberOfConnectedJoints(eo_appTheDB_GetHandle());
    for(jId = 0; jId<numofjoint; jId++)
    {
        res = eo_appTheDB_GetJointStatusPtr(eo_appTheDB_GetHandle(), jId, &jstatus_ptr);
        if(eores_OK != res)
        {
            return; //error
        }
        
        eo_emsController_GetJointStatus(jId, &jstatus_ptr->basic);
        
        eo_emsController_GetActivePidStatus(jId, &jstatus_ptr->ofpid);
        
        if(eomc_motionmonitorstatus_setpointnotreachedyet == jstatus_ptr->basic.motionmonitorstatus)
        {
            /* if motionmonitorstatus is equal to _setpointnotreachedyet, i send motion done message. 
            - if (motionmonitorstatus == eomc_motionmonitorstatus_setpointisreached), i don't send
            message because the setpoint is alredy reached. this means that:
                - if monitormode is forever, no new set point has been configured 
                - if monitormode is _untilreached, the joint reached the setpoint already.
            - if (motionmonitorstatus == eomc_motionmonitorstatus_notmonitored), i don't send
            message because pc104 is not interested in getting motion done.
            */
            if(eo_emsController_GetMotionDone(jId))
            {
                jstatus_ptr->basic.motionmonitorstatus = eomc_motionmonitorstatus_setpointisreached;
            }
        }
        
        
    }
}

static void s_eom_emsrunner_hid_userdef_taskDO_activity_mc4(EOMtheEMSrunner *p)
{
    eOresult_t                              res;
    uint8_t                                 send_virtualStrainData;
    uint16_t                                numofjoint;
    eOmc_joint_status_t                     *jstatus_ptr;
    uint16_t                                *virtStrain_ptr;
    eOappTheDB_jointOrMotorCanLocation_t    canLoc;
    EOappTheDB                              *db_ptr; 
    eOicubCanProto_msgDestination_t         msgdest;
    eOicubCanProto_msgCommand_t             msgCmd = 
    {
        EO_INIT(.class) eo_icubCanProto_msgCmdClass_pollingMotorBoard,
        EO_INIT(.cmdId) ICUBCANPROTO_POL_MB_CMD__MOTION_DONE
    };
    
    EOappCanSP *appCanSP_ptr = eo_emsapplBody_GetCanServiceHandle(eo_emsapplBody_GetHandle());
    
    db_ptr = eo_appTheDB_GetHandle();
    
    numofjoint = eo_appTheDB_GetNumeberOfConnectedJoints(db_ptr);

    res = eo_appTheDB_GetJointStatusPtr(db_ptr, motionDoneJoin2Use, &jstatus_ptr);
    if(eores_OK != res)
    {
        return; //error
    }
    
    if(jstatus_ptr->basic.motionmonitorstatus == eomc_motionmonitorstatus_setpointnotreachedyet)
    {
        /* if motionmonitorstatus is equal to _setpointnotreachedyet, i send motion done message. 
        - if (motionmonitorstatus == eomc_motionmonitorstatus_setpointisreached), i don't send
        message because the setpoint is alredy reached. this means that:
            - if monitormode is forever, no new set point has been configured 
            - if monitormode is _untilreached, the joint reached the setpoint already.
        - if (motionmonitorstatus == eomc_motionmonitorstatus_notmonitored), i don't send
        message because pc104 is not interested in getting motion done.
        */
        eo_appCanSP_SendCmd2Joint(appCanSP_ptr, motionDoneJoin2Use, msgCmd, NULL);
    }
    
    motionDoneJoin2Use++;
    if(motionDoneJoin2Use == numofjoint)
    {
        motionDoneJoin2Use = 0;
    }
    
 

    //prepare virtual strain data
    if(eo_appTheDB_IsVirtualStrainDataUpdated(db_ptr))
    {
        send_virtualStrainData = 1;
        count_watchdog_virtaulStrain = 0;
    }
    else
    {
        count_watchdog_virtaulStrain++;
        if(count_watchdog_virtaulStrain <= COUNT_WATCHDOG_VIRTUALSTRAIN_MAX)
        {
            send_virtualStrainData = 1;
        }
        else
        {
            send_virtualStrainData = 0;
        }

    }
    
    
    
    if(send_virtualStrainData)
    {
        eo_appTheDB_GetVirtualStrainDataPtr(db_ptr, &virtStrain_ptr);
        
        res = eo_appTheDB_GetJointCanLocation(db_ptr, 0,  &canLoc, NULL);
        if(eores_OK != res)
        {
            return;
        }

        //set destination of all messages 
        msgdest.dest = ICUBCANPROTO_MSGDEST_CREATE(0, 12); //virtual ft sensor has address 12
        
        //set command (calss + id) and send it
        msgCmd.class = eo_icubCanProto_msgCmdClass_periodicSensorBoard;
        msgCmd.cmdId = ICUBCANPROTO_PER_SB_CMD__FORCE_VECTOR;
        eo_appCanSP_SendCmd(appCanSP_ptr, canLoc.emscanport, msgdest, msgCmd, (void*)&virtStrain_ptr[0]);
        
        msgCmd.cmdId = ICUBCANPROTO_PER_SB_CMD__TORQUE_VECTOR;
        eo_appCanSP_SendCmd(appCanSP_ptr, canLoc.emscanport, msgdest, msgCmd, (void*)&virtStrain_ptr[3]);
    }
    s_checkEthLinks();
    
//     for(jId = 0; jId<numofjoint; jId++)
//     {
//         res = eo_appTheDB_GetJointStatusPtr(eo_appTheDB_GetHandle(), jId, &jstatus_ptr);
//         if(eores_OK != res)
//         {
//             return; //error
//         }
//         
//         if(jstatus_ptr->basic.motionmonitorstatus == eomc_motionmonitorstatus_setpointnotreachedyet)
//         {
//             /* if motionmonitorstatus is equal to _setpointnotreachedyet, i send motion done message. 
//             - if (motionmonitorstatus == eomc_motionmonitorstatus_setpointisreached), i don't send
//             message because the setpoint is alredy reached. this means that:
//                 - if monitormode is forever, no new set point has been configured 
//                 - if monitormode is _untilreached, the joint reached the setpoint already.
//             - if (motionmonitorstatus == eomc_motionmonitorstatus_notmonitored), i don't send
//             message because pc104 is not interested in getting motion done.
//             */
//             eo_appCanSP_SendCmd2Joint(appCanSP_ptr, jId, msgCmd, NULL);
//         }

//     }   
}




#define ethLinksCount_max 3000 //5 sec
static void s_checkEthLinks(void)
{
    static uint32_t ethLinksCount;
    static uint32_t ethLinksCount_partial;
    static uint8_t linknum = 255;
    
    ethLinksCount++;
    
    if((ethLinksCount< ethLinksCount_max) && (255 == linknum))
    {
        return;
    }

    if(255 == linknum)
    {
        linknum = 0;
        #if defined(EVIEWER_ENABLED)    
        evEntityId_t prev = eventviewer_switch_to(EVIEWER_userDef_hwErrCntr);
        #endif
        
        eo_theEMSdgn_checkEthLinkErrors(eo_theEMSdgn_GetHandle(), linknum);
        
        #if defined(EVIEWER_ENABLED)    
        eventviewer_switch_to(prev);
        #endif  
        linknum++;
    }
    else
    {
        if(ethLinksCount_partial == 20)
        {
            ethLinksCount_partial = 0;
            #if defined(EVIEWER_ENABLED)    
            evEntityId_t prev = eventviewer_switch_to(EVIEWER_userDef_hwErrCntr);
            #endif
            
            eo_theEMSdgn_checkEthLinkErrors(eo_theEMSdgn_GetHandle(), linknum);
            
            #if defined(EVIEWER_ENABLED)    
            eventviewer_switch_to(prev);
            #endif  
            linknum++;
            if(linknum == 3)
            {
                if((eo_dgn_cmds.signalEthCounters) && (eo_theEMSdgn_EthLinksInError(eo_theEMSdgn_GetHandle())) )
                //if((eo_dgn_cmds.signalEthCounters) )
                {
                    eo_theEMSdgn_Signalerror(eo_theEMSdgn_GetHandle(), eodgn_nvidbdoor_emsperiph , 0);
                }

                linknum = 255;
                ethLinksCount = 0;
            }
        }
    }
    ethLinksCount_partial++;
}
// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------



