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

// - include guard ----------------------------------------------------------------------------------------------------
#ifndef _EOTRANSCEIVER_H_
#define _EOTRANSCEIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @file       EOtransceiver.h
    @brief      This header file implements public interface to a frame.
    @author     marco.accame@iit.it
    @date       01/11/2010
**/

/** @defgroup eo_transceiver Object EOtransceiver
    The EOtransceiver object is used as ...
         
    @{        
 **/



// - external dependencies --------------------------------------------------------------------------------------------

#include "EoCommon.h"
#include "EOpacket.h"
#include "EOnvsCfg.h"
#include "EOrop.h"
#include "EOVmutex.h"




// - public #define  --------------------------------------------------------------------------------------------------
// empty-section
  

// - declaration of public user-defined types ------------------------------------------------------------------------- 


/** @typedef    typedef struct EOtransceiver_hid EOtransceiver
    @brief      EOtransceiver is an opaque struct. It is used to implement data abstraction for the datagram 
                object so that the user cannot see its private fields and he/she is forced to manipulate the
                object only with the proper public functions. 
 **/  
typedef struct EOtransceiver_hid EOtransceiver;


typedef enum
{
    eo_trans_protection_none                    = 0,
    eo_trans_protection_enabled                 = 1       
} eOtransceiver_protection_t;


typedef struct   
{
    uint16_t        capacityoftxpacket; 
    uint16_t        capacityofrop;    
    uint16_t        capacityofropframeregulars; 
    uint16_t        capacityofropframeoccasionals;
    uint16_t        capacityofropframereplies;
    uint16_t        maxnumberofregularrops;
} eOtransceiver_sizes_t; 


typedef struct
{
    uint16_t                        capacityoftxpacket; 
    uint16_t                        capacityofrop;    
    uint16_t                        capacityofropframeregulars; 
    uint16_t                        capacityofropframeoccasionals;
    uint16_t                        capacityofropframereplies;
    uint16_t                        maxnumberofregularrops;
    eOipv4addr_t                    remipv4addr;           
    eOipv4port_t                    remipv4port;    
    EOnvsCfg*                       nvscfg;         // later on we could split it into a locnvscfg and a remnvscfg
    eov_mutex_fn_mutexderived_new   mutex_fn_new;
    eOtransceiver_protection_t      protection;
} eOtransceiver_cfg_t;


// typedef struct      // 12 bytes           
// {
//     eOropconfig_t   ropcfg;     // 4B
//     eOropcode_t     ropcode;    // 1B
//     eOnvEP_t        nvep;       // 2B
//     eOnvID_t        nvid;       // 2B
//     uint32_t        signature;
// } eo_transceiver_ropinfo_t;


// typedef struct      // 16 bytes
// {
//     eOropconfig_t   ropconfig;      // 4B
//     eOropconfinfo_t ropconfinfo;    // 1B
//     eOropcode_t     ropcode;        // 1B
//     eOnvEP_t        ep;             // 2B
//     eOnvID_t        id;             // 2B
//     uint32_t        signature;      // 4B
// } eOropdescriptor_t;   
    
// - declaration of extern public variables, ... but better using use _get/_set instead -------------------------------

extern const eOtransceiver_cfg_t eo_transceiver_cfg_default; //= {512, 128, 256, 128, 128, 16, EO_COMMON_IPV4ADDR_LOCALHOST, 10001, NULL, NULL};


// - declaration of extern public functions ---------------------------------------------------------------------------
 
 
/** @fn         extern EOtransceiver* eo_transceiver_New(uint16_t capacity)
    @brief      Creates a new frame object and allocates memory able to store @e capacity bytes. If @e capacity is
                zero, then the object shall have external storage mode.
    @param      capacity   The max size of the packet.
    @return     The pointer to the required object.
 **/
 
 
 // gestisce 1 solo indirizzo ip di destinazione in modo da avere 1 solo EOpacket in uscita.
 // 
extern EOtransceiver* eo_transceiver_New(const eOtransceiver_cfg_t *cfg);

extern eOresult_t eo_transceiver_Receive(EOtransceiver *p, EOpacket *pkt, uint16_t *numberofrops, eOabstime_t* txtime); 



/** @fn         extern eOresult_t eo_transceiver_outpacket_Prepare(EOtransceiver *p, uint16_t *numberofrops)
    @brief      prepares outpacket to send. It contains all rops to send.   
    @param      p               poiter to transceiver        
    @param      numberofrops    in outpunt will contain number of rops cointained in outpacket
    @return     eores_OK or eores_NOK_nullpointer
 **/
extern eOresult_t eo_transceiver_outpacket_Prepare(EOtransceiver *p, uint16_t *numberofrops);


/** @fn         extern eOresult_t eo_transceiver_outpacket_Get(EOtransceiver *p, EOpacket **pkt)
    @brief      returns a pointer to outpacket. the aoutpacket is well formed only if eo_transceiver_outpacket_Prepare 
                function is called before eo_transceiver_outpacket_Get.  
    @param      p         poiter to transceiver        
    @param      pkt       in outpunt will contain poiter to aiutpacket
    @return     eores_OK or eores_NOK_nullpointer
 **/
extern eOresult_t eo_transceiver_outpacket_Get(EOtransceiver *p, EOpacket **pkt);

extern eOresult_t eo_transceiver_rop_regular_Clear(EOtransceiver *p);
extern eOresult_t eo_transceiver_rop_regular_Load(EOtransceiver *p, eOropdescriptor_t *ropdes); 
extern eOresult_t eo_transceiver_rop_regular_Unload(EOtransceiver *p, eOropdescriptor_t *ropdes); 

extern eOresult_t eo_transceiver_rop_occasional_Load_without_data(EOtransceiver *p, eOropdescriptor_t *ropdesc, uint8_t itisobsolete);

extern eOresult_t eo_transceiver_rop_occasional_Load(EOtransceiver *p, eOropdescriptor_t *ropdes);




/** @}            
    end of group eo_transceiver  
 **/

#ifdef __cplusplus
}       // closing brace for extern "C"
#endif 

#endif  // include-guard


// - end-of-file (leave a blank line after)----------------------------------------------------------------------------

