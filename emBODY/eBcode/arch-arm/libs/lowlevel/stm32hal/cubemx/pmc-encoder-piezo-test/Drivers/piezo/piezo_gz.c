/*******************************************************************************************************************//**
 * @file    piezo.c
 * @author  G.Zini
 * @version 1.0
 * @date    2020 October, 16
 * @brief   Piezo motors management
 **********************************************************************************************************************/

/* Includes ***********************************************************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include "gpl_utils.h"
#include "piezo_gz.h"

#if (USE_HAL_COMP_REGISTER_CALLBACKS != 1)
    #error Flag COMP in menu "Project Manager -> Advanced Settings -> Register CallBack" in CubeMx must be ENABLED
#endif

#if (USE_HAL_SPI_REGISTER_CALLBACKS != 1)
    #error Flag SPI in menu "Project Manager -> Advanced Settings -> Register CallBack" in CubeMx must be ENABLED
#endif

/* Private macros *****************************************************************************************************/

/* Values for functions piezoSetBrake() and piezoSetFreewheeling() */
#define PIEZO_NORMAL        (0x00)
#define PIEZO_BRAKE         (0x01)
#define PIEZO_FREEWHEELING  (0x02)


/* QUAD-DAC amplifiers gain */
#define PIEZO_QDACGAIN      (19.0)

/* QUAD-DAC reference voltage */
#define PIEZO_QDACVREF      (2.500/*Volts*/)

/* Piezo motors phases voltage range */
#define PIEZO_MINVOLTAGE    (2.0/*Volts*/)
#define PIEZO_MAXVOLTAGE    (44.0/*Volts*/)

/* QUAD-DAC value range */
#define PIEZO_MINVALUE      ((uint32_t)(PIEZO_MINVOLTAGE*(65536.0/(PIEZO_QDACVREF*PIEZO_QDACGAIN))))
#define PIEZO_MAXVALUE      ((uint32_t)(PIEZO_MAXVOLTAGE*(65536.0/(PIEZO_QDACVREF*PIEZO_QDACGAIN))))

/* Motor max step freqency */
#define PIEZO_MAXFREQ       3000

/* Macros to control reentrancy */
#define __PIEZO_ENTER_CRITICAL()    vPortEnterCritical()
#define __PIEZO_EXIT_CRITICAL()     vPortExitCritical()
//#define __PIEZO_ENTER_CRITICAL()    __HAL_DMA_DISABLE_IT(hspi1.hdmatx, DMA_IT_TC|DMA_IT_HT)
//#define __PIEZO_EXIT_CRITICAL()     __HAL_DMA_ENABLE_IT(hspi1.hdmatx, DMA_IT_TC|DMA_IT_HT)

/* Number of microsteps per step */
#define MICROSTEPS_NUMBER           (8192)

/* Must be 32 - (n.bits of MICROSTEPS_NUMBER) */
#define SHIFT_FACTOR                (19U)
#define SHIFT_MASK                  (0xFFFFFFFFU >> SHIFT_FACTOR)

/* Half round-angle */
#define HALF_ROUND                  (0x80000000U)

/* Number of quad-samples in DMA buffers */
#define QUADSAMPLES_BUFFER_LENGHT   (256)

/* Starting index of the lower and upper halves of the DMA buffer */
#define LOWER_HALF_INDEX            (0)
#define UPPER_HALF_INDEX            (QUADSAMPLES_BUFFER_LENGHT/2)

/* Macro to set the value to be sent to a DAC */
#define SETDACVALUE(cmd,dac,val) ((((uint32_t)(cmd)<<11) & 0x00003800UL) \
				  |(((uint32_t)(dac)<<8)  & 0x00000700UL) \
				  |(((uint32_t)(val)>>8)  & 0x000000FFUL) \
				  |(((uint32_t)(val)<<24) & 0xFF000000UL))


/* Private typedef ****************************************************************************************************/

typedef struct __attribute__((packed))
{
    uint32_t dacA;
    uint32_t dacB;
    uint32_t dacC;
    uint32_t dacD;
} QuadSample_t;

typedef struct
{
    volatile int32_t    phaseAngle;
    volatile int32_t    phaseDelta;
    volatile uint32_t   phaseCntrl;
    QuadSample_t dmaBuffer[QUADSAMPLES_BUFFER_LENGHT];
} PiezoMotorStatus_t ;


/* Private variables **************************************************************************************************/

/* Update frequency of a single QUAD-DAC */
static int32_t piezoFreqConst;

/* Voltage descriptor for a single step of piezo motors */
static const uint16_t phaseTable[MICROSTEPS_NUMBER + 1] =
{
    /* Include CSV file (edit and substitute "semicolon" with "comma") */
    #include "PhaseTable.csv"
};

/* Piezo motors status descriptors */
static PiezoMotorStatus_t piezoMotor1 ;
static PiezoMotorStatus_t piezoMotor2 ;
static PiezoMotorStatus_t piezoMotor3 ;

/* Piezo Motors number conversion */
static PiezoMotorStatus_t * const pStatusTable[] = {&piezoMotor1, &piezoMotor2, &piezoMotor3};


/* Local functions ****************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief   Load half buffer of the specified Quad-DAC with consecutive samples taken from phaseTable[]. The function
 *          scans phaseTable with the speed phaseDelta given for the motor, and updates phaseAngle at the end.
 *          This function must be called by the callback functions of the DMA linked to SPI1 peripheral.
 * @param   *pStatus    Pointer to the PiezoMotorStatus_t structure associated to the motor to be updated
 *          index       Index of the half buffer to be updated. It must be one of the followin values:
 *                      LOWER_HALF_INDEX    Select the first half buffer
 *                      UPPER_HALF_INDEX    Select the second half buffer
 * @return  void
 */

static void piezoLoadBuffer(PiezoMotorStatus_t *pStatus, unsigned index)
{
    /* Number of samples to process in the buffer */
    unsigned n = QUADSAMPLES_BUFFER_LENGHT/2;

    /* The phaseAngle, phaseDelta and phaseControl are volatile, keep local */
    uint32_t angle = (uint32_t)(pStatus->phaseAngle);
    uint32_t delta = (uint32_t)(pStatus->phaseDelta);
    uint32_t cntrl = (uint32_t)(pStatus->phaseCntrl);

    /* Set the pointer to the selected buffer */
    QuadSample_t *pQSmp = &(pStatus->dmaBuffer[index]);


    /* Is the BRAKE active? */
    if (cntrl & PIEZO_BRAKE)
    {
        /* Fill the half-buffer with the value for the minimum voltage */
        for ( ; n-- ; pQSmp++)
        {
            /* Load value for PHASE #1 DAC */
            pQSmp->dacA = SETDACVALUE(0x00, 0, PIEZO_MINVALUE);
            /* Load value for PHASE #2 DAC */
            pQSmp->dacB = SETDACVALUE(0x00, 1, PIEZO_MINVALUE);
            /* Load value for PHASE #3 DAC */
            pQSmp->dacC = SETDACVALUE(0x00, 2, PIEZO_MINVALUE);
            /* Load value for PHASE #4 DAC */
            pQSmp->dacD = SETDACVALUE(0x02, 3, PIEZO_MINVALUE);
        }
    }
    /* Is the motor freewheeling? */
    else if (cntrl & PIEZO_FREEWHEELING)
    {
        /* Fill the half-buffer with the value for the maximum voltage */
        for ( ; n-- ; pQSmp++)
        {
            /* Load value for PHASE #1 DAC */
            pQSmp->dacA = SETDACVALUE(0x00, 0, PIEZO_MAXVALUE);
            /* Load value for PHASE #2 DAC */
            pQSmp->dacB = SETDACVALUE(0x00, 1, PIEZO_MAXVALUE);
            /* Load value for PHASE #3 DAC */
            pQSmp->dacC = SETDACVALUE(0x00, 2, PIEZO_MAXVALUE);
            /* Load value for PHASE #4 DAC */
            pQSmp->dacD = SETDACVALUE(0x02, 3, PIEZO_MAXVALUE);
        }
    }
    /* Normal mode */
    else
    {
        /* Fill the half-buffer with values taken from phaseTable[] */
        for ( ; n-- ; pQSmp++)
        {
            /* Temporary variables */
            uint32_t valA, valB, valC, valD;

            /* Phase value for normal wave */
            valA = phaseTable[(angle >> SHIFT_FACTOR) & SHIFT_MASK] ;
            /* Phase value for time-reversed wave */
            valB = phaseTable[(~angle >> SHIFT_FACTOR) & SHIFT_MASK] ;
            /* Phase value for 180° delayed wave */
            valC = phaseTable[((angle + HALF_ROUND) >> SHIFT_FACTOR) & SHIFT_MASK] ;
            /* Phase value for time-reversed and 180° delayed wave */
            valD = phaseTable[((~angle + HALF_ROUND) >> SHIFT_FACTOR) & SHIFT_MASK] ;

            /* Load value for PHASE #1 DAC */
            pQSmp->dacA = SETDACVALUE(0x00, 0, valA);
            /* Load value for PHASE #2 DAC */
            pQSmp->dacB = SETDACVALUE(0x00, 1, valB);
            /* Load value for PHASE #3 DAC */
            pQSmp->dacC = SETDACVALUE(0x00, 2, valC);
            /* Load value for PHASE #4 DAC */
            pQSmp->dacD = SETDACVALUE(0x02, 3, valD);


            /* Increment step-angle */
            angle += delta ;
        }
        /* Updates phaseAngle only when phaseDelta is not null */
        if (delta != 0) pStatus->phaseAngle = angle;
    }
}


/*******************************************************************************************************************//**
 * @brief   Interrupt callback function. This function is called by the DMA interrupt handler following the completion
 *          of  transmission of the first half buffer to the SPI channel connected to external DAC of motor 1.
 *          The callback function writes new values on the first half of the buffers of all the three motors
 *          This function must be registered with the HAL function HAL_SPI_RegisterCallback()
 * @param   *hspi   Unused
 * @return  void
 */
static void piezoDMA_ISR_HT(SPI_HandleTypeDef *hspi)
{
    UNUSED(hspi);
    //LED_ON(LED_GREENPORT, LED_GREEN0);  // DEBUG ONLY
    /* Update the first half of the buffer for the QUAD_DAC of motor #1 */
    piezoLoadBuffer(&piezoMotor1, LOWER_HALF_INDEX);
    /* Update the first half of the buffer for the QUAD_DAC of motor #2 */
    piezoLoadBuffer(&piezoMotor2, LOWER_HALF_INDEX);
    /* Update the first half of the buffer for the QUAD_DAC of motor #3 */
    piezoLoadBuffer(&piezoMotor3, LOWER_HALF_INDEX);
    //LED_OFF(LED_GREENPORT, LED_GREEN1);  // DEBUG ONLY
}


/*******************************************************************************************************************//**
 * @brief   Interrupt callback function. This function is called by the DMA interrupt handler following the completion
 *          of  transmission of the second half buffer to the SPI channel connected to external DAC of motor 1.
 *          The callback function writes new values on the second half of the buffers of all the three motors
 *          This function must be registered with the HAL function HAL_SPI_RegisterCallback()
 * @param   *hspi   Unused
 * @return  void
 */
static void piezoDMA_ISR_TC(SPI_HandleTypeDef *hspi)
{
    UNUSED(hspi);
    //LED_ON(LED_GREENPORT, LED_GREEN1);  // DEBUG ONLY
    /* Update the second half of the buffer for the QUAD_DAC of motor #1 */
    piezoLoadBuffer(&piezoMotor1, UPPER_HALF_INDEX);
    /* Update the second half of the buffer for the QUAD_DAC of motor #2 */
    piezoLoadBuffer(&piezoMotor2, UPPER_HALF_INDEX);
    /* Update the second half of the buffer for the QUAD_DAC of motor #3 */
    piezoLoadBuffer(&piezoMotor3, UPPER_HALF_INDEX);
    //LED_OFF(LED_GREENPORT, LED_GREEN0);  // DEBUG ONLY
}


/*******************************************************************************************************************//**
 * @brief   Callback function. This function is lauched by the COMP1 peripheral when it detects a current above 1A
 *          over the ground-return wire of MOTOR 1
 * @param   hcomp   Pointer to the COMP handler structure
 * @return  void
 */
static void piezoCOMP_ISR_Trigger1(COMP_HandleTypeDef *hcomp)
{
    /* Stop the comparator */
    HAL_COMP_Stop(hcomp);
    /* Activate the motor BRAKE */
    piezoSetBrake(PIEZO_MOTOR1, ENABLE);
    /* Error code */
    //LED_CODE(LED_REDPORT, LED_REDMASK, 1);
}


/*******************************************************************************************************************//**
 * @brief   Callback function. This function is lauched by the COMP1 peripheral when it detects a current above 1A
 *          over the ground-return wire of MOTOR 1
 * @param   hcomp   Pointer to the COMP handler structure
 * @return  void
 */
static void piezoCOMP_ISR_Trigger2(COMP_HandleTypeDef *hcomp)
{
    /* Stop the comparator */
    HAL_COMP_Stop(hcomp);
    /* Activate the motor BRAKE */
    piezoSetBrake(PIEZO_MOTOR2, ENABLE);
    /* Error code */
    //LED_CODE(LED_REDPORT, LED_REDMASK, 2);
}


/*******************************************************************************************************************//**
 * @brief   Callback function. This function is lauched by the COMP1 peripheral when it detects a current above 1A
 *          over the ground-return wire of MOTOR 1
 * @param   hcomp   Pointer to the COMP handler structure
 * @return  void
 */
static void piezoCOMP_ISR_Trigger3(COMP_HandleTypeDef *hcomp)
{
    /* Stop the comparator */
    HAL_COMP_Stop(hcomp);
    /* Activate the motor BRAKE */
    piezoSetBrake(PIEZO_MOTOR3, ENABLE);
    /* Error code */
    //LED_CODE(LED_REDPORT, LED_REDMASK, 4);
}


/* Exported functions *************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief   Initialize the SPI and the DMA subsystems for transmission to the external QUAD-DACs that controls the
 *          signals that drives the three piezo Motors. The function assumes that the SPI and the DMA peripheral have
 *          been already initialized with the CubeMx functions. After the call to piezoInit() all motors are stopped
 *          at the first microstep, with null speed. Motors will start to run after the call to the function
 *          piezoSpeed()
 * @param   void
 * @return  void
 */
void piezoInit(void)
{
    /* Constant to compute the step angle:
     * 2            PCLK2 prescaler
     * 4            number of DAC words per sample
     * 36           number of clock to be transmitted per word
     * 4294967296   Full round-angle (2**32)
     * For PCK2 frequency 21 MHz the constant is 58902. For 20 MHz is 61847
     */
    piezoFreqConst = (2LL * 4LL * 36LL * 4294967296LL)/(int32_t)HAL_RCC_GetPCLK2Freq();

    /* Clear DAC sync circuit */
    HAL_GPIO_WritePin(DAC_SYNCEN_GPIO_Port, DAC_SYNCEN_Pin, GPIO_PIN_RESET);

    /* Clear all varibles for motor 1 */
    piezoMotor1.phaseAngle = 0;
    piezoMotor1.phaseDelta = 0;
    piezoMotor1.phaseCntrl = PIEZO_NORMAL;
    memset(piezoMotor1.dmaBuffer, 0, sizeof(piezoMotor1.dmaBuffer));

    /* Clear all varibles for motor 2 */
    piezoMotor2.phaseAngle = 0;
    piezoMotor2.phaseDelta = 0;
    piezoMotor2.phaseCntrl = PIEZO_NORMAL;
    memset(piezoMotor2.dmaBuffer, 0, sizeof(piezoMotor2.dmaBuffer));

    /* Clear all varibles for motor 3 */
    piezoMotor3.phaseAngle = 0;
    piezoMotor3.phaseDelta = 0;
    piezoMotor3.phaseCntrl = PIEZO_NORMAL;
    memset(piezoMotor3.dmaBuffer, 0, sizeof(piezoMotor3.dmaBuffer));

    /* Register the comparator callback functions */
    HAL_COMP_RegisterCallback(&hcomp1, HAL_COMP_TRIGGER_CB_ID, piezoCOMP_ISR_Trigger1);
    HAL_COMP_RegisterCallback(&hcomp2, HAL_COMP_TRIGGER_CB_ID, piezoCOMP_ISR_Trigger2);
    HAL_COMP_RegisterCallback(&hcomp3, HAL_COMP_TRIGGER_CB_ID, piezoCOMP_ISR_Trigger3);

    /* Start the comparators */
    HAL_COMP_Start(&hcomp1);
    HAL_COMP_Start(&hcomp2);
    HAL_COMP_Start(&hcomp3);

    /* Register CALLBACK functions for DMA SPI1 */
    HAL_SPI_RegisterCallback(&hspi1, HAL_SPI_TX_HALF_COMPLETE_CB_ID, piezoDMA_ISR_HT);
    HAL_SPI_RegisterCallback(&hspi1, HAL_SPI_TX_COMPLETE_CB_ID,      piezoDMA_ISR_TC);
    __HAL_DMA_ENABLE_IT(hspi1.hdmatx, DMA_IT_TC|DMA_IT_HT);

    /* Enable DAC sync circuit */
    HAL_GPIO_WritePin(DAC_SYNCEN_GPIO_Port, DAC_SYNCEN_Pin, GPIO_PIN_SET);

    piezoLoadBuffer(&piezoMotor1, LOWER_HALF_INDEX);
    piezoLoadBuffer(&piezoMotor2, LOWER_HALF_INDEX);
    piezoLoadBuffer(&piezoMotor3, LOWER_HALF_INDEX);
    piezoLoadBuffer(&piezoMotor1, UPPER_HALF_INDEX);
    piezoLoadBuffer(&piezoMotor2, UPPER_HALF_INDEX);
    piezoLoadBuffer(&piezoMotor3, UPPER_HALF_INDEX);

    /* Start DMA for SPI 3 */
    SPI_1LINE_TX(&hspi3);   /* This must be always done in slave mode */
    HAL_SPI_Transmit_DMA(&hspi3,(void *)&(piezoMotor3.dmaBuffer), sizeof(piezoMotor3.dmaBuffer)/sizeof(uint16_t));

    /* Start DMA for SPI 2 */
    SPI_1LINE_TX(&hspi2);   /* This must be always done in slave mode */
    HAL_SPI_Transmit_DMA(&hspi2,(void *)&(piezoMotor2.dmaBuffer), sizeof(piezoMotor2.dmaBuffer)/sizeof(uint16_t));

    /* Start DMA for SPI 1 (this must be the last operation) */
    HAL_SPI_Transmit_DMA(&hspi1,(void *)&(piezoMotor1.dmaBuffer), sizeof(piezoMotor1.dmaBuffer)/sizeof(uint16_t));
}


/*******************************************************************************************************************//**
 * @brief   Control the ON/OFF status of the voltage booster. The VPP voltage rises to 47.5V in less than 1 ms
 * @param   ENABLE  Switch ON the voltage booster. VPP rises to 47.5V
 *          DISABLE Switch OFF the voltage booster. VPP falls to VIN voltage
 * @return  void
 */
void piezoHighVoltage(FunctionalState enable)
{
    /* Enable or disable the VPP power supply */
    HAL_GPIO_WritePin(VPPEN_GPIO_Port, VPPEN_Pin, (enable != DISABLE)? GPIO_PIN_SET : GPIO_PIN_RESET);
}


/*******************************************************************************************************************//**
 * @brief   Set the step frequency and the revolution direction of the piezo-motor.
 * @param   motor   Number of the motor
 *          freq    Desidered step frequency. The sign of the argument determines the direction of revolution
 *                  The value must be in the range [-PIEZO_MAXFREQ .. +PIEZO_MAXFREQ]
 * @return  One of the following values:
 *          HAL_ERROR   arguments error
 *          HAL_OK      operation terminated without errors
 */
HAL_StatusTypeDef piezoSetStepFrequency(piezoMotor_t motor, int32_t freq)
{
    /* Check arguments */
    HAL_StatusTypeDef result = HAL_ERROR;
    if ((motor < 3u) && (freq >= -PIEZO_MAXFREQ) && (freq <= PIEZO_MAXFREQ) && (piezoFreqConst != 0))
    {
        /* Convert in angle-units and set the new step value */
        pStatusTable[motor]->phaseDelta = piezoFreqConst * freq;
        /* No errors */
        result = HAL_OK;
    }
    return result;
}


/*******************************************************************************************************************//**
 * @brief   Get the step frequency and the revolution direction of the piezo-motor.
 * @param   motor   Number of the motor
 *          pFreq   Pointer to a variable int32_t where to load the step frequency of the selected motor
 * @return  One of the following values:
 *          HAL_ERROR   arguments error
 *          HAL_OK      operation terminated without errors
 */
HAL_StatusTypeDef piezoGetStepFrequency(piezoMotor_t motor, int32_t *pFreq)
{
    /* Check arguments */
    HAL_StatusTypeDef result = HAL_ERROR;
    if ((motor < 3u) && (pFreq != NULL) && (piezoFreqConst != 0))
    {
        /* Read the step value and convert in Hz */
        *pFreq = pStatusTable[motor]->phaseDelta / piezoFreqConst;
        /* No errors */
        result = HAL_OK;
    }
    return result;
}


/*******************************************************************************************************************//**
 * @brief   Null the motor speed and set the step angle the piezo-motor
 * @param   motor   Number of the motor
 *          angle   Step angle. Any value is valid
 * @return  One of the following values:
 *          HAL_ERROR   arguments error
 *          HAL_OK      operation terminated without errors
 */
HAL_StatusTypeDef piezoSetStepAngle(piezoMotor_t motor, uint32_t angle)
{
    /* Check arguments */
    HAL_StatusTypeDef result = HAL_ERROR;
    if ((motor < 3u) && (piezoFreqConst != 0))
    {
        /* Null speed: the interrupt does not revert phaseAngle changes */
        pStatusTable[motor]->phaseDelta = 0;
        /* Set the new step angle value */
        pStatusTable[motor]->phaseAngle = angle;
        /* No errors */
        result = HAL_OK;
    }
    return result;
}


/*******************************************************************************************************************//**
 * @brief   Get the step-angle of the piezo-motor.
 * @param   motor   Number of the motor
 *          pFreq   Pointer to a variable int32_t where to load the step-angle of the selected motor
 * @return  One of the following values:
 *          HAL_ERROR   arguments error
 *          HAL_OK      operation terminated without errors
 */
HAL_StatusTypeDef piezoGetStepAngle(piezoMotor_t motor, int32_t *pAngle)
{
    /* Check arguments */
    HAL_StatusTypeDef result = HAL_ERROR;
    if ((motor < 3u) && (pAngle != NULL) && (piezoFreqConst != 0))
    {
        /* Read the angle value */
        *pAngle = pStatusTable[motor]->phaseAngle;
        /* No errors */
        result = HAL_OK;
    }
    return result;
}


/*******************************************************************************************************************//**
 * @brief   Set or reset the BRAKE mode of the motor
 * @param   motor   Number of the motor
 *          enable  One of the following values:
 *                  ENABLE  Activate the BRAKE mode
 *                  DISABLE Deactivate the BRAKE mode
 * @return  One of the following values:
 *          HAL_ERROR   arguments error
 *          HAL_OK      operation terminated without errors
 */
HAL_StatusTypeDef piezoSetBrake(piezoMotor_t motor, FunctionalState enable)
{
    /* Check arguments */
    HAL_StatusTypeDef result = HAL_ERROR;
    if ((motor < 3u) && (piezoFreqConst != 0))
    {
        /* Null speed: the interrupt does not revert the changes */
        pStatusTable[motor]->phaseDelta = 0;
        /* BRAKE active */
        if (DISABLE != enable)
        {
            /* Set the BRAKE flag */
            pStatusTable[motor]->phaseCntrl |= PIEZO_BRAKE;
        }
        /* BRAKE not active */
        else
        {
            /* Reset the BRAKE flag */
            pStatusTable[motor]->phaseCntrl &= ~PIEZO_BRAKE;
        }
        /* No errors */
        result = HAL_OK;
    }
    return result;
}


/*******************************************************************************************************************//**
 * @brief   Set or reset the FREEWHEELING mode of the motor
 * @param   motor   Number of the motor
 *          enable  One of the following values:
 *                  ENABLE  Activate the FREEWHEELING mode
 *                  DISABLE Deactivate the FREEWHEELING mode
 * @return  One of the following values:
 *          HAL_ERROR   arguments error
 *          HAL_OK      operation terminated without errors
 */
HAL_StatusTypeDef piezoSetFreewheeling(piezoMotor_t motor, FunctionalState enable)
{
    /* Check arguments */
    HAL_StatusTypeDef result = HAL_ERROR;
    if ((motor < 3u) && (piezoFreqConst != 0))
    {
        /* Null speed: the interrupt does not revert the changes */
        pStatusTable[motor]->phaseDelta = 0;
        /* FREEWHEELING active */
        if (DISABLE != enable)
        {
            /* Set the FREEWHEELING flag */
            pStatusTable[motor]->phaseCntrl |= PIEZO_FREEWHEELING;
        }
        /* FREEWHEELING not active */
        else
        {
            /* Reset the FREEWHEELING flag */
            pStatusTable[motor]->phaseCntrl &= ~PIEZO_FREEWHEELING;
        }
        /* No errors */
        result = HAL_OK;
    }
    return result;
}


/* END OF FILE ********************************************************************************************************/
