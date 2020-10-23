#include <string.h>
#include <malloc.h>
#include <errno.h>
#include "piezo_ad5664.h"

#define DAC_CMD_LOAD 0x0
#define DAC_CMD_LOADLATCH 0x2

/* stolen from Linux kernel -> GPL */
#define __ACCESS_ONCE(x) ({		 \
	typeof(x) __var = (typeof(x)) 0; \
	(volatile typeof(x) *)&(x); })
#define ACCESS_ONCE(x) (*__ACCESS_ONCE(x))

static int fls(unsigned int x)
{
	int r = 32;

	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}
/* end of stolen from linux kernel */

/* Macro: set a 32 bits word to be transmitted as two 16 bits half-word */
#define PIEZO_SETDACVALUE(cmd,dac,val)		\
	(((uint32_t)(cmd)<<11) & 0x00003800UL	\
	 |((uint32_t)(dac)<<8) & 0x00000700UL	\
	 |((uint32_t)(val)>>8) & 0x000000FFUL	\
	|((uint32_t)(val)<<24) & 0xFF000000UL)

int piezo_init(piezo_handle_t *p, piezo_cfg_t *cfg)
{
	memset(p, 0, sizeof(*p));
	p->cfg = *cfg;
	p->dma_buffer = (piezo_dma_buf_t*)calloc(p->cfg.dma_elem_num,
						 sizeof(piezo_dma_buf_t));
	if (!p->dma_buffer)
		return -ENOMEM;

	return 0;
}

void piezo_free(piezo_handle_t *p)
{
	free(p->dma_buffer);
}

static void piezo_dma_update(piezo_handle_t *p, int half)
{
	int i, j;
	uint16_t val;
	int idx[4];
	piezo_dma_buf_t *buf;
	uint32_t cmd;
	int vel = ACCESS_ONCE(p->v);
	int len = p->cfg.dma_elem_num;
	int _shift = fls(p->cfg.phasetable_len) - 1;
	int shift = 32 - _shift;
	uint32_t mask = (1 << _shift) - 1;
	/*
	 * on half DMA IRQ    :  DMA works on 2nd part, FW on 1st part.
	 * on complete DMA IRQ:  FW works on 1st part, DMA on 2nd part.
	 */
	buf = p->dma_buffer + (half ? 0 : len);

	/* for each entry in (each of 4) buffer.. */
	for (i = 0; i < len; i++) {
		p->phase += vel;
		/* GZ magic... */
		idx[0] = (p->phase >> shift) & mask;
		idx[1] = (~p->phase >> shift) & mask;
		idx[2] = (( p->phase + 2147483648UL) >>
			  shift) & mask;
		idx[3] = ((~p->phase + 2147483648UL) >>
			  shift) & mask;

		for (j = 0; j < 4; j++) {
			val = p->cfg.phasetable[idx[j]];
			cmd = (j == 3) ? DAC_CMD_LOADLATCH : DAC_CMD_LOAD;
			ACCESS_ONCE(buf[i].DAC[j]) = PIEZO_SETDACVALUE(cmd, j, val);
		}
	}
}

static void piezo_dma_complete_cb(void *arg)
{
	piezo_handle_t *p = arg;
	piezo_dma_update(p, 0);
}

static void piezo_dma_half_cb(void *arg)
{
	piezo_handle_t *p = arg;
	piezo_dma_update(p, 1);
}

static void piezo_release(piezo_handle_t *p)
{
	piezo_stop(p);
	free(p->dma_buffer);
}

void piezo_start(piezo_handle_t *p)
{
	if (p->running)
		return;

	p->running = 1;

	piezo_dma_update(p, 0);
	piezo_dma_update(p, 1);

	/* xfer N 16-bit words, hence we divide len by 2 */
	dmaspi_start_cyclic(p->cfg.dmaspi,
			    p->dma_buffer, p->cfg.dma_elem_num *
			    sizeof(piezo_dma_buf_t) / 2,
			    piezo_dma_half_cb, piezo_dma_complete_cb, p);
}

void piezo_stop(piezo_handle_t *p)
{
	if (!p->running)
		return;

	dmaspi_stop_cyclic(p->cfg.dmaspi);
}

int piezo_set_v(piezo_handle_t *p, int v)
{
	if ((v > p->cfg.max_v) || (v < -p->cfg.max_v))
		return -EINVAL;

	ACCESS_ONCE(p->v) = v;

	return 0;
}

#if 0
uint32_t piezo_interpolate(uint32_t a, uint32_t b, int idx, int max)
{
	double ret;

	ret = (double)a / (double)max * (double)idx;
	ret += (double)b / (double)max * (double)(max - idx);
	return (uint32_t)ret;
}
#endif
