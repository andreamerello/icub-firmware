#ifndef __QE_ENCODER_H__
#define __QE_ENCODER_H__

#include <stdbool.h>
#include "hal.h"

typedef struct {
    TIM_HandleTypeDef *htim;
} qe_encoder_cfg_t;

typedef struct {
    qe_encoder_cfg_t cfg;
} qe_encoder_t;

extern bool qe_encoder_init(qe_encoder_t *e, qe_encoder_cfg_t *c);
extern bool qe_encoder_get(qe_encoder_t *e, int *val);

#endif
