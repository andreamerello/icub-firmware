#include "demo.h"
#include "leds.h"
#include "cmsis_os.h"
#include "../Drivers/encoder/qe_encoder.h"
#include "../Drivers/encoder/lr17_encoder.h"
#include "../Drivers/piezo/tables/generated/tables.h"
#include "../Drivers/piezo/piezo.h"

/* mapping qe encoders vs motors */
int qe_motor[] = {0, 1};
int qe_zero_direction[] = {1, 1};
int qe_zero_vel[] = {10, 10};

int spi_motor = 2;

int motor_target[] = {20000, 20000, 20000};
int motor_home[] = {200, 200, 200};
int motor_ramp_steepness[] = {1, 1, 1};
int motor_max_vel[] = {400, 400, 400};

qe_encoder_cfg_t qe_cfg[2] = {
    {.htim = &htim2},
    {.htim = &htim5}
};

#define QE_THR 5
#define QE_N 2
#define TURN_THR 1000

qe_encoder_t qe[2];
int qe_zero[2];
int motor_ramp[3] = {0, 0, 0};

void demo_find_zero(void)
{
    int i;
    int qe_val[2];
    int qe_prev_val[2];
    int found = 0;


    for (i = 0; i < QE_N; i++) {
        qe_encoder_get(&qe[i], &qe_prev_val[i]);
    }

    for (i = 0; i < QE_N; i++) {
        piezoSetStepFrequency(i, qe_zero_vel[i] * qe_zero_direction[i]);
    }

    while (found < QE_N) {
        for (i = 0; i < QE_N; i++) {
            qe_encoder_get(&qe[i], &qe_val[i]);
            if (abs(qe_val[i] - qe_prev_val[i]) < QE_THR) {
                piezoSetStepFrequency(qe_motor[i], 0);
                qe_zero[i] = qe_val[i];
                found++;
            }
        }
        osDelay(20);
    }
}

void demo_init(void)
{
    piezoMotorCfg_t cfg;
    int i;

    //analogInit();
    lr17_encoder_init();

    for (i = 0; i < 2; i++)
        qe_encoder_init(&qe[i], &qe_cfg[i]);

    cfg.phaseTable = delta_8192;
    cfg.phaseTableLen = 8192;

    if (HAL_OK != piezoInit(&cfg, &cfg, &cfg)) {
            while(1) {
            osDelay(20);
            LED_TOGGLE(LED_REDPORT, LED_RED0);
            LED_TOGGLE(LED_REDPORT, LED_RED1);
            LED_TOGGLE(LED_REDPORT, LED_RED2);
        }
    }
}

int motor_move(int motor, int max, int min, int pos, int direction)
{
    int vel;

    if (((direction == 1) && (pos > max)) ||
        ((direction == -1) && (pos < min))) {
            direction *= -1;
    }

    if ((direction == 1) && (motor_ramp[motor] < motor_max_vel[motor]))
        motor_ramp[motor] += motor_ramp_steepness[motor];
    else if ((direction == -1) && (motor_ramp[motor] > -motor_max_vel[motor]))
        motor_ramp[motor] -= motor_ramp_steepness[motor];

    vel = motor_ramp[motor];

    piezoSetStepFrequency(motor, vel);

    return direction;
}

int count_turn(int val, int prev, int max)
{
    if ((val > prev) &&
        (val > (max - TURN_THR)) &&
        (prev < TURN_THR ))
        return -1;

    if ((val < prev) &&
        (val < TURN_THR) &&
        (prev > (max - TURN_THR)))
        return 1;

    return 0;
}

void demo_loop(void)
{
    int qe_val;
    int spi_val, spi_prev_val;
    int pos;
    int i;
    int m;
    int turn = 0;
    int motor_direction[] = {1, 1, 1};

    /* bootstrap "prev" val spi encoder turn count */
    lr17_encoder_acquire(NULL, NULL);
    osDelay(2);
    lr17_encoder_get(&spi_prev_val);

    while(1) {
        /* qe motors */
        for (i = 0; i < 2; i++) {
            qe_encoder_get(&qe[i], &qe_val);
            qe_val -= qe_zero[i];
            m = qe_motor[i];
            motor_direction[m] = motor_move(m,
                                         motor_target[m], motor_home[m],
                                         qe_val, motor_direction[m]);
        }

        /* spi motor */
        lr17_encoder_get(&spi_val);
        turn = count_turn(spi_val, spi_prev_val, 0x7FFF);
        spi_prev_val = spi_val;
        pos = spi_val + (turn * 0x8000);
        m = spi_motor;
        motor_direction[m] = motor_move(m,
                                   motor_target[m], motor_home[m],
                                   pos, motor_direction[m]);
        lr17_encoder_acquire(NULL, NULL);

        /* control loop 1KHz */
        osDelay(1);
    }
}

void demo_main(void)
{
    demo_init();
    demo_find_zero();
    demo_loop();

    while(1);
}
