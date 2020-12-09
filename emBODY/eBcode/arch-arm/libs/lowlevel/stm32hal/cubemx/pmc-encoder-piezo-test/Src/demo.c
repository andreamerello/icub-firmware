#include "demo.h"
#include "leds.h"

void demo_main(void)
{
    while(1) {
        LED_TOGGLE(LED_ORANGEPORT, LED_ORANGE0);
        osDelay(100);
    }
}
