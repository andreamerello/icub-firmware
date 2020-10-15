#include <stdlib.h>
#include <errno.h>
#include "dmaspi.h"

struct _dmaspi_table_t{
	struct _dmaspi_table_t *next;
	dmaspi_handle_t *data;
};

typedef struct _dmaspi_table_t dmaspi_table_t;

static dmaspi_table_t *dmaspi_global_table = NULL;

static int dmaspi_register(dmaspi_handle_t *h)
{
	dmaspi_table_t **tmp;

	for (tmp = &dmaspi_global_table; *tmp; tmp = &((*tmp)->next));

	*tmp = calloc(1, sizeof(dmaspi_handle_t));
	if (!*tmp)
		return -ENOMEM;

	(*tmp)->next = NULL;
	(*tmp)->data = h;

	return 0;
}

static dmaspi_handle_t *dmaspi_from_dma(DMA_HandleTypeDef *dma)
{
	dmaspi_table_t *tmp;

	for (tmp = dmaspi_global_table; tmp; tmp = tmp->next)
		if (tmp->data->dma == dma)
			return tmp->data;

	return NULL;
}

void dmaspi_cb(DMA_HandleTypeDef *hdma)
{
	dmaspi_handle_t *dmaspi;

	dmaspi = dmaspi_from_dma(hdma);
	dmaspi->cb(dmaspi->cb_arg);
}

void dmaspi_hcb(DMA_HandleTypeDef *hdma)
{
	dmaspi_handle_t *dmaspi;

	dmaspi = dmaspi_from_dma(hdma);
	dmaspi->hcb(dmaspi->cb_arg);
}

int dmaspi_init(dmaspi_handle_t *h,
		DMA_HandleTypeDef *dma, SPI_HandleTypeDef *spi)
{
	dmaspi_handle_t *tmp = dmaspi_from_dma(dma);
	if (tmp)
		return -EINVAL;

	h->dma = dma;
	h->spi = spi;

	return dmaspi_register(h);
}

static void dmaspi_ssel_quirk(void)
{
	HAL_GPIO_WritePin(DAC_SYNCEN_GPIO_Port,
			  DAC_SYNCEN_Pin, GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(DAC_SYNCEN_GPIO_Port,
			  DAC_SYNCEN_Pin, GPIO_PIN_SET);
}

void dmaspi_start_cyclic(dmaspi_handle_t *h, void *data, int size,
			 void (*hcb)(void *arg), void (*cb)(void *arg),
			 void *arg)
{
	h->cb = cb;
	h->hcb = hcb;
	h->cb_arg = arg;
	HAL_DMA_RegisterCallback(h->dma, HAL_DMA_XFER_CPLT_CB_ID, dmaspi_cb);
	HAL_DMA_RegisterCallback(h->dma, HAL_DMA_XFER_HALFCPLT_CB_ID, dmaspi_hcb);
	dmaspi_ssel_quirk();
	if (h->spi->Init.Mode == SPI_MODE_MASTER)
		HAL_SPI_Transmit_DMA(h->spi, data, size);
	/*
	HAL_DMA_Start_IT(p->dma_handle, p->dma_buffer,
			 SPI_DR, sizeof(PIEZO_PHASETABLE));
	*/

}

void dmaspi_stop_cyclic(dmaspi_handle_t *h)
{
	/*HAL_DMA_Abort(p->dma_handle); */
	HAL_SPI_DMAStop(h->spi);
}
