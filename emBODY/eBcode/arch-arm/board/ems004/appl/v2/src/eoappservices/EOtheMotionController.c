/*
 * Copyright (C) 2015 iCub Facility - Istituto Italiano di Tecnologia
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

#include "stdlib.h"
#include "string.h"

#include "EoCommon.h"
#include "EOtheErrorManager.h"
#include "EoError.h"
#include "EOtheEntities.h"

#include "EOtheCANservice.h"
#include "EOtheCANmapping.h"
#include "EOtheCANprotocol.h"

#include "EoProtocolAS.h"
#include "EoProtocolMC.h"

#include "EOMtheEMSappl.h"

#include "EOmcController.h"
#include "EOemsController.h"

#include "hal_sys.h"
#include "hal_motor.h"
#include "hal_adc.h"
#include "hal_quad_enc.h"

#include "EOVtheCallbackManager.h"

//#warning TODO: i have kept inclusion of EOemsControllerCfg.h, but it must be removed. read following comment
// there must be another way to propagate externally to the motor-controller library some properties .... macros must be removed
#warning TODO: -> remove inclusion of EOemsControllerCfg.h and find a better mode to propagate the macro USE_ONLY_QE
#include "EOemsControllerCfg.h"

// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "EOtheMotionController.h"



// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------

#include "EOtheMotionController_hid.h"


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables. deprecated: better using _get(), _set() on static variables 
// --------------------------------------------------------------------------------------------------------------------
// empty-section

// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------

static void s_eo_motioncontrol_send_periodic_error_report(void *p);

static eOresult_t s_eo_motioncontrol_mc4plus_onendofverify_encoder(EOaService* s, eObool_t operationisok);

static eOresult_t s_eo_motioncontrol_onendofverify_encoder(EOaService* s, eObool_t operationisok);

static eOresult_t s_eo_motioncontrol_onstop_search4focs(void *par, EOtheCANdiscovery2* p, eObool_t searchisok);

static eOresult_t s_eo_motioncontrol_SetCurrentSetpoint(EOtheMotionController *p, int16_t *pwmList, uint8_t size);

static eOresult_t s_eo_motioncontrol_onendofverify_mais(EOaService* s, eObool_t operationisok);

static eOresult_t s_eo_motioncontrol_onstop_search4mc4s(void *par, EOtheCANdiscovery2* p, eObool_t searchisok);
    
static void s_eo_motioncontrol_UpdateJointStatus(EOtheMotionController *p);


static void s_eo_motioncontrol_proxy_config(eObool_t on);

static eObool_t s_eo_motioncontrol_mc4based_variableisproxied(eOnvID32_t id);

static void s_eo_motioncontrol_mc4plusbased_hal_init_motors_adc_feedbacks(void);
static void s_eo_motioncontrol_mc4plusbased_hal_init_quad_enc_indexes_interrupt(void);
static void s_eo_motioncontrol_mc4plusbased_enable_all_motors(EOtheMotionController *p);

// todo: rename them with usual prefix and hal_XXX
static void s_eo_mcserv_pwm_set(uint8_t i, int16_t v);
static void s_eo_mcserv_pwm_enable(uint8_t i);
static void s_eo_mcserv_pwm_disable(uint8_t i);
static eObool_t s_eo_mcserv_are_motors_ext_faulted(void);

static eOresult_t s_eo_mcserv_do_mc4plus(EOtheMotionController *p);

static void s_eo_mcserv_disable_all_motors(EOtheMotionController *p);

// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------

static EOtheMotionController s_eo_themotcon = 
{
    .service = 
    {
        .tmpcfg                 = NULL,
        .servconfig             = { .type = eomn_serv_NONE },
        .initted                = eobool_false,
        .active                 = eobool_false,
        .activateafterverify    = eobool_false,
        .running                = eobool_false,
        .onverify               = NULL            
    },
    .diagnostics = 
    {
        .reportTimer            = NULL,
        .reportPeriod           = 10*EOK_reltime1sec,
        .errorDescriptor        = {0},
        .errorType              = eo_errortype_info,
        .errorCallbackCount     = 0,
        .repetitionOKcase       = 10
    },     
    .sharedcan =
    {
        .boardproperties        = NULL,
        .entitydescriptor       = NULL,
        .discoverytarget        = {0},
        .ondiscoverystop        = {0},
        .command                = {0}, 
    },
    
    .numofjomos                 = 0,

    .mcfoc = 
    {
        .thecontroller          = NULL,
        .theencoderreader       = NULL
    },
    .mcmc4 =
    {
        .servconfigmais         = {0},
        .themais                = NULL,
        .themc4boards           = NULL
    },
    .mcmc4plus = 
    {
        .thecontroller          = NULL,
        .theencoderreader       = NULL,
        .pwmvalue               = {0},
        .pwmport                = {0}        
    }
};

static const char s_eobj_ownname[] = "EOtheMotionController";


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------


extern EOtheMotionController* eo_motioncontrol_Initialise(void)
{
    if(eobool_true == s_eo_themotcon.service.initted)
    {
        return(&s_eo_themotcon);
    }
    
    s_eo_themotcon.service.active = eobool_false;
    s_eo_themotcon.service.running = eobool_false;

    s_eo_themotcon.numofjomos = 0;

    s_eo_themotcon.service.tmpcfg = NULL;
    s_eo_themotcon.service.servconfig.type = eomn_serv_NONE;
    
    // up to to 12 mc4 OR upto 4 foc (the MAIS is managed directly by the EOtheMAIS object)
    //#warning CHECK: we could use only 3 mc4 boards instead of 12 in candiscovery. shall we do it?
    s_eo_themotcon.sharedcan.boardproperties = eo_vector_New(sizeof(eOcanmap_board_properties_t), eo_motcon_maxJOMOs, NULL, NULL, NULL, NULL);
    
    // up to 12 jomos
    s_eo_themotcon.sharedcan.entitydescriptor = eo_vector_New(sizeof(eOcanmap_entitydescriptor_t), eo_motcon_maxJOMOs, NULL, NULL, NULL, NULL);
    
    s_eo_themotcon.mcfoc.thecontroller = s_eo_themotcon.mcmc4plus.thecontroller  = NULL;
    s_eo_themotcon.mcfoc.theencoderreader = s_eo_themotcon.mcmc4plus.theencoderreader = eo_encoderreader_Initialise();
            
    s_eo_themotcon.mcmc4.themais = eo_mais_Initialise();
           
    s_eo_themotcon.diagnostics.reportTimer = eo_timer_New();

    s_eo_themotcon.service.initted = eobool_true;
    
    return(&s_eo_themotcon);   
}


extern EOtheMotionController* eo_motioncontrol_GetHandle(void)
{
    if(eobool_true == s_eo_themotcon.service.initted)
    {
        return(&s_eo_themotcon);
    }
    
    return(NULL);
}


extern eOmotioncontroller_mode_t eo_motioncontrol_GetMode(EOtheMotionController *p)
{
    if(NULL == p)
    {
        return(eo_motcon_mode_NONE);
    }

    return((eOmotioncontroller_mode_t)p->service.servconfig.type);
}


extern eOresult_t eo_motioncontrol_Verify(EOtheMotionController *p, const eOmn_serv_configuration_t * servcfg, eOservice_onendofoperation_fun_t onverify, eObool_t activateafterverify)
{
    if((NULL == p) || (NULL == servcfg))
    {
        return(eores_NOK_nullpointer);
    }  
    
    
    if((eo_motcon_mode_foc != servcfg->type) && (eo_motcon_mode_mc4 != servcfg->type) && (eo_motcon_mode_mc4plus != servcfg->type) )
    {
        return(eores_NOK_generic);
    }

    if(eobool_true == s_eo_themotcon.service.active)
    {
        eo_motioncontrol_Deactivate(p);        
    } 

    // make sure the timer is not running
    eo_timer_Stop(s_eo_themotcon.diagnostics.reportTimer);        
    
    s_eo_themotcon.service.tmpcfg = servcfg;
    
    if(eomn_serv_MC_foc == servcfg->type)
    {
        
        s_eo_themotcon.service.onverify = onverify;
        s_eo_themotcon.service.activateafterverify = activateafterverify;

        // 1. prepare the can discovery for foc boards 
        memset(&s_eo_themotcon.sharedcan.discoverytarget, 0, sizeof(s_eo_themotcon.sharedcan.discoverytarget));
        s_eo_themotcon.sharedcan.discoverytarget.boardtype = eobrd_cantype_foc;
        s_eo_themotcon.sharedcan.discoverytarget.protocolversion.major = servcfg->data.mc.foc_based.version.protocol.major; 
        s_eo_themotcon.sharedcan.discoverytarget.protocolversion.minor = servcfg->data.mc.foc_based.version.protocol.minor;
        s_eo_themotcon.sharedcan.discoverytarget.firmwareversion.major = servcfg->data.mc.foc_based.version.firmware.major; 
        s_eo_themotcon.sharedcan.discoverytarget.firmwareversion.minor = servcfg->data.mc.foc_based.version.firmware.minor;   
        
        EOconstarray* carray = eo_constarray_Load((EOarray*)&servcfg->data.mc.foc_based.arrayofjomodescriptors);
        
        uint8_t numofjomos = eo_constarray_Size(carray);
        uint8_t i = 0;
        for(i=0; i<numofjomos; i++)
        {
            const eOmn_serv_jomo_descriptor_t *jomodes = (eOmn_serv_jomo_descriptor_t*) eo_constarray_At(carray, i);
            eo_common_hlfword_bitset(&s_eo_themotcon.sharedcan.discoverytarget.canmap[jomodes->actuator.foc.canloc.port], jomodes->actuator.foc.canloc.addr);         
        }
                
        s_eo_themotcon.sharedcan.ondiscoverystop.function = s_eo_motioncontrol_onstop_search4focs;
        s_eo_themotcon.sharedcan.ondiscoverystop.parameter = (void*)servcfg;
        
        // 2. at first i verify the encoders. then, function s_eo_motioncontrol_onendofverify_encoder() shall either issue an encoder error or start discovery of foc boards
        eo_encoderreader_Verify(eo_encoderreader_GetHandle(), &servcfg->data.mc.foc_based.arrayofjomodescriptors, s_eo_motioncontrol_onendofverify_encoder, eobool_true); 
        
    }
    else if(eomn_serv_MC_mc4 == servcfg->type)
    {
        
        s_eo_themotcon.service.onverify = onverify;
        s_eo_themotcon.service.activateafterverify = activateafterverify;
        
        // 1. prepare the verify of mais
        s_eo_themotcon.mcmc4.servconfigmais.type = eomn_serv_AS_mais;
        memcpy(&s_eo_themotcon.mcmc4.servconfigmais.data.as.mais, &servcfg->data.mc.mc4_based.mais, sizeof(eOmn_serv_config_data_as_mais_t)); 

        // 2. prepare the can discovery of mc4 boards
        memset(&s_eo_themotcon.sharedcan.discoverytarget, 0, sizeof(s_eo_themotcon.sharedcan.discoverytarget));
        s_eo_themotcon.sharedcan.discoverytarget.boardtype = eobrd_cantype_mc4;
        s_eo_themotcon.sharedcan.discoverytarget.protocolversion.major = servcfg->data.mc.mc4_based.mc4version.protocol.major; 
        s_eo_themotcon.sharedcan.discoverytarget.protocolversion.minor = servcfg->data.mc.mc4_based.mc4version.protocol.minor;
        s_eo_themotcon.sharedcan.discoverytarget.firmwareversion.major = servcfg->data.mc.mc4_based.mc4version.firmware.major; 
        s_eo_themotcon.sharedcan.discoverytarget.firmwareversion.minor = servcfg->data.mc.mc4_based.mc4version.firmware.minor;   
        
        uint8_t i = 0;
        for(i=0; i<12; i++)
        {            
            const eOmn_serv_canlocation_t *canloc = &servcfg->data.mc.mc4_based.mc4joints[i];
            eo_common_hlfword_bitset(&s_eo_themotcon.sharedcan.discoverytarget.canmap[canloc->port], canloc->addr);         
        }
                       
        s_eo_themotcon.sharedcan.ondiscoverystop.function = s_eo_motioncontrol_onstop_search4mc4s;
        s_eo_themotcon.sharedcan.ondiscoverystop.parameter = (void*)servcfg;
             
        // 3. at first i verify the mais. then, function s_eo_motioncontrol_onendofverify_mais() shall either issue a mais error or start discovery of mc4 boards      
        eo_mais_Verify(eo_mais_GetHandle(), &s_eo_themotcon.mcmc4.servconfigmais, s_eo_motioncontrol_onendofverify_mais, eobool_true);
      
    }
    else if(eomn_serv_MC_mc4plus == servcfg->type)
    {
        
        // for now verify the encoder reader. 
        // we dont verify the pwm actuators. only way to do that is to add a hal_pwm_supported_is()
        s_eo_themotcon.service.onverify = onverify;
        s_eo_themotcon.service.activateafterverify = activateafterverify;
        eo_encoderreader_Verify(eo_encoderreader_GetHandle(), &servcfg->data.mc.mc4plus_based.arrayofjomodescriptors, s_eo_motioncontrol_mc4plus_onendofverify_encoder, eobool_true);   
        
    }
    
 
    return(eores_OK);   
}


extern eOresult_t eo_motioncontrol_Deactivate(EOtheMotionController *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }

    if(eobool_false == s_eo_themotcon.service.active)
    {
        return(eores_OK);        
    } 
    
    
    // at first we stop the service
    if(eobool_true == s_eo_themotcon.service.running)
    {
        eo_motioncontrol_Stop(p);   
    }        
    
    // then we deconfig things
    if(eomn_serv_MC_foc == s_eo_themotcon.service.servconfig.type)
    {
        
        // for foc-based, we must ... deconfig mc foc boards, unload them, set num of entities = 0, clear status, deactivate encoder 
        
        eo_canmap_DeconfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_joint, s_eo_themotcon.sharedcan.entitydescriptor); 
        eo_canmap_DeconfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_motor, s_eo_themotcon.sharedcan.entitydescriptor); 
        

        eo_canmap_UnloadBoards(eo_canmap_GetHandle(), s_eo_themotcon.sharedcan.boardproperties); 
        
        eo_encoderreader_Deactivate(p->mcfoc.theencoderreader);
        
        // ems controller ... unfortunately cannot be deinitted
        ///// to be done a deinit to the ems controller
        
        // now i reset 
        eo_vector_Clear(s_eo_themotcon.sharedcan.entitydescriptor);
        eo_vector_Clear(s_eo_themotcon.sharedcan.boardproperties);     

        // proxy deconfig
        s_eo_motioncontrol_proxy_config(eobool_false);        
    
    }
    else if(eomn_serv_MC_mc4 == s_eo_themotcon.service.servconfig.type)
    {
        
        // for mc4-based, we must ... deconfig mc4 foc boards, unload them, set num of entities = 0, clear status, deactivate mais 
        
        eo_canmap_DeconfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_joint, s_eo_themotcon.sharedcan.entitydescriptor); 
        eo_canmap_DeconfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_motor, s_eo_themotcon.sharedcan.entitydescriptor); 
        

        eo_canmap_UnloadBoards(eo_canmap_GetHandle(), s_eo_themotcon.sharedcan.boardproperties); 
               
        // now i reset 
        eo_vector_Clear(s_eo_themotcon.sharedcan.entitydescriptor);
        eo_vector_Clear(s_eo_themotcon.sharedcan.boardproperties);
        
        eo_mais_Deactivate(p->mcmc4.themais);        
        memset(&s_eo_themotcon.mcmc4.servconfigmais, 0, sizeof(s_eo_themotcon.mcmc4.servconfigmais));   

        // proxy deconfig
        s_eo_motioncontrol_proxy_config(eobool_false); 

    }
    else if(eomn_serv_MC_mc4plus == s_eo_themotcon.service.servconfig.type)   
    {
        
        eo_encoderreader_Deactivate(p->mcmc4plus.theencoderreader);
        #warning TODO: in eo_motioncontrol_Deactivate() for mc4plus call: eo_emsController_Deinit()
        #warning TODO: in eo_motioncontrol_Deactivate() for mc4plus call: disable the motors ...
            
    }        
    
    
    s_eo_themotcon.numofjomos = 0;
    eo_entities_SetNumOfJoints(eo_entities_GetHandle(), 0);
    eo_entities_SetNumOfMotors(eo_entities_GetHandle(), 0);
    
    memset(&s_eo_themotcon.service.servconfig, 0, sizeof(eOmn_serv_configuration_t));
    s_eo_themotcon.service.servconfig.type = eo_motcon_mode_NONE;
    memset(&s_eo_themotcon.sharedcan.discoverytarget, 0, sizeof(eOcandiscovery_target_t));
    
    s_eo_themotcon.service.onverify = NULL;
    s_eo_themotcon.service.activateafterverify = eobool_false;
    s_eo_themotcon.sharedcan.ondiscoverystop.function = NULL;
    s_eo_themotcon.sharedcan.ondiscoverystop.parameter = NULL;
    
    // make sure the timer is not running
    eo_timer_Stop(s_eo_themotcon.diagnostics.reportTimer);  
    
    s_eo_themotcon.service.active = eobool_false;    
    
    return(eores_OK);
}


extern eOresult_t eo_motioncontrol_Activate(EOtheMotionController *p, const eOmn_serv_configuration_t * servcfg)
{
    if((NULL == p) || (NULL == servcfg))
    {
        return(eores_NOK_nullpointer);
    }  

    if((eo_motcon_mode_foc != servcfg->type) && (eo_motcon_mode_mc4 != servcfg->type) && (eo_motcon_mode_mc4plus != servcfg->type))
    {
        return(eores_NOK_generic);
    }    
    
    if(eobool_true == s_eo_themotcon.service.active)
    {
        eo_motioncontrol_Deactivate(p);        
    }   

    if(eomn_serv_MC_foc == servcfg->type)
    {
        
        EOconstarray* carray = eo_constarray_Load((EOarray*)&servcfg->data.mc.foc_based.arrayofjomodescriptors);
        
        uint8_t numofjomos = eo_constarray_Size(carray);
        
        eo_entities_SetNumOfJoints(eo_entities_GetHandle(), numofjomos);
        eo_entities_SetNumOfMotors(eo_entities_GetHandle(), numofjomos);
        

        if(0 == eo_entities_NumOfJoints(eo_entities_GetHandle()))
        {
            s_eo_themotcon.service.active = eobool_false;
            return(eores_NOK_generic);
        }
        else
        {                
            memcpy(&s_eo_themotcon.service.servconfig, servcfg, sizeof(eOmn_serv_configuration_t));
          
            s_eo_themotcon.numofjomos = numofjomos;
            
            // now... use the servcfg
            uint8_t i = 0;
            
            EOconstarray* carray = eo_constarray_Load((EOarray*)&servcfg->data.mc.foc_based.arrayofjomodescriptors);

            
            // load the can mapping 
            for(i=0; i<numofjomos; i++)
            {
                const eOmn_serv_jomo_descriptor_t *jomodes = (eOmn_serv_jomo_descriptor_t*) eo_constarray_At(carray, i);
                
                eOcanmap_board_properties_t prop = {0};
                
                prop.type = eobrd_cantype_foc;
                prop.location.port = jomodes->actuator.foc.canloc.port;
                prop.location.addr = jomodes->actuator.foc.canloc.addr;
                prop.location.insideindex = jomodes->actuator.foc.canloc.insideindex;
                prop.requiredprotocol.major = servcfg->data.mc.foc_based.version.protocol.major;
                prop.requiredprotocol.minor = servcfg->data.mc.foc_based.version.protocol.minor;
                
                eo_vector_PushBack(s_eo_themotcon.sharedcan.boardproperties, &prop);            
            }
            eo_canmap_LoadBoards(eo_canmap_GetHandle(), s_eo_themotcon.sharedcan.boardproperties); 
            
            // load the entity mapping.
            for(i=0; i<numofjomos; i++)
            {
                const eOmn_serv_jomo_descriptor_t *jomodes = (eOmn_serv_jomo_descriptor_t*) eo_constarray_At(carray, i);
                
                eOcanmap_entitydescriptor_t des = {0};
                
                des.location.port = jomodes->actuator.foc.canloc.port;
                des.location.addr = jomodes->actuator.foc.canloc.addr;
                des.location.insideindex = jomodes->actuator.foc.canloc.insideindex;
                des.index = (eOcanmap_entityindex_t)i;

                eo_vector_PushBack(s_eo_themotcon.sharedcan.entitydescriptor, &des);            
            }        
            eo_canmap_ConfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_joint, s_eo_themotcon.sharedcan.entitydescriptor); 
            eo_canmap_ConfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_motor, s_eo_themotcon.sharedcan.entitydescriptor);        

            // init the encoders
            
            eo_encoderreader_Activate(p->mcfoc.theencoderreader, &servcfg->data.mc.foc_based.arrayofjomodescriptors);

            
            // init the emscontroller.
            #warning TODO: change the emsController. see comments below
            //                the emscontroller is not a singleton which can be initted and deinitted. 
            // it should have a _Initialise(), a _GetHandle(), a _Config(cfg) and a _Deconfig().
            if(NULL == s_eo_themotcon.mcfoc.thecontroller)
            {
                s_eo_themotcon.mcfoc.thecontroller = eo_emsController_Init((eOemscontroller_board_t)servcfg->data.mc.foc_based.boardtype4mccontroller, emscontroller_actuation_2FOC, numofjomos);   
            }
            
            
            // proxy config
            s_eo_motioncontrol_proxy_config(eobool_true);
            
            s_eo_themotcon.service.active = eobool_true;        
        }
    
    }
    else if(eomn_serv_MC_mc4 == servcfg->type)
    {
        
        const uint8_t numofjomos = 12;
        
        eo_entities_SetNumOfJoints(eo_entities_GetHandle(), numofjomos);
        eo_entities_SetNumOfMotors(eo_entities_GetHandle(), numofjomos);
        

        if(0 == eo_entities_NumOfJoints(eo_entities_GetHandle()))
        {
            s_eo_themotcon.service.active = eobool_false;
            return(eores_NOK_generic);
        }
        else
        {                
            memcpy(&s_eo_themotcon.service.servconfig, servcfg, sizeof(eOmn_serv_configuration_t));
          
            s_eo_themotcon.numofjomos = numofjomos;
            
            // now... use the servcfg
            uint8_t i = 0;
          
            eOcanmap_board_properties_t prop = {0};
            const eOmn_serv_canlocation_t *canloc = NULL;
            
            // load the can mapping for the 12 boards ... (only mc4 boards as teh mais was added bt eo_mais_Activate()
            
            // push the 12 mc4 boards
            for(i=0; i<numofjomos; i++)
            {
                canloc = &servcfg->data.mc.mc4_based.mc4joints[i];
                               
                prop.type = eobrd_cantype_mc4;
                prop.location.port = canloc->port;
                prop.location.addr = canloc->addr;
                prop.location.insideindex = canloc->insideindex;
                prop.requiredprotocol.major = servcfg->data.mc.mc4_based.mc4version.protocol.major;
                prop.requiredprotocol.minor = servcfg->data.mc.mc4_based.mc4version.protocol.minor;
                
                eo_vector_PushBack(s_eo_themotcon.sharedcan.boardproperties, &prop);            
            }
           

            // load the 12 of them
            eo_canmap_LoadBoards(eo_canmap_GetHandle(), s_eo_themotcon.sharedcan.boardproperties); 
            
            // load the entity mapping.
            eOcanmap_entitydescriptor_t des = {0};
            for(i=0; i<numofjomos; i++)
            {               
                canloc = &servcfg->data.mc.mc4_based.mc4joints[i];
                                              
                des.location.port = canloc->port;
                des.location.addr = canloc->addr;
                des.location.insideindex = canloc->insideindex;
                des.index = (eOcanmap_entityindex_t)i;

                eo_vector_PushBack(s_eo_themotcon.sharedcan.entitydescriptor, &des);            
            }        
            eo_canmap_ConfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_joint, s_eo_themotcon.sharedcan.entitydescriptor); 
            eo_canmap_ConfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_motor, s_eo_themotcon.sharedcan.entitydescriptor);        

            // init the mais
            
            eo_mais_Activate(p->mcmc4.themais, &s_eo_themotcon.mcmc4.servconfigmais);
            eo_mais_Start(p->mcmc4.themais);
            
            // do something with the mc4s....
            
            s_eo_themotcon.mcmc4.themc4boards = eo_mc4boards_Initialise(NULL);            
            eo_mc4boards_Config(eo_mc4boards_GetHandle());
            
            // init others
            eo_virtualstrain_Initialise();
            // notused anymore: eo_motiondone_Initialise();
            
            
            // proxy config
            s_eo_motioncontrol_proxy_config(eobool_true);
                    
            s_eo_themotcon.service.active = eobool_true;         
        }            
        
    }
    else if(eomn_serv_MC_mc4plus == servcfg->type)
    {
        
        EOconstarray* carray = eo_constarray_Load((EOarray*)&servcfg->data.mc.mc4plus_based.arrayofjomodescriptors);
        
        uint8_t numofjomos = eo_constarray_Size(carray);
        
        eo_entities_SetNumOfJoints(eo_entities_GetHandle(), numofjomos);
        eo_entities_SetNumOfMotors(eo_entities_GetHandle(), numofjomos);
        

        if(0 == eo_entities_NumOfJoints(eo_entities_GetHandle()))
        {
            s_eo_themotcon.service.active = eobool_false;
            return(eores_NOK_generic);
        }
        else
        {  

            memcpy(&s_eo_themotcon.service.servconfig, servcfg, sizeof(eOmn_serv_configuration_t));
          
            s_eo_themotcon.numofjomos = numofjomos;
            
            // now... use the servcfg
            uint8_t i=0;
           
            
            // marco.accame: i keep the same initialisation order as davide.pollarolo did in his EOmcService.c (a, b, c etc)
            // a. init the emscontroller.
            #warning TODO: change the emsController. see comments below
            // the emscontroller is not a singleton which can be initted and deinitted. 
            // it should have a _Initialise(), a _GetHandle(), a _Config(cfg) and a _Deconfig().
            if(NULL == s_eo_themotcon.mcmc4plus.thecontroller)
            {
                s_eo_themotcon.mcmc4plus.thecontroller = eo_emsController_Init((eOemscontroller_board_t)servcfg->data.mc.mc4plus_based.boardtype4mccontroller, emscontroller_actuation_LOCAL, numofjomos);   
            }       

            // b. clear the pwm values and the port mapping
            memset(p->mcmc4plus.pwmvalue, 0, sizeof(p->mcmc4plus.pwmvalue));
            memset(p->mcmc4plus.pwmport, 0, sizeof(p->mcmc4plus.pwmport));
            
            // b1. init the port mapping
            for(i=0; i<numofjomos; i++)
            {
                const eOmn_serv_jomo_descriptor_t *jomodes = (eOmn_serv_jomo_descriptor_t*) eo_constarray_At(carray, i);            
                p->mcmc4plus.pwmport[i] = jomodes->actuator.pwm.port;                
            }
            
            // c. low level init for motors and adc           
            s_eo_motioncontrol_mc4plusbased_hal_init_motors_adc_feedbacks();

            // d. init the encoders            
            eo_encoderreader_Activate(p->mcmc4plus.theencoderreader, &servcfg->data.mc.mc4plus_based.arrayofjomodescriptors);

            
            // e. activate interrupt line for quad_enc indexes check
            #warning: marco.accame: maybe it is better to move it inside eo_appEncReader_Activate()
            s_eo_motioncontrol_mc4plusbased_hal_init_quad_enc_indexes_interrupt();
            
           
            s_eo_themotcon.service.active = eobool_true;        
        }
        
    }

    
    return(eores_OK);   
}



extern eOresult_t eo_motioncontrol_Start(EOtheMotionController *p)
{    
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }    
    
    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do
        return(eores_OK);
    } 
        
    if(eobool_true == s_eo_themotcon.service.running)
    {
        return(eores_OK);
    }
    
    // mc4based: enable broadcast etc
    // focbased: just init a read of the encoder
    if(eomn_serv_MC_foc == p->service.servconfig.type)
    {   
#ifndef USE_ONLY_QE
        // just start a reading of encoders        
        eo_encoderreader_StartReading(p->mcfoc.theencoderreader);
#endif
    }
    else if(eomn_serv_MC_mc4 == p->service.servconfig.type)
    {
        eo_mc4boards_BroadcastStart(eo_mc4boards_GetHandle());
    }
    else if(eomn_serv_MC_mc4plus == p->service.servconfig.type)
    {
        eo_encoderreader_StartReading(p->mcmc4plus.theencoderreader);
        s_eo_motioncontrol_mc4plusbased_enable_all_motors(p);      
    }
    
    
    s_eo_themotcon.service.running = eobool_true;
    
    return(eores_OK);    
}


extern eOresult_t eo_motioncontrol_Tick(EOtheMotionController *p)
{
    
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }    
    
    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do 
        return(eores_OK);
    } 
    
    if(eobool_false == s_eo_themotcon.service.running)
    {
        return(eores_OK);
    }
    
    
    if(eomn_serv_MC_foc == p->service.servconfig.type)
    {
        uint8_t error_mask = 0;
        eOresult_t res = eores_NOK_generic;
        uint32_t encvalue[4] = {0}; 
        hal_encoder_errors_flags encflags[4] = {0};
        int16_t pwm[4] = {0};
        
        uint8_t i = 0;

#ifndef USE_ONLY_QE   
        
        // wait for the encoders for some time 
        for(i=0; i<30; ++i)
        {
            if(eobool_true == eo_encoderreader_IsReadingAvailable(p->mcfoc.theencoderreader))
            {
                break;
            }
            else
            {
                hal_sys_delay(5*hal_RELTIME_1microsec);
            }
        }        
        
        // read the encoders        
        if(eobool_true == eo_encoderreader_IsReadingAvailable(p->mcfoc.theencoderreader))
        {    
            for(i=0; i<p->numofjomos; i++)
            {
                uint32_t extra = 0;
                res = eo_encoderreader_Read(p->mcfoc.theencoderreader, i, &(encvalue[i]), &extra, &(encflags[i]));
                if (res != eores_OK)
                {
                    error_mask |= 1<<(i<<1);
                    //encvalue[enc] = (uint32_t)ENC_INVALID;
                }
            }        
        }
        else
        {
            error_mask = 0xAA; // timeout
        }

        // Restart the reading of the encoders
        eo_encoderreader_StartReading(p->mcfoc.theencoderreader);
        
#endif // USE_ONLY_QE        
            
        eo_emsController_AcquireAbsEncoders((int32_t*)encvalue, error_mask);
        
        eo_emsController_CheckFaults();
            
        /* 2) pid calc */
        eo_emsController_PWM(pwm);

        /* 3) prepare and punt in rx queue new setpoint */
        s_eo_motioncontrol_SetCurrentSetpoint(p, pwm, 4); 
     
        /* 4) update joint status */
        s_eo_motioncontrol_UpdateJointStatus(p);
  
    }
    else if(eomn_serv_MC_mc4 == p->service.servconfig.type)
    {
        // motion done
        // not used anymore: eo_motiondone_Tick(eo_motiondone_GetHandle());
    
        // virtual strain
        eo_virtualstrain_Tick(eo_virtualstrain_GetHandle());        
        
    }
    else if(eomn_serv_MC_mc4plus == p->service.servconfig.type)
    {
        s_eo_mcserv_do_mc4plus(p);        
    }

    return(eores_OK);    
}


extern eOresult_t eo_motioncontrol_Stop(EOtheMotionController *p)
{    
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }    
    
    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do 
        return(eores_OK);
    } 
    
    if(eobool_false == s_eo_themotcon.service.running)
    {
        return(eores_OK);
    }
    
    if(eomn_serv_MC_foc == p->service.servconfig.type)
    {   
        // just put controller in idle mode        
        eo_emsMotorController_GoIdle();
    }      
    else if(eomn_serv_MC_mc4 == p->service.servconfig.type)
    {
        // just stop broadcast of the mc4 boards
        eo_mc4boards_BroadcastStop(eo_mc4boards_GetHandle()); 
    }    
    else if(eomn_serv_MC_mc4plus == p->service.servconfig.type)
    {   
        s_eo_mcserv_disable_all_motors(p);
    }
      
    s_eo_themotcon.service.running = eobool_false;
    
    return(eores_OK);    
}



#warning TODO: move MotorEnable inside the controller ... it must check vs coupled joints ...
extern eOresult_t eo_motioncontrol_extra_MotorEnable(EOtheMotionController *p, uint8_t jomo)
{   // former eo_mcserv_EnableMotor()
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    } 
    
    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do 
        return(eores_OK);
    } 
    
    if(eobool_false == s_eo_themotcon.service.running)
    {
        return(eores_OK);
    }    
    
    if(eomn_serv_MC_mc4plus != p->service.servconfig.type)
    {   // so far only for mc4plus service
        return(eores_NOK_generic);
    }
    
    if(jomo >= p->numofjomos)
    {
        return(eores_NOK_generic);
    }
    
    eOemscontroller_board_t board_control = (eOemscontroller_board_t)p->service.servconfig.data.mc.mc4plus_based.boardtype4mccontroller;
    
    // check coupled joints
    if(/*(emscontroller_board_SHOULDER == board_control) || (emscontroller_board_WAIST == board_control) ||*/ (emscontroller_board_CER_WRIST == board_control))    
    {
        if(jomo < 3) // mi piace pochissimo questo codice ...
        {
            s_eo_mcserv_pwm_enable(p->mcmc4plus.pwmport[0]);
            s_eo_mcserv_pwm_enable(p->mcmc4plus.pwmport[1]);
            s_eo_mcserv_pwm_enable(p->mcmc4plus.pwmport[2]);
        }
        else
        {
            s_eo_mcserv_pwm_enable(p->mcmc4plus.pwmport[jomo]);
        }
    }
    else if(emscontroller_board_HEAD_neckpitch_neckroll == board_control)  
    {
        s_eo_mcserv_pwm_enable(p->mcmc4plus.pwmport[0]);
        s_eo_mcserv_pwm_enable(p->mcmc4plus.pwmport[1]);
    }
    else if(emscontroller_board_HEAD_neckyaw_eyes == board_control) 
    {
        if((jomo == 2) || (jomo == 3) ) 
        {
            s_eo_mcserv_pwm_enable(p->mcmc4plus.pwmport[2]);
            s_eo_mcserv_pwm_enable(p->mcmc4plus.pwmport[3]);
        }
        else
        {
            s_eo_mcserv_pwm_enable(p->mcmc4plus.pwmport[jomo]);
        }      
    }
    else  // the joint is not coupled to any other joint 
    { 
        s_eo_mcserv_pwm_enable(p->mcmc4plus.pwmport[jomo]);
    }          
    
    return(eores_OK);   
}


extern eOresult_t eo_motioncontrol_extra_FaultDetectionEnable(EOtheMotionController *p)
{   // former eo_mcserv_EnableFaultDetection()
    
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do 
        return(eores_OK);
    } 
    
    if(eobool_false == s_eo_themotcon.service.running)
    {
        return(eores_OK);
    }    
    
    if(eomn_serv_MC_mc4plus != p->service.servconfig.type)
    {   // so far only for mc4plus service
        return(eores_NOK_generic);
    }
    
//#if defined(USE_MC4PLUS)    
    hal_motor_reenable_break_interrupts();
//#endif

    return(eores_OK);
}


extern eObool_t eo_motioncontrol_extra_AreMotorsExtFaulted(EOtheMotionController *p)
{   // former eo_mcserv_AreMotorsExtFaulted()     
    if(NULL == p)
    {
        return(eobool_false);
    }
    
    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do 
        return(eobool_false);
    } 
    
    if(eobool_false == s_eo_themotcon.service.running)
    {
        return(eobool_false);
    }    
    
    if(eomn_serv_MC_mc4plus != p->service.servconfig.type)
    {   // so far only for mc4plus service
        return(eobool_false);
    }
    
    return(s_eo_mcserv_are_motors_ext_faulted());
}


extern eOresult_t eo_motioncontrol_extra_SetMotorFaultMask(EOtheMotionController *p, uint8_t jomo, uint8_t* fault_mask)
{   // former eo_mcserv_SetMotorFaultMask()
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }

    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do 
        return(eores_OK);
    } 
    
    if(eobool_false == s_eo_themotcon.service.running)
    {
        return(eores_OK);
    }    
    
    if(eomn_serv_MC_mc4plus != p->service.servconfig.type)
    {   // so far only for mc4plus service
        return(eores_NOK_generic);
    }
    
    if(jomo >= p->numofjomos)
    {
        return(eores_NOK_generic);
    }
    
    eOemscontroller_board_t board_control = (eOemscontroller_board_t)p->service.servconfig.data.mc.mc4plus_based.boardtype4mccontroller;

    
    // check coupled joints
    if(/*(emscontroller_board_SHOULDER == board_control) || (emscontroller_board_WAIST == board_control) ||*/ (emscontroller_board_CER_WRIST == board_control))    
    {
        if(jomo <3) 
        {
            // don't need to use p->mcmc4plus.pwmport[jomo], because the emsController (and related objs) use the same indexing of the highlevel
            eo_motor_set_motor_status(eo_motors_GetHandle(), 0, fault_mask);
            eo_motor_set_motor_status(eo_motors_GetHandle(), 1, fault_mask);
            eo_motor_set_motor_status(eo_motors_GetHandle(), 2, fault_mask);
        }
        else
        {
            eo_motor_set_motor_status(eo_motors_GetHandle(), jomo, fault_mask);
        }
    }
    else if(emscontroller_board_HEAD_neckpitch_neckroll == board_control)  
    {
        eo_motor_set_motor_status(eo_motors_GetHandle(),0, fault_mask);
        eo_motor_set_motor_status(eo_motors_GetHandle(),1, fault_mask);
    }
    else if(emscontroller_board_HEAD_neckyaw_eyes == board_control) 
    {
        if((jomo == 2) || (jomo == 3) ) 
        {
            eo_motor_set_motor_status(eo_motors_GetHandle(), 2, fault_mask);
            eo_motor_set_motor_status(eo_motors_GetHandle(), 3, fault_mask);
        }
        else
        {
            eo_motor_set_motor_status(eo_motors_GetHandle(), jomo, fault_mask);
        }      
    }
    else  // the joint is not coupled to any other joint 
    { 
        eo_motor_set_motor_status(eo_motors_GetHandle(), jomo, fault_mask);
    }      
    
    return eores_OK;
}

extern uint32_t eo_motioncontrol_extra_GetMotorFaultMask(EOtheMotionController *p, uint8_t jomo)
{   // former eo_mcserv_GetMotorFaultMask()
    if(NULL == p)
    {
        return(0);
    }
 
    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do 
        return(0);
    } 
    
    if(eobool_false == s_eo_themotcon.service.running)
    {
        return(0);
    }    
    
    if(eomn_serv_MC_mc4plus != p->service.servconfig.type)
    {   // so far only for mc4plus service
        return(0);
    }
    
    if(jomo >= p->numofjomos)
    {
        return(0);
    }
    
    // don't need to use p->mcmc4plus.pwmport[jomo], because the emsController (and related objs) use the same indexing of the highlevel
    uint32_t state_mask = eo_get_motor_fault_mask(eo_motors_GetHandle(), jomo);
    return state_mask;
}

extern uint16_t eo_motioncontrol_extra_GetMotorCurrent(EOtheMotionController *p, uint8_t jomo)
{   // former eo_mcserv_GetMotorCurrent()
    if(NULL == p)
    {
        return(0);
    }
    
    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do 
        return(0);
    } 
    
    if(eobool_false == s_eo_themotcon.service.running)
    {
        return(0);
    }    
    
    if(eomn_serv_MC_mc4plus != p->service.servconfig.type)
    {   // so far only for mc4plus service
        return(0);
    }
    
    if(jomo >= p->numofjomos)
    {
        return(0);
    }    
   
    int16_t curr_val = 0;
    
    // if local motor (MC4plus)
//    if(1 == p->config.jomos[joint].actuator.local.type)
//    {
//#if defined(USE_MC4PLUS)     
    curr_val = hal_adc_get_current_motor_mA(p->mcmc4plus.pwmport[jomo]);
//#endif    
//    }
    
//    // if remote (EMS4rd-2foc or mc4) --> should return what's inside the array ems->motor_current [motor]
//    // the structure is filled with the CAN callbacks
//    else if (0 == p->config.jomos[joint].actuator.local.type)
//    {
//    }
    return(curr_val);
}

extern uint32_t eo_motioncontrol_extra_GetMotorAnalogSensor(EOtheMotionController *p, uint8_t jomo)
{   // former eo_mcserv_GetMotorAnalogSensor
    if(NULL == p)
    {
        return(NULL);
    }

    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do 
        return(0);
    } 
    
    if(eobool_false == s_eo_themotcon.service.running)
    {
        return(0);
    }    
    
    if(eomn_serv_MC_mc4plus != p->service.servconfig.type)
    {   // so far only for mc4plus service
        return(0);
    }
    
    if(jomo >= p->numofjomos)
    {
        return(0);
    }
    
    uint32_t voltage = 0;
    
    // if local motor (MC4plus)
//    if(1 == p->config.jomos[joint].actuator.local.type)
//    {
//#if defined(USE_MC4PLUS)    
    voltage = hal_adc_get_hall_sensor_analog_input_mV(p->mcmc4plus.pwmport[jomo]);
//#endif    
//    }
   
    return(voltage);
}


extern uint32_t eo_motioncontrol_extra_GetMotorPositionRaw(EOtheMotionController *p, uint8_t jomo)
{   // former eo_mcserv_GetMotorPositionRaw()
    if(NULL == p)
    {
        return(0);
    }
 
    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do 
        return(0);
    } 
    
    if(eobool_false == s_eo_themotcon.service.running)
    {
        return(0);
    }    
    
    if(eomn_serv_MC_mc4plus != p->service.servconfig.type)
    {   // so far only for mc4plus service
        return(0);
    }
    
    if(jomo >= p->numofjomos)
    {
        return(0);
    }
    
    uint32_t pos_val = 0;
    
    // if local motor (MC4plus)
//    if(1 == p->config.jomos[joint].actuator.local.type)
//    {
//#if defined(USE_MC4PLUS)    
    // use inc port not pwm port ??
    pos_val = hal_quad_enc_get_counter((hal_quad_enc_t)p->mcmc4plus.pwmport[jomo]);
//#endif    
//    }
    
//    // if remote (EMS4rd-2foc or mc4) --> should return what's inside the array ems->motor_position[motor]
//    // the structure is filled with the CAN callbacks
//    else if (0 == p->config.jomos[joint].actuator.local.type)
//    {
//    }
    
    return(pos_val);
}


extern void eo_motioncontrol_extra_ResetQuadEncCounter(EOtheMotionController *p, uint8_t jomo)
{   // former eo_mcserv_ResetQuadEncCounter()
    if(NULL == p)
    {
        return;
    }

    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do 
        return;
    } 
    
    if(eobool_false == s_eo_themotcon.service.running)
    {
        return;
    }    
    
    if(eomn_serv_MC_mc4plus != p->service.servconfig.type)
    {   // so far only for mc4plus service
        return;
    }
    
    if(jomo >= p->numofjomos)
    {
        return;
    }
    
    // if local motor (MC4plus)
//    if(1 == p->config.jomos[joint].actuator.local.type)
//    {
//#if defined(USE_MC4PLUS)     
    hal_quad_enc_reset_counter((hal_quad_enc_t)p->mcmc4plus.pwmport[jomo]);
//#endif    
//    }
     
}


extern eObool_t eo_motioncontrol_extra_IsMotorEncoderIndexReached(EOtheMotionController *p, uint8_t jomo)
{   // former eo_mcserv_IsMotorEncoderIndexReached()
    if(NULL == p)
    {
        return(eobool_false);
    }

    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do 
        return(eobool_false);
    } 
    
    if(eobool_false == s_eo_themotcon.service.running)
    {
        return(eobool_false);
    }    
    
    if(eomn_serv_MC_mc4plus != p->service.servconfig.type)
    {   // so far only for mc4plus service
        return(eobool_false);
    }
    
    eObool_t indx_reached = eobool_false;
    
    // if local motor (MC4plus)
//    if(1 == p->config.jomos[joint].actuator.local.type)
//    {
//#if defined(USE_MC4PLUS)     
    //#warning marco.accame: why do we use a pwm port for an inc encoder? we should use the inc port instead.   
    indx_reached = (eObool_t) hal_quad_enc_is_index_found((hal_quad_enc_t)p->mcmc4plus.pwmport[jomo]);
//#endif    
//    }
    
    return(indx_reached);
}


#warning marco.accame: TODO: make eo_motioncontrol_extra_ManageEXTfault() private and put it inside eo_motioncontrol_Tick() and only for mc4plus-based control ...
extern eOresult_t eo_motioncontrol_extra_ManageEXTfault(EOtheMotionController *p)
{   // formerly this code was in s_overriden_runner_CheckAndUpdateExtFaults().
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }

    if(eobool_false == s_eo_themotcon.service.active)
    {   // nothing to do 
        return(eores_OK);
    } 
    
    if(eobool_false == s_eo_themotcon.service.running)
    {
        return(eores_OK);
    }    
    
    if(eomn_serv_MC_mc4plus != p->service.servconfig.type)
    {   // so far only for mc4plus service
        return(eores_NOK_generic);
    }
    
    if(eobool_false == eo_motioncontrol_extra_AreMotorsExtFaulted(p))
    {
        return(eores_OK);
    }
    
    // set the fault mask for ALL the motors
    for(uint8_t i=0; i<p->numofjomos; i++)
    {
        uint32_t state = eo_motioncontrol_extra_GetMotorFaultMask(p, i); 
        if((state & MOTOR_EXTERNAL_FAULT) == 0) // external fault bit not set
        {
            // simulate the CANframe used by 2FOC to signal the status
            uint64_t fault_mask = (((uint64_t)(state | MOTOR_EXTERNAL_FAULT)) << 32) & 0xFFFFFFFF00000000; // adding the error to the current state
            eo_motor_set_motor_status(eo_motors_GetHandle(),i, (uint8_t*)&fault_mask);
        }
    }
   
    return(eores_OK);
}


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------

// - in here i put the functions used to initialise the values in ram of the joints and motors ... better in here rather than elsewhere.

static const eOmc_joint_t s_joint_default_value =
{
    .config =             
    {
        .pidposition =
        {
            .kp =                    0,
            .ki =                    0,
            .kd =                    0,
            .limitonintegral =       0,
            .limitonoutput =         0,
            .scale =                 0,
            .offset =                0,
            .kff =                   0,
            .stiction_up_val =       0,
            .stiction_down_val =     0,
            .filler =                {0}
        },
        .pidvelocity =
        {
            .kp =                    0,
            .ki =                    0,
            .kd =                    0,
            .limitonintegral =       0,
            .limitonoutput =         0,
            .scale =                 0,
            .offset =                0,
            .kff =                   0,
            .stiction_up_val =       0,
            .stiction_down_val =     0,
            .filler =                {0}
        },
        .pidtorque =
        {
            .kp =                    0,
            .ki =                    0,
            .kd =                    0,
            .limitonintegral =       0,
            .limitonoutput =         0,
            .scale =                 0,
            .offset =                0,
            .kff =                   0,
            .stiction_up_val =       0,
            .stiction_down_val =     0,
            .filler =                {0}
        }, 
        .limitsofjoint =
        {
            .min =                   0,
            .max =                   0
        },
        .impedance =
        {
            .stiffness =             0,
            .damping =               0,
            .offset =                0          
        },               
        .maxvelocityofjoint =        0,
        .motor_params =
        {
            .bemf_value =            0,
            .ktau_value =            0,
            .bemf_scale =            0,
            .ktau_scale =            0,
            .filler02 =              {0}
        },
        .velocitysetpointtimeout =   0,
        .tcfiltertype =              0,
        .jntEncoderType =            0,
        .filler04 =                  {0}
    },
    .status =                       
    {
        .basic =
        {
            .jnt_position =          0,
            .jnt_velocity =          0,
            .jnt_acceleration =      0,
            .jnt_torque =            0
        },
        .ofpid =                     {0},
        .modes = 
        {
            .controlmodestatus =        eomc_controlmode_idle,
            .interactionmodestatus =    eOmc_interactionmode_stiff,
            .ismotiondone =             eobool_false,
            .filler =                   {0}
        }
    },
    .inputs =                        {0},
    .cmmnds =                       
    {
        .calibration =               {0},
        .setpoint =                  {0},
        .stoptrajectory =            0,
        .controlmode =               eomc_controlmode_cmd_switch_everything_off,
        .interactionmode =           eOmc_interactionmode_stiff,
        .filler =                    {0}        
    }
}; 

static const eOmc_motor_t s_motor_default_value =
{
    .config =             
    {
        .pidcurrent =
        {
            .kp =                    0,
            .ki =                    0,
            .kd =                    0,
            .limitonintegral =       0,
            .limitonoutput =         0,
            .scale =                 0,
            .offset =                0,
            .kff =                   0,
            .stiction_up_val =       0,
            .stiction_down_val =     0,
            .filler =                {0}
        },
        .gearboxratio =              0,
        .rotorEncoderResolution =    0,
        .maxvelocityofmotor =        0,
        .maxcurrentofmotor =         0,
        .rotorIndexOffset =          0,
        .motorPoles =                0,
        .hasHallSensor =             eobool_false,
        .hasTempSensor =             eobool_false,
        .hasRotorEncoder =           eobool_false,
        .hasRotorEncoderIndex =      eobool_false,
        .rotorEncoderType =          0,
        .filler02 =                  {0},
        .limitsofrotor =
        {
            .max = 0,
            .min = 0
        }
    },
    .status =                       {0}
}; 


extern void eoprot_fun_INIT_mc_joint_config(const EOnv* nv)
{
    eOmc_joint_config_t *cfg = (eOmc_joint_config_t*)eo_nv_RAM(nv);
    memcpy(cfg, &s_joint_default_value.config, sizeof(eOmc_joint_config_t));
}

extern void eoprot_fun_INIT_mc_joint_status(const EOnv* nv)
{
    eOmc_joint_status_t *sta = (eOmc_joint_status_t*)eo_nv_RAM(nv);
    memcpy(sta, &s_joint_default_value.status, sizeof(eOmc_joint_status_t));
}

extern void eoprot_fun_INIT_mc_motor_config(const EOnv* nv)
{
    eOmc_motor_config_t *cfg = (eOmc_motor_config_t*)eo_nv_RAM(nv);
    memcpy(cfg, &s_motor_default_value.config, sizeof(eOmc_motor_config_t));
}

extern void eoprot_fun_INIT_mc_motor_status(const EOnv* nv)
{
    eOmc_motor_status_t *sta = (eOmc_motor_status_t*)eo_nv_RAM(nv);
    memcpy(sta, &s_motor_default_value.status, sizeof(eOmc_motor_status_t));
}


// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------


static void s_eo_motioncontrol_send_periodic_error_report(void *p)
{
    s_eo_themotcon.diagnostics.errorDescriptor.par64 ++;
    eo_errman_Error(eo_errman_GetHandle(), s_eo_themotcon.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_themotcon.diagnostics.errorDescriptor);
    
    if(EOK_int08dummy != s_eo_themotcon.diagnostics.errorCallbackCount)
    {
        s_eo_themotcon.diagnostics.errorCallbackCount--;
    }
    if(0 == s_eo_themotcon.diagnostics.errorCallbackCount)
    {
        eo_timer_Stop(s_eo_themotcon.diagnostics.reportTimer);
    }
}


static eOresult_t s_eo_motioncontrol_onstop_search4focs(void *par, EOtheCANdiscovery2* p, eObool_t searchisok)
{
    
    if((eobool_true == searchisok) && (eobool_true == s_eo_themotcon.service.activateafterverify))
    {
        const eOmn_serv_configuration_t * mcserv = (const eOmn_serv_configuration_t *)par;
        eo_motioncontrol_Activate(&s_eo_themotcon, mcserv);
    }
    
    s_eo_themotcon.diagnostics.errorDescriptor.sourcedevice     = eo_errman_sourcedevice_localboard;
    s_eo_themotcon.diagnostics.errorDescriptor.sourceaddress    = 0;
    s_eo_themotcon.diagnostics.errorDescriptor.par16            = 0;
    s_eo_themotcon.diagnostics.errorDescriptor.par64            = 0;    
    EOaction_strg astrg = {0};
    EOaction *act = (EOaction*)&astrg;
    eo_action_SetCallback(act, s_eo_motioncontrol_send_periodic_error_report, NULL, eov_callbackman_GetTask(eov_callbackman_GetHandle()));    
    
    if(eobool_true == searchisok)
    {        
        s_eo_themotcon.diagnostics.errorType = eo_errortype_debug;
        s_eo_themotcon.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_foc_ok);
        eo_errman_Error(eo_errman_GetHandle(), s_eo_themotcon.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_themotcon.diagnostics.errorDescriptor);
        
        if((0 != s_eo_themotcon.diagnostics.repetitionOKcase) && (0 != s_eo_themotcon.diagnostics.reportPeriod))
        {
            s_eo_themotcon.diagnostics.errorCallbackCount = s_eo_themotcon.diagnostics.repetitionOKcase;        
            eo_timer_Start(s_eo_themotcon.diagnostics.reportTimer, eok_abstimeNOW, s_eo_themotcon.diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    } 

    if(eobool_false == searchisok)
    {
        s_eo_themotcon.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_foc_failed_candiscovery_of_foc);
        s_eo_themotcon.diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), s_eo_themotcon.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_themotcon.diagnostics.errorDescriptor);
        
        if(0 != s_eo_themotcon.diagnostics.reportPeriod)
        {
            s_eo_themotcon.diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(s_eo_themotcon.diagnostics.reportTimer, eok_abstimeNOW, s_eo_themotcon.diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    }     
           
    if(NULL != s_eo_themotcon.service.onverify)
    {
        s_eo_themotcon.service.onverify(&s_eo_themotcon, searchisok); 
    }    
    
    return(eores_OK);   
}

static eOresult_t s_eo_motioncontrol_mc4plus_onendofverify_encoder(EOaService* s, eObool_t operationisok)
{
    if((eobool_true == operationisok) && (eobool_true == s_eo_themotcon.service.activateafterverify))
    {
        const eOmn_serv_configuration_t * mcserv = s_eo_themotcon.service.tmpcfg;
        eo_motioncontrol_Activate(&s_eo_themotcon, mcserv);
    }
    
    s_eo_themotcon.diagnostics.errorDescriptor.sourcedevice     = eo_errman_sourcedevice_localboard;
    s_eo_themotcon.diagnostics.errorDescriptor.sourceaddress    = 0;
    s_eo_themotcon.diagnostics.errorDescriptor.par16            = 0;
    s_eo_themotcon.diagnostics.errorDescriptor.par64            = 0;    
    EOaction_strg astrg = {0};
    EOaction *act = (EOaction*)&astrg;
    eo_action_SetCallback(act, s_eo_motioncontrol_send_periodic_error_report, NULL, eov_callbackman_GetTask(eov_callbackman_GetHandle()));    
    
    if(eobool_true == operationisok)
    {     
        #warning TODO: add and use eoerror_value_CFG_mc_mc4plus_ok    
        
        s_eo_themotcon.diagnostics.errorType = eo_errortype_debug;
        s_eo_themotcon.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_foc_ok);
        eo_errman_Error(eo_errman_GetHandle(), s_eo_themotcon.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_themotcon.diagnostics.errorDescriptor);
        
        if((0 != s_eo_themotcon.diagnostics.repetitionOKcase) && (0 != s_eo_themotcon.diagnostics.reportPeriod))
        {
            s_eo_themotcon.diagnostics.errorCallbackCount = s_eo_themotcon.diagnostics.repetitionOKcase;        
            eo_timer_Start(s_eo_themotcon.diagnostics.reportTimer, eok_abstimeNOW, s_eo_themotcon.diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    } 

    if(eobool_false == operationisok)
    {
        #warning TODO: add and use eoerror_value_CFG_mc_mc4plus_failed_encoder
        s_eo_themotcon.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_foc_failed_candiscovery_of_foc);
        s_eo_themotcon.diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), s_eo_themotcon.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_themotcon.diagnostics.errorDescriptor);
        
        if(0 != s_eo_themotcon.diagnostics.reportPeriod)
        {
            s_eo_themotcon.diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(s_eo_themotcon.diagnostics.reportTimer, eok_abstimeNOW, s_eo_themotcon.diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    }     
           
    if(NULL != s_eo_themotcon.service.onverify)
    {
        s_eo_themotcon.service.onverify(&s_eo_themotcon, operationisok); 
    }    
    
    return(eores_OK);       
    
}

static eOresult_t s_eo_motioncontrol_onendofverify_encoder(EOaService* s, eObool_t operationisok)
{    
    if(eobool_true == operationisok)
    {
        eo_candiscovery2_Start(eo_candiscovery2_GetHandle(), &s_eo_themotcon.sharedcan.discoverytarget, &s_eo_themotcon.sharedcan.ondiscoverystop);        
    }    
    else
    {
        // the encoder reader fails. we dont even start the discovery of the foc boards. we just issue an error report and call onverify() w/ false argument
        
        // prepare things
        s_eo_themotcon.diagnostics.errorDescriptor.sourcedevice     = eo_errman_sourcedevice_localboard;
        s_eo_themotcon.diagnostics.errorDescriptor.sourceaddress    = 0;
        s_eo_themotcon.diagnostics.errorDescriptor.par16            = 0;
        s_eo_themotcon.diagnostics.errorDescriptor.par64            = 0;    
        EOaction_strg astrg = {0};
        EOaction *act = (EOaction*)&astrg;
        eo_action_SetCallback(act, s_eo_motioncontrol_send_periodic_error_report, NULL, eov_callbackman_GetTask(eov_callbackman_GetHandle()));    
        

        // fill error description. and transmit it
        s_eo_themotcon.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_foc_failed_encoders_verify);
        s_eo_themotcon.diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), s_eo_themotcon.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_themotcon.diagnostics.errorDescriptor);
        
        if(0 != s_eo_themotcon.diagnostics.reportPeriod)
        {
            s_eo_themotcon.diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(s_eo_themotcon.diagnostics.reportTimer, eok_abstimeNOW, s_eo_themotcon.diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
  
        // call onverify
        if(NULL != s_eo_themotcon.service.onverify)
        {
            s_eo_themotcon.service.onverify(&s_eo_themotcon, eobool_false); 
        }            
                
    }  
    
    return(eores_OK);    
}



static eOresult_t s_eo_motioncontrol_onendofverify_mais(EOaService* s, eObool_t operationisok)
{
    if(eobool_true == operationisok)
    {
        eo_candiscovery2_Start(eo_candiscovery2_GetHandle(), &s_eo_themotcon.sharedcan.discoverytarget, &s_eo_themotcon.sharedcan.ondiscoverystop);        
    } 
    else
    {
        // the mais fails. we dont even start the discovery of the mc4 boards. we just issue an error report and call onverify() w/ false argument
        
        // prepare things        
        s_eo_themotcon.diagnostics.errorDescriptor.sourcedevice     = eo_errman_sourcedevice_localboard;
        s_eo_themotcon.diagnostics.errorDescriptor.sourceaddress    = 0;
        s_eo_themotcon.diagnostics.errorDescriptor.par16            = 0;
        s_eo_themotcon.diagnostics.errorDescriptor.par64            = 0;    
        EOaction_strg astrg = {0};
        EOaction *act = (EOaction*)&astrg;
        eo_action_SetCallback(act, s_eo_motioncontrol_send_periodic_error_report, NULL, eov_callbackman_GetTask(eov_callbackman_GetHandle()));    
               
        // fill error description. and transmit it
        s_eo_themotcon.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_mc4_failed_mais_verify);
        s_eo_themotcon.diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), s_eo_themotcon.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_themotcon.diagnostics.errorDescriptor);
        
        if(0 != s_eo_themotcon.diagnostics.reportPeriod)
        {
            s_eo_themotcon.diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(s_eo_themotcon.diagnostics.reportTimer, eok_abstimeNOW, s_eo_themotcon.diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
 
        // call onverify
        if(NULL != s_eo_themotcon.service.onverify)
        {
            s_eo_themotcon.service.onverify(&s_eo_themotcon, eobool_false); 
        }            
        
    }
 
    return(eores_OK);    
}


static eOresult_t s_eo_motioncontrol_onstop_search4mc4s(void *par, EOtheCANdiscovery2* p, eObool_t searchisok)
{
    
    if((eobool_true == searchisok) && (eobool_true == s_eo_themotcon.service.activateafterverify))
    {
        const eOmn_serv_configuration_t * mcserv = (const eOmn_serv_configuration_t *)par;
        eo_motioncontrol_Activate(&s_eo_themotcon, mcserv);
    }

    s_eo_themotcon.diagnostics.errorDescriptor.sourcedevice     = eo_errman_sourcedevice_localboard;
    s_eo_themotcon.diagnostics.errorDescriptor.sourceaddress    = 0;
    s_eo_themotcon.diagnostics.errorDescriptor.par16            = 0;
    s_eo_themotcon.diagnostics.errorDescriptor.par64            = 0;    
    EOaction_strg astrg = {0};
    EOaction *act = (EOaction*)&astrg;
    eo_action_SetCallback(act, s_eo_motioncontrol_send_periodic_error_report, NULL, eov_callbackman_GetTask(eov_callbackman_GetHandle()));    
    
    if(eobool_true == searchisok)
    {        
        s_eo_themotcon.diagnostics.errorType = eo_errortype_debug;
        s_eo_themotcon.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_mc4_ok);
        eo_errman_Error(eo_errman_GetHandle(), s_eo_themotcon.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_themotcon.diagnostics.errorDescriptor);
        
        if((0 != s_eo_themotcon.diagnostics.repetitionOKcase) && (0 != s_eo_themotcon.diagnostics.reportPeriod))
        {
            s_eo_themotcon.diagnostics.errorCallbackCount = s_eo_themotcon.diagnostics.repetitionOKcase;        
            eo_timer_Start(s_eo_themotcon.diagnostics.reportTimer, eok_abstimeNOW, s_eo_themotcon.diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    } 
    
    if(eobool_false == searchisok)
    {
        s_eo_themotcon.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_mc4_failed_candiscovery_of_mc4);
        s_eo_themotcon.diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), s_eo_themotcon.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_themotcon.diagnostics.errorDescriptor);
        
        if(0 != s_eo_themotcon.diagnostics.reportPeriod)
        {
            s_eo_themotcon.diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(s_eo_themotcon.diagnostics.reportTimer, eok_abstimeNOW, s_eo_themotcon.diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    }    
    
    
    if(NULL != s_eo_themotcon.service.onverify)
    {
        s_eo_themotcon.service.onverify(&s_eo_themotcon, searchisok); 
    }    
        
    return(eores_OK);   
}



// want to send a canframe with pwm onto can bus. 
static eOresult_t s_eo_motioncontrol_SetCurrentSetpoint(EOtheMotionController *p, int16_t *pwmList, uint8_t size)
{
    eOcanport_t port = eOcanport1;
    
    // now i need to assign port and command with correct values.
    
    // i manage 2foc boards. they are at most 4. they must be all in the same can bus. 
    int16_t pwmValues[4] = {0, 0, 0, 0};
    
    uint8_t i=0;
    for(i=0; i<p->numofjomos; i++)
    {
        eOcanmap_location_t loc = {0};
        // search the address of motor i-th and fill the pwmValues[] in relevant position.
        if(eores_OK == eo_canmap_GetEntityLocation(eo_canmap_GetHandle(), eoprot_ID_get(eoprot_endpoint_motioncontrol, eoprot_entity_mc_motor, i, 0), &loc, NULL, NULL))
        {
            port = (eOcanport_t)loc.port;  // marco.accame: i dont check if the port is not always the same ... it MUST be.          
            if((loc.addr > 0) && (loc.addr <= 4))
            {
                pwmValues[loc.addr-1] = pwmList[i];
            }            
        }        
    }
    
    // ok, now i fill command and location
    eOcanprot_command_t command = {0};
    command.class = eocanprot_msgclass_periodicMotorControl;    
    command.type  = ICUBCANPROTO_PER_MC_MSG__EMSTO2FOC_DESIRED_CURRENT;
    command.value = &pwmValues[0];
    
    eOcanmap_location_t location = {0};
    location.port = port;
    location.addr = 0; // marco.accame: we put 0 just because it is periodic and this is the source address (the EMS has can address 0).
    location.insideindex = eocanmap_insideindex_first; // because all 2foc have motor on index-0. 

    // and i send the command
    return(eo_canserv_SendCommandToLocation(eo_canserv_GetHandle(), &command, location));   
}


static void s_eo_motioncontrol_UpdateJointStatus(EOtheMotionController *p)
{
    const uint8_t transmit_decoupled_pwms = 1;
    
    eOmc_joint_status_t *jstatus = NULL;
    eOmc_motor_status_t *mstatus = NULL;
    
    uint8_t jId = 0;
        
    for(jId = 0; jId<p->numofjomos; jId++)
    {
        if(NULL != (jstatus = eo_entities_GetJointStatus(eo_entities_GetHandle(), jId)))
        {
       
            eo_emsController_GetJointStatus(jId, jstatus);
            
            eo_emsController_GetActivePidStatus(jId, &jstatus->ofpid);
            
            if(1 == transmit_decoupled_pwms) 
            {  
                // this functions is used to get the motor PWM after the decoupling matrix
                eo_emsController_GetPWMOutput(jId, &jstatus->ofpid.generic.output);
            }
            
            jstatus->modes.ismotiondone = eo_emsController_GetMotionDone(jId);
           
        }
 
    }   
    
    
    for(jId = 0; jId<p->numofjomos; jId++)
    {
        if(NULL != (mstatus = eo_entities_GetMotorStatus(eo_entities_GetHandle(), jId)))
        {
            eo_emsController_GetMotorStatus(jId, mstatus);
        }
    }
}


static void s_eo_motioncontrol_proxy_config(eObool_t on)
{    
    if(eomn_serv_MC_mc4 == s_eo_themotcon.service.servconfig.type)
    {
        if(eobool_true == on)
        {
            eoprot_config_proxied_variables(eoprot_board_localboard, eoprot_endpoint_motioncontrol, s_eo_motioncontrol_mc4based_variableisproxied);
        }
        else
        {
            eoprot_config_proxied_variables(eoprot_board_localboard, eoprot_endpoint_motioncontrol, NULL);
        }       
    }
    else
    {   // no proxied variables
        eoprot_config_proxied_variables(eoprot_board_localboard, eoprot_endpoint_motioncontrol, NULL); 
    }
          
}


// review it ....
static eObool_t s_eo_motioncontrol_mc4based_variableisproxied(eOnvID32_t id)
{    
    eOprotEndpoint_t ep = eoprot_ID2endpoint(id);
    
    eOprotEntity_t ent = eoprot_ID2entity(id);
    if(eoprot_entity_mc_joint != ent)
    {
        return(eobool_false);
    }
    
    eOprotTag_t tag = eoprot_ID2tag(id);
    
    switch(tag)
    {
        case eoprot_tag_mc_joint_config_pidposition:
        // case eoprot_tag_mc_joint_config_pidvelocity:     // marco.accame on 03mar15: the pidvelocity propagation to mc4 is is not implemented, thus i must remove from proxy.
        case eoprot_tag_mc_joint_config_pidtorque:
        case eoprot_tag_mc_joint_config_limitsofjoint:
        case eoprot_tag_mc_joint_config_impedance:
        case eoprot_tag_mc_joint_status_modes_ismotiondone:
        {
            return(eobool_true);
        }
        
        default:
        {
            return(eobool_false);
        }
     }
}


static void s_eo_motioncontrol_mc4plusbased_hal_init_motors_adc_feedbacks(void)
{   // low level init for motors and adc in mc4plus
//#if defined(USE_MC4PLUS)    
    // currently the motors are initialized all together and without config
    hal_motor_init(hal_motorALL, NULL);
        
    // first initialize all ADC as indipendents
    hal_adc_common_structure_init();
        
    // init the ADC to read the (4) current values of the motor and (4) analog inputs (hall_sensors, if any) using ADC1/ADC3
    // always initialized at the moment, but the proper interface for reading the values is in EOappEncodersReader
    hal_adc_dma_init_ADC1_ADC3_hall_sensor_current();    
//#endif    
}


static void s_eo_motioncontrol_mc4plusbased_hal_init_quad_enc_indexes_interrupt(void)
{   // activate interupt line for quad_enc indexes check. the call of hal_quad_enc_init() is inside eo_appEncReader_Activate() ... maybe move it inside there too
//#if defined(USE_MC4PLUS)   
    hal_quad_enc_init_indexes_flags();    
//#endif    
}

static void s_eo_motioncontrol_mc4plusbased_enable_all_motors(EOtheMotionController *p)
{
    // enable PWM of the motors (if not faulted)
    if (!s_eo_mcserv_are_motors_ext_faulted())
    {
        EOconstarray* carray = eo_constarray_Load((EOarray*)&p->service.servconfig.data.mc.mc4plus_based.arrayofjomodescriptors);
        
        for(uint8_t i=0; i<p->numofjomos; i++)
        {
            //const eOmn_serv_jomo_descriptor_t *jomodes = (eOmn_serv_jomo_descriptor_t*) eo_constarray_At(carray, i);            
            //uint8_t port = jomodes->actuator.pwm.port;
            s_eo_mcserv_pwm_enable(p->mcmc4plus.pwmport[i]);
        }
    }
    
    return;
}


static void s_eo_mcserv_pwm_set(uint8_t i, int16_t v)
{
    //out of bound
    if (i > 3)
        return;
//#if defined(USE_MC4PLUS)    
    hal_motor_pwmset((hal_motor_t)i, v);
//#endif    
}

static void s_eo_mcserv_pwm_enable(uint8_t i)
{
    //out of bound
    if (i > 3)
        return;
//#if defined(USE_MC4PLUS)    
    hal_motor_enable((hal_motor_t)i);
//#endif    
}

static void s_eo_mcserv_pwm_disable(uint8_t i)
{
    //out of bound
    if (i > 3)
        return;
//#if defined(USE_MC4PLUS)    
    hal_motor_disable((hal_motor_t)i);
//#endif    
}


static eObool_t s_eo_mcserv_are_motors_ext_faulted(void)
{
//#if defined(USE_MC4PLUS)    
    if(hal_true == hal_motor_externalfaulted())
    {
        return eobool_true;
    }
//#endif       
    return eobool_false;
}


static eOresult_t s_eo_mcserv_do_mc4plus(EOtheMotionController *p)
{
    uint8_t error_mask = 0;
    eOresult_t res = eores_NOK_generic;
    uint32_t encvalue[4] = {0}; 
    uint32_t extra[4] = {0};
    hal_encoder_errors_flags encflags[4] = {0};
    int16_t pwm[4] = {0};  
    
    EOconstarray* carray = eo_constarray_Load((EOarray*)&p->service.servconfig.data.mc.mc4plus_based.arrayofjomodescriptors);

    uint8_t i = 0;
    
    // wait for the encoders for some time 
    for(i=0; i<30; ++i)
    {
        if(eobool_true == eo_encoderreader_IsReadingAvailable(p->mcmc4plus.theencoderreader))
        {
            break;
        }
        else
        {
            hal_sys_delay(5*hal_RELTIME_1microsec);
        }
    }  
    
    // read the encoders as 2foc does       
    if(eobool_true == eo_encoderreader_IsReadingAvailable(p->mcmc4plus.theencoderreader))
    {    
        for(i=0; i<p->numofjomos; i++)
        {
            const eOmn_serv_jomo_descriptor_t *jomodes = (eOmn_serv_jomo_descriptor_t*) eo_constarray_At(carray, i);   
            
            // the object EOtheEncoderReader (but maybe it may be called EOthePositionReader) will internaly manage any type of sensor, even the MAIS
            res = eo_encoderreader_Read(p->mcmc4plus.theencoderreader, i, &(encvalue[i]), &(extra[i]), &(encflags[i]));
            if(res != eores_OK)
            {
                error_mask |= 1<<(i<<1);
            }                          
            
            // if the extra encoder is a rotor position, update the proper value
            if(eomn_serv_mc_sensor_pos_atmotor == jomodes->extrasensor.pos) 
            {
                eo_emsController_AcquireMotorPosition(i, extra[i]);
            }
        }        
    }
    else
    {
        error_mask = 0xAA; // timeout
    }    
    
//    // read the encoders as davide does
//    if (eo_appEncReader_isReady(app_enc_reader))
//    {
//        for(jm=0; jm<p->config.jomosnumber; jm++)
//        {
//            // for some type of encoder I need to get the values from EOappEncodersReader, for the others we get the value in other ways
//            // at least one of the two encoder is local 
//            if (LOCAL_ENCODER(p->config.jomos[jm].encoder.etype) || LOCAL_ENCODER(p->config.jomos[jm].extra_encoder.etype))
//            {
//                res = eo_appEncReader_GetValue (app_enc_reader,
//                                                     p->config.jomos[jm].encoder.enc_joint,
//                                                     &p->valuesencoder[jm],
//                                                     &p->valuesencoder_extra[jm],
//                                                     &fl);
//            }
//            // MAIS is always primary
//            if (p->config.jomos[jm].encoder.etype == 4) //MAIS
//            {
//            /* it should be a MAIS encoder! So here:
//                - call a function to convert the last MAIS values (saved into the 15values array) to iCubDegrees
//                - copy inside p->valuesencoder[jm] the right position value
//            */
//                
//                //using encoder.index as prot index for getting the mais entity inside the function
//                res = s_eo_mcserv_MaisValues2Pos(p->config.jomos[jm].encoder.index,
//                                                 p->config.jomos[jm].actuator.local.index,
//                                                 jm,
//                                                 &p->valuesencoder[jm]);
//            }
//            
//            //if the extra encoder is a rotor position, update the proper value
//            if((p->config.jomos[jm].extra_encoder.isthere == 1) && (p->config.jomos[jm].extra_encoder.pos_type == 1)) // rotor position
//            {
//                eo_emsController_AcquireMotorPosition(jm, p->valuesencoder_extra[jm]);
//            }
//            
//            //if the extra encoder is another joint position...where can I put those values?
//            else if((p->config.jomos[jm].extra_encoder.isthere == 1) && (p->config.jomos[jm].extra_encoder.pos_type == 0)) // joint position
//            {
//   
//            }
//            
//             //fill the errormask
//             if (res != eores_OK)
//                errormask |= 1<<(jm<<1);
//        }
//    }   
//    else
//    {
//        errormask = 0xAA; // timeout
//    }
    
    // Restart the reading of the encoders
    eo_encoderreader_StartReading(p->mcmc4plus.theencoderreader);
    
    // apply the readings (primary encoder) to local controller and check for possible faults
    eo_emsController_AcquireAbsEncoders((int32_t*)encvalue, error_mask);
    eo_emsController_CheckFaults();
    
    // compute the pwm using pid
    eo_emsController_PWM(pwm);
    
    #warning: for MAIS-controlled joints... do a check on the limits (see MC4 firmware) before physically applying PWM
    
    // 6. apply the pwm. for the case of mc4plus we call hal_pwm();
    for(i=0; i<p->numofjomos; i++)
    {
        s_eo_mcserv_pwm_set(p->mcmc4plus.pwmport[i], pwm[i]);
    }

    // 7. propagate the status of joint motors locally computed in localcontroller to the joints / motors in ram
    {   // so far in here. but later on move it in a function.... also 2foc mode does that 
        const uint8_t transmit_decoupled_pwms = 1;
                
        for(i=0; i<p->numofjomos; i++)
        {
            eOmc_joint_status_t *jstatus = NULL;
            if(NULL != (jstatus = eo_entities_GetJointStatus(eo_entities_GetHandle(), i)))
            {
            
                eo_emsController_GetJointStatus(i, jstatus);
                eo_emsController_GetActivePidStatus(i, &(jstatus->ofpid)); 
                if(1 == transmit_decoupled_pwms) 
                {   //this functions is used to get the motor PWM after the decoupling matrix
                    eo_emsController_GetPWMOutput(i, &(jstatus->ofpid.generic.output));
                }
                
                jstatus->modes.ismotiondone = eo_emsController_GetMotionDone(i);
            }
            
        }

            
        for(i=0; i<p->numofjomos; i++)
        {
            eOmc_motor_status_t *mstatus = NULL;  
            if(NULL != (mstatus = eo_entities_GetMotorStatus(eo_entities_GetHandle(), i)))
            {
                eo_emsController_GetMotorStatus(i, mstatus);
            }
        }
        
    } // propagate status
    
    return(res);
}


static void s_eo_mcserv_disable_all_motors(EOtheMotionController *p)
{
     for(uint8_t i=0; i<p->numofjomos; i++)
     {
//        if(1 == p->config.jomos[jm].actuator.local.type)
//        {   // on board 
        s_eo_mcserv_pwm_set(p->mcmc4plus.pwmport[i], 0);
        s_eo_mcserv_pwm_disable(p->mcmc4plus.pwmport[i]);
//        }
     }
     
     return;
}


// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------


