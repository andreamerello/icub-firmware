#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#define FILE_NAME "test_move_out.csv"

int motor_move(int max, int min, int pos, int maxvel,
               void *state);

int main()
{
	int i;
	int vel;
	int _pos;
	float pos, prev_pos;
	int boot;
	int max = 5000;
	int min = 1000;
	int tolerance = 2;
	int err = 0;
	int delta;
	int prev_delta;
	int oscillations;
	volatile int end = 0;
	FILE *f = NULL;

	struct {
		int direction;
		int start;
	} state = {0, 0};


	void bail(int signo) {
		fprintf(stderr, "BAILING OUT\n");
		end = 1;
	}

	signal(SIGINT, bail);

	f = fopen(FILE_NAME, "w");
	for (_pos = 0; _pos <= max * 2; _pos++) {
		oscillations = 0;
		prev_delta = 0;
		delta = 0;
		printf("testing start pos %d/%d\n", _pos, max * 2);
		fseek(f, 0, SEEK_SET);
		pos = _pos;
		boot = 1;
		for (i = 0; i < 20000; i++) {
			vel = motor_move(max, min, (int)pos, 500, &state);
			prev_pos = pos;
			pos += (float)vel * 0.01;

			fprintf(f, "%f, %d\n", pos, vel);
			if (boot && (_pos > max) && (pos < prev_pos))
				continue;

			if (boot && (_pos < min) && (pos > prev_pos))
				continue;
			boot = 0;

			if (pos > (max + tolerance)) {
				printf("err: pos (%f) > max (%d)\n",
				       pos, max);
				err = 1;
				break;
			}

			if (pos < (min - tolerance)) {
				printf("err: pos (%f) < min (%d)\n",
				       pos, min);
				err = 1;
				break;
			}

			if (pos > prev_pos)
				delta = 1;
			if (pos < prev_pos)
				delta = -1;

			if (delta && prev_delta && (prev_delta != delta))
				oscillations++;
			prev_delta = delta;

		}

		if (oscillations < 3) {
			printf("didn't oscillate enough?\n");
			break;
		}
		if (err || end)
			break;
	}
	fclose(f);
	printf("written "FILE_NAME"\n");
}
