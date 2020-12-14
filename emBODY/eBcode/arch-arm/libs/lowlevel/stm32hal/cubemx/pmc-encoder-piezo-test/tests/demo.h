
#define abs(a) (((a) > 0) ? (a) : (-(a)))

typedef struct
{
	void *htim;
} qe_encoder_cfg_t;

typedef struct
{
	void *a;
} qe_encoder_t;

typedef struct {
	void *phaseTable;
	int phaseTableLen;
}  piezoMotorCfg_t;

#define LED_TOGGLE(...)
#define LED_ON(...)
#define LED_OFF(...)
#define HAL_OK 0

#define delta_8192 NULL
#define delta_1024 NULL
#define rhomb_1024 NULL

int htim2 = 0;
int htim5 = 0;

#define piezoInit(...) HAL_OK
#define piezoSetStepFrequency(...)
#define lr17_encoder_init(...) HAL_OK
#define lr17_encoder_get(...) 0
#define lr17_encoder_acquire(...)
#define qe_encoder_init(...) HAL_OK
#define qe_encoder_get(...) 0
#define osDelay(...)
