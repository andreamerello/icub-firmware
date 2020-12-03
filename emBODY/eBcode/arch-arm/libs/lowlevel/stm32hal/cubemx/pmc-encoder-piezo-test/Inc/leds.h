/*******************************************************************************************************************//**
 * @file    leds.c
 * @author  G.Zini
 * @version 1.0
 * @date    2020 October, 15
 * @brief   LEDs management functions
 **********************************************************************************************************************/

/* Define to prevent recursive inclusion ******************************************************************************/
#ifndef __LEDS_H
#define __LEDS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Include files ******************************************************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "main.h"


/* Exported macros ****************************************************************************************************/
#define LED_ON(port, pin)           WRITE_REG(port->BSRR, pin<<16);
#define LED_OFF(port, pin)          WRITE_REG(port->BSRR, pin);
#define LED_TOGGLE(port, pin)       WRITE_REG(port->BSRR, (pin<<16)|(~READ_REG(port->ODR)&pin))
#define LED_CODE(port, mask, code)  WRITE_REG(port->BSRR, ((code&mask)<<16)|(~code&mask))

/* Pin assignements */
#define LED_RED0        (LED1_Pin)
#define LED_RED1        (LED2_Pin)
#define LED_RED2        (LED3_Pin)
#define LED_GREEN0      (LED4_Pin)
#define LED_GREEN1      (LED5_Pin)
#define LED_ORANGE0     (LED6_Pin)
#define LED_ORANGE1     (LED7_Pin)

/* Port assignements */
#define LED_REDPORT     (LED1_GPIO_Port)
#define LED_GREENPORT   (LED4_GPIO_Port)
#define LED_ORANGEPORT  (LED6_GPIO_Port)

/* Code and masks assignements */
#define LED_REDCODE0    (0)
#define LED_REDCODE1    (LED_RED0)
#define LED_REDCODE2    (LED_RED1)
#define LED_REDCODE3    (LED_RED1|LED_RED0)
#define LED_REDCODE4    (LED_RED2)
#define LED_REDCODE5    (LED_RED2|LED_RED0)
#define LED_REDCODE6    (LED_RED2|LED_RED1)
#define LED_REDCODE7    (LED_RED2|LED_RED1|LED_RED0)
#define LED_REDMASK     (LED_RED2|LED_RED1|LED_RED0)

#define LED_GREENCODE0  (0)
#define LED_GREENCODE1  (LED_GREEN0)
#define LED_GREENCODE2  (LED_GREEN1)
#define LED_GREENCODE3  (LED_GREEN1|LED_GREEN0)
#define LED_GREENMASK   (LED_GREEN1|LED_GREEN0)

#define LED_ORANGEMASK  (LED_ORANGE1|LED_ORANGE0)
   
#ifdef __cplusplus
  } /* extern "C" */
#endif

#endif /* __LEDS_H */
/* END OF FILE ********************************************************************************************************/
