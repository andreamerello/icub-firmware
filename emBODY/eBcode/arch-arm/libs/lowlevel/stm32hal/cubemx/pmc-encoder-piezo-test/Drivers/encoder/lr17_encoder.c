#include "lr17_encoder.h"

void(*lr17_encoder_cb)(void *arg) = NULL;
void *lr17_encoder_cb_arg;

int lr17_encoder_val = 0;

union {
    uint8_t spi[2];
    uint16_t val;
} lr17_encoder_buf;

bool lr17_encoder_init()
{
    HAL_NVIC_SetPriority(SPI4_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(SPI4_IRQn);

    return true;
}

bool lr17_encoder_acquire(void(*cb)(void *arg), void *arg)
{
    lr17_encoder_cb_arg = arg;
    lr17_encoder_cb = cb;

    HAL_SPI_Receive_IT(&hspi4, lr17_encoder_buf.spi, 2);
    return true;
}

void lr17_spi_cb(SPI_HandleTypeDef *spi)
{
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
