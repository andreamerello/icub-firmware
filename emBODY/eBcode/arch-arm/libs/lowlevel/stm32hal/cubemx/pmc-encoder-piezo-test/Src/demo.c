#include <string.h>
#include "demo.h"

#define DEBUG

#ifndef DEBUG
#define coprintf(...)
#else
#include "console.h"
#endif

//#define SPI_SEARCH_ZERO

/* mapping qe encoders vs motors */
int qe_motor[] = {1, 2};
int spi_motor = 0;

int motor_zero_vel[] = {100, 10, 10};
int motor_direction_sign[] = {-1, 1, 1};
int motor_target[] = {6800 , 20000, 20000};
int motor_home[] = {250, 100, 100};
int motor_max_vel[] = {1000, 400, 400};

qe_encoder_cfg_t qe_cfg[2] = {
    {.htim = &htim2},
    {.htim = &htim5}
};

typedef struct {
    int direction;
    int start;
} motion_state_t;

#define QE_THR 5
#define SPI_THR 10
#define TURN_THR 1000

/* the SPI encoder ranges from 27000 to 32768, then from 0 to 1200.
 * When we read a value in the 1st range then our zero is @ 27000, otherwise -5768
 */
#define SPI_CHECKPOINT 10000
#define SPI_ZERO_GT 27000
#define SPI_ZERO_LT -5768

qe_encoder_t qe[2];
int qe_zero[2];
int spi_zero;
int motor_ramp[3] = {0, 0, 0};

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

void demo_find_zero(void)
{
    int i;
    int val;
    int prev_val[3];
    int m;
    int found = 0;
#ifdef SPI_SEARCH_ZERO
    int prev_turn_val;
    int turn = 0;

    lr17_encoder_acquire(NULL, NULL);
    osDelay(2);
    lr17_encoder_get(&prev_val[spi_motor]);
    prev_turn_val = prev_val[spi_motor];
    /* move in encoder-decreasing dir, so that min enc value became 0 */
    piezoSetStepFrequency(spi_motor,
                          motor_zero_vel[spi_motor] * -motor_direction_sign[spi_motor]);
#endif


    for (i = 0; i < 2; i++) {
        m = qe_motor[i];
        /* move in encoder-decreasing dir, so that min enc value became 0 */
        piezoSetStepFrequency(m, motor_zero_vel[m] * -motor_direction_sign[m]);
        qe_encoder_get(&qe[i], &prev_val[qe_motor[i]]);
    }

    osDelay(50);
    while (found != 7) {

        /* qe encoders */
        for (i = 0; i < 2; i++) {
            m = qe_motor[i];
            qe_encoder_get(&qe[i], &val);
            if (abs(val - prev_val[m]) < QE_THR) {
                piezoSetStepFrequency(m, 0);
                qe_zero[i] = val;
                found |= 1 << m;
            }
            prev_val[m] = val;
        }
        lr17_encoder_acquire(NULL, NULL);
        osDelay(50);
        /* SPI encoder */
        lr17_encoder_get(&val);

#ifdef SPI_SEARCH_ZERO
        turn += count_turn(val, prev_turn_val, 0x7FFF);
        prev_turn_val = val;
        val = val + (turn * 0x8000);
        coprintf("spi zero %d %d \n", val, prev_val[spi_motor]);
        if (abs(val - prev_val[spi_motor]) < SPI_THR) {
            piezoSetStepFrequency(spi_motor, 0);
            spi_zero = val;
            found |= 1 << spi_motor;
            coprintf("found\n");
        }
        prev_val[spi_motor] = val;
#else
        if (val > SPI_CHECKPOINT)
            spi_zero = SPI_ZERO_GT;
        else
            spi_zero = SPI_ZERO_LT;
        found |= 1 << spi_motor;
#endif
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

int motor_move(int max, int min, int pos, int maxvel,
               motion_state_t *state)
{
    int vel, vel1, vel2;

    if (!state->direction) {
        state->direction = 1;
        state->start = pos;
    }

    if (((state->direction == 1) && (pos > max)) ||
        ((state->direction == -1) && (pos < min))) {
             state->direction *= -1;
             state->start = pos;
    }

    /* vel considering ramp up from start */
    vel1 = abs(state->start - pos);
    /* vel considering ramp down reaching target */
    vel2 = (state->direction == 1) ? (max - pos) : (pos - min);
    /* we might be ramping up or down; don't mind, just take the slowest */
    vel = vel1 < vel2 ? vel1 : vel2;

#ifdef DEBUG
    coprintf("\033[5;0H");
    coprintf("\033[K");
    coprintf("p: %d, d: %d, s: %d v: %d\n", pos, state->direction, state->start, vel);
#endif
    /* "bootstrap" velocity to move away from target (ramp will be zero here */
    if (vel < 20)
        vel = 20;

    /* vel profile must be clamped, otherwise a triangle will be generated */
    if (vel > maxvel) {
        vel = maxvel;
    }

    return vel * state->direction;
}

void demo_loop(void)
{
    int qe_val;
    int spi_val, spi_prev_val;
    int pos;
    int i;
    int m;
    int turn = 0;
    motion_state_t motion[3];
    int vel;

    memset(motion, 0, sizeof(motion));
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
/*            vel = motor_move(motor_target[m], motor_home[m],
                                            qe_val,
                                            motor_max_vel[m], &motion[m]);
            piezoSetStepFrequency(m, vel * motor_direction_sign[m]);
*/
        }

        /* spi motor */
        lr17_encoder_get(&spi_val);
        turn += count_turn(spi_val, spi_prev_val, 0x7FFF);
        spi_prev_val = spi_val;
        pos = spi_val + (turn * 0x8000) - spi_zero;
        m = spi_motor;

        vel = motor_move(motor_target[m], motor_home[m],
                                        pos,
                                        motor_max_vel[m], &motion[m]);

        piezoSetStepFrequency(m, vel * motor_direction_sign[m]);
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
