#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include "../piezo_gz.h"
#include "../gpl_utils.h"
#include "../tables/generated/delta_8192_table.c"
#include "../tables/generated/rhomb_8192_table.c"

int dbg_count = 0;

#define DAC_CMD_LOADLATCH 0x2
#define DAC_CMD_LOAD 0x0

SPI_HandleTypeDef hspi1 = {.id = 0};
SPI_HandleTypeDef hspi2 = {.id = 1};
SPI_HandleTypeDef hspi3 = {.id = 2};

pthread_t dma_thread;
struct  {
	int size;
	void *src;
	void *arg;
	int idx;
} dma_data[3];

int dma_stop = 0;

void(*dma_cb)(DMA_HandleTypeDef *_hdma) = NULL;
void(*dma_h_cb)(DMA_HandleTypeDef *_hdma) = NULL;

void(*spi_cb[3])(SPI_HandleTypeDef *_hdma) = {[0 ... 2] = NULL};
void(*spi_h_cb[3])(SPI_HandleTypeDef *_hdma) = {[0 ... 2] = NULL};


#define DAC_DECODE_VAL(x) ((((x) & 0xFF) << 8) | (((x) & 0xFF000000) >> 24))
#define DAC_DECODE_ADR(x) (((x) & 0x700) >> 8)
#define DAC_LDAC(x) (((x) >> 11) & 0x7)

void *dma_worker(void *arg)
{
	char fname[128];
	uint32_t out;
	int ldac = 0;
	int dac[3] = {0, 0, 0};
	int err;
	int id;
	FILE *f[3];

	for (id = 0; id < 3; id++) {
		sprintf(fname, "test_out_%d.csv", id);
		f[id] = fopen(fname, "w");
	}

	while (!ACCESS_ONCE(dma_stop)) {
		for (id = 2; id >= 0; id--) {
			err = 0;
			out = ACCESS_ONCE(((uint32_t*)dma_data[id].src)[dma_data[id].idx++]);
			/*
			 * dma_data.size is in 16-bit words
			 * the DMA buffer is accessed in 32-bit words
			 */
			if (dma_data[id].idx == (dma_data[id].size / 4)) {
				if (dma_h_cb)
					dma_h_cb(dma_data[id].arg);
				if (spi_h_cb[id])
					spi_h_cb[id](dma_data[id].arg);
				//printf("xfer h\n");
			}

			if (dma_data[id].idx == (dma_data[id].size / 2)) {
				dma_data[id].idx = 0;
				if (dma_cb)
					dma_cb(dma_data[id].arg);
				if (spi_cb[id]) {
					spi_cb[id](dma_data[id].arg);
					dbg_count++;
				}
				//printf("xfer c\n");
			}
			usleep(50);

			if (dac[id] != DAC_DECODE_ADR(out)) {
				printf("DAC idx SEQ error; motor %d\n", id);
				err = 1;
			}

			if ((dac[id] == 3) && (DAC_LDAC(out) != DAC_CMD_LOADLATCH)) {
				printf("missing LOADLATCH error; motor %d\n", id);
				err = 1;
			}

			if ((dac[id] != 3) && (DAC_LDAC(out) != DAC_CMD_LOAD)) {
				printf("missing LOAD error; motor %d\n", id);
				err = 1;
			}

//		if (!err)
//			printf("OK\n");

			dac[id] = (dac[id] + 1) % 4;

			fprintf(f[id], "%u", DAC_DECODE_VAL(out));
			if (dac[id] == 3)
				fprintf(f[id], "\n");
			else
				fprintf(f[id], ", ");
//		printf("0x%08x - %d %d %s\n", out,
//		       DAC_DECODE_ADR(out), DAC_DECODE_VAL(out),
//		       DAC_LDAC(out) ? "LDAC" : "");

		}
	}
	for (id = 0; id < 3; id++)
		fclose(f[id]);
}

void HAL_DMA_RegisterCallback(DMA_HandleTypeDef *hdma,
			      int CallbackID,
			      void(*cb)(DMA_HandleTypeDef *_hdma))
{
	dma_data[0].arg = hdma;
	if (CallbackID == HAL_DMA_XFER_CPLT_CB_ID)
		dma_cb = cb;
	else
		dma_h_cb = cb;
}

void HAL_SPI_RegisterCallback(SPI_HandleTypeDef *hspi,
			      int CallbackID,
			      void(*cb)(SPI_HandleTypeDef *_hspi))
{
	dma_data[hspi->id].arg = hspi;
	if (CallbackID == HAL_SPI_TX_COMPLETE_CB_ID)
		spi_cb[hspi->id] = cb;
	else
		spi_h_cb[hspi->id] = cb;
}

void HAL_SPI_Transmit_DMA(SPI_HandleTypeDef *hspi,
			  uint8_t *pData, uint16_t Size)
{
	ACCESS_ONCE(dma_data[hspi->id].size) = Size;
	ACCESS_ONCE(dma_data[hspi->id].src) = pData;

	if (hspi->id == 0)
		pthread_create(&dma_thread, NULL, &dma_worker, NULL);
}

void HAL_SPI_DMAStop(SPI_HandleTypeDef *hspi)
{

	ACCESS_ONCE(dma_stop) = 1;

	pthread_join(dma_thread, NULL);

}

int main()
{
	piezoMotorCfg_t cfg1, cfg2, cfg3;
	int i;

	//static const uint16_t phaseTable[] =
	//{
	    /* Include CSV file (edit and substitute "semicolon" with "comma") */
	//    #include "PhaseTable.csv"
	//};

	cfg1.phaseTable = delta_8192;
	cfg2.phaseTable = delta_8192;
	cfg3.phaseTable = rhomb_8192;

	cfg1.phaseTableLen = 8192;
	cfg2.phaseTableLen = 8192;
	cfg3.phaseTableLen = 8192;


	piezoInit(&cfg1, &cfg2, &cfg3);
	piezoSetStepFrequency(0, 200);
	piezoSetStepFrequency(1, 100);
	piezoSetStepFrequency(2, 57);
	sleep(1);
	piezoSetMode(0, PIEZO_BRAKE);
	sleep(1);
	piezoSetMode(1, PIEZO_BRAKE);
	sleep(1);
	piezoSetMode(0, PIEZO_NORMAL);
	for (i = 0; i < 50; i++) {
		usleep(20000);
		piezoSetStepFrequency(0, 200 - i*2);
	}

	piezoSetMode(0, PIEZO_FREEWHEELING);
	piezoSetMode(1, PIEZO_FREEWHEELING);
	sleep(1);
	piezoSetMode(0, PIEZO_NORMAL);
	piezoSetMode(2, PIEZO_BRAKE);
	sleep(1);

	HAL_SPI_DMAStop(&hspi1);
//	printf("count %d\n", dbg_count);
}