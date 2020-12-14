#include <stdio.h>

#define FILE_NAME "test_move_out.csv"

int _vel;

int motor_move(int max, int min, int pos, int maxvel,
               void *state);

int main()
{
	int i;
	int vel;
	float pos = 0;
	struct {
		int direction;
		int start;
	} state = {0, 0};

	FILE *f = fopen(FILE_NAME, "w");

	for (i = 0; i < 20000; i++) {
		vel = motor_move(5000, 1000, pos, 500, &state);

		pos += (float)vel * 0.01;

		fprintf(f, "%f, %d\n", pos, vel);
	}
	fclose(f);
	printf("written "FILE_NAME"\n");
}
