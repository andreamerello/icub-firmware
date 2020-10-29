/*******************************************************************************************************************//**
 * @file    piezo.h
 * @author  G.Zini
 * @version 1.0
 * @date    2020 October, 16
 * @brief   Piezo motors management
 **********************************************************************************************************************/

/* Define to prevent recursive inclusion ******************************************************************************/
#ifndef __PIEZO_H
#define __PIEZO_H

#ifdef __cplusplus
  extern "C" {
#endif


/* Include files ******************************************************************************************************/
#include "hal.h"


/* Exported typedefs **************************************************************************************************/

/* Motor ordinals */
typedef enum
{
    PIEZO_MOTOR1 = 0,
    PIEZO_MOTOR2,
    PIEZO_MOTOR3
} piezoMotor_t ;


/* Exported macros ****************************************************************************************************/

/* Exported functions *************************************************************************************************/

extern void piezoInit(void);
extern void piezoHighVoltage(FunctionalState enable);
extern HAL_StatusTypeDef piezoSetStepFrequency(piezoMotor_t motor, int32_t freq);
extern HAL_StatusTypeDef piezoGetStepFrequency(piezoMotor_t motor, int32_t *pFreq);
extern HAL_StatusTypeDef piezoSetStepAngle(piezoMotor_t motor, uint32_t angle);
extern HAL_StatusTypeDef piezoGetStepAngle(piezoMotor_t motor, int32_t *pAngle);
extern HAL_StatusTypeDef piezoSetBrake(piezoMotor_t motor, FunctionalState enable);
extern HAL_StatusTypeDef piezoSetFreewheeling(piezoMotor_t motor, FunctionalState enable);


#ifdef __cplusplus
  } /* extern "C" */
#endif

#endif /* __PIEZO_H */
/* END OF FILE ********************************************************************************************************/
