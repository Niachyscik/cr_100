#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f0xx_hal.h"
#include "stm32f0xx_ll_conf.h"

#define ADC_TIMER_MS                100
#define ADC_OFFSET_DEFAULT          2187 //(2.5  -> 1.76 -> 2187)
#define ADC_FILTER_RATIO            10


#define CALIB_0_CODE                1000
#define CALIB_1_CODE                1001

#define DO_NUM_CHANNELS                 2
// DO_Mode
 #define  DO_MODE_OFF                   0
 #define  DO_MODE_MODBUS                1
 #define  DO_MODE_ALARM                 2
 #define  DO_MODE_NOTIFY_ALARM          3
 #define  DO_MODE_READY                 4
 #define  DO_MODE_NOTIFY_READY          5

#define DO_MIN_TIMER_MS                 10
#define DO_SETTING_OFFSET               4   // (modbus config)

#define VERSION                         101

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H_INCLUDED */
