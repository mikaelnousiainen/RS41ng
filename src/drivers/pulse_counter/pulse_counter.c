#include "pulse_counter.h"
#include "stm32f1xx.h"
#include "stm32f1xx_exti.h"
#include "misc.h"


bool pulse_counter_init()
{
	//TODO: start interrupt handling, attach it to the designated pin.
    return true;
}

uint16_t pulse_counter_get_counts(){
	//TODO: Retrieve total counts and return it.
	return 1666;
}