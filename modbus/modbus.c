// version 1_01
#include "modbus.h"
#include "modbus_config.h"
#include <stdio.h>

#if ((MB_CALLBACK_REG == 1) || (MB_LIMIT_REG == 1))
#include "modbus_reg.h"
#endif
//-----------------------------------------------------------------------
//  CRC
//----------------------------------------------------------------------

// Table of CRC values for high–order byte
static const uint16_t wCRCTable[] =
{
    0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
    0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
    0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
    0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
    0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
    0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
    0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
    0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
    0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
    0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
    0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
    0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
    0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
    0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
    0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
    0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
    0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
    0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
    0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
    0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
    0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
    0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
    0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
    0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
    0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
    0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
    0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
    0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
    0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
    0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
    0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
    0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};

//-----------------------------------------------------------------------
static unsigned int mb_crc16 ( unsigned char *puchMsg, unsigned int usDataLen )
{
    uint8_t nTemp;
    uint16_t wCRCWord = 0xFFFF;

    while (usDataLen--)
    {
        nTemp = *puchMsg++ ^ wCRCWord;
        wCRCWord >>= 8;
        wCRCWord ^= wCRCTable[nTemp];
    }
    return wCRCWord;
}

//-----------------------------------------------------------------------
//  function
//----------------------------------------------------------------------

//-----------------------------------------------------------------------
// check cb presence
//-----------------------------------------------------------------------
#if (MB_CALLBACK_REG == 1)
static mb_cb_state_t mb_cb_check_in_request (uint16_t start_reg, uint16_t count)
{
    for (int32_t i = 0; i < count; i++)
    {
        if(mb_reg_cb_option_check(start_reg+i) == MB_OK)
        {
            return MB_CB_PRESENT;
        }
    }
    return 	MB_CB_FREE;
}
#endif

//-----------------------------------------------------------------------
// check register limits
//-----------------------------------------------------------------------
#if (MB_LIMIT_REG == 1)
static mb_excep_t mb_reg_limit_check_in_request (uint16_t number_reg, uint16_t value)
{
    if (mb_reg_write_option_check(number_reg) == MB_ERROR)
    {
        return MBE_ILLEGAL_DATA_ADDRESS;
    }
    if (mb_reg_limit_check(number_reg, value) == MB_ERROR)
    {
        return	MBE_ILLEGAL_DATA_VALUE;
    }
    return MBE_NONE;
}
#endif

//-----------------------------------------------------------------------
// check invalid_frame
//-----------------------------------------------------------------------
static bool invalid_frame(mb_slave_t *p_instance)
{
    uint8_t *buf_ptr;
    uint32_t PDU_len;

#if (MB_TCP_PERMISSION == 1)
    if (p_instance->mb_frame_type == MB_TYPE_TCP)
    {
        if (p_instance->p_mb_buff == NULL)
        {
            return true;
        }
        buf_ptr = &p_instance->p_mb_buff[6];
        PDU_len = (p_instance->p_mb_buff[4] << 8) | (p_instance->p_mb_buff[5] & 0xFF);
        if ((PDU_len >= MB_FRAME_MAX) || ((p_instance->mb_index - 6) != PDU_len))
        {
            return true;
        }
    }
    else
    {
        PDU_len = p_instance->mb_index;
        buf_ptr = p_instance->p_mb_buff;
    }
#else
    PDU_len = p_instance->mb_index;
    buf_ptr = p_instance->p_mb_buff;
#endif
    if (EV_HAPPEND == p_instance->er_frame_bad)
    {
        return true;
    }
    if (PDU_len < MB_FRAME_MIN)
    {
        return true;
    }
    if ((p_instance->slave_address != buf_ptr[0]) && (buf_ptr[0] != MB_ADDRESS_BROADCAST) && (p_instance->slave_address != 0))
    {
        return true;
    }
#if (MB_CALLBACK_REG == 1)
    if (p_instance->cb_state != MB_CB_FREE)
    {
        return true;
    }
#endif
#if (MB_TCP_PERMISSION == 1)
    if (p_instance->mb_frame_type == MB_TYPE_RTU)
    {
        if (mb_crc16((uint8_t *)buf_ptr, PDU_len))
        {
            return true;
        }
    }
#else
    if (mb_crc16((uint8_t *)p_instance->p_mb_buff, p_instance->mb_index))
    {
        return true;
    }
#endif
    return false;
}

//-----------------------------------------------------------------------
// parse
//-----------------------------------------------------------------------
static bool frame_parse (mb_slave_t *p_instance)
{
    // Returns TRUE if a response is needed
#if (MB_LIMIT_REG == 1)
    uint16_t 	value, reg_indx_iter, j;
#endif
    uint16_t 	reg_indx, reg_nmb, reg_last, i;
    uint8_t		bytes_num;
    mb_excep_t	exception;  // If a Modbus exception happens - we put the var in MBBuff[2]
    bool need_response = true;
    uint8_t *buf_ptr;
    uint32_t PDU_len;
#if (MB_TCP_PERMISSION == 1)
    if (p_instance->mb_frame_type == MB_TYPE_TCP)
    {
        buf_ptr = &p_instance->p_mb_buff[6];
        PDU_len = p_instance->mb_index - 4; // not -6 bytes head length, because in the function
        // there is a comparison with the frame length
        // including a checksum of 2 bytes
    }
    else
    {
        buf_ptr = p_instance->p_mb_buff;
        PDU_len = p_instance->mb_index;
    }
#else
    buf_ptr = p_instance->p_mb_buff;
    PDU_len = p_instance->mb_index;
#endif
    if (buf_ptr[0] == MB_ADDRESS_BROADCAST)
    {
        need_response = false; // We parse the request but we don'n give a response
    }
    switch (buf_ptr[1]) // Function code
    {
    case MB_FUNC_READ_INPUT_REGISTER:
    case MB_FUNC_READ_HOLDING_REGISTER:
        // 03 0...LASTINDEX-1 0...125
        // 0=addr 1=func 2=AddrHi 3=AddrLo 4=QuantHi 5=QuantLo 6=CRC 7=CRC
        if( 8 == PDU_len)
        {
            // In this function mb_index == 8.
            reg_indx = (buf_ptr[2]<<8) | (buf_ptr[3]&0xFF);
            reg_nmb  = (buf_ptr[4]<<8) | (buf_ptr[5]&0xFF);
            reg_last = reg_indx + reg_nmb;
            if((reg_indx > p_instance->reg_read_last) ||
                    ((reg_last-1) > p_instance->reg_read_last) ||
                    (reg_nmb>MB_MAX_REG) )	// max quantity registers in inquiry
            {
                exception = MBE_ILLEGAL_DATA_ADDRESS;
                break;
            }
            // Make response. MBBuff[0] and MBBuff[1] are ready
            buf_ptr[2] = reg_nmb << 1;
            p_instance->mb_index = 3;
            while( reg_indx < reg_last )
            {
                buf_ptr[p_instance->mb_index++] = (uint8_t)(*(p_instance->p_read+reg_indx) >> 8)  ;
                buf_ptr[p_instance->mb_index++] = (uint8_t)(*(p_instance->p_read+reg_indx) & 0xFF);
                ++reg_indx;
            }
            exception = MBE_NONE;	// OK, make CRC to MBBuff[mb_index] and response is ready
            break;
        }
        exception = MBE_ILLEGAL_DATA_VALUE;	// PDU length incorrect
        break;
    case MB_FUNC_WRITE_MULTIPLE_REGISTERS:
        // 16 0...LASTINDEX-1 0...125 0...250
        // 0=addr 1=func 2=AddrHi 3=AddrLo 4=QuantHi 5=QuantLo 6=Bytes 7=RG1Hi 8=RG1LO ...CRC CRC
        if(p_instance->p_write == NULL)
        {
            exception = MBE_ILLEGAL_FUNCTION;
            break;
        }
        else if( PDU_len > 10)
        {
            exception = MBE_NONE;	// ...and mb_index is a length of response
            // The must: mb_index == 11, 13, 15, ...
            reg_indx = (buf_ptr[2]<<8) | (buf_ptr[3]&0xFF);
            reg_nmb  = (buf_ptr[4]<<8) | (buf_ptr[5]&0xFF);
            reg_last = reg_indx + reg_nmb;
            bytes_num = buf_ptr[6];
            if(	(reg_indx > p_instance->reg_write_last) ||
                    ((reg_last-1) > p_instance->reg_write_last) ||
                    (reg_nmb>MB_MAX_REG))
            {
                exception = MBE_ILLEGAL_DATA_ADDRESS;
                break;
            }
            if(bytes_num != (reg_nmb << 1) ||
                    PDU_len != (9 + bytes_num))
            {
                // 1 reg - mb_index=11, 2 regs - 13,... 5 regs - 19, etc.
                exception = MBE_ILLEGAL_DATA_VALUE;
                break;
            }
            i = 7;	// Registers' values are from MBBuff[7] and more
#if (MB_CALLBACK_REG == 1)  //check EEPROM start
            p_instance->cb_state = mb_cb_check_in_request(reg_indx, reg_nmb);
            if(p_instance->cb_state==MB_CB_PRESENT)
            {
                p_instance->cb_reg_start = reg_indx;
                p_instance->cb_index = reg_nmb;
            }
#endif  //check EEPROM end
#if (MB_LIMIT_REG == 1) //check data start
            j = i;
            for (reg_indx_iter = reg_indx; reg_indx_iter < reg_last; reg_indx_iter++)
            {
                value = buf_ptr[j++]<<8;
                value |= buf_ptr[j++];
                exception = mb_reg_limit_check_in_request(reg_indx_iter, value);
                if	(exception != MBE_NONE)
                {
                    break;
                }
            }
            if(exception != MBE_NONE)
            {
                break;
            }
#endif  //check data end
            while( reg_indx < reg_last)
            {
                *(p_instance->p_write+reg_indx)  = buf_ptr[i++]<<8;	// High, then Low byte
                *(p_instance->p_write+reg_indx) |= buf_ptr[i++]	;	// ... are packed in a WORD
                ++reg_indx;
            }
            p_instance->mb_index = 6;	// MBBuff[0] to MBBuff[5] are ready (unchanged)
            break;
        }
        exception = MBE_ILLEGAL_DATA_VALUE;	// PDU length incorrect
        break;
    case MB_FUNC_WRITE_REGISTER:
        // 06 0...250 0...255
        // 0=addr 1=func 2=AddrHi 3=AddrLo 4=RG1Hi 5=RG1LO 6=CRC 7=CRC
        if(p_instance->p_write == NULL)
        {
            exception = MBE_ILLEGAL_FUNCTION;
            break;
        }
        else if( PDU_len == 8)
        {
            exception = MBE_NONE;	// ...and mb_index is a length of response
            reg_indx = (buf_ptr[2]<<8) | (buf_ptr[3]&0xFF);

            if((reg_indx > p_instance->reg_write_last))
            {
                exception = MBE_ILLEGAL_DATA_ADDRESS;
                break;
            }
            i = 4;	// Registers' value are from MBBuff[4]-[5]
#if (MB_CALLBACK_REG == 1)  // check EEPROM start
            p_instance->cb_state = mb_cb_check_in_request(reg_indx, 1);
            if(p_instance->cb_state==MB_CB_PRESENT)
            {
                p_instance->cb_reg_start = reg_indx;
                p_instance->cb_index = 1;
            }
#endif  // check EEPROM end
#if (MB_LIMIT_REG  == 1) // check data start
            reg_indx_iter = reg_indx;
            j = i;
            value = buf_ptr[j++]<<8;
            value |= buf_ptr[j++];
            exception = mb_reg_limit_check_in_request(reg_indx_iter, value);
            if	(exception != MBE_NONE)
            {
                break;
            }
#endif  // check data end
            *(p_instance->p_write+reg_indx)  = buf_ptr[i++]<<8;	// High, then Low byte
            *(p_instance->p_write+reg_indx) |= buf_ptr[i++];	// ... are packed in a WORD
            p_instance->mb_index = 6;	// MBBuff[0] to MBBuff[5] are ready (unchanged)
            break;
        }
        exception = MBE_ILLEGAL_DATA_VALUE;	// PDU length incorrect
        break;
    default:
        exception = MBE_ILLEGAL_FUNCTION;
        break;
    }
    /*
     * At this point the "mb_index" is a length of the response (w/o CRC bytes)
     * mb_index is variable if MB_FUNC_READ_HOLDING_REGISTER, 6 if MB_FUNC_WRITE_MULTIPLE_REGISTERS)
     * but if there's some exception - it will be shortened to 3 in both cases
     */

    if( exception != MBE_NONE)
    {
        // Any exception?
        buf_ptr[1] |= MB_FUNC_ERROR;						// Add 0x80 to function code
        buf_ptr[2] =  exception;							// exception code
        p_instance->mb_index = 3;							// Length of response is fixed if exception
#if (MB_CALLBACK_REG == 1)
        p_instance->cb_state = MB_CB_FREE;
#endif
    }
#if (MB_TCP_PERMISSION == 1)
    if (p_instance->mb_frame_type == MB_TYPE_RTU)
    {
        i = mb_crc16( (uint8_t*)buf_ptr, p_instance->mb_index);   // MBBuff is a pointer, mb_index is a size
        buf_ptr[p_instance->mb_index++] = i & 0xFF;               // CRC: Lo then Hi
        buf_ptr[p_instance->mb_index++] = i >> 8;
        p_instance->response_size = p_instance->mb_index;
    }
    else
    {
        p_instance->p_mb_buff[4] = 0;                               //(mbb->mb_index >> 8); //uint8_t type
        p_instance->p_mb_buff[5] = (p_instance->mb_index & 0xFF);   // uint8_t type
        p_instance->response_size = p_instance->mb_index + 6;
    }
#else
    i = mb_crc16((uint8_t *)buf_ptr, p_instance->mb_index); // MBBuff is a pointer, mb_index is a size
    buf_ptr[p_instance->mb_index++] = i & 0xFF;             // CRC: Lo then Hi
    buf_ptr[p_instance->mb_index++] = i >> 8;
    p_instance->response_size = p_instance->mb_index;
#endif
    return need_response? true:false;

}

//-----------------------------------------------------------------------
// mb
//-----------------------------------------------------------------------
uint32_t mb_parsing(mb_slave_t *p_instance)
{
    uint32_t ret = MB_ERROR;
    p_instance->response_size = 0;
    if( invalid_frame(p_instance))
    {
        p_instance->mb_state = MB_STATE_IDLE;
        if (p_instance->start_receive != NULL)
        {
            p_instance->start_receive(p_instance);
        }
        return ret;
    }
    if( frame_parse(p_instance))        // Returns TRUE if a response is needed
    {
        ret = MB_OK_NEED_RESPONSE;
        if(  p_instance->start_trans != NULL)
        {
            p_instance->mb_state = MB_STATE_SEND;
            p_instance->mb_index = 0;
            p_instance->start_trans(p_instance);
        }
    }
    else
    {
        ret = MB_OK_NO_RESPONSE;
        p_instance->mb_state = MB_STATE_IDLE;
        if (p_instance->start_receive != NULL)
        {
            p_instance->start_receive(p_instance);
        }
    }
#if (MB_CALLBACK_REG == 1)
    if  (p_instance->cb_state == MB_CB_PRESENT)
    {
        p_instance->wr_callback(p_instance);
        p_instance->cb_state = MB_CB_FREE;
    }
#endif
    return ret;
}

//-----------------------------------------------------------------------
// mb change address
//-----------------------------------------------------------------------
void mb_set_address(mb_slave_t *p_instance, uint8_t address)
{
    p_instance->slave_address = address;
}
