#ifndef __PWM_H
#define __PWM_H

#include <stddef.h>
#include <stdbool.h>

#define PWM_TIMER_DMA_BUFFER_SIZE 256

void pwm_data_timer_init();
void pwm_data_timer_dma_request_enable(bool enabled);
void pwm_data_timer_uninit();

void pwm_timer_init(uint32_t frequency_hz_100);
void pwm_timer_pwm_enable(bool enabled);
void pwm_timer_use(bool use);
void pwm_timer_uninit();
uint16_t pwm_calculate_period(uint32_t frequency_hz_100);
void pwm_timer_set_frequency(uint32_t frequency_hz_100);

void pwm_dma_init();
void pwm_dma_interrupt_enable(bool enabled);
void pwm_dma_start();
void pwm_dma_stop();

extern uint16_t (*pwm_handle_dma_transfer_half)(uint16_t buffer_size, uint16_t *buffer);
extern uint16_t (*pwm_handle_dma_transfer_full)(uint16_t buffer_size, uint16_t *buffer);

extern uint16_t pwm_timer_dma_buffer[PWM_TIMER_DMA_BUFFER_SIZE];

#endif
