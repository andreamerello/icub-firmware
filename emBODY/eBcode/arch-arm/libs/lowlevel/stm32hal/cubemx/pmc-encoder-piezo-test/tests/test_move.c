#include <stdio.h>

int motor_max_vel[] = {123};
int motor_ramp[] = {0};
int motor_ramp_steepness[] = {1};

int _vel;
void piezoSetStepFrequency(int motor, int vel)
{
    _vel = vel;
}

int motor_move(int motor, int max, int min, int pos, int direction)
{
    int vel;

    if (((direction == 1) && (pos > max)) ||
        ((direction == -1) && (pos < min))) {
            direction *= -1;
    }

    if ((direction == 1) && (motor_ramp[motor] < motor_max_vel[motor]))
        motor_ramp[motor] += motor_ramp_steepness[motor];
    else if ((direction == -1) && (motor_ramp[motor] > -motor_max_vel[motor]))
        motor_ramp[motor] -= motor_ramp_steepness[motor];

    vel = motor_ramp[motor];

    piezoSetStepFrequency(motor, vel);

    return direction;
}



int main()
{
	int i;
	int direction = 1;
	double pos = 0;
	FILE *f = fopen("test_move_out.csv", "w");

	for (i = 0; i < 200000; i++) {
		direction = motor_move(0, 14324, 11, pos, direction);

		pos += _vel * 0.01;

		fprintf(f, "%f, %d\n", pos, _vel);
	}
	fclose(f);
}
