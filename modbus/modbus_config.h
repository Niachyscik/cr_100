#ifndef MODBUS_X_H_INCLUDED
#define MODBUS_X_H_INCLUDED

#include <stdint.h>
#include "main.h"
#include "modbus_cb.h"
//-----------------------------------------------------------------------
// user define
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// define
//-----------------------------------------------------------------------
#define MB_LIMIT_REG	    1//check limit
#define MB_CALLBACK_REG	    1//use write callback
#define MB_USER_ARG1_REG	1//use user argument (for example: run user callback after write function)
//#define MB_USER_ARG2_REG	1//not implement now
// If not define "MB_REG_END_TO_END" then number register is determined in "a" field from X-macros
//#define MB_REG_END_TO_END
//#define MB_TCP_PERMISSION   1 // TCP frame avalible

#define REG_END_REGISTER                REG_END
//-----------------------------------------------------------------------
// Modbus registers X macros
//-----------------------------------------------------------------------
//  MAIN_BUF_Start_Table_Mask
#define READ_R		    (0)
#define WRITE_R		    (0x01)	// 0 bit                        <--|
#define CB_WR		    (0x02)	// 1 bit                        <--|
#define LIM_SIGN		(0x04)	// 2 bit for limit              <--|
#define LIM_UNSIGN	    (0x08)  // 3 bit for limit	            <--|
#define LIM_MASK	    (0x0C)	// 2 and 3 bit for limit        <--|____________
#define CB_USER		    (0x10)	// 4 bit
#define CB_EEPROM		(0x20)	// 5 bit
//                                                                          |
#define LIM_BIT_MASK    LIM_MASK
//	 Number		Name for enum	       Arg1  Default   Min	   Max     __________Options________
//										      Value   Level   Level   |                         |
//														     or Mask  |                         |
#define MB_BUF_TABLE\
    X_BUF(0,	REG_VER,                0,	    0,	    0,		0,	    READ_R)\
    X_BUF(1,	REG_CUR_RMS,            0,	    0,	    0,		0,	    READ_R)\
    X_BUF(2,	REG_CUR_MID_FILTERED,   0,	    0,	    0,		0,	    READ_R)\
    X_BUF(5,	REG_TRIG,               0,	    0,	    0,		0,	    READ_R )\
    X_BUF(6,	REG_TRIG_NOTIFY,        0,	    0,	    0,		0,	    READ_R )\
    X_BUF(7,	REG_TRIG_WORKING,       0,	    0,	    0,		0,	    READ_R )\
    X_BUF(8,	REG_DO_STATUS,          0,	    0,	    0,		0,	    READ_R)\
    X_BUF(10,	REG_SETPOINT_CUR_1_A,   0,	    100,    0,		1000,   WRITE_R | CB_WR | CB_EEPROM | LIM_UNSIGN)\
    X_BUF(11,	REG_SETPOINT_CUR_1_B,   0,	    80,     0,		1000,   WRITE_R | CB_WR | CB_EEPROM | LIM_UNSIGN)\
    X_BUF(12,	REG_SETPOINT_TIME_100MS_1_A, 0,	50,	    0,		6000,   WRITE_R | CB_WR | CB_EEPROM | LIM_UNSIGN)\
    X_BUF(13,	REG_SETPOINT_TIME_100MS_1_B, 0,	50,	    0,		6000,   WRITE_R | CB_WR | CB_EEPROM | LIM_UNSIGN)\
    X_BUF(14,	REG_SETPOINT_CUR_2_A,   0,	    100,    0,		1000,   WRITE_R | CB_WR | CB_EEPROM | LIM_UNSIGN)\
    X_BUF(15,	REG_SETPOINT_CUR_2_B,   0,	    80,     0,		1000,   WRITE_R | CB_WR | CB_EEPROM | LIM_UNSIGN)\
    X_BUF(16,	REG_SETPOINT_TIME_100MS_2_A, 0, 50,	    0,		6000,   WRITE_R | CB_WR | CB_EEPROM | LIM_UNSIGN)\
    X_BUF(17,	REG_SETPOINT_TIME_100MS_2_B, 0,	50,	    0,		6000,   WRITE_R | CB_WR | CB_EEPROM | LIM_UNSIGN)\
    X_BUF(19,	REG_TRIG_NOTIFY_RESET,  &trig_reset_cb,\
                                                0,	    0,		0,	    WRITE_R | CB_WR | CB_USER )\
    X_BUF(21,	REG_DO_1_MODE,          0,	    DO_MODE_ALARM,\
                                                        0,	    DO_MODE_NOTIFY_READY,\
                                                                        WRITE_R | CB_WR | CB_EEPROM | LIM_UNSIGN)\
    X_BUF(22,	REG_DO_2_MODE,          0,	    DO_MODE_ALARM,\
                                                        0,	    DO_MODE_NOTIFY_READY,\
                                                                        WRITE_R | CB_WR | CB_EEPROM | LIM_UNSIGN)\
    X_BUF(24,	REG_DO_ON,              0,	    0,	    0,		0,	    WRITE_R )\
    X_BUF(25,	REG_DO_OFF,             0,	    0,	    0,		0,	    WRITE_R )\
    X_BUF(30,	REG_CALIB_START,      &calibration_cb,\
                                                0,	    0,		0,	    WRITE_R | CB_WR | CB_USER )\
    X_BUF(31,	REG_CALIB_VALUE,        0,	    100,	0,		0,	    WRITE_R)\
    X_BUF(32,	REG_ADC_RMS,            0,	    0,	    0,		0,	    READ_R)\
    X_BUF(33,	REG_ADC_FILTERED,       0,	    0,	    0,		0,	    READ_R)\
    X_BUF(34,	REG_ADC_K_FILTERED,     0,	    ADC_DEFAULT_FILTER_RATIO,\
                                                        1,		0xFFFF, WRITE_R | CB_WR | CB_EEPROM | LIM_UNSIGN)\
    X_BUF(35,	REG_CALIB_OFFSET_W1,    0,	    ADC_OFFSET_DEFAULT_W1,\
                                                        0,		0,	    WRITE_R | CB_WR | CB_EEPROM)\
    X_BUF(36,	REG_CALIB_OFFSET_W2,    0,	    0,      0,		0,	    WRITE_R | CB_WR | CB_EEPROM)\
    X_BUF(37,	REG_CALIB_GAIN_W1,      0,	    ADC_K_GAIN_DEFAULT_W1,\
                                                        0,		0,	    WRITE_R | CB_WR | CB_EEPROM)\
    X_BUF(38,	REG_CALIB_GAIN_W2,      0,	    0,	    0,		0,	    WRITE_R | CB_WR | CB_EEPROM)\
    X_BUF(40,	REG_RS485_BAUD,	        0,		1,		0,		0x03,	WRITE_R | CB_WR | CB_EEPROM | LIM_MASK)\
	X_BUF(41,	REG_RS485_ADDR,         0,		1,		1,		0xFA,	WRITE_R | CB_WR | CB_EEPROM | LIM_UNSIGN)\
	X_BUF(42,	REG_RS485_PARITY, 	    0,	    0,	    0,		0x03,	WRITE_R | CB_WR | CB_EEPROM | LIM_UNSIGN)\
    X_BUF(45,	REG_RESET_TO_DEFAULT,   &mh_default_cb,\
                                                0,	    0,		0,	    WRITE_R | CB_WR | CB_USER )\
    X_BUF(46,	REG_RESTART,            &mh_restart_cb,\
                                                0,	    0,		0,	    WRITE_R | CB_WR | CB_USER )\
	X_BUF(49,	REG_END,				0,	    0,      0,      0,      READ_R)\


#endif /* MODBUS_X_H_INCLUDED */
