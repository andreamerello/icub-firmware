#include <stdio.h>

#define TURN_THR 1000

extern int count_turn(int val, int prev, int max);

int main()
{
	int val;
	int enc_val, prev_val = 0;
	int test_val;
	int turn = 0;
	int max = 0x7fff;
	int pass = 1;

	for (val = 0; val < 100000; val += 100) {
		enc_val = val & max;
		turn += (max + 1) * count_turn(enc_val, prev_val, max);
		prev_val = enc_val;
		test_val = enc_val + turn;
		if (test_val != val) {
			printf("conter failure: %d %d (%d)\n", test_val, val, turn);
			pass = 0;
		}
	}

	for (;val > -100000; val -= 80) {
		enc_val = val & max;
		turn += (max + 1) * count_turn(enc_val, prev_val, max);
		prev_val = enc_val;
		test_val = enc_val + turn;
		if (test_val != val) {
			printf("conter failure(2): %d %d (%d)\n", test_val, val, turn);
			pass = 0;
		}
	}
	printf("test %s\n", pass ? "PASSED" : "FAILED");
}
