#ifndef MODBUS_REG_H_INCLUDED
#define MODBUS_REG_H_INCLUDED

#include <stdint.h>
#include "modbus_config.h"
#include "modbus.h"
//-----------------------------------------------------------------------
// Configurations
//-----------------------------------------------------------------------
typedef struct
{
    uint16_t		default_value;
    uint16_t		min_level;
    uint16_t		max_level_mask;
    uint32_t		options;
} mbreg_parameters_t;

#if (MB_REG_END_TO_END == 1)
enum
{
#define X_BUF(a,b,c,d,e,f) b,
    MB_BUF_TABLE
#undef  X_BUF
    MB_NUM_BUF  //Reg count
};
#else
enum
{
#define X_BUF(a,b,c,d,e,f,g) b=a,
    MB_BUF_TABLE
#undef  X_BUF
    MB_NUM_BUF = (REG_END_REGISTER+1)
};
#endif
//-----------------------------------------------------------------------
// Instance
//-----------------------------------------------------------------------
#if (MB_TCP_PERMISSION == 1)

#define MB_SLAVE_RTU_WITH_BUF_INSTANCE_DEF(instance_name,    \
                                main_buf,               \
                                write_callback,         \
                                start_trans_cb,         \
                                start_recieve_cb)       \
static uint8_t instance_name##_mb_buf[MB_FRAME_MAX];    \
static volatile mb_slave_t (instance_name) = {          \
            .p_write = main_buf,                        \
            .p_read = main_buf,                         \
            .reg_read_last = MB_NUM_BUF-1,              \
            .reg_write_last = MB_NUM_BUF-1,             \
            .cb_state = MB_CB_FREE,                     \
            .er_frame_bad = EV_NOEVENT,                 \
            .mb_state = MB_STATE_IDLE,                  \
            .p_mb_buff = instance_name##_mb_buf,        \
            .wr_callback = write_callback,              \
            .start_trans = start_trans_cb,              \
            .start_receive = start_recieve_cb,          \
            .slave_address = MB_ANY_ADDRESS,            \
            .mb_frame_type = MB_TYPE_RTU,               \
             };

// set buffer manually
#define MB_SLAVE_TCP_INSTANCE_DEF(instance_name,        \
                                main_buf,               \
                                write_callback,         \
                                start_trans_cb,         \
                                start_recieve_cb)       \
static volatile mb_slave_t (instance_name) = {          \
            .p_write = main_buf,                        \
            .p_read = main_buf,                         \
            .reg_read_last = MB_NUM_BUF-1,              \
            .reg_write_last = MB_NUM_BUF-1,             \
            .cb_state = MB_CB_FREE,                     \
            .er_frame_bad = EV_NOEVENT,                 \
            .mb_state = MB_STATE_IDLE,                  \
            .p_mb_buff = NULL,                          \
            .wr_callback = write_callback,              \
            .start_trans = start_trans_cb,              \
            .start_receive = start_recieve_cb,          \
            .slave_address = MB_ANY_ADDRESS,            \
            .mb_frame_type = MB_TYPE_TCP,               \
             };

#else   // MB_TCP_PERMISSION == 0

#define MB_SLAVE_RTU_WITH_BUF_INSTANCE_DEF(instance_name,    \
                                main_buf,               \
                                write_callback,         \
                                start_trans_cb,         \
                                start_recieve_cb)       \
static uint8_t instance_name##_mb_buf[MB_FRAME_MAX];    \
static volatile mb_slave_t (instance_name) = {          \
            .p_write = main_buf,                        \
            .p_read = main_buf,                         \
            .reg_read_last = MB_NUM_BUF-1,              \
            .reg_write_last = MB_NUM_BUF-1,             \
            .cb_state = MB_CB_FREE,                     \
            .er_frame_bad = EV_NOEVENT,                 \
            .mb_state = MB_STATE_IDLE,                  \
            .p_mb_buff = instance_name##_mb_buf,        \
            .wr_callback = write_callback,              \
            .start_trans = start_trans_cb,              \
            .start_receive = start_recieve_cb,          \
            .slave_address = MB_ANY_ADDRESS,            \
             };

#endif  // MB_TCP_PERMISSION

//-----------------------------------------------------------------------
//  prototype
//-----------------------------------------------------------------------
mb_ret_t mb_instance_idle_check (mb_slave_t *p_instance);
mb_ret_t mb_reg_limit_check (uint16_t number_reg, uint16_t value);
mb_ret_t mb_reg_option_check (uint16_t number, uint16_t option_mask);
const mbreg_parameters_t* mb_reg_get_param (uint16_t number);
const void* mb_reg_get_user_arg1 (uint16_t number);
mb_ret_t mb_reg_write_option_check (uint16_t number);
mb_ret_t mb_reg_cb_option_check (uint16_t number);
#endif /* MODBUS_REG_H_INCLUDED*/
