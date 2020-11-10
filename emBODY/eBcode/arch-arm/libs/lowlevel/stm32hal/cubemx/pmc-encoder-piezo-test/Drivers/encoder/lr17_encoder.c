#include "lr17_encoder.h"

void(*lr17_encoder_cb)(void *arg) = NULL;
void *lr17_encoder_cb_arg;

int lr17_encoder_val = 0;

union {
    uint8_t spi[2];
    uint16_t val;
} lr17_encoder_buf;

int lr17_encoder_time;


bool lr17_encoder_init()
{
#warning FIXME_dummy
	lr17_encoder_time = 0;
}

bool lr17_encoder_acquire(void(*cb)(void *arg), void *arg)
{
    int time;

    lr17_encoder_cb_arg = arg;
    lr17_encoder_cb = cb;

#warning FIXME_dummy
    time = 501;

    if ((lr17_encoder_time - time) > 500) {
        HAL_SPI_Receive_IT(&hspi4, lr17_encoder_buf.spi, 2);
        return true;
    } else {
        return false;
    }
}

void lr17_spi_cb(SPI_HandleTypeDef *spi)
{
#warning FIXME_dummy
    lr17_encoder_time = 0;

    ACCESS_ONCE(lr17_encoder_val) =
        ACCESS_ONCE(lr17_encoder_buf.val) & 0x7fff;
    if (lr17_encoder_cb)
        lr17_encoder_cb(lr17_encoder_cb_arg);
}

bool lr17_encoder_get(int *angle)
{
    *angle = ACCESS_ONCE(lr17_encoder_val);
    return true;
}
