#include <stdint.h>
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "leds.h"
#include "analog.h"
#include "console.h"
#include "queue.h"
#include "usbd_vcom.h"
#include "utilities.h"
#include "../Drivers/piezo/piezo.h"
#include "../Drivers/piezo/tables/generated/tables.h"
#include "../Drivers/encoder/qe_encoder.h"
#include "../Drivers/encoder/lr17_encoder.h"

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

void eng_test_main(void)
{

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

    cmd_queue = xQueueCreate(1, sizeof(demo_cmd_t));
    if (NULL == cmd_queue) {
        while(1) {
            LED_TOGGLE(LED_ORANGEPORT, LED_ORANGE0);
            osDelay(100);
        }
    }

    printf("ENG test. Press any key to start\n");
    coRxChar();

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
    printf("Cmd are in form '<mot> <cmd> [<par>]' Where 0 =< mot <= 2; cmd is a char\n");
    printf("cmds: Recover, miNimum, maXimum, Step, Go, Freewheel, Brake\n");

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

    if (HAL_OK != piezoInit(&cfg1, &cfg2, &cfg3)) {
	    printf("ERROR: piezo init failure\n");
	    while(1);
    }

    for (i = 0; i < 3; i++)
        piezoSetMode(i, PIEZO_BRAKE);

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
            printf("\033[K");
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
}
