#ifndef __PIEZO_H__
#define __PIEZO_H__

#include <stdint.h>
#include "hal.h"
#include "dmaspi.h"

typedef struct  __attribute__((packed)) {
	uint32_t DAC[4];
} piezo_dma_buf_t;

typedef struct {
	int running;
	piezo_dma_buf_t* dma_buffer;
	int v;
	uint32_t phase;
	int max_v;
	int dma_elem_num;
	dmaspi_handle_t *dmaspi;
} piezo_handle_t;

typedef struct {
	int max_v;
	int dma_elem_num;
	dmaspi_handle_t *dmaspi;
	/* dma */
} piezo_cfg_t;


void piezo_start(piezo_handle_t *p);
void piezo_stop(piezo_handle_t *p);
int piezo_set_v(piezo_handle_t *p, int v);
int piezo_init(piezo_handle_t *p, piezo_cfg_t *cfg);
#endif
