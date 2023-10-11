#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "config.h"
#include "millis.h"
#include "timepulse.h"

// This define copied from .../src/hal/stm_lib/src/stm32f10x_rcc.c
#define CR_HSITRIM_Mask           ((uint32_t)0xFFFFFF07)

// Define below pulls the current trim register value out of the Calibration Register (CR)
// Resulting value will be between 0-31.

#define CURRENT_TRIM	((RCC->CR & ~CR_HSITRIM_Mask) >>3)

/*
On the DFM-17, PB8 is wired to the GPS Timepulse.  We take advantage of this to do a 
processor speed calibration.  HSITRIM[4:0] allows for 32 values to adjust the HSI clock
speed.  The center (16) value is "neutral".  Each trim value above or below 16 adjusts
the clock by approximately 40kHZ (0.5% of the 8MHZ clock speed) (per AN2868).  0.5% is about 
5ms per second, so if we detect that we're off by more than 5 millisconds between timepulses, we 
will suggest a recalibration.  The "calib_suggestion" variable is a static that will be maintained 
by the timepulse IRQ and can be used at any time it's convenient to adjust the clock speed.
*/

int calib_suggestion = 16;  // Default, but we will check it in the init routine below.
uint32_t old_millis = 0;
volatile int timepulsed = 0;
volatile uint32_t d_millis = 0;


void timepulse_init(void)
{
    // Initialize pin PB8 as floating input
    GPIO_InitTypeDef gpio_init;
    gpio_init.GPIO_Pin = GPIO_Pin_8;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_init.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOB, &gpio_init);

    // PB8 is connected to interrupt line 8, set trigger on the configured edge and enable the interrupt
    EXTI_InitTypeDef exti_init;
    exti_init.EXTI_Line = EXTI_Line8;
    exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = EXTI_Trigger_Rising;
    exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);

    // Attach interrupt line to port B
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8);

    // PB8 is connected to EXTI_Line8, which has EXTI9_5_IRQn vector. Use priority 0 for now.
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    // Pull the current calibration to start
    calib_suggestion = CURRENT_TRIM;		
}

// This handler is (at present) only being used for the Timepulse interrupt, so we shouldn't need
// to do additional testing for the cause of the interrupt.

void EXTI9_5_IRQHandler(void)
{
    uint32_t	m = millis();
    int		delta;

    EXTI_ClearITPendingBit(EXTI_Line8);
    timepulsed++;
    if (old_millis == 0) {
       old_millis = m;		// First timepulse.  Just store millis.
    } else {
       d_millis = m - old_millis;
       delta = (int) (1000 - d_millis) / 5;	
       if (((delta + calib_suggestion) >= 0) &&
           ((delta + calib_suggestion <= 31)) ) {
          // If the delta makes sense, apply to the suggestion.  Otherwise, skip.
	  calib_suggestion += delta;
       }
       old_millis = m;
    }
}
