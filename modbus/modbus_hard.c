#include <stdbool.h>
#include "inttypes.h"
#include <stdio.h>

#include "modbus_hard.h"
#include "modbus.h"
#include "modbus_reg.h"
#include "modbus_cb.h"
#include <stm32f0xx_ll_bus.h>
#include <stm32f0xx_ll_usart.h>

#include "main.h"
#include "io_f042.h"
#include "timer_dispatcher.h"
#include <eeprom_emulation.h>
#include "usbd_cdc_if.h"
//-----------------------------------------------------------------------
// print module
//-----------------------------------------------------------------------
//#define LOG_MODULE  MODBUS_HARD
#include "log_nel.h"
//-----------------------------------------------------------------------
// Variable
//-----------------------------------------------------------------------
uint16_t mb_buf_main[MB_NUM_BUF]= {0};
const uint32_t baud_rate[BAUD_NUMBER]= RS_485_BAUD_LIST;

//-----------------------------------------------------------------------
// Prototype
//-----------------------------------------------------------------------
static void mh_enable_transmission(const bool enable);
static void mh_rs485_transmit_start (mb_slave_t *p_instance);
static void mh_usb_transmit_start (mb_slave_t *p_instance);
static void io_uart2_init(void);
static void mh_task_modbus_rs485 (void);
static void mh_task_modbus_usb (void);
//-----------------------------------------------------------------------
// create MODBUS instance
//-----------------------------------------------------------------------
MB_SLAVE_RTU_WITH_BUF_INSTANCE_DEF(mb_rs485,\
                                   mb_buf_main,\
                                   mh_write_cb,\
                                   mh_rs485_transmit_start,\
                                   NULL)

MB_SLAVE_RTU_WITH_BUF_INSTANCE_DEF(mb_usb,\
                                   mb_buf_main,\
                                   mh_write_cb,\
                                   mh_usb_transmit_start,\
                                   NULL)

//-----------------------------------------------------------------------
// Task function
//-----------------------------------------------------------------------
static void mh_task_modbus_rs485 (void)
{
    LOG_INFO("mh_task_modbus");
    mb_parsing((mb_slave_t*)&mb_rs485);
}

static void mh_task_modbus_usb (void)
{
    LOG_INFO("mh_task_modbus_usb");
    mb_parsing((mb_slave_t*)&mb_usb);
}
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Interrupt function
//-----------------------------------------------------------------------
void USART2_IRQHandler (void)
{
    uint8_t cnt;
    (void) cnt;
    if (USART2->ISR & (USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE))
    {
        mb_rs485.er_frame_bad = EV_HAPPEND;
        cnt = USART2->RDR;
        LL_USART_ClearFlag_ORE(USART2);
        LL_USART_ClearFlag_FE(USART2);
        LL_USART_ClearFlag_NE(USART2);
    }
    if (USART2->ISR & USART_ISR_RXNE)
    {
        if( MB_STATE_RCVE == mb_rs485.mb_state)
        {
            if(mb_rs485.mb_index >= MB_FRAME_MAX-1)
            {
                mb_rs485.er_frame_bad = EV_HAPPEND;	                 // This error will be processed later
                cnt = USART2->RDR;                        // Nothing more to do in RECEIVE state
            }
            else
            {
                mb_rs485.p_mb_buff[mb_rs485.mb_index++] = USART2->RDR;	 // MAIN DOING: New byte to buffer
            }
        }
        else if(mb_instance_idle_check((mb_slave_t*)&mb_rs485)==MB_OK)
        {
            // 1-st symbol come!
            mb_rs485.p_mb_buff[0] = USART2->RDR; 		// Put it to buffer
            mb_rs485.mb_index = 1;						// "Clear" the rest of buffer
            mb_rs485.er_frame_bad = EV_NOEVENT;			// New buffer, no old events
            mb_rs485.mb_state=MB_STATE_RCVE;				// MBMachine: begin of receiving the request
        }
        else
        {
            cnt = USART2->RDR;
        }
    }
    if (USART2->ISR & USART_ISR_TC)
    {
        LL_USART_ClearFlag_TC(USART2);
        mb_rs485.mb_state = MB_STATE_IDLE;
        mh_enable_transmission(false);
    }
    if (USART2->ISR & USART_ISR_TXE)
    {
        if( MB_STATE_SEND == mb_rs485.mb_state)
        {
            if( mb_rs485.mb_index < mb_rs485.response_size)
            {
                USART2->TDR = mb_rs485.p_mb_buff[mb_rs485.mb_index++];   //  sending of the next byte
            }
            else
            {
                mb_rs485.mb_state=MB_STATE_SENT;
                USART2->CR1 &= ~USART_CR1_TXEIE;
            }
        }

        else if(LL_USART_IsEnabledIT_TXE(USART2))
        {
            mh_enable_transmission(false);
            LL_USART_DisableIT_TXE(USART2);
        }
    }
    if (USART2->ISR & USART_ISR_IDLE)
    {
        LL_USART_ClearFlag_IDLE(USART2);
        if (MB_STATE_RCVE == mb_rs485.mb_state)
        {
            mb_rs485.mb_state = MB_STATE_PARS;
            td_set_timer_task(mh_task_modbus_rs485, RS485_TIMEOUT_MS);
        }
    }
}

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
static void mh_enable_transmission(const bool enable)
{
    if (enable)
        io_set_line(IO_RS485_SWITCH, HIGH);
    else
        io_set_line(IO_RS485_SWITCH, LOW);
}

//-----------------------------------------------------------------------
static void mh_rs485_transmit_start (mb_slave_t *p_instance)
{
    mh_enable_transmission(true);
    LL_USART_EnableIT_TXE(USART2);
}


//-----------------------------------------------------------------------
static void mh_usb_transmit_start (mb_slave_t *p_instance)
{
    // memcpy(mb_usb.p_mb_buff, usb_buf, len);
    CDC_Transmit_FS (p_instance->p_mb_buff, p_instance->response_size);
    p_instance->mb_state = MB_STATE_IDLE;
}

//-----------------------------------------------------------------------
static void io_uart2_init(void)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);

    LL_USART_SetOverSampling(USART2, LL_USART_OVERSAMPLING_8);
    if (mb_buf_main[REG_RS485_BAUD] < BAUD_NUMBER)
        LL_USART_SetBaudRate(USART2, SystemCoreClock, LL_USART_OVERSAMPLING_8, baud_rate[mb_buf_main[REG_RS485_BAUD]]);
    else
        LL_USART_SetBaudRate(USART2, SystemCoreClock, LL_USART_OVERSAMPLING_8, baud_rate[0]);
    LL_USART_SetTransferDirection(USART2, LL_USART_DIRECTION_TX_RX);
    LL_USART_DisableIT_CTS(USART2);
    LL_USART_EnableIT_IDLE(USART2);
    LL_USART_EnableIT_RXNE(USART2);
    LL_USART_EnableIT_TC(USART2);
    switch (mb_buf_main[REG_RS485_PARITY])
    {
    case NO_PARITY_1_STOP:
        break; //default setting
    case NO_PARITY_2_STOP:
        LL_USART_SetStopBitsLength(USART2, LL_USART_STOPBITS_2);
        break;
    case EVEN_PARITY_1_STOP:
        LL_USART_SetDataWidth(USART2, LL_USART_DATAWIDTH_9B);
        LL_USART_SetParity(USART2, LL_USART_PARITY_EVEN);
        break;
    case ODD_PARITY_1_STOP:
        LL_USART_SetDataWidth(USART2, LL_USART_DATAWIDTH_9B);
        LL_USART_SetParity(USART2, LL_USART_PARITY_ODD);
        break;
    default:
        break;
    }
    LL_USART_Enable(USART2);
    NVIC_SetPriority(USART2_IRQn,13);
    NVIC_EnableIRQ (USART2_IRQn);
}

//-----------------------------------------------------------------------
void mh_write_eeprom (mb_slave_t *p_instance)
{
    LOG_INFO ("mh_write_eeprom");
    for (int32_t i = 0; i < (p_instance->cb_index); i++)
    {
        if((mb_reg_option_check(i+(p_instance->cb_reg_start), CB_EEPROM) == MB_OK))
        {
            EE_UpdateVariable(((p_instance->cb_reg_start)+i), p_instance->p_write[i+(p_instance->cb_reg_start)]);
        }
    }
}

//-----------------------------------------------------------------------
//Callback for usb com
void mh_usb_recieve(uint8_t *usb_buf, uint16_t len)	//interrupt	function
{
    if (mb_instance_idle_check((mb_slave_t*)&mb_usb)==MB_OK)
    {
        if(len > MB_FRAME_MAX)
        {
            len = MB_FRAME_MAX;
        }
        mb_usb.mb_state = MB_STATE_PARS;
        mb_usb.mb_index = len;
        memcpy(mb_usb.p_mb_buff, usb_buf, len);

        td_set_timer_task(mh_task_modbus_usb, RS485_TIMEOUT_MS);

    }
}

//-----------------------------------------------------------------------
void mh_modbus_init(void)
{
    mb_rs485.slave_address = mb_rs485.p_read[REG_RS485_ADDR];
    io_uart2_init();
}

//-----------------------------------------------------------------------
void mh_factory (void)
{
    LOG_INFO ("mh_factory");
    __disable_irq();
    for (int32_t i=0; i< MB_NUM_BUF; i++)
    {
        if (mb_reg_option_check(i, CB_EEPROM) == MB_OK)
        {
            mb_buf_main[i] = mb_reg_get_param(i)->default_value;
            EE_UpdateVariable(i, mb_buf_main[i]);
        }
    }
    mb_buf_main[REG_RESET_TO_DEFAULT] = 0;
    __enable_irq();
}

//-----------------------------------------------------------------------
void mh_buf_init (void)
{
    LOG_INFO ("mh_buf_init");
    int32_t i=0;
    EEPRESULT stat;
    __disable_irq();
    for (i=0; i< MB_NUM_BUF; i++)
    {
        if(mb_reg_option_check(i, CB_EEPROM) == MB_OK)
        {
            stat = EE_ReadVariable(i, &mb_buf_main[i]);
            if((mb_reg_limit_check(i, mb_buf_main[i]) == MB_ERROR) || (stat != RES_OK))
            {
                mb_buf_main[i]=mb_reg_get_param(i)->default_value;
                EE_UpdateVariable(i, mb_buf_main[i]);
            }
        }
    }
    __enable_irq();
}
//-----------------------------------------------------------------------
void mh_buf_set_reg (uint16_t reg_index, uint16_t reg_val)
{
    if (reg_index >= MB_NUM_BUF)
        return;
    mb_buf_main[reg_index] = reg_val;
    // check main callback available for this register
    if((mb_reg_option_check(reg_index, CB_EEPROM) == MB_OK))
    {
        EE_UpdateVariable(reg_index, reg_val);
    }
}
//-----------------------------------------------------------------------
void mh_update_all_eeprom ()
{
    __disable_irq();
    for (int32_t i=0; i< MB_NUM_BUF; i++)
    {
        if (mb_reg_option_check(i, CB_EEPROM)==MB_OK)
        {
            EE_UpdateVariable(i, mb_buf_main[i]);
        }
    }
    __enable_irq();
}

//-----------------------------------------------------------------------
EEPRESULT GetNextVirtAddrData(uint16_t VirtAddressLast, uint16_t *VirtAddressNext, uint16_t *NextData)
{
    uint32_t i;
    VirtAddressLast ++;

    for (i = VirtAddressLast ; i< MB_NUM_BUF ; i++)
    {
        if (mb_reg_option_check(i, CB_EEPROM) == MB_OK)
        {
            *VirtAddressNext = i;
            *NextData = mb_buf_main[i];
            return RES_OK;
        }
    }
    return RES_ERROR;
}

//-----------------------------------------------------------------------
void mh_write_cb (mb_slave_t *p_instance)
{
    LOG_INFO ("mh_write_cb");
    void (*mb_cb)(void);

    for (int32_t i = 0; i < (p_instance->cb_index); i++)
    {
        // check main callback available for this register
        if((mb_reg_option_check(i+(p_instance->cb_reg_start), CB_WR) == MB_OK))
        {
            //check user callback available for this register
            if((mb_reg_option_check(i+(p_instance->cb_reg_start), CB_USER) == MB_OK))
            {
                mb_cb = mb_reg_get_user_arg1(i+(p_instance->cb_reg_start));
                if (mb_cb != 0)
                {
                    mb_cb();
                }
            }
            if ((mb_reg_option_check(i+(p_instance->cb_reg_start), CB_EEPROM) == MB_OK))
            {
                EE_UpdateVariable(((p_instance->cb_reg_start)+i), p_instance->p_write[i+(p_instance->cb_reg_start)]);
            }
        }
    }
}
//-----------------------------------------------------------------------
void mh_restart_cb()
{
    if (mb_buf_main[REG_RESTART] == RESET_VALUE)
    {
        //  SystemInit();
        NVIC_SystemReset();
        while (1);
    }
}

//-----------------------------------------------------------------------
void mh_default_cb()
{
    if (mb_buf_main[REG_RESET_TO_DEFAULT] == FACTORY_SET_VALUE)
    {
        mh_factory();
    }
    mb_buf_main[REG_RESET_TO_DEFAULT] = 0;
}
