#ifndef LED_H_INCLUDED
#define LED_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include "timer_dispatcher.h"
#include "io_f042.h"

#define LED_DEFAULT_INTERVAL_MS     250


//          NAME            number
#define LED_TABLE\
    X_LED(LED_GREEN,    IO_LED_GREEN) \
    X_LED(LED_RED,      IO_LED_RED) \


typedef struct
{
    uint8_t io_number;
    uint8_t permanent_mode;
    uint8_t temp_mode;
    uint8_t permanent_state;
    uint32_t permanent_interval_ms;
    uint32_t temp_interval_ms;
    uint32_t temp_timer_ms;
    uint32_t blink_timer;
} led_t;

typedef enum
{
#define X_LED(a,b)	a,
    LED_TABLE
#undef X_LED
    NUM_LED		//count
} led_inst_name_t;

typedef enum
{
    LED_MODE_OFF = 0,
    LED_SIMPLE_MODE,
    LED_BLINK_MODE,
} led_mode_t;

void led_dispatcher(void);
void led_set_permanent_mode(uint8_t number, uint8_t mode, uint8_t state, uint32_t period);
void led_set_temp_mode(uint8_t number, uint8_t mode, uint8_t state, uint32_t period, uint32_t temp_time);

#endif /* LED_H_INCLUDED */
