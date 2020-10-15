#include <pthread.h>
#include <unistd.h>
#include "piezo.h"

pthread_t dma_thread;
struct  {
	int size;
	void *src;
	int idx;
	void *arg;
} dma_data;

int dma_stop = 0;

#define __ACCESS_ONCE(x) ({		 \
	typeof(x) __var = (typeof(x)) 0; \
	(volatile typeof(x) *)&(x); })
#define ACCESS_ONCE(x) (*__ACCESS_ONCE(x))

void(*dma_cb)(DMA_HandleTypeDef *_hdma);
void(*dma_h_cb)(DMA_HandleTypeDef *_hdma);

void HAL_GPIO_WritePin(int port, int pin, int state)
{
}

void *dma_worker(void *arg)
{
	uint32_t out;

	while(!ACCESS_ONCE(dma_stop)) {
		out = ((uint32_t*)dma_data.src)[dma_data.idx++ / 4];
		if (dma_data.idx == 0)
			dma_cb(dma_data.arg);
		if (dma_data.idx == dma_data.size / 2)
			dma_h_cb(dma_data.arg);
		if (dma_data.idx == dma_data.size)
			dma_data.idx = 0;
		usleep(1000);
	}
}

void HAL_DMA_RegisterCallback(DMA_HandleTypeDef *hdma,
			      int CallbackID,
			      void(*cb)(DMA_HandleTypeDef *_hdma))
{
	dma_data.arg = hdma;
	if (CallbackID == HAL_DMA_XFER_CPLT_CB_ID)
		dma_cb = cb;
	else
		dma_h_cb = cb;
}

void HAL_SPI_Transmit_DMA(SPI_HandleTypeDef *hspi,
			  uint8_t *pData, uint16_t Size)
{
	dma_data.size = Size;
	dma_data.src = pData;
	dma_data.idx = 0;
	pthread_create(&dma_thread, NULL, &dma_worker, NULL);
}

void HAL_SPI_DMAStop(SPI_HandleTypeDef *hspi)
{
	ACCESS_ONCE(dma_stop) = 1;
	pthread_join(dma_thread, NULL);
}

int dummy_dma;
int main()
{
	piezo_handle_t p;
	piezo_cfg_t cfg;

	cfg.max_v = 1000;
	/* must be pow of 2 */
	cfg.dma_elem_num = 512;
	cfg.dmaspi = (void*)&dummy_dma;

	piezo_init(&p, &cfg);
	piezo_start(&p);
	sleep(5);
	piezo_stop(&p);
}
