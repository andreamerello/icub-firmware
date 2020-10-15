#include <string.h>
#include <malloc.h>
#include <errno.h>
#include "piezo.h"

static const uint32_t PIEZO_PHASETABLE[128] = {
	0 ,	49 ,	98 ,	147 ,	196 ,	244 ,	292 ,	339 ,
	385 ,	430 ,	474 ,	517 ,	559 ,	599 ,	638 ,	675 ,
	711 ,	745 ,	777 ,	807 ,	835 ,	861 ,	885 ,	907 ,
	927 ,	944 ,	959 ,	972 ,	982 ,	990 ,	996 ,	999 ,
	999 ,	998 ,	993 ,	987 ,	978 ,	966 ,	952 ,	936 ,
	918 ,	897 ,	874 ,	849 ,	822 ,	793 ,	762 ,	729 ,
	694 ,	658 ,	620 ,	580 ,	539 ,	497 ,	454 ,	409 ,
	363 ,	317 ,	270 ,	222 ,	173 ,	124 ,	75 ,	26 ,
	-23 ,	-72 ,	-121 ,	-170 ,	-219 ,	-267 ,	-314 ,	-360 ,
	-406 ,	-451 ,	-494 ,	-537 ,	-578 ,	-617 ,	-655 ,	-692 ,
	-727 ,	-760 ,	-791 ,	-820 ,	-847 ,	-873 ,	-896 ,	-916 ,
	-935 ,	-951 ,	-965 ,	-977 ,	-986 ,	-993 ,	-997 ,	-999 ,
	-999 ,	-996 ,	-991 ,	-983 ,	-973 ,	-960 ,	-945 ,	-928 ,
	-908 ,	-887 ,	-863 ,	-837 ,	-809 ,	-779 ,	-747 ,	-713 ,
	-677 ,	-640 ,	-602 ,	-561 ,	-520 ,	-477 ,	-433 ,	-388 ,
	-342 ,	-295 ,	-247 ,	-199 ,	-150 ,	-101 ,	-52 ,	-3 ,
};

#define DAC_CMD_LOAD 0x0
#define DAC_CMD_LOADLATCH 0x2

/* stolen from Linux kernel -> GPL */
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

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
	p->max_v = cfg->max_v;
	p->dma_elem_num = cfg->dma_elem_num;
	p->dmaspi = cfg->dmaspi;
	p->dma_buffer = (piezo_dma_buf_t*)calloc(cfg->dma_elem_num,
						 sizeof(piezo_dma_buf_t));
	if (!p->dma_buffer)
		return -ENOMEM;

	return 0;
}


static void piezo_dma_update(piezo_handle_t *p, int half)
{
	int i, j;
	uint32_t val[4];
	piezo_dma_buf_t *buf;
	uint32_t cmd;
	int vel = ACCESS_ONCE(p->v);
	int len = p->dma_elem_num / 2;
	int _shift = fls(ARRAY_SIZE(PIEZO_PHASETABLE)) - 1;
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
		val[0] = PIEZO_PHASETABLE[( p->phase >> shift) & mask];
		val[1] = PIEZO_PHASETABLE[( ~p->phase >> shift) & mask];
		val[2] = PIEZO_PHASETABLE[(( p->phase + 2147483648UL) >>
					   shift) & mask];
		val[3] = PIEZO_PHASETABLE[((~p->phase + 2147483648UL) >>
					   shift) & mask];

		for (j = 0; j < 4; j++) {
			cmd = (j == 3) ? DAC_CMD_LOAD : DAC_CMD_LOADLATCH;
			buf[i].DAC[j] = PIEZO_SETDACVALUE(cmd, j, val[j]);
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

	dmaspi_start_cyclic(p->dmaspi,
			    p->dma_buffer, sizeof(PIEZO_PHASETABLE),
			    piezo_dma_half_cb, piezo_dma_complete_cb, p);
}

void piezo_stop(piezo_handle_t *p)
{
	if (!p->running)
		return;

	dmaspi_stop_cyclic(p->dmaspi);
}

int piezo_set_v(piezo_handle_t *p, int v)
{
	if (v > p->max_v)
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
