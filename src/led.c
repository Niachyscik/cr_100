#include "led.h"

//--------------X macros---------------------------------------------------------
static led_t led_array[NUM_LED] =
{
#define X_LED(a,b)	{b, LED_MODE_OFF, LED_MODE_OFF, 0, LED_DEFAULT_INTERVAL_MS, LED_DEFAULT_INTERVAL_MS, 0, 0},
    LED_TABLE
#undef X_LED
};


void led_set_permanent_mode(uint8_t number, uint8_t mode, uint8_t state, uint32_t period)
{
    if (number >= NUM_LED)
        return;
    if (led_array[number].permanent_mode == mode &&
        led_array[number].permanent_state == state &&
        led_array[number].permanent_interval_ms == period)
    {
        return;
    }

    led_array[number].permanent_mode = mode;
    led_array[number].permanent_state = state;
    //led_array[number].temp_mode = LED_MODE_OFF;
    if (mode == LED_BLINK_MODE)
    {
        if (led_array[number].temp_mode == LED_MODE_OFF)
        {
            led_array[number].blink_timer = main_timer_set(period);
        }
        led_array[number].permanent_interval_ms = period;
    }
    else if (led_array[number].temp_mode == LED_MODE_OFF)
        io_set_line(led_array[number].io_number, state);

}

void led_set_temp_mode(uint8_t number, uint8_t mode, uint8_t state, uint32_t period, uint32_t temp_time)
{
    if (number >= NUM_LED)
        return;

    led_array[number].temp_mode = mode;
    led_array[number].temp_interval_ms = period;
    led_array[number].temp_timer_ms = main_timer_set(temp_time);
    if (mode == LED_BLINK_MODE)
    {
        led_array[number].blink_timer = main_timer_set(period);
    }
    io_set_line(led_array[number].io_number, state);
}

void led_dispatcher(void)
{
    for (int led = 0; led < NUM_LED; led++)
    {
        if(led_array[led].temp_mode != LED_MODE_OFF)
        {
            if(timer_is_expired (led_array[led].temp_timer_ms))
            {
                led_array[led].temp_mode = LED_MODE_OFF;
                io_set_line(led_array[led].io_number, led_array[led].permanent_state);
            }
            else if(led_array[led].temp_mode == LED_BLINK_MODE)
            {
                if(timer_is_expired (led_array[led].blink_timer))
                {
                  io_set_line(led_array[led].io_number, !io_get_line(led_array[led].io_number));
                  led_array[led].blink_timer = main_timer_set(led_array[led].temp_interval_ms);
                }
            }
        }
        else if(led_array[led].permanent_mode == LED_BLINK_MODE)
        {
            if(timer_is_expired (led_array[led].blink_timer))
            {
                io_set_line(led_array[led].io_number, !io_get_line(led_array[led].io_number));
                led_array[led].blink_timer = main_timer_set(led_array[led].permanent_interval_ms);
            }
        }
    }
}

