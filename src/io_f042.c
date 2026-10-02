#include "io_f042.h"
//#include "stm32f0xx_hal.h"
#include "stm32f0xx.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_dma.h"
#include "stm32f0xx_ll_adc.h"
#include "stm32f0xx_ll_utils.h"
#include "stm32f0xx_ll_iwdg.h"
//------------------------------- prototype -------------------------------------
static void io_config_line(io_gpio_line_t io);
void adc_with_dma_init();
//--------------X macros---------------------------------------------------------

const io_gpio_line_t io_table_array[NUM_IO] =
{
#define X_IO(a,b,c,d,e,f,g)	{b,c,d,e,f,g},
    IO_TABLE
#undef X_IO
};
//--------------------------------- variable -------------------------------------

volatile uint16_t adc_data[ADC_CHANNEL_BUF_LEN];
//---------------------------------------------------------------------------------
void io_clock_init(void)
{
    LL_FLASH_EnablePrefetch();
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    // Enable the PA11/PA12 remap bit in the SYSCFG configuration register
    SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_PA11_PA12_RMP);
    //  SystemClock_Config
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
    while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1) {}
    LL_RCC_HSI14_Enable();

    /* Wait till HSI14 is ready */
    while(LL_RCC_HSI14_IsReady() != 1) {}
    LL_RCC_HSI14_SetCalibTrimming(16);
    LL_RCC_HSI48_Enable();

    /* Wait till HSI48 is ready */
    while(LL_RCC_HSI48_IsReady() != 1) {}
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI48);

    /* Wait till System clock is ready */
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI48) {}
    LL_SetSystemCoreClock(48000000);
    LL_RCC_HSI14_EnableADCControl();
    LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_HSI48);
}

//---------------------------------------------------------------------------------
void io_set_line(io_line_t line, bool state)
{
    if (state)
        io_table_array[line].gpio_x->BSRR = 1 << (io_table_array[line].gpio_pin);
    else
        io_table_array[line].gpio_x->BRR = 1 << (io_table_array[line].gpio_pin);
}
//---------------------------------------------------------------------------------
bool io_get_line(io_line_t line)
{
    if (line < NUM_IO)
        return (((io_table_array[line].gpio_x->IDR) & (1<<(io_table_array[line].gpio_pin))) != 0);
    else
        return false;
}
//---------------------------------------------------------------------------------
bool io_get_line_active(io_line_t line)
{
    if (line < NUM_IO)
    {
        bool pin_set = (((io_table_array[line].gpio_x->IDR) & (1<<(io_table_array[line].gpio_pin))) ? true : false);
        return (pin_set == ( io_table_array[line].active_state ? true : false));
    }
    else
        return false;
}
//---------------------------------------------------------------------------------
void io_set_line_active(io_line_t line, bool state)
{
    if (state ^ io_table_array[line].active_state)
        io_table_array[line].gpio_x->BRR = 1 << (io_table_array[line].gpio_pin);   //reset
    else
        io_table_array[line].gpio_x->BSRR = 1 << (io_table_array[line].gpio_pin);  //set
}
//---------------------------------------------------------------------------------
static void io_config_line(io_gpio_line_t io)
{
    io.gpio_x->MODER &= ~(0x03 << (io.gpio_pin * 2));
    io.gpio_x->MODER |= (GET_GPIOx_MODER(io.mode) << (io.gpio_pin * 2));

    io.gpio_x->OTYPER &= ~(0x01 << io.gpio_pin);
    io.gpio_x->OTYPER |= (GET_GPIOx_OTYPER(io.mode) << io.gpio_pin);

    io.gpio_x->OSPEEDR &= ~(0x03 << (io.gpio_pin * 2));
    io.gpio_x->OSPEEDR |= (GET_GPIOx_OSPEEDR(io.mode) << (io.gpio_pin * 2));

    io.gpio_x->PUPDR &= ~(0x03 << (io.gpio_pin * 2));
    io.gpio_x->PUPDR |= (GET_GPIOx_PUPDR(io.mode) << (io.gpio_pin * 2));

    if(io.gpio_pin < 8)
    {
        io.gpio_x->AFR[0] &=  ~(0x0F << (io.gpio_pin * 4));
        io.gpio_x->AFR[0] |=  io.af << (io.gpio_pin * 4);
    }
    else
    {
        io.gpio_x->AFR[1] &=  ~(0x0F << ((io.gpio_pin - 8) * 4));
        io.gpio_x->AFR[1] |=  io.af << ((io.gpio_pin - 8) * 4);
    }

    io.gpio_x->ODR &= ~(1 << io.gpio_pin);
    io.gpio_x->ODR |= (io.def_state << io.gpio_pin);
}
//---------------------------------------------------------------------------------
void io_init()
{
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    // Set all pins
    for (int line = 0; line < NUM_IO; line++)
    {
        io_config_line(io_table_array[line]);
    }
    adc_with_dma_init();
}
//---------------------------------------------------------------------------------
void adc_with_dma_init()
{
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_ADC1);

    LL_DMA_ConfigTransfer(DMA1, LL_DMA_CHANNEL_1, (LL_DMA_DIRECTION_PERIPH_TO_MEMORY |\
                          LL_DMA_PRIORITY_VERYHIGH|\
                          LL_DMA_MODE_CIRCULAR |\
                          LL_DMA_MEMORY_INCREMENT |\
                          LL_DMA_PERIPH_NOINCREMENT |\
                          LL_DMA_MDATAALIGN_HALFWORD |\
                          LL_DMA_PDATAALIGN_HALFWORD ));

    LL_ADC_REG_SetSequencerChAdd(ADC1, LL_ADC_CHANNEL_5);
    LL_ADC_SetClock(ADC1, LL_ADC_CLOCK_ASYNC);
    LL_ADC_SetResolution(ADC1, LL_ADC_RESOLUTION_12B);
    LL_ADC_SetDataAlignment(ADC1, LL_ADC_DATA_ALIGN_RIGHT);
    LL_ADC_SetLowPowerMode(ADC1, LL_ADC_LP_MODE_NONE);

    LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_SOFTWARE);
    LL_ADC_REG_SetSequencerDiscont(ADC1, LL_ADC_REG_SEQ_DISCONT_DISABLE);
    LL_ADC_REG_SetContinuousMode(ADC1, LL_ADC_REG_CONV_CONTINUOUS);
    LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);
    LL_ADC_REG_SetOverrun(ADC1, LL_ADC_REG_OVR_DATA_OVERWRITTEN);
    LL_ADC_REG_SetSequencerScanDirection(ADC1, LL_ADC_REG_SEQ_SCAN_DIR_FORWARD);
    LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_41CYCLES_5);//LL_ADC_SAMPLINGTIME_28CYCLES_5);

    LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_1,
                           LL_ADC_DMA_GetRegAddr(ADC1, LL_ADC_DMA_REG_REGULAR_DATA),
                           (uint32_t)&adc_data,
                           LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, ADC_CHANNEL_BUF_LEN);

    LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_1);
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_1);
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    NVIC_SetPriority(DMA1_Channel1_IRQn,1);

    if (LL_ADC_IsEnabled(ADC1) == 0)
    {
        LL_ADC_StartCalibration(ADC1);
        while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0) {}
    }

    LL_ADC_Enable(ADC1);
    while (LL_ADC_IsActiveFlag_ADRDY(ADC1) == 0) {}

    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);
    //LL_ADC_REG_StartConversion(ADC1);
}


uint16_t io_get_adc_val()
{
    int i, addr = 0;
#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
#define PIX_SWAP(a,b) {register uint16_t temp=(a);(a)=(b);(b)=temp; }
    uint16_t p[9];
    for(i = 0; i < 9; ++i, addr += 1) // first we should prepare array for optmed
        p[i] = adc_data[addr];
    PIX_SORT(p[1], p[2]) ;
    PIX_SORT(p[4], p[5]) ;
    PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[1]) ;
    PIX_SORT(p[3], p[4]) ;
    PIX_SORT(p[6], p[7]) ;
    PIX_SORT(p[1], p[2]) ;
    PIX_SORT(p[4], p[5]) ;
    PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[3]) ;
    PIX_SORT(p[5], p[8]) ;
    PIX_SORT(p[4], p[7]) ;
    PIX_SORT(p[3], p[6]) ;
    PIX_SORT(p[1], p[4]) ;
    PIX_SORT(p[2], p[5]) ;
    PIX_SORT(p[4], p[7]) ;
    PIX_SORT(p[4], p[2]) ;
    PIX_SORT(p[6], p[4]) ;
    PIX_SORT(p[4], p[2]) ;
    return p[4];
#undef PIX_SORT
#undef PIX_SWAP
}

void IWDG_init_500ms(void)
{

  LL_RCC_LSI_Enable();
  while (LL_RCC_LSI_IsReady() != 1)
  {
  }
  LL_IWDG_Enable(IWDG);
  LL_IWDG_EnableWriteAccess(IWDG);
  LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_64);
  LL_IWDG_SetReloadCounter(IWDG, 312);
  while (LL_IWDG_IsReady(IWDG) != 1)
  {
  }
  LL_IWDG_ReloadCounter(IWDG);
}

io_result_t io_flash_protect(void)
{
    FLASH_OBProgramInitTypeDef ob_init;

    // 1. Получаем текущую конфигурацию Option Bytes
    HAL_FLASHEx_OBGetConfig(&ob_init);

    // 2. Если уровень защиты равен Level 0 (заводской без защиты)
    if (ob_init.RDPLevel == OB_RDP_LEVEL_0)
    {
        // Разблокируем FLASH и интерфейс Option Bytes
        HAL_FLASH_Unlock();
        HAL_FLASH_OB_Unlock();

        // Настраиваем структуру на изменение уровня защиты RDP на Level 1
        ob_init.OptionType = OPTIONBYTE_RDP;
        ob_init.RDPLevel = OB_RDP_LEVEL_1;

        // Записываем новые Option Bytes
        if (HAL_FLASHEx_OBProgram(&ob_init) != HAL_OK)
        {
           // LOG_ALARM("io_flash_protect error");

            // В случае ошибки закрываем доступ к FLASH и выходим
            HAL_FLASH_OB_Lock();
            HAL_FLASH_Lock();
            return IO_ERROR; // Или другой статус ошибки из вашего enum
        }

        // Блокируем доступ обратно
        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();

        // Перезагружаем Option Bytes (это вызовет немедленный системный Reset)
        HAL_FLASH_OB_Launch();

        // Эта строчка выполнится только в редком случае, если Launch дал сбой
        return IO_FLASH_LOCK_NOW;
    }

    // Если защита уже была включена ранее (уровень 1 или 2)
    return IO_OK;
}



