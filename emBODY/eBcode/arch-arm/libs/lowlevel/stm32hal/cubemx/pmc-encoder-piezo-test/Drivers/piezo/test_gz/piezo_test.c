#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include "../piezo_gz.h"
#include "../gpl_utils.h"

int dbg_count = 0;

#define DAC_CMD_LOADLATCH 0x2
#define DAC_CMD_LOAD 0x0

SPI_HandleTypeDef hspi1 = {.id = 1};
SPI_HandleTypeDef hspi2 = {.id = 2};
SPI_HandleTypeDef hspi3 = {.id = 3};

pthread_t dma_thread;
struct  {
	int size;
	void *src;
	int idx;
	void *arg;
} dma_data;

int dma_stop = 0;

void(*dma_cb)(DMA_HandleTypeDef *_hdma) = NULL;
void(*dma_h_cb)(DMA_HandleTypeDef *_hdma) = NULL;

void(*spi_cb)(SPI_HandleTypeDef *_hdma) = NULL;
void(*spi_h_cb)(SPI_HandleTypeDef *_hdma) = NULL;


#define DAC_DECODE_VAL(x) ((((x) & 0xFF) << 8) | (((x) & 0xFF000000) >> 24))
#define DAC_DECODE_ADR(x) (((x) & 0x700) >> 8)
#define DAC_LDAC(x) (((x) >> 11) & 0x7)

void *dma_worker(void *arg)
{
	uint32_t out;
	int ldac = 0;
	int dac = 0;
	int err;

	FILE *f = fopen("test_out.csv", "w");
	while (!ACCESS_ONCE(dma_stop)) {
		err = 0;
		out = ACCESS_ONCE(((uint32_t*)dma_data.src)[dma_data.idx++]);
		/*
		 * dma_data.size is in 16-bit words
		 * the DMA buffer is accessed in 32-bit words
		 */
		if (dma_data.idx == (dma_data.size / 4)) {
			if (dma_h_cb)
				dma_h_cb(dma_data.arg);
			if (spi_h_cb)
				spi_h_cb(dma_data.arg);
			//printf("xfer h\n");
		}

		if (dma_data.idx == (dma_data.size / 2)) {
			dma_data.idx = 0;
			if (dma_cb)
				dma_cb(dma_data.arg);
			if (spi_cb) {
				spi_cb(dma_data.arg);
				dbg_count++;
			}
			//printf("xfer c\n");
		}
		usleep(50);

		if (dac != DAC_DECODE_ADR(out)) {
			printf("DAC idx SEQ error\n");
			err = 1;
		}

		if ((dac == 3) && (DAC_LDAC(out) != DAC_CMD_LOADLATCH)) {
			printf("missing LOADLATCH error\n");
			err = 1;
		}

		if ((dac != 3) && (DAC_LDAC(out) != DAC_CMD_LOAD)) {
			printf("missing LOAD error\n");
			err = 1;
		}

//		if (!err)
//			printf("OK\n");

		dac = (dac + 1) % 4;

		fprintf(f, "%u", DAC_DECODE_VAL(out));
		if (dac == 3)
			fprintf(f, "\n");
		else
			fprintf(f, ", ");
//		printf("0x%08x - %d %d %s\n", out,
//		       DAC_DECODE_ADR(out), DAC_DECODE_VAL(out),
//		       DAC_LDAC(out) ? "LDAC" : "");

	}
	fclose(f);
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

void HAL_SPI_RegisterCallback(SPI_HandleTypeDef *hspi,
			      int CallbackID,
			      void(*cb)(SPI_HandleTypeDef *_hspi))
{
	/* emulate just 1 ch for now */
	if (hspi->id != hspi1.id)
		return;

	dma_data.arg = hspi;
	if (CallbackID == HAL_SPI_TX_COMPLETE_CB_ID)
		spi_cb = cb;
	else
		spi_h_cb = cb;
}

void HAL_SPI_Transmit_DMA(SPI_HandleTypeDef *hspi,
			  uint8_t *pData, uint16_t Size)
{
	/* emulate just 1 ch for now */
	if (hspi->id != hspi1.id)
		return;

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

int main()
{
	piezoInit();
	piezoHighVoltage(ENABLE);
	piezoSetStepFrequency(0, 2000);
	sleep(10);
	piezoSetBrake(0, ENABLE);
	piezoSetBrake(1, ENABLE);
	piezoSetBrake(2, ENABLE);

	HAL_SPI_DMAStop(&hspi1);
//	printf("count %d\n", dbg_count);
}
