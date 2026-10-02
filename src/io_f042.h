#ifndef IO_F042_H_INCLUDED
#define IO_F042_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include "stm32f0xx.h"
//----------------------------------------------
// ALL IN ONE REG,  USE -> (OUT | PP | SP_VHI | NO_PULL)
// GPIOx_MODER -> ((X >> 0) & 0x03)
#define GET_GPIOx_MODER(x)  (x & 0x03)
#define IN          (0x00)
#define OUT         (0x01)
#define ALT         (0x02)
#define AI          (0x03)
// GPIOx_OTYPER -> ((X >> 2) & 0x01)
#define GET_GPIOx_OTYPER(x)  ((x >> 2) & 0x01)
#define PP          (0x00)
#define OD          (0x04)
//GPIOx_OSPEEDR -> ((X >> 3) & 0x03)
#define GET_GPIOx_OSPEEDR(x)  ((x >> 3) & 0x03)
#define SP_LOW      (0x00)
#define SP_MED      (0x08)
#define SP_HI       (0x18)
//GPIOx_PUPDR -> ((X >> 5) & 0x03)
#define GET_GPIOx_PUPDR(x)  ((x >> 5) & 0x03)
#define NO_PULL     (0x00)
#define PULL_UP     (0x20)
#define PULL_DOWN   (0x40)
//----------------------------------------------

// GPIOx_AFRL
#define AF_0   (0x00)
#define AF_1   (0x01)
#define AF_2   (0x02)
#define AF_3   (0x03)
#define AF_4   (0x04)
#define AF_5   (0x05)
#define AF_6   (0x06)
#define AF_7   (0x07)


typedef struct
{
    GPIO_TypeDef* gpio_x;
    uint16_t gpio_pin;
    uint8_t mode;
    uint8_t af;
    uint8_t def_state;
    uint8_t active_state;
} io_gpio_line_t;

typedef enum
{
    OFF = 0,
    ON = 1,
    LOW = 0,
    HIGH =1,
} io_state_t;

#define ADC_CHANNEL_BUF_LEN                 9

//          NAME        GPIOx   GPIO_Pin            MODE                AF      DefState    ActiveState
#define IO_TABLE\
    X_IO(IO_DO_1,           GPIOA,  0,      (OUT | PP | SP_HI | NO_PULL),  AF_0,   0,  HIGH) \
    X_IO(IO_DO_2,           GPIOA,  1,      (OUT | PP | SP_HI | NO_PULL),  AF_0,   0,  HIGH) \
    X_IO(IO_UART_TX,        GPIOA,  2,      (ALT | PP | SP_HI | NO_PULL),  AF_1,   0,  LOW)	\
    X_IO(IO_UART_RX,        GPIOA,  3,      (ALT | PP | SP_HI | NO_PULL),  AF_1,   0,  LOW)	\
    X_IO(IO_RS485_SWITCH,   GPIOA,  4,      (OUT | PP | SP_HI | NO_PULL),  AF_0,   0,  HIGH) \
    X_IO(IO_ADC_CURRENT,    GPIOA,  5,      (AI  | NO_PULL),               AF_0,   0,  HIGH) \
    X_IO(IO_LED_GREEN,      GPIOA,  6,      (OUT | PP | SP_HI | NO_PULL),  AF_0,   0,  HIGH) \
    X_IO(IO_LED_RED,        GPIOA,  7,      (OUT | PP | SP_HI | NO_PULL),  AF_0,   0,  HIGH) \


typedef enum
{
#define X_IO(a,b,c,d,e,f,g)	a,
    IO_TABLE
#undef X_IO
    NUM_IO		//count
} io_line_t;

typedef enum {
	IO_OK = 0,
	IO_ERROR,
	IO_FLASH_LOCK_NOW
} io_result_t;

void io_clock_init(void);
void io_init();
void io_set_line_active(io_line_t line, bool state);
void io_set_line(io_line_t line, bool state);
bool io_get_line(io_line_t line);
uint16_t io_get_adc_val();
void IWDG_init_500ms(void);
io_result_t io_flash_protect(void);
#endif /* IO_F042_H_INCLUDED */
