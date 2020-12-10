#include <stdio.h>

#define FILE_NAME "test_move_out.csv"

int _vel;

extern int motor_move(int motor, int max, int min, int pos, int direction);
void piezoSetStepFrequency(int motor, int vel)
{
    _vel = vel;
}



int main()
{
	int i;
	int direction = 1;
	double pos = 0;
	FILE *f = fopen(FILE_NAME, "w");

	for (i = 0; i < 200000; i++) {
		direction = motor_move(0, 14324, 11, pos, direction);

		pos += _vel * 0.01;

		fprintf(f, "%f, %d\n", pos, _vel);
	}
	fclose(f);
	printf("written "FILE_NAME"\n");
}
