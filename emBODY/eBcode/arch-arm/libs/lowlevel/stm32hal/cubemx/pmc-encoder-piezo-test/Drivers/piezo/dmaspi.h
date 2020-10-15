#ifndef __DMASPI_H__
#define __DMASPI_H__

#include "hal.h"

typedef struct {
	DMA_HandleTypeDef *dma;
	SPI_HandleTypeDef *spi;
	void (*hcb)(void *);
	void (*cb)(void *);
	void *cb_arg;
} dmaspi_handle_t;

int dmaspi_init(dmaspi_handle_t *h,
		DMA_HandleTypeDef *dma, SPI_HandleTypeDef *spi);
void dmaspi_start_cyclic(dmaspi_handle_t *h, void *data, int size,
			 void (*hcb)(void *arg), void (*cb)(void *arg),
			 void *arg);
void dmaspi_stop_cyclic(dmaspi_handle_t *h);

#endif
