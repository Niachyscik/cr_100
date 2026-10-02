#ifndef MODBUS_HARD_H_INCLUDED
#define MODBUS_HARD_H_INCLUDED

#include "modbus.h"


#define	 BAUD_9600		                    9600
#define	 BAUD_19200		                    19200
#define	 BAUD_57600		                    57600
#define	 BAUD_115200	                    115200
#define  BAUD_NUMBER                        4
#define	 RS_485_BAUD_LIST                   {BAUD_9600, BAUD_19200, BAUD_57600, BAUD_115200}

#define RESET_VALUE					        0xA01	//2561
#define FACTORY_SET_VALUE 			        0xB01	//2817

#define	 RS485_TIMEOUT_MS                   5

typedef enum
{
    NO_PARITY_1_STOP	= 0x00,
    NO_PARITY_2_STOP	= 0x01,
    EVEN_PARITY_1_STOP	= 0x02,
    ODD_PARITY_1_STOP	= 0x03,

} parity_stop_bits_t;

void mh_write_eeprom (mb_slave_t *p_instance);
void mh_modbus_init(void);
void mh_usb_recieve(uint8_t *USB_buf, uint16_t len);
void mh_factory (void);
void mh_buf_init (void);
void mh_update_all_eeprom ();
void mh_write_cb (mb_slave_t *p_instance);
void mh_buf_set_reg (uint16_t reg_index, uint16_t reg_val);
#endif /* MODBUS_HARD_H_INCLUDED */
