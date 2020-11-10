#ifndef __LR17_ENCODER_H__
#define __LR17_ENCODER_H__

#include <stdint.h>
#include <stdbool.h>
#include "../gpl_utils.h"
#include "hal.h"

extern bool lr17_encoder_init();
extern bool lr17_encoder_acquire(void(*cb)(void *arg), void *arg);
extern bool lr17_encoder_get(int *angle);

#endif
