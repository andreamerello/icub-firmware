/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h> /* abs() */
#include <stdio.h>
#include "usbd_vcom.h"
#include "console.h"
#include "leds.h"
#include "analog.h"
#include "queue.h"
#include "utilities.h"
#include "../Drivers/piezo/piezo.h"
#include "../Drivers/piezo/tables/generated/delta_8192_table.c"
#include "../Drivers/piezo/tables/generated/delta_1024_table.c"
#include "../Drivers/piezo/tables/generated/rhomb_8192_table.c"
#include "../Drivers/encoder/qe_encoder.h"
#include "../Drivers/encoder/lr17_encoder.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for Main */
osThreadId_t MainHandle;
const osThreadAttr_t Main_attributes = {
  .name = "Main",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 1024 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void MainTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Main */
  MainHandle = osThreadNew(MainTask, NULL, &Main_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_MainTask */
/**
  * @brief  Function implementing the Main thread.
  * @param  argument: Not used
  * @retval None
  */

#define printf coprintf
#define UI_STAT_DOWNSAMPLING 5

typedef struct {
    int ival;
    int mot;
    enum {
        ACT_GO,
        ACT_FREE,
        ACT_BRAKE,
        ACT_MAX,
        ACT_MIN,
        ACT_STEP,
        ACT_RECOVER,
    } action;
} demo_cmd_t;

uint32_t led_red[] = {LED_RED0, LED_RED1, LED_RED2};
QueueHandle_t cmd_queue  = NULL ;
void CmdTask(void *argument)
{
    char cobuf[64];
    const char *coptr;
    char ch;
    demo_cmd_t cmd;
    long tmpsl;

    while (1) {
        coLockedEditString(portMAX_DELAY, cobuf, sizeof(cobuf));
        coprintf("\r");
        printf("\033[K");
        coptr = cobuf;

        if (!atosl(&coptr, &tmpsl) || (tmpsl > 3))
            continue;
        ch = toupper(skipblank(&coptr));
        coptr++;
        cmd.mot = tmpsl;
        switch ((int)ch) {
        case 'G':
            cmd.action = ACT_GO;
            break;
        case 'F':
            cmd.action = ACT_FREE;
            break;
        case 'B':
            cmd.action = ACT_BRAKE;
            break;
        case 'X':
            cmd.action = ACT_MAX;
            if (!atosl(&coptr, &tmpsl) || (tmpsl > 1000))
                continue;
            cmd.ival = tmpsl;
            break;
        case 'N':
            cmd.action = ACT_MIN;
            if (!atosl(&coptr, &tmpsl) || (tmpsl > 1000)) {
                continue;
            }
            cmd.ival = tmpsl;
            break;
        case 'S':
            cmd.action = ACT_STEP;
            if (!atosl(&coptr, &tmpsl) || (tmpsl > 100))
                continue;
            cmd.ival = tmpsl;
            break;
        case 'R':
            cmd.action = ACT_RECOVER;
            break;

        default:
            continue;
        }

        xQueueSendToFront(cmd_queue, &cmd, 0);
    }
}

/* USER CODE END Header_MainTask */
void MainTask(void *argument)
{
  /* USER CODE BEGIN MainTask */
    demo_cmd_t cmd;
    int i;
    piezoMotorCfg_t cfg1, cfg2, cfg3;
    qe_encoder_cfg_t qe_cfg[2] = {
        {.htim = &htim2},
        {.htim = &htim5}
    };
    qe_encoder_t qe[2];
    int qe_val[2];
    int lr17_val;
    int ui_stat_counter = UI_STAT_DOWNSAMPLING;
    piezoMotorState_t state[3];
    int vel[3] = {0, 0, 0};
    int vel_min[3] = {0, 0, 0};
    int vel_max[3] = {100, 100, 100};
    int delta[3] = {10, 10, 10};

    if (VCOM_OK != vcomInit()) {
        while(1) {
            LED_TOGGLE(LED_ORANGEPORT, LED_ORANGE0);
            osDelay(50);
        }
    }

    LED_ON(LED_ORANGEPORT, LED_ORANGE0);

    vcomRxChar();
    cmd_queue = xQueueCreate(1, sizeof(demo_cmd_t));
    if (NULL == cmd_queue) {
        while(1) {
            LED_TOGGLE(LED_ORANGEPORT, LED_ORANGE0);
            osDelay(100);
        }
    }

#ifndef DEBUG_SPI
    if (pdPASS != xTaskCreate(CmdTask, "CmdTask",
                              configMINIMAL_STACK_SIZE, NULL,
                              osPriorityNormal, NULL)) {
        while(1) {
            LED_TOGGLE(LED_ORANGEPORT, LED_ORANGE0);
            osDelay(1000);
        }
    }
#endif
    printf("Start\n");

    analogInit();
    lr17_encoder_init();

    for (i = 0; i < 2; i++)
             qe_encoder_init(&qe[i], &qe_cfg[i]);

    cfg1.phaseTable = delta_1024;
    cfg2.phaseTable = delta_8192;
    cfg3.phaseTable = rhomb_8192;

    cfg1.phaseTableLen = 1024;
    cfg2.phaseTableLen = 8192;
    cfg3.phaseTableLen = 8192;

    piezoInit(&cfg1, &cfg2, &cfg3);

    vel[0] = 0;

    while (1) {
        if (pdPASS == xQueueReceive(cmd_queue, &cmd, 0)) {
            switch (cmd.action) {
            case ACT_RECOVER:
                piezoOvercurrentClear(cmd.mot);
                break;
            case ACT_GO:
                piezoSetMode(cmd.mot, PIEZO_NORMAL);
                break;
            case ACT_FREE:
                piezoSetMode(cmd.mot, PIEZO_FREEWHEELING);
                break;
            case ACT_BRAKE:
                piezoSetMode(cmd.mot, PIEZO_BRAKE);
                break;
            case ACT_MAX:
                vel_max[cmd.mot] = cmd.ival;
                break;
            case ACT_MIN:
                vel_min[cmd.mot] = cmd.ival;
                break;
            case ACT_STEP:
                delta[cmd.mot] = cmd.ival;
                break;
            break;
            }
        }

        for (i = 0; i < 3; i++) {
            piezoSetStepFrequency(i, vel[i]);
            vel[i] += delta[i];
            if (vel[i] > vel_max[i]) {
                vel[i] = vel_max[i];
                delta[i] = -abs(delta[i]);
            }

            if (vel[i] < vel_min[i]) {
                vel[i] = vel_min[i];
                delta[i] = abs(delta[i]);
            }
        }
        osDelay(100);

        for (i = 0; i < 3; i++) {
            piezoGetState(i, &state[i]);
            if (state[i] == STATE_OVERCURRENT) {
                LED_ON(LED_REDPORT, led_red[i]);
            } else {
                LED_OFF(LED_REDPORT, led_red[i]);
            }
        }

        if (ui_stat_counter == 1)
            lr17_encoder_acquire(NULL, NULL);
        if (!ui_stat_counter--) {
            ui_stat_counter = UI_STAT_DOWNSAMPLING;
            for (i = 0; i < 2; i++)
                qe_encoder_get(&qe[i], &qe_val[i]);
            lr17_encoder_get(&lr17_val);
            printf("\033[s");
            printf("\033[5;0H");
            printf("encoders: QE1: %d, QE2: %d, ABS: %d\n",
                   qe_val[0], qe_val[1], lr17_val);

            printf("protection: %d %d %d\n",
                   state[0] == STATE_OVERCURRENT,
                   state[1] == STATE_OVERCURRENT,
                   state[2] == STATE_OVERCURRENT);
            printf("VREFINT = %5u mV, VIN = %5u mV, CIN = %5u mA, VPP = %5u mV, VCC = %5u mV\n",
                     analogGetVref_mV(), analogGetVin_mV(),
                     analogGetIin_mA(), analogGetVpp_mV(), analogGetVcc_mV());
            printf("\033[u");

        }
    }
  /* USER CODE END MainTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
