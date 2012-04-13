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

/* @file       eOcfg_nvsEP_base_con.c
    @brief      This file keeps constant configuration for the NVs of the updater
    @author     marco.accame@iit.it
    @date       09/06/2011
**/


// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

#include "stdlib.h" 
#include "string.h"
#include "stdio.h"

#include "eOcommon.h"
#include "EOnv_hid.h"
#include "EOtreenode_hid.h"
#include "EOconstvector_hid.h"

#include "eOcfg_nvsEP_mc_jxx_con.h" // to see eo_cfg_nvsEP_mc_jxx_default

#include "eOcfg_nvsEP_mc_mxx_con.h"





// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "eOcfg_nvsEP_mc_leftleg_con.h"


// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------

static uint16_t s_hash(uint16_t id);

// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------


#define JMACRO_EXTERNALPREFIX_GETID                EOK_cfg_nvsEP_mc_NVID


// joint 00
#define JMACRO_PSTR    _leftleg
#define JMACRO_JSTR    _j00
#define JMACRO_PNUM    5
#define JMACRO_JNUM    0    
#define JMACRO_JOFF    (JMACRO_JNUM*sizeof(eOmc_joint_t))

#include "eOcfg_nvsEP_mc_jpragma_con.c"


// joint 01
#define JMACRO_PSTR    _leftleg
#define JMACRO_JSTR    _j01
#define JMACRO_PNUM    5
#define JMACRO_JNUM    1    
#define JMACRO_JOFF    (JMACRO_JNUM*sizeof(eOmc_joint_t))

#include "eOcfg_nvsEP_mc_jpragma_con.c"



// joint 02
#define JMACRO_PSTR    _leftleg
#define JMACRO_JSTR    _j02
#define JMACRO_PNUM    5
#define JMACRO_JNUM    2    
#define JMACRO_JOFF    (JMACRO_JNUM*sizeof(eOmc_joint_t))

#include "eOcfg_nvsEP_mc_jpragma_con.c"


// joint 03
#define JMACRO_PSTR    _leftleg
#define JMACRO_JSTR    _j03
#define JMACRO_PNUM    5
#define JMACRO_JNUM    3    
#define JMACRO_JOFF    (JMACRO_JNUM*sizeof(eOmc_joint_t))

#include "eOcfg_nvsEP_mc_jpragma_con.c"


// joint 04
#define JMACRO_PSTR    _leftleg
#define JMACRO_JSTR    _j04
#define JMACRO_PNUM    5
#define JMACRO_JNUM    4    
#define JMACRO_JOFF    (JMACRO_JNUM*sizeof(eOmc_joint_t))

#include "eOcfg_nvsEP_mc_jpragma_con.c"


// joint 05
#define JMACRO_PSTR    _leftleg
#define JMACRO_JSTR    _j05
#define JMACRO_PNUM    5
#define JMACRO_JNUM    5    
#define JMACRO_JOFF    (JMACRO_JNUM*sizeof(eOmc_joint_t))

#include "eOcfg_nvsEP_mc_jpragma_con.c"


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables
// --------------------------------------------------------------------------------------------------------------------

#define Jn      (12)

#define J00p    (0)
#define J01p    (1*Jn)
#define J02p    (2*Jn)
#define J03p    (3*Jn)
#define J04p    (4*Jn)
#define J05p    (5*Jn)

//extern EOtreenode eo_cfg_nvsEP_mc_leftleg_tree_con[];

extern EOtreenode eo_cfg_nvsEP_mc_leftleg_tree_con[] =
{
    // joint00
    {   // 0
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j00_jconfig,
        EO_INIT(.index)     0+J00p,
        EO_INIT(.nchildren) 7,
        EO_INIT(.ichildren) {1+J00p, 2+J00p, 3+J00p, 4+J00p, 5+J00p, 6+J00p, 7+J00p},
        EO_INIT(.pchildren) {&eo_cfg_nvsEP_mc_leftleg_tree_con[1+J00p], &eo_cfg_nvsEP_mc_leftleg_tree_con[2+J00p], &eo_cfg_nvsEP_mc_leftleg_tree_con[3+J00p], 
                             &eo_cfg_nvsEP_mc_leftleg_tree_con[4+J00p], &eo_cfg_nvsEP_mc_leftleg_tree_con[5+J00p], &eo_cfg_nvsEP_mc_leftleg_tree_con[6+J00p], 
                             &eo_cfg_nvsEP_mc_leftleg_tree_con[7+J00p]}
    },
        {   // 1
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j00_jconfig__pidposition,
            EO_INIT(.index)     1+J00p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },  
        {   // 2
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j00_jconfig__pidvelocity,
            EO_INIT(.index)     2+J00p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },
        {   // 3
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j00_jconfig__pidtorque,
            EO_INIT(.index)     3+J00p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },        
        {   // 4
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j00_jconfig__minpositionofjoint,
            EO_INIT(.index)     4+J00p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },         
        {   // 5
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j00_jconfig__maxpositionofjoint,
            EO_INIT(.index)     5+J00p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },   
        {   // 6
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j00_jconfig__velocitysetpointtimeout,
            EO_INIT(.index)     6+J00p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },         
        {   // 7
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j00_jconfig__upto02descrforchameleon02,
            EO_INIT(.index)     7+J00p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },   
    {   // 8
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j00__jstatus,
        EO_INIT(.index)     8+J00p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   // 9
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j00__calibrator,
        EO_INIT(.index)     9+J00p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   //10
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j00__setpoint,
        EO_INIT(.index)     10+J00p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   //11
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j00__controlmode,
        EO_INIT(.index)     11+J00p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },


    // joint01
    {   // 0
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j01_jconfig,
        EO_INIT(.index)     0+J01p,
        EO_INIT(.nchildren) 7,
        EO_INIT(.ichildren) {1+J01p, 2+J01p, 3+J01p, 4+J01p, 5+J01p, 6+J01p, 7+J01p},
        EO_INIT(.pchildren) {&eo_cfg_nvsEP_mc_leftleg_tree_con[1+J01p], &eo_cfg_nvsEP_mc_leftleg_tree_con[2+J01p], &eo_cfg_nvsEP_mc_leftleg_tree_con[3+J01p], 
                             &eo_cfg_nvsEP_mc_leftleg_tree_con[4+J01p], &eo_cfg_nvsEP_mc_leftleg_tree_con[5+J01p], &eo_cfg_nvsEP_mc_leftleg_tree_con[6+J01p], 
                             &eo_cfg_nvsEP_mc_leftleg_tree_con[7+J01p]}
    },
        {   // 1
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j01_jconfig__pidposition,
            EO_INIT(.index)     1+J01p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },  
        {   // 2
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j01_jconfig__pidvelocity,
            EO_INIT(.index)     2+J01p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },
        {   // 3
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j01_jconfig__pidtorque,
            EO_INIT(.index)     3+J01p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },        
        {   // 4
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j01_jconfig__minpositionofjoint,
            EO_INIT(.index)     4+J01p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },         
        {   // 5
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j01_jconfig__maxpositionofjoint,
            EO_INIT(.index)     5+J01p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },   
        {   // 6
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j01_jconfig__velocitysetpointtimeout,
            EO_INIT(.index)     6+J01p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },         
        {   // 7
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j01_jconfig__upto02descrforchameleon02,
            EO_INIT(.index)     7+J01p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },   
    {   // 8
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j01__jstatus,
        EO_INIT(.index)     8+J01p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   // 9
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j01__calibrator,
        EO_INIT(.index)     9+J01p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   //10
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j01__setpoint,
        EO_INIT(.index)     10+J01p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   //11
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j01__controlmode,
        EO_INIT(.index)     11+J01p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    

    // joint02
    {   // 0
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j02_jconfig,
        EO_INIT(.index)     0+J02p,
        EO_INIT(.nchildren) 7,
        EO_INIT(.ichildren) {1+J02p, 2+J02p, 3+J02p, 4+J02p, 5+J02p, 6+J02p, 7+J02p},
        EO_INIT(.pchildren) {&eo_cfg_nvsEP_mc_leftleg_tree_con[1+J02p], &eo_cfg_nvsEP_mc_leftleg_tree_con[2+J02p], &eo_cfg_nvsEP_mc_leftleg_tree_con[3+J02p], 
                             &eo_cfg_nvsEP_mc_leftleg_tree_con[4+J02p], &eo_cfg_nvsEP_mc_leftleg_tree_con[5+J02p], &eo_cfg_nvsEP_mc_leftleg_tree_con[6+J02p], 
                             &eo_cfg_nvsEP_mc_leftleg_tree_con[7+J02p]}
    },
        {   // 1
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j02_jconfig__pidposition,
            EO_INIT(.index)     1+J02p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },  
        {   // 2
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j02_jconfig__pidvelocity,
            EO_INIT(.index)     2+J02p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },
        {   // 3
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j02_jconfig__pidtorque,
            EO_INIT(.index)     3+J02p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },        
        {   // 4
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j02_jconfig__minpositionofjoint,
            EO_INIT(.index)     4+J02p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },         
        {   // 5
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j02_jconfig__maxpositionofjoint,
            EO_INIT(.index)     5+J02p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },   
        {   // 6
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j02_jconfig__velocitysetpointtimeout,
            EO_INIT(.index)     6+J02p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },         
        {   // 7
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j02_jconfig__upto02descrforchameleon02,
            EO_INIT(.index)     7+J02p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },   
    {   // 8
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j02__jstatus,
        EO_INIT(.index)     8+J02p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   // 9
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j02__calibrator,
        EO_INIT(.index)     9+J02p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   //10
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j02__setpoint,
        EO_INIT(.index)     10+J02p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   //11
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j02__controlmode,
        EO_INIT(.index)     11+J02p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },

    
    // joint03
    {   // 0
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j03_jconfig,
        EO_INIT(.index)     0+J03p,
        EO_INIT(.nchildren) 7,
        EO_INIT(.ichildren) {1+J03p, 2+J03p, 3+J03p, 4+J03p, 5+J03p, 6+J03p, 7+J03p},
        EO_INIT(.pchildren) {&eo_cfg_nvsEP_mc_leftleg_tree_con[1+J03p], &eo_cfg_nvsEP_mc_leftleg_tree_con[2+J03p], &eo_cfg_nvsEP_mc_leftleg_tree_con[3+J03p], 
                             &eo_cfg_nvsEP_mc_leftleg_tree_con[4+J03p], &eo_cfg_nvsEP_mc_leftleg_tree_con[5+J03p], &eo_cfg_nvsEP_mc_leftleg_tree_con[6+J03p], 
                             &eo_cfg_nvsEP_mc_leftleg_tree_con[7+J03p]}
    },
        {   // 1
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j03_jconfig__pidposition,
            EO_INIT(.index)     1+J03p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },  
        {   // 2
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j03_jconfig__pidvelocity,
            EO_INIT(.index)     2+J03p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },
        {   // 3
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j03_jconfig__pidtorque,
            EO_INIT(.index)     3+J03p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },        
        {   // 4
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j03_jconfig__minpositionofjoint,
            EO_INIT(.index)     4+J03p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },         
        {   // 5
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j03_jconfig__maxpositionofjoint,
            EO_INIT(.index)     5+J03p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },   
        {   // 6
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j03_jconfig__velocitysetpointtimeout,
            EO_INIT(.index)     6+J03p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },         
        {   // 7
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j03_jconfig__upto02descrforchameleon02,
            EO_INIT(.index)     7+J03p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },   
    {   // 8
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j03__jstatus,
        EO_INIT(.index)     8+J03p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   // 9
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j03__calibrator,
        EO_INIT(.index)     9+J03p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   //10
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j03__setpoint,
        EO_INIT(.index)     10+J03p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   //11
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j03__controlmode,
        EO_INIT(.index)     11+J03p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },


    // joint04
    {   // 0
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j04_jconfig,
        EO_INIT(.index)     0+J04p,
        EO_INIT(.nchildren) 7,
        EO_INIT(.ichildren) {1+J04p, 2+J04p, 3+J04p, 4+J04p, 5+J04p, 6+J04p, 7+J04p},
        EO_INIT(.pchildren) {&eo_cfg_nvsEP_mc_leftleg_tree_con[1+J04p], &eo_cfg_nvsEP_mc_leftleg_tree_con[2+J04p], &eo_cfg_nvsEP_mc_leftleg_tree_con[3+J04p], 
                             &eo_cfg_nvsEP_mc_leftleg_tree_con[4+J04p], &eo_cfg_nvsEP_mc_leftleg_tree_con[5+J04p], &eo_cfg_nvsEP_mc_leftleg_tree_con[6+J04p], 
                             &eo_cfg_nvsEP_mc_leftleg_tree_con[7+J04p]}
    },
        {   // 1
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j04_jconfig__pidposition,
            EO_INIT(.index)     1+J04p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },  
        {   // 2
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j04_jconfig__pidvelocity,
            EO_INIT(.index)     2+J04p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },
        {   // 3
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j04_jconfig__pidtorque,
            EO_INIT(.index)     3+J04p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },        
        {   // 4
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j04_jconfig__minpositionofjoint,
            EO_INIT(.index)     4+J04p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },         
        {   // 5
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j04_jconfig__maxpositionofjoint,
            EO_INIT(.index)     5+J04p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },   
        {   // 6
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j04_jconfig__velocitysetpointtimeout,
            EO_INIT(.index)     6+J04p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },         
        {   // 7
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j04_jconfig__upto02descrforchameleon02,
            EO_INIT(.index)     7+J04p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },   
    {   // 8
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j04__jstatus,
        EO_INIT(.index)     8+J04p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   // 9
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j04__calibrator,
        EO_INIT(.index)     9+J04p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   //10
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j04__setpoint,
        EO_INIT(.index)     10+J04p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   //11
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j04__controlmode,
        EO_INIT(.index)     11+J04p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },

    // joint05
    {   // 0
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j05_jconfig,
        EO_INIT(.index)     0+J05p,
        EO_INIT(.nchildren) 7,
        EO_INIT(.ichildren) {1+J05p, 2+J05p, 3+J05p, 4+J05p, 5+J05p, 6+J05p, 7+J05p},
        EO_INIT(.pchildren) {&eo_cfg_nvsEP_mc_leftleg_tree_con[1+J05p], &eo_cfg_nvsEP_mc_leftleg_tree_con[2+J05p], &eo_cfg_nvsEP_mc_leftleg_tree_con[3+J05p], 
                             &eo_cfg_nvsEP_mc_leftleg_tree_con[4+J05p], &eo_cfg_nvsEP_mc_leftleg_tree_con[5+J05p], &eo_cfg_nvsEP_mc_leftleg_tree_con[6+J05p], 
                             &eo_cfg_nvsEP_mc_leftleg_tree_con[7+J05p]}
    },
        {   // 1
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j05_jconfig__pidposition,
            EO_INIT(.index)     1+J05p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },  
        {   // 2
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j05_jconfig__pidvelocity,
            EO_INIT(.index)     2+J05p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },
        {   // 3
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j05_jconfig__pidtorque,
            EO_INIT(.index)     3+J05p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },        
        {   // 4
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j05_jconfig__minpositionofjoint,
            EO_INIT(.index)     4+J05p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },         
        {   // 5
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j05_jconfig__maxpositionofjoint,
            EO_INIT(.index)     5+J05p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },   
        {   // 6
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j05_jconfig__velocitysetpointtimeout,
            EO_INIT(.index)     6+J05p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },         
        {   // 7
            EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j05_jconfig__upto02descrforchameleon02,
            EO_INIT(.index)     7+J05p,
            EO_INIT(.nchildren) 0,
            EO_INIT(.ichildren) {0},
            EO_INIT(.pchildren) {NULL}
        },   
    {   // 8
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j05__jstatus,
        EO_INIT(.index)     8+J05p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   // 9
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j05__calibrator,
        EO_INIT(.index)     9+J05p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   //10
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j05__setpoint,
        EO_INIT(.index)     10+J05p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    },
    {   //11
        EO_INIT(.data)      (void*)&eo_cfg_nvsEP_mc_leftleg_j05__controlmode,
        EO_INIT(.index)     11+J05p,
        EO_INIT(.nchildren) 0,
        EO_INIT(.ichildren) {0},
        EO_INIT(.pchildren) {NULL}
    }
    
    
        #warning and in here come the 6 motors
 
};





const EOconstvector  s_eo_cfg_nvsEP_mc_leftleg_constvector_of_treenodes_EOnv_con = 
{
    EO_INIT(.size)              sizeof(eo_cfg_nvsEP_mc_leftleg_tree_con)/sizeof(EOtreenode), //EOK_cfg_nvsEP_leftleg_numberof,
    EO_INIT(.item_size)         sizeof(EOtreenode),
    EO_INIT(.item_array_data)   eo_cfg_nvsEP_mc_leftleg_tree_con
};


extern const EOconstvector* const eo_cfg_nvsEP_mc_leftleg_constvector_of_treenodes_EOnv_con = &s_eo_cfg_nvsEP_mc_leftleg_constvector_of_treenodes_EOnv_con;


extern const eOuint16_fp_uint16_t eo_cfg_nvsEP_mc_leftleg_fptr_hashfunction_id2index = eo_cfg_nvsEP_mc_leftleg_hashfunction_id2index;


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------

extern uint16_t eo_cfg_nvsEP_mc_leftleg_hashfunction_id2index(uint16_t id)
{


    #define IDTABLESIZE     (EOK_cfg_nvsEP_mc_jvarnumTOTAL*6)
    // 12*6

    // in order to always have a hit the table s_idtable[] it must be of size equal to max{ s_hash(id) }, thus if we
    // use an id of value 16 and s_hash() just keeps the lsb, then the size must be 17 
    // if there are holes, they shall have EOK_uint16dummy in other entries. for example, if we have ids = {0, 7, 16}
    // then the table shall be of size 17, shall contain 0xffff everywhere but in positions 0, 7, 16 where the values
    // are ... 0, 7, 16

    static const uint16_t s_idtablejoints[IDTABLESIZE] = 
    { 
        // j00
        EOK_cfg_nvsEP_mc_jxx_NVID_jconfig(0), EOK_cfg_nvsEP_mc_NVID_jconfig__pidposition(0), EOK_cfg_nvsEP_mc_NVID_jconfig__pidvelocity(0),
        EOK_cfg_nvsEP_mc_NVID_jconfig__pidtorque(0), EOK_cfg_nvsEP_mc_NVID_jconfig__minpositionofjoint(0), EOK_cfg_nvsEP_mc_NVID_jconfig__maxpositionofjoint(0), 
        EOK_cfg_nvsEP_mc_NVID_jconfig__velocitysetpointtimeout(0), EOK_cfg_nvsEP_mc_NVID_jconfig__upto02descrforchameleon02(0), EOK_cfg_nvsEP_mc_NVID__jstatus(0),
        EOK_cfg_nvsEP_mc_NVID__calibrator(0), EOK_cfg_nvsEP_mc_NVID__setpoint(0), EOK_cfg_nvsEP_mc_NVID__controlmode(0),
        // j01
        EOK_cfg_nvsEP_mc_jxx_NVID_jconfig(1), EOK_cfg_nvsEP_mc_NVID_jconfig__pidposition(1), EOK_cfg_nvsEP_mc_NVID_jconfig__pidvelocity(1),
        EOK_cfg_nvsEP_mc_NVID_jconfig__pidtorque(1), EOK_cfg_nvsEP_mc_NVID_jconfig__minpositionofjoint(1), EOK_cfg_nvsEP_mc_NVID_jconfig__maxpositionofjoint(1), 
        EOK_cfg_nvsEP_mc_NVID_jconfig__velocitysetpointtimeout(1), EOK_cfg_nvsEP_mc_NVID_jconfig__upto02descrforchameleon02(1), EOK_cfg_nvsEP_mc_NVID__jstatus(1),
        EOK_cfg_nvsEP_mc_NVID__calibrator(1), EOK_cfg_nvsEP_mc_NVID__setpoint(1), EOK_cfg_nvsEP_mc_NVID__controlmode(1),
        // j02
        EOK_cfg_nvsEP_mc_jxx_NVID_jconfig(2), EOK_cfg_nvsEP_mc_NVID_jconfig__pidposition(2), EOK_cfg_nvsEP_mc_NVID_jconfig__pidvelocity(2),
        EOK_cfg_nvsEP_mc_NVID_jconfig__pidtorque(2), EOK_cfg_nvsEP_mc_NVID_jconfig__minpositionofjoint(2), EOK_cfg_nvsEP_mc_NVID_jconfig__maxpositionofjoint(2), 
        EOK_cfg_nvsEP_mc_NVID_jconfig__velocitysetpointtimeout(2), EOK_cfg_nvsEP_mc_NVID_jconfig__upto02descrforchameleon02(2), EOK_cfg_nvsEP_mc_NVID__jstatus(2),
        EOK_cfg_nvsEP_mc_NVID__calibrator(2), EOK_cfg_nvsEP_mc_NVID__setpoint(2), EOK_cfg_nvsEP_mc_NVID__controlmode(2),       
        // j03
        EOK_cfg_nvsEP_mc_jxx_NVID_jconfig(3), EOK_cfg_nvsEP_mc_NVID_jconfig__pidposition(3), EOK_cfg_nvsEP_mc_NVID_jconfig__pidvelocity(3),
        EOK_cfg_nvsEP_mc_NVID_jconfig__pidtorque(3), EOK_cfg_nvsEP_mc_NVID_jconfig__minpositionofjoint(3), EOK_cfg_nvsEP_mc_NVID_jconfig__maxpositionofjoint(3), 
        EOK_cfg_nvsEP_mc_NVID_jconfig__velocitysetpointtimeout(3), EOK_cfg_nvsEP_mc_NVID_jconfig__upto02descrforchameleon02(3), EOK_cfg_nvsEP_mc_NVID__jstatus(3),
        EOK_cfg_nvsEP_mc_NVID__calibrator(3), EOK_cfg_nvsEP_mc_NVID__setpoint(3), EOK_cfg_nvsEP_mc_NVID__controlmode(3),       
        // j04
        EOK_cfg_nvsEP_mc_jxx_NVID_jconfig(4), EOK_cfg_nvsEP_mc_NVID_jconfig__pidposition(4), EOK_cfg_nvsEP_mc_NVID_jconfig__pidvelocity(4),
        EOK_cfg_nvsEP_mc_NVID_jconfig__pidtorque(4), EOK_cfg_nvsEP_mc_NVID_jconfig__minpositionofjoint(4), EOK_cfg_nvsEP_mc_NVID_jconfig__maxpositionofjoint(4), 
        EOK_cfg_nvsEP_mc_NVID_jconfig__velocitysetpointtimeout(4), EOK_cfg_nvsEP_mc_NVID_jconfig__upto02descrforchameleon02(4), EOK_cfg_nvsEP_mc_NVID__jstatus(4),
        EOK_cfg_nvsEP_mc_NVID__calibrator(4), EOK_cfg_nvsEP_mc_NVID__setpoint(4), EOK_cfg_nvsEP_mc_NVID__controlmode(4),       
        // j05
        EOK_cfg_nvsEP_mc_jxx_NVID_jconfig(5), EOK_cfg_nvsEP_mc_NVID_jconfig__pidposition(5), EOK_cfg_nvsEP_mc_NVID_jconfig__pidvelocity(5),
        EOK_cfg_nvsEP_mc_NVID_jconfig__pidtorque(5), EOK_cfg_nvsEP_mc_NVID_jconfig__minpositionofjoint(5), EOK_cfg_nvsEP_mc_NVID_jconfig__maxpositionofjoint(5), 
        EOK_cfg_nvsEP_mc_NVID_jconfig__velocitysetpointtimeout(5), EOK_cfg_nvsEP_mc_NVID_jconfig__upto02descrforchameleon02(5), EOK_cfg_nvsEP_mc_NVID__jstatus(5),
        EOK_cfg_nvsEP_mc_NVID__calibrator(5), EOK_cfg_nvsEP_mc_NVID__setpoint(5), EOK_cfg_nvsEP_mc_NVID__controlmode(5)        
    };
    
    uint16_t index = s_hash(id);
    
    #warning --> so far the hashing is ok only for teh joints. for the motor it is a different story 
    
    if((index < IDTABLESIZE) && (id == s_idtablejoints[index]) )
    {
        return(index);
    }
    else
    {
        return(EOK_uint16dummy);
    }

      
}



// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------
// empty-section

// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------


// but EOK_cfg_nvsEP_mc_con_joint_maxnumof_id must be 32

typedef uint8_t sdfg[ ( EOK_cfg_nvsEP_mc_con_joint_maxnumof_id == 32 ) ? (1) : (0)];
typedef uint8_t redf[ ( EOK_cfg_nvsEP_mc_con_motor_maxnumof_id == 16 ) ? (1) : (0)];

static uint16_t s_hash(uint16_t id)
{
    uint16_t off = EO_nv_OFF(id);
    uint16_t a;
    uint16_t b;
    uint16_t r;
    
    if(off < EOK_cfg_nvsEP_mc_con_bodypart_nvidstart_in_motor(0))
    {
        a = off >> 5;
        b = off - (a << 5);
        r = a*EOK_cfg_nvsEP_mc_jvarnumTOTAL+b;
    }
    else
    {
        off -= EOK_cfg_nvsEP_mc_con_bodypart_nvidstart_in_motor(0);
        a = off >> 4;
        b = off - (a << 4);
        r = a*EOK_cfg_nvsEP_mc_mvarnumTOTAL+b;
    }
    
    return(r);
}


// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------



