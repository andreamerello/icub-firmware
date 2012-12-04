/*
 * Copyright (C) 2012 iCub Facility - Istituto Italiano di Tecnologia
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

#ifndef _HAL_SENSOR_GYRO_H_
#define _HAL_SENSOR_GYRO_H_

// - doxy begin -------------------------------------------------------------------------------------------------------

/** @file       hal_sensor_gyro.h
    @brief      This header file implements interface to a generic hal sensor angular rate module (gyroscope)
    @author     marco.accame@iit.it
    @date       10/24/2012
**/

/** @defgroup arm_hal_sensor_gyro HAL SNSR_GYRO

    The HAL SNSR_TEMP is a high level module whcih manages a angular rate sensor
 
    @todo acemor-facenda: review documentation.
    
    @{        
 **/


// - external dependencies --------------------------------------------------------------------------------------------

#include "hal_base.h"



// - public #define  --------------------------------------------------------------------------------------------------
// empty-section
  

// - declaration of public user-defined types ------------------------------------------------------------------------- 


/** @typedef    typedef enum hal_sensor_gyro_t 
    @brief      hal_sensor_gyro_t contains the possible sensors
 **/
typedef enum
{
    hal_sensor_gyro1 = 0
} hal_sensor_gyro_t;

enum { hal_sensor_gyros_number = 1 };


/** @typedef    typedef int8_t hal_sensor_gyro_angular_rate_t 
    @brief      expresses the angular rate in milli degrees per second.
 **/
typedef struct
{
    int32_t     xar;
    int32_t     yar;
    int32_t     zar;
} hal_sensor_gyro_angular_rate_t;


/** @typedef    typedef enum hal_sensor_gyro_cfg_t 
    @brief      hal_sensor_gyro_cfg_t contains the configuration for the sensor.
 **/
typedef struct
{
    uint8_t         dummy;
} hal_sensor_gyro_cfg_t;

 
// - declaration of extern public variables, ... but better using use _get/_set instead -------------------------------

extern const hal_sensor_gyro_cfg_t hal_sensor_gyro_cfg_default; // = { .dummy = 0 };


// - declaration of extern public functions ---------------------------------------------------------------------------


/** @fn			extern hal_result_t hal_sensor_gyro_init(hal_sensor_gyro_t sensor, const hal_sensor_gyro_cfg_t *cfg)
    @brief  	this function initializes a given gyro sensor 
    @param  	sensor	        the sensor
    @param  	cfg 	        pointer to configuration data
    @return 	hal_res_NOK_generic on error else hal_res_OK
  */
extern hal_result_t hal_sensor_gyro_init(hal_sensor_gyro_t sensor, const hal_sensor_gyro_cfg_t *cfg);


/** @fn			extern hal_result_t hal_sensor_gyro_read(hal_sensor_gyro_t sensor, hal_sensor_gyro_degree_t* degrees)
    @brief  	this function reads temperatures on a given sensor.
    @param  	sensor	        the sensor
    @param  	degrees 	    the read degrees
    @return 	hal_res_NOK_generic on error else hal_res_OK
  */
extern hal_result_t hal_sensor_gyro_read(hal_sensor_gyro_t sensor, hal_sensor_gyro_angular_rate_t* angrate);


/** @}            
    end of group arm_hal_sensor_gyro  
 **/

#endif  // include-guard


// - end-of-file (leave a blank line after)----------------------------------------------------------------------------



