#include "lr17_encoder.h"

void(*lr17_encoder_cb)(void *arg) = NULL;
void *lr17_encoder_cb_arg;

int lr17_encoder_val = 0;
uint16_t lr17_encoder_buf;

int lr17_encoder_time;


bool lr17_encoder_init()
{
	lr17_encoder_time = ...;
}

bool lr17_encoder_acquire(void(*cb)(void *arg), void *arg)
{
    int time;

    lr17_encoder_cb_arg = arg;
    lr17_encoder_cb = cb;

    time = ...;

    if ((lr17_encoder_time - time) > 500uS) {
        HAL_SPI_Receive_IT(&hspi4, &lr17_encoder_buf, 2));
        return true;
    } else {
        return false;
    }
}

void lr17_spi_cb(HALSPI spi)
{
    lr17_encoder_time = ...;
    ACCESS_ONCE(lr17_encoder_val) = ACCESS_ONCE(lr17_encoder_buf) & 0x7fffffff;
    if (lr17_encoder_cb)
        lr17_encoder_cb(lr17_encoder_cb_arg);
}

bool lr17_encoder_get(int *angle)
{
    *angle = ACCESS_ONCE(lr17_encoder_val);
    return true;
}
