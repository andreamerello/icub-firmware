/*******************************************************************************************************************//**
 * @file    analog.c
 * @author  G.Zini
 * @version 1.0
 * @date    2020 October, 21
 * @brief   A(D converter management
 **********************************************************************************************************************/

/* Includes ***********************************************************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include "adc.h"
#include "analog.h"
//#include "console.h"
#include "main.h"
//#include "leds.h"
#include "tim.h"
#include "utilities.h"
#include "FreeRTOS.h"
#include "stm32g4xx_hal_adc.h"
#include "stm32g4xx_hal_dma.h"
#include "stm32g4xx_hal_tim.h"

/**********************************************************************************************************************
 * The A/D converter ADC1 measures the following channels:
 * 
 * ADC1_IN2     VPP_TEST (PA1)  Measures the VPP voltage through the attenuator R174/(R173+R174)
 * ADC1_IN4     VIN_TEST (PA3)  Measures the VIN voltage through the attenuator R176/(R175+R176)
 * ADC1_IN5     CIN_TEST (PB14) Measures the input current with the shunt R177, the amplifier INA293A1 (20V/V) and
 *                              the attenuator R179/(R178+R179)
 * ADC1_IN13    Temperature     (internal) Measures the temperature of the silicon
 * ADC1_IN17    VBAT            (internal) Measures VCC through a 1/3 attenuator
 * ADC1_IN18    VREFINT         (internal) Measures the internal voltage reference 1.212V 
 *
 * ADC1 is triggered by timer TIM7, Trigger Out event. The timer has been configured for a slow trigger rate (100Hz).
 * For each trigger ADC1 scans and convert all six channels in sequence. The DMA2 Channel 4 transfers each converted
 * sample to memory. At the end of the last converted data the  DMA2 Channel 4 Transfer Complete interrupt is rised
 * and the analog_DMA_ISR_TC() callback function is called by the interrupt handler.
 *
 *  ADC Parameter Settings:
 *  Clock Prescaler                         Synchronous clock mode divided by 4
 *  Resolution ADC                          12-bit resolution
 *  Data Alignment                          Right alignment
 *  Gain Compensation                       0
 *  Scan Conversion Mode                    Enabled
 *  End Of Conversion Selection             End of single conversion
 *  Low Power Auto Wait                     Disabled
 *  Continuous Conversion Mode              Disabled
 *  Discontinuous Conversion Mode           Disabled
 *  Number Of Discontinuous Conversions     0
 *  DMA Continuous Requests                 Disabled
 *  Overrun behaviour                       Overrun data preserved
 *  Enable Regular Conversions              Enable
 *  Enable Regular Oversampling             Disable
 *  Number Of Conversion                    12
 *  External Trigger Conversion Source      Timer 7 Trigger Out event
 *  External Trigger Conversion Edge        Trigger detection on the rising edge
 *  Rank 1 & 2
 *      Channel                             Channel 2
 *      Sampling Time                       12.5 Cycles
 *      Offset Number                       No offset
 *  Rank 3 & 4
 *      Channel                             Channel 4
 *      Sampling Time                       12.5 Cycles
 *      Offset Number                       No offset
 *  Rank 5 & 6
 *      Channel                             Channel 5
 *      Sampling Time                       12.5 Cycles
 *      Offset Number                       No offset
 *  Rank 7 & 8
 *      Channel                             Channel Temperature Sensor
 *      Sampling Time                       12.5 Cycles
 *      Offset Number                       No offset
 *  Rank 9 & 10
 *      Channel                             Channel Vbat
 *      Sampling Time                       12.5 Cycles
 *      Offset Number                       No offset
 *  Rank 11 & 12
 *      Channel                             Channel Vrefint
 *      Sampling Time                       12.5 Cycles
 *      Offset Number                       No offset
 *  Enable Injected Conversions             Disable
 *  Enable Analog WatchDog1 Mode            false 
 *  Enable Analog WatchDog2 Mode            false 
 *  Enable Analog WatchDog3 Mode            false 
 *
 * TIM7 Parameter Settings:
 *  Prescaler (PSC - 16 bits value)         83  (division 1:84)
 *  Counter Mode                            Up
 *  Dithering                               Disable
 *  Counter Period (ARR - 16bit)            9999 (division 1:10000)
 *  auto-reload preload                     Disable
 *  Trigger Event Selection                 Update Event
 *
 * CAUTION: The microcontroller revisions Z and the previous ones are affected by the device-errata described in
 *          document "ES0430 - Rev 4 - July 2020". Since current version Y is not affected by the problem, the
 *          suggested workaround has been implemented in the following code.
 *          Device revision code is given by register DBGMCU->IDCODE (see macro __DEVICE_REVISION__): code of version
 *          Y is 0x2002. Previous versions have lower code values
 */

#if (USE_HAL_ADC_REGISTER_CALLBACKS != 1)
    #error Flag ADC1 in menu "Project Manager -> Advanced Settings -> Register CallBack" of CubeMx must be ENABLED
#endif
    

/* Private macros *****************************************************************************************************/

/* Number of analog channels, doubled as indicated by the workaround */
#define NUMBER_OF_ANALOG_CHANNELS   (2u*6u)

/* Analog channels index */
#define VPP_INDEX   (0u)
#define VIN_INDEX   (2u)
#define CIN_INDEX   (4u)
#define TEMP_INDEX  (6u)
#define VBAT_INDEX  (8u)
#define VREF_INDEX  (10u)

/* Hardware component definitions */
#define kOhm        (1000.0)
#define Ohm         (1.000)
#define mOhm        (0.001)
#define Volt        (1.000)
#define mVolt       (0.001)
#define R173        (390.000 * kOhm)
#define R174        ( 20.000 * kOhm)
#define R175        (390.000 * kOhm)
#define R176        ( 20.000 * kOhm)
#define R177        ( 50.000 * mOhm)
#define R178        (  4.990 * kOhm)
#define R179        ( 20.000 * kOhm)
#define AMP_GAIN    (20.000 * Volt/Volt)

/* Attenuators */
#define VBAT_ATTN   (3.000 * Volt/Volt)
#define VIN_ATTN    (1.0 + R175/R176)
#define VPP_ATTN    (1.0 + R173/R174)
#define CIN_ATTN    ((1.0 + R178/R179)/(AMP_GAIN * R177))

#define VREF_CAL    (3.000 * Volt)
#define VREF_NOM    (2.500 * Volt)
#define VREFINT_NOM (1.212 * Volt)
#define NREFINT_NOM ((uint16_t)((VREFINT_NOM*4096.0)/VREF_NOM + 0.5))
#define NREFINT_VAR ((uint16_t)((2 * mVolt * 4096.0)/VREF_NOM + 0.5))
#define NREFINT_MAX (NREFINT_NOM + NREFINT_VAR)
#define NREFINT_MIN (NREFINT_NOM - NREFINT_VAR)
#define GAIN_MAX    (4096u + 50u)
#define GAIN_MIN    (4096u - 50u)

#define VIN_mV      ((uint32_t)(65536.0*1000.0*(1.0 + R175/R176)*VREF_NOM/4096.0 + 0.5))
#define VPP_mV      ((uint32_t)(65536.0*1000.0*(1.0 + R173/R174)*VREF_NOM/4096.0 + 0.5))
#define VCC_mV      ((uint32_t)(65536.0*1000.0*3.0*VREF_NOM/4096.0 + 0.5))
#define VREF_mV     ((uint32_t)(65536.0*1000.0*VREF_NOM/4096.0 + 0.5))
#define CIN_mA      ((uint32_t)(65536.0*1000.0*(1.0 + R178/R179)*VREF_NOM/(4096.0*AMP_GAIN*R177) + 0.5))


/* Private variables **************************************************************************************************/

/* Buffer of the analog samples. It is filled by DMA */
static volatile uint16_t DmaSampleBuf[NUMBER_OF_ANALOG_CHANNELS] = {0};

/* Refint voltage setpoint */
static uint16_t NRefint_set;


/* Local functions ****************************************************************************************************/


/*******************************************************************************************************************//**
 * @brief   Callback function launched by the "Transfer Complete" interrupt of the DMA used by ADC1. The function
 *          tracks and adjust the GAIN parameter of ADC1, using the measurements of the REFINT voltage
 *
 * @param   hadc    Pointer to the ADC_HandleTypeDef structure associated to ADC1
 *
 * @return  void
 */
static void analog_DMA_ISR_TC(ADC_HandleTypeDef *hadc)
{
    UNUSED(hadc);
    /* The value is too high */
    if (DmaSampleBuf[VREF_INDEX] > (NRefint_set + NREFINT_VAR)) 
    {
        /* Decrease the ADC gain */
        if (ADC1->GCOMP > GAIN_MIN) ADC1->GCOMP = 0x3FFF & (ADC1->GCOMP - 1);
    }
    /* The value is too low */
    else if (DmaSampleBuf[VREF_INDEX] < (NRefint_set - NREFINT_VAR)) 
    {
        /* Increase the ADC gain */
        if (ADC1->GCOMP < GAIN_MAX) ADC1->GCOMP = 0x3FFF & (ADC1->GCOMP + 1);
    }
}


/* Exported functions *************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief   Start conversions of ADC1, triggered by TIM7, in DMA mode. At the end the functions the conversions are
 * running in background and the measured samples are stored in memory by DMA.
 *
 * @param   void
 *
 * @return  HAL_StatusTypeDef   Operation result
 */
HAL_StatusTypeDef analogInit( void )
{
    HAL_StatusTypeDef result = HAL_ERROR;

    /* Stop any operation */
    HAL_TIM_Base_Stop(&htim7);
    HAL_ADC_Stop_DMA(&hadc1);

    /* Calibrate the ADC1 */
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    
    /* Calculate the setpoint for the Vrefint measurement */
    NRefint_set = (*VREFINT_CAL_ADDR*(uint32_t)(65536.0*VREF_CAL/VREF_NOM) + 32768u) >> 16u;

    /* Clear the input buffer */
    memset((void *)DmaSampleBuf, 0, sizeof(DmaSampleBuf));
    
    /* Register the interrupt callback function for the DMA Transfer Complete interrupt*/
    if (HAL_OK == HAL_ADC_RegisterCallback(&hadc1, HAL_ADC_CONVERSION_COMPLETE_CB_ID, analog_DMA_ISR_TC))
    {
        /* Enable the ADC in DMA mode */
        if (HAL_OK == HAL_ADC_Start_DMA(&hadc1, (void *)DmaSampleBuf, sizeof(DmaSampleBuf)/sizeof(uint16_t)))
        {
            /* Enable the interrupts */
            __HAL_DMA_ENABLE_IT(hadc1.DMA_Handle, DMA_IT_TC);
            /* Start the timer that triggers the conversions */
            result = HAL_TIM_Base_Start(&htim7);
        }
    }
    return result;
}


/*******************************************************************************************************************//**
 * @brief   Reads the VREFINT voltage in mV
 *
 * @param   void
 *
 * @return  VREFINT voltage in mV
 */
uint32_t analogGetVref_mV(void)
{
    return (VREF_mV * DmaSampleBuf[VREF_INDEX]) >> 16u;
}


/*******************************************************************************************************************//**
 * @brief   Reads the VIN voltage in mV
 *
 * @param   void
 *
 * @return  VIN voltage in mV
 */
uint32_t analogGetVin_mV(void)
{
    return (VIN_mV * DmaSampleBuf[VIN_INDEX]) >> 16u;
}


/*******************************************************************************************************************//**
 * @brief   Reads the VPP voltage in mV
 *
 * @param   void
 *
 * @return  VPP voltage in mV
 */
uint32_t analogGetVpp_mV(void)
{
    return (VPP_mV * DmaSampleBuf[VPP_INDEX]) >> 16u;
}


/*******************************************************************************************************************//**
 * @brief   Reads the VCC voltage in mV
 *
 * @param   void
 *
 * @return  VCC voltage in mV
 */
uint32_t analogGetVcc_mV(void)
{
    return (VCC_mV * DmaSampleBuf[VBAT_INDEX]) >> 16u;
}


/*******************************************************************************************************************//**
 * @brief   Reads the Iin current in mA
 *
 * @param   void
 *
 * @return  Iin current in mA
 */
uint32_t analogGetIin_mA(void)
{
    return (CIN_mA * DmaSampleBuf[CIN_INDEX]) >> 16u;
}


/* END OF FILE ********************************************************************************************************/
