#include "qe_encoder.h"

bool qe_encoder_init(qe_encoder_t *e, qe_encoder_cfg_t *c)
{
    e->cfg = *c;
	HAL_TIM_Encoder_Start(e->cfg.htim, TIM_CHANNEL_ALL);

    return true;
}

bool qe_encoder_get(qe_encoder_t *e, int *val)
{
    *val = __HAL_TIM_GET_COUNTER(e->cfg.htim);
    return true;
}
