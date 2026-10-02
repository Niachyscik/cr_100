#include "main.h"
#include "io_f042.h"
#include "timer_dispatcher.h"
#include "modbus_hard.h"
#include "modbus.h"
#include "modbus_reg.h"
#include "modbus_cb.h"
#include "led.h"
#include "usb_device.h"
//-----------------------------------------------------------------------
// print module
//-----------------------------------------------------------------------
#define LOG_MODULE  main
#include "log_nel.h"

//-----------------------------------------------------------------------
// variable
//-----------------------------------------------------------------------
extern uint16_t mb_buf_main[];
extern PCD_HandleTypeDef hpcd_USB_FS;
volatile uint32_t adc_timer = 0, adc_busy = 0, adc_offset = 0;
volatile int32_t    adc_filtered = 0, adc_mid, adc_rms;
volatile int32_t    adc_gain = 0;
static uint32_t calib_flag=0;
volatile uint32_t current_rms_A = 0;
uint16_t trig=0, trig_working=0, trig_timer=0;

//-----------------------------------------------------------------------
// prototype
//-----------------------------------------------------------------------
void adc_iteration(void);
void calib_0_task (void);
void calib_1_task (void);

//-----------------------------------------------------------------------
// Interrupt
//-----------------------------------------------------------------------
void SysTick_Handler()
{
    td_time_handler();
}
//-----------------------------------------------------------------------
void USB_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_FS);
}
//-----------------------------------------------------------------------
void DMA1_Channel1_IRQHandler()
{
    if (LL_DMA_IsActiveFlag_TC1(DMA1))
    {
        LL_DMA_ClearFlag_TC1(DMA1);
    }
    if (LL_DMA_IsActiveFlag_TE1(DMA1))
    {
        LL_DMA_ClearFlag_TE1(DMA1);
    }
    adc_busy = 1;
}
//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
void td_init_rtos_clock(void)
{
    SysTick_Config(SystemCoreClock/SYSTIMER_TICK);
}

uint32_t isqrt(uint64_t n)
{
    uint64_t res = 0;
    uint64_t bit = (uint64_t)1 << 62;
    while (bit > n) bit >>= 2;
    while (bit != 0)
    {
        if (n >= res + bit)
        {
            n -= res + bit;
            res = (res >> 1) + bit;
        }
        else
        {
            res >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)res;
}


//-----------------------------------------------------------------------
void calibration_cb(void)
{
    if (calib_flag != 0)
        return;

    if (mb_buf_main[REG_CALIB_START] ==  CALIB_0_CODE)
    {
        td_set_timer_task(calib_0_task, 2000);
        //io_set_line(IO_LED_RED, 1);
        calib_flag = 1;
        return;
    }
    else if (mb_buf_main[REG_CALIB_START] ==  CALIB_1_CODE)
    {
        td_set_timer_task(calib_1_task, 2000);
        //io_set_line(IO_LED_RED, 1);
        calib_flag = 1;
        return;
    }
    mb_buf_main[REG_CALIB_START] = 0;
    LOG_INFO("calibration_cb");
}
//-----------------------------------------------------------------------
void trig_reset_cb(void)
{
    if (mb_buf_main[REG_TRIG_NOTIFY_RESET] )
    {
        mb_buf_main[REG_TRIG_NOTIFY] = mb_buf_main[REG_TRIG];
        mb_buf_main[REG_TRIG_NOTIFY_RESET] = 0;
    }
}


//-----------------------------------------------------------------------
void do_iteration(void)
{
    static uint32_t mode_do[DO_NUM_CHANNELS] = {0};
    uint16_t temp_mask = 0;
    for (uint32_t i = 0; i < DO_NUM_CHANNELS; i++)
    {
        // reset if change output mode
        if(mode_do[i] != mb_buf_main[REG_DO_1_MODE+i])
        {
            mode_do[i] = mb_buf_main[REG_DO_1_MODE+i];
            io_set_line((IO_DO_1+i), LOW);
            mb_buf_main[REG_DO_STATUS] &= ~(1<<i);
            mb_buf_main[REG_DO_ON] = 0;
            mb_buf_main[REG_DO_OFF] = 0;
            break;
        }
        // mode 0 -> modbus control
        switch (mode_do[i])
        {
        case DO_MODE_MODBUS:
            if(mb_buf_main[REG_DO_ON] || mb_buf_main[REG_DO_OFF])
            {
                temp_mask = mb_buf_main[REG_DO_ON] & (~mb_buf_main[REG_DO_OFF]);
                if ((temp_mask >> i) & 1)
                    io_set_line((IO_DO_1+i), HIGH);
                if ((mb_buf_main[REG_DO_OFF]>>i) & 1)
                    io_set_line((IO_DO_1+i), LOW);
            }
            break;
        case DO_MODE_ALARM:
            if ((mb_buf_main[REG_TRIG] >> i) & 1)
                io_set_line_active((IO_DO_1+i), HIGH);
            else
                io_set_line_active((IO_DO_1+i), LOW);
            break;
        case DO_MODE_NOTIFY_ALARM:
            if ((mb_buf_main[REG_TRIG_NOTIFY] >> i) & 1)
                io_set_line_active((IO_DO_1+i), HIGH);
            else
                io_set_line_active((IO_DO_1+i), LOW);
            break;
        case DO_MODE_READY:
            if ((mb_buf_main[REG_TRIG] >> i) & 1)
                io_set_line_active((IO_DO_1+i), LOW);
            else
                io_set_line_active((IO_DO_1+i), HIGH);
            break;
        case DO_MODE_NOTIFY_READY:
            if ((mb_buf_main[REG_TRIG_NOTIFY] >> i) & 1)
                io_set_line_active((IO_DO_1+i), LOW);
            else
                io_set_line_active((IO_DO_1+i), HIGH);
            break;
        default:
            break;
        }
        if(io_get_line(IO_DO_1+i))	mb_buf_main[REG_DO_STATUS] |= 1<<i;
        else mb_buf_main[REG_DO_STATUS] &= ~(1<<i);
    }
    mb_buf_main[REG_DO_OFF] = 0;
    mb_buf_main[REG_DO_ON] = 0;
}

//-----------------------------------------------------------------------
void adc_iteration(void)
{
    static uint64_t adc_sum_rms = 0;
    static int32_t adc_measure_count = 0;
    static int32_t adc_intermediate_fp = 0;
    static bool is_initialized = false;
    uint64_t mean_square, current_square;
    int32_t adc_raw_val;
    int32_t adc_value;

    adc_raw_val = (int32_t)io_get_adc_val();

    if (!is_initialized)
    {
        adc_filtered = adc_raw_val;
        adc_intermediate_fp = adc_raw_val << 16;
        adc_offset = (mb_buf_main[REG_CALIB_OFFSET_W1] & 0xFFFF) | ((mb_buf_main[REG_CALIB_OFFSET_W2] & 0xFFFF) << 16);
        adc_gain =   (mb_buf_main[REG_CALIB_GAIN_W1] & 0xFFFF) | ((mb_buf_main[REG_CALIB_GAIN_W2] & 0xFFFF) << 16);
        adc_timer = main_timer_set(ADC_TIMER_MS);
        is_initialized = true;
    }
    else
    {
        adc_intermediate_fp += (adc_raw_val - adc_filtered) * ADC_FILTER_RATIO;
        adc_filtered = adc_intermediate_fp >> 16;
    }

    // 1. Accumulate the current sample first so it is included in the expiring block
    adc_measure_count++;
    adc_value = adc_raw_val - adc_offset;
    current_square = (uint64_t)((int64_t)adc_value * adc_value);
    adc_sum_rms += current_square;

    // 2. Check if the block timing window has finished
    if (timer_is_expired(adc_timer))
    {
        adc_timer = main_timer_set(ADC_TIMER_MS);
        if (adc_measure_count > 0)
        {
            // Changed format specifier to %ld to match signed int32_t
            LOG_INFO("adc_measure_count = %ld", (long)adc_measure_count);

            mean_square = adc_sum_rms / (uint64_t)adc_measure_count;
            adc_rms = isqrt(mean_square);
            LOG_INFO("adc_rms (block) = %lu", (unsigned long)adc_rms);
        }

        // 3. Cleanly clear the accumulators ONLY after processing the block
        adc_sum_rms = 0;
        adc_measure_count = 0;
    }
}


//-----------------------------------------------------------------------
// Task
//-----------------------------------------------------------------------
void Task1 (void)
{
    static uint32_t do_timer[DO_NUM_CHANNELS]= {0}, do_timer_flag[DO_NUM_CHANNELS]= {0}, do_timer_reset_flag[DO_NUM_CHANNELS]= {0};

    td_set_timer_task(Task1, 205);
    current_rms_A = (uint32_t)(((uint64_t)adc_rms * adc_gain) >> 16);
    LOG_INFO("current_rms_A = %ld", current_rms_A);

    mb_buf_main[REG_CUR_RMS] = current_rms_A;
    mb_buf_main[REG_ADC_RMS]= adc_rms;
    mb_buf_main[REG_CUR_MID_FILTERED]= (uint32_t)(((uint64_t)(adc_filtered - adc_offset) * adc_gain) >> 16);
    mb_buf_main[REG_ADC_FILTERED]= adc_filtered;

    // compare
    for (uint32_t i = 0, j = 0; i < DO_NUM_CHANNELS; i++, j=j+DO_SETTING_OFFSET)
    {
        if(current_rms_A >= mb_buf_main[REG_SETPOINT_CUR_1_A + j] && current_rms_A >= mb_buf_main[REG_SETPOINT_CUR_1_B + j])
        {
            if (!(trig & (1<<i)))
            {
                if (!do_timer_flag[i])
                {
                    do_timer[i] = main_timer_set(mb_buf_main[REG_SETPOINT_TIME_100MS_1_A] * 100 + DO_MIN_TIMER_MS);
                    do_timer_flag[i] = 1;
                    trig_timer |= 1<<i;
                }
                else if(timer_is_expired(do_timer[i]))
                {
                    trig |= 1<<i;
                }
            }
            trig_working |=1<<i;
            do_timer_reset_flag[i] = 0;
        }
        else if (current_rms_A >= mb_buf_main[REG_SETPOINT_CUR_1_A + j] || current_rms_A >= mb_buf_main[REG_SETPOINT_CUR_1_A + j])
        {
            trig_working |= 1<<i;
            trig_timer &= ~(1<<i);
            do_timer_flag[i] = 0;
            do_timer_reset_flag[i] = 0;
        }
        else
        {
            if (trig & 1<<i)
            {
                if (!do_timer_reset_flag[i])
                {
                    do_timer[i] = main_timer_set(mb_buf_main[REG_SETPOINT_TIME_100MS_1_B + j] * 100 + DO_MIN_TIMER_MS);
                    do_timer_reset_flag[i] = 1;
                }
                else if(timer_is_expired(do_timer[i]))
                {
                    trig &= ~(1<<i);
                }
            }
            trig_working &= ~(1<<i);
            trig_timer &= ~(1<<i);
            do_timer_flag[i] = 0;
        }
    }

    //  mb data
    mb_buf_main[REG_TRIG] = trig;
    mb_buf_main[REG_TRIG_NOTIFY] |= trig;
    mb_buf_main[REG_TRIG_WORKING] = trig_working;

    //led blink
    if (trig & 1)
    {
        led_set_permanent_mode(LED_RED, LED_SIMPLE_MODE, 1, 1000);
    }
    else if(trig_timer & 1)
    {
         led_set_permanent_mode(LED_RED, LED_BLINK_MODE, 1, 100);
    }
    else if (trig_working & 1)
    {
        led_set_permanent_mode(LED_RED, LED_BLINK_MODE, 1, 500);
    }
    else
    {
        led_set_permanent_mode(LED_RED, LED_SIMPLE_MODE, 0, 1000);
    }
    do_iteration();
    LL_IWDG_ReloadCounter(IWDG);
}



void Task2 (void)
{
    td_set_timer_task(Task2,10);
    led_dispatcher();
}

void calib_0_task (void)
{
    LOG_INFO("calib_0_task");

    adc_offset = adc_filtered;
    mh_buf_set_reg (REG_CALIB_OFFSET_W1, (uint16_t) (adc_offset & 0xFFFF));
    mh_buf_set_reg (REG_CALIB_OFFSET_W2, (uint16_t) ((adc_offset >>16) & 0xFFFF));

    calib_flag = 0;
    mb_buf_main[REG_CALIB_START] = 0;
    led_set_temp_mode(LED_RED, LED_BLINK_MODE, 1, 100, 2000);
    led_set_temp_mode(LED_GREEN, LED_BLINK_MODE, 0, 100, 2000);
}

void calib_1_task (void)
{
    LOG_INFO("calib_1_task");
    if (adc_rms != 0 && mb_buf_main[REG_CALIB_VALUE])
    {
        adc_gain = (uint32_t)(((uint64_t)mb_buf_main[REG_CALIB_VALUE] << 16) / adc_rms);
        mh_buf_set_reg (REG_CALIB_GAIN_W1, (uint16_t) (adc_gain & 0xFFFF));
        mh_buf_set_reg (REG_CALIB_GAIN_W2, (uint16_t) ((adc_gain >>16) & 0xFFFF));
        led_set_temp_mode(LED_RED, LED_BLINK_MODE, 1, 100, 2000);
        led_set_temp_mode(LED_GREEN, LED_BLINK_MODE, 0, 100, 2000);
    }
    mb_buf_main[REG_CALIB_START] = 0;
    calib_flag = 0;
}

void td_idle(void)
{
    if (adc_busy)
    {
        adc_busy = 0;
        adc_iteration();
    }
}
//-----------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------

int main(void)
{
    io_clock_init();
    io_init();
    mh_buf_init();
    MX_USB_DEVICE_Init();
    td_init_rtos();
    mb_buf_main[REG_VER] = VERSION;
    mh_modbus_init();
#ifndef DEBUG_TARGET
    IWDG_init_500ms();
    if (io_flash_protect() == IO_FLASH_LOCK_NOW)
        mh_factory();
#endif

    LOG_INFO("start");
    led_set_permanent_mode(LED_GREEN, LED_BLINK_MODE, 1, 500);
    led_set_permanent_mode(LED_RED, LED_SIMPLE_MODE, 0, 1000);
    td_set_timer_task(Task1, 200);
    td_set_timer_task(Task2, 200);
    //start adc
    LL_ADC_REG_StartConversion(ADC1);
    while(1)
    {
        td_task_manager();
    }
}
//-----------------------------------------------------------------------
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

void SystemClock_Config(void)
{

}
