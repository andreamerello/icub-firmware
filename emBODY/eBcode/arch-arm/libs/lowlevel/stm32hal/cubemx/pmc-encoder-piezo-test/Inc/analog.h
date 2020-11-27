/*******************************************************************************************************************//**
 * @file    analog.h
 * @author  G.Zini
 * @version 1.0
 * @date    2020 October, 21
 * @brief   A(D converter management
 **********************************************************************************************************************/

/* Define to prevent recursive inclusion ******************************************************************************/
#ifndef __ANALOG_H
#define __ANALOG_H

#ifdef __cplusplus
  extern "C" {
#endif


/* Include files ******************************************************************************************************/
#include "stm32g4xx.h"
#include "stm32g4xx_hal_def.h"


/* Exported typedefs **************************************************************************************************/

/* Exported macros ****************************************************************************************************/

/* Exported functions *************************************************************************************************/

extern HAL_StatusTypeDef analogInit( void );
extern uint32_t analogGetVref_mV(void);
extern uint32_t analogGetVin_mV(void);
extern uint32_t analogGetVpp_mV(void);
extern uint32_t analogGetVcc_mV(void);
extern uint32_t analogGetIin_mA(void);


#ifdef __cplusplus
  } /* extern "C" */
#endif

#endif /* __ANALOG_H */
/* END OF FILE ********************************************************************************************************/
