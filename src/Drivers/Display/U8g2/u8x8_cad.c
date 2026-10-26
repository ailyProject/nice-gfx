/*
  
  ngfx_u8x8_cad.c
  
  "command arg data" interface to the graphics controller

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  


  The following sequence must be used for any data, which is set to the display:
  
  
  uint8_t ngfx_u8x8_cad_StartTransfer(ngfx_u8x8_t *u8x8)

  any of the following calls
    uint8_t ngfx_u8x8_cad_SendCmd(ngfx_u8x8_t *u8x8, uint8_t cmd)
    uint8_t ngfx_u8x8_cad_SendArg(ngfx_u8x8_t *u8x8, uint8_t arg)
    uint8_t ngfx_u8x8_cad_SendData(ngfx_u8x8_t *u8x8, uint8_t cnt, uint8_t *data)
  
  uint8_t ngfx_u8x8_cad_EndTransfer(ngfx_u8x8_t *u8x8)



*/
/*
uint8_t ngfx_u8x8_cad_template(ngfx_u8x8_t *u8x8, uint8_t msg, uint16_t arg_int, void *arg_ptr)
{
  uint8_t i;
  
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
      ngfx_u8x8_mcd_byte_SetDC(mcd->next, 1);
      ngfx_u8x8_mcd_byte_Send(mcd->next, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      ngfx_u8x8_mcd_byte_SetDC(mcd->next, 1);
      ngfx_u8x8_mcd_byte_Send(mcd->next, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
      ngfx_u8x8_mcd_byte_SetDC(mcd->next, 0);
      for( i = 0; i < 8; i++ )
	ngfx_u8x8_mcd_byte_Send(mcd->next, ((uint8_t *)arg_ptr)[i]);
      break;
    case NGFX_U8X8_MSG_CAD_RESET:
      return mcd->next->cb(mcd->next, msg, arg_int, arg_ptr);
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
      return mcd->next->cb(mcd->next, msg, arg_int, arg_ptr);
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      return mcd->next->cb(mcd->next, msg, arg_int, arg_ptr);
    default:
      break;
  }
  return 1;
}

*/

#include "u8x8.h"

uint8_t ngfx_u8x8_cad_SendCmd(ngfx_u8x8_t *u8x8, uint8_t cmd)
{
  return u8x8->cad_cb(u8x8, NGFX_U8X8_MSG_CAD_SEND_CMD, cmd, NULL);
}

uint8_t ngfx_u8x8_cad_SendArg(ngfx_u8x8_t *u8x8, uint8_t arg)
{
  return u8x8->cad_cb(u8x8, NGFX_U8X8_MSG_CAD_SEND_ARG, arg, NULL);
}

uint8_t ngfx_u8x8_cad_SendMultipleArg(ngfx_u8x8_t *u8x8, uint8_t cnt, uint8_t arg)
{
  while( cnt > 0 )
  {
    u8x8->cad_cb(u8x8, NGFX_U8X8_MSG_CAD_SEND_ARG, arg, NULL);
    cnt--;
  }
  return 1;
}

uint8_t ngfx_u8x8_cad_SendData(ngfx_u8x8_t *u8x8, uint8_t cnt, uint8_t *data)
{
  return u8x8->cad_cb(u8x8, NGFX_U8X8_MSG_CAD_SEND_DATA, cnt, data);
}

uint8_t ngfx_u8x8_cad_StartTransfer(ngfx_u8x8_t *u8x8)
{
  return u8x8->cad_cb(u8x8, NGFX_U8X8_MSG_CAD_START_TRANSFER, 0, NULL);
}

uint8_t ngfx_u8x8_cad_EndTransfer(ngfx_u8x8_t *u8x8)
{
  return u8x8->cad_cb(u8x8, NGFX_U8X8_MSG_CAD_END_TRANSFER, 0, NULL);
}

void ngfx_u8x8_cad_vsendf(ngfx_u8x8_t * u8x8, const char *fmt, va_list va)
{
  uint8_t d;
  ngfx_u8x8_cad_StartTransfer(u8x8);
  while( *fmt != '\0' )
  {
    d = (uint8_t)va_arg(va, int);
    switch(*fmt)
    {
      case 'a':  ngfx_u8x8_cad_SendArg(u8x8, d); break;
      case 'c':  ngfx_u8x8_cad_SendCmd(u8x8, d); break;
      case 'd':  ngfx_u8x8_cad_SendData(u8x8, 1, &d); break;
    }
    fmt++;
  }
  ngfx_u8x8_cad_EndTransfer(u8x8);
}

void ngfx_u8x8_SendF(ngfx_u8x8_t * u8x8, const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  ngfx_u8x8_cad_vsendf(u8x8, fmt, va);
  va_end(va);
}

/*
  21 c		send command c
  22 a		send arg a
  23 d		send data d
  24			CS on
  25			CS off
  254 milli	delay by milliseconds
  255		end of sequence
*/

void ngfx_u8x8_cad_SendSequence(ngfx_u8x8_t *u8x8, uint8_t const *data)
{
  uint8_t cmd;
  uint8_t v;

  for(;;)
  {
    cmd = *data;
    data++;
    switch( cmd )
    {
      case NGFX_U8X8_MSG_CAD_SEND_CMD:
      case NGFX_U8X8_MSG_CAD_SEND_ARG:
	  v = *data;
	  u8x8->cad_cb(u8x8, cmd, v, NULL);
	  data++;
	  break;
      case NGFX_U8X8_MSG_CAD_SEND_DATA:
	  v = *data;
	  ngfx_u8x8_cad_SendData(u8x8, 1, &v);
	  data++;
	  break;
      case NGFX_U8X8_MSG_CAD_START_TRANSFER:
      case NGFX_U8X8_MSG_CAD_END_TRANSFER:
	  u8x8->cad_cb(u8x8, cmd, 0, NULL);
	  break;
      case 0x0fe:
	  v = *data;
	  ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_MILLI, v);	    
	  data++;
	  break;
      default:
	return;
    }
  }
}


uint8_t ngfx_u8x8_cad_empty(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
    case NGFX_U8X8_MSG_CAD_INIT:
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
      return 0;
  }
  return 1;
}


/*
  convert to bytes by using 
    dc = 1 for commands and args and
    dc = 0 for data
*/
uint8_t ngfx_u8x8_cad_110(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
      ngfx_u8x8_byte_SetDC(u8x8, 1);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      ngfx_u8x8_byte_SetDC(u8x8, 1);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
      ngfx_u8x8_byte_SetDC(u8x8, 0);
      //ngfx_u8x8_byte_SendBytes(u8x8, arg_int, arg_ptr);
      //break;
      /* fall through */
    case NGFX_U8X8_MSG_CAD_INIT:
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
      return 0;
  }
  return 1;
}

/*
  convert to bytes by using 
    dc = 1 for commands and args and
    dc = 0 for data
*/
uint8_t ngfx_u8x8_gu800_cad_110(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t *data;
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
      ngfx_u8x8_byte_SetDC(u8x8, 1);
      ngfx_u8x8_byte_StartTransfer(u8x8);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      ngfx_u8x8_byte_EndTransfer(u8x8);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      ngfx_u8x8_byte_SetDC(u8x8, 1);
      ngfx_u8x8_byte_StartTransfer(u8x8);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      ngfx_u8x8_byte_EndTransfer(u8x8);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
      ngfx_u8x8_byte_SetDC(u8x8, 0);
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
        ngfx_u8x8_byte_StartTransfer(u8x8);
        ngfx_u8x8_byte_SendByte(u8x8, *data);
        ngfx_u8x8_byte_EndTransfer(u8x8);
        data++;
        arg_int--;
      }
      break;
    case NGFX_U8X8_MSG_CAD_INIT:
      u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
      break;
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      break;
    default:
      return 0;
  }
  return 1;
}

/*
  convert to bytes by using 
    dc = 1 for commands and args and
    dc = 0 for data
    t6963
*/
uint8_t ngfx_u8x8_cad_100(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
      ngfx_u8x8_byte_SetDC(u8x8, 1);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      ngfx_u8x8_byte_SetDC(u8x8, 0);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
      ngfx_u8x8_byte_SetDC(u8x8, 0);
      //ngfx_u8x8_byte_SendBytes(u8x8, arg_int, arg_ptr);
      //break;
      /* fall through */
    case NGFX_U8X8_MSG_CAD_INIT:
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
      return 0;
  }
  return 1;
}

/*
  convert to bytes by using 
    dc = 0 for commands and args and
    dc = 1 for data
*/
uint8_t ngfx_u8x8_cad_001(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
      ngfx_u8x8_byte_SetDC(u8x8, 0);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      ngfx_u8x8_byte_SetDC(u8x8, 0);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
      ngfx_u8x8_byte_SetDC(u8x8, 1);
      //ngfx_u8x8_byte_SendBytes(u8x8, arg_int, arg_ptr);
      //break;
      /* fall through */
    case NGFX_U8X8_MSG_CAD_INIT:
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
      return 0;
  }
  return 1;
}

/*
  convert to bytes by using 
    dc = 0 for commands 
    dc = 1 for args and data
*/
uint8_t ngfx_u8x8_cad_011(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
      ngfx_u8x8_byte_SetDC(u8x8, 0);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      ngfx_u8x8_byte_SetDC(u8x8, 1);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
      ngfx_u8x8_byte_SetDC(u8x8, 1);
      //ngfx_u8x8_byte_SendBytes(u8x8, arg_int, arg_ptr);
      //break;
      /* fall through */
    case NGFX_U8X8_MSG_CAD_INIT:
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
      return 0;
  }
  return 1;
}

/* cad procedure for the ST7920 in SPI mode */
/* ngfx_u8x8_byte_SetDC is not used */
uint8_t ngfx_u8x8_cad_st7920_spi(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t *data;
  uint8_t b;
  uint8_t i;
  static uint8_t buf[16];
  uint8_t *ptr;
  
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
      ngfx_u8x8_byte_SendByte(u8x8, 0x0f8);
      ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, 1);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int & 0x0f0);
      ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, 1);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int << 4);
      ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, 1);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      ngfx_u8x8_byte_SendByte(u8x8, 0x0f8);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int & 0x0f0);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int << 4);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
    
      ngfx_u8x8_byte_SendByte(u8x8, 0x0fa);
      ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, 1);

      /* this loop should be optimized: multiple bytes should be sent */
      /* ngfx_u8x8_byte_SendBytes(u8x8, arg_int, arg_ptr); */
      data = (uint8_t *)arg_ptr;
    
      /* the following loop increases speed by 20% */
      while( arg_int >= 8 )
      {
	i = 8;
	ptr = buf;
	do
	{
	  b = *data++;
	  *ptr++= b & 0x0f0;
	  b <<= 4;
	  *ptr++= b;
	  i--;
	} while( i > 0 );
	arg_int -= 8;
	ngfx_u8x8_byte_SendBytes(u8x8, 16, buf); 
      }
      
    
      while( arg_int > 0 )
      {
	b = *data;
	ngfx_u8x8_byte_SendByte(u8x8, b & 0x0f0);
	ngfx_u8x8_byte_SendByte(u8x8, b << 4);
	data++;
	arg_int--;
      }
      ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, 1);
      break;
    case NGFX_U8X8_MSG_CAD_INIT:
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
      return 0;
  }
  return 1;
}


/* cad procedure for the SSD13xx family in I2C mode */
/* this procedure is also used by the ST7588 */
/* ngfx_u8x8_byte_SetDC is not used */
/* NGFX_U8X8_MSG_BYTE_START_TRANSFER starts i2c transfer, NGFX_U8X8_MSG_BYTE_END_TRANSFER stops transfer */
/* After transfer start, a full byte indicates command or data mode */

static void ngfx_u8x8_i2c_data_transfer(ngfx_u8x8_t *u8x8, uint8_t arg_int, void *arg_ptr) NGFX_U8X8_NOINLINE;
static void ngfx_u8x8_i2c_data_transfer(ngfx_u8x8_t *u8x8, uint8_t arg_int, void *arg_ptr)
{
    ngfx_u8x8_byte_StartTransfer(u8x8);    
    ngfx_u8x8_byte_SendByte(u8x8, 0x040);
    u8x8->byte_cb(u8x8, NGFX_U8X8_MSG_CAD_SEND_DATA, arg_int, arg_ptr);
    ngfx_u8x8_byte_EndTransfer(u8x8);
}

/* classic version: will put a start/stop condition around each command and arg */
/* implements CAD = 001 */
uint8_t ngfx_u8x8_cad_ssd13xx_i2c(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t *p;
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      /* 7 Nov 2016: Can this be improved?  */
      //ngfx_u8x8_byte_SetDC(u8x8, 0);
      ngfx_u8x8_byte_StartTransfer(u8x8);
      //ngfx_u8x8_byte_SendByte(u8x8, ngfx_u8x8_GetI2CAddress(u8x8));
      ngfx_u8x8_byte_SendByte(u8x8, 0x000);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      ngfx_u8x8_byte_EndTransfer(u8x8);      
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
      //ngfx_u8x8_byte_SetDC(u8x8, 1);
    
      /* the FeatherWing OLED with the 32u4 transfer of long byte */
      /* streams was not possible. This is broken down to */
      /* smaller streams, 32 seems to be the limit... */
      /* I guess this is related to the size of the Wire buffers in Arduino */
      /* Unfortunately, this can not be handled in the byte level drivers, */
      /* so this is done here. Even further, only 24 bytes will be sent, */
      /* because there will be another byte (DC) required during the transfer */
      p = (uint8_t *)arg_ptr;
       while( arg_int > 24 )
      {
	ngfx_u8x8_i2c_data_transfer(u8x8, 24, p);
	arg_int-=24;
	p+=24;
      }
      ngfx_u8x8_i2c_data_transfer(u8x8, arg_int, p);
      break;
    case NGFX_U8X8_MSG_CAD_INIT:
      /* apply default i2c adr if required so that the start transfer msg can use this */
      if ( u8x8->i2c_address == 255 )
	u8x8->i2c_address = 0x078;
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      /* cad transfer commands are ignored */
      break;
    default:
      return 0;
  }
  return 1;
}



/* classic version: will put a start/stop condition around each command and arg */
/* implements CAD = 011 for the SSD1363 */
uint8_t ngfx_u8x8_cad_011_ssd13xx_i2c(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t *p;
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
      ngfx_u8x8_byte_StartTransfer(u8x8);
      ngfx_u8x8_byte_SendByte(u8x8, 0x000);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      ngfx_u8x8_byte_EndTransfer(u8x8);      
      break;
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      ngfx_u8x8_byte_StartTransfer(u8x8);
      ngfx_u8x8_byte_SendByte(u8x8, 0x040);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      ngfx_u8x8_byte_EndTransfer(u8x8);      
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
      p = (uint8_t *)arg_ptr;
       while( arg_int > 24 )
      {
	ngfx_u8x8_i2c_data_transfer(u8x8, 24, p);
	arg_int-=24;
	p+=24;
      }
      ngfx_u8x8_i2c_data_transfer(u8x8, arg_int, p);
      break;
    case NGFX_U8X8_MSG_CAD_INIT:
      /* apply default i2c adr if required so that the start transfer msg can use this */
      if ( u8x8->i2c_address == 255 )
	u8x8->i2c_address = 0x078;
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      /* cad transfer commands are ignored */
      break;
    default:
      return 0;
  }
  return 1;
}


/* fast version with reduced data start/stops, issue 735 */
uint8_t ngfx_u8x8_cad_ssd13xx_fast_i2c(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t in_transfer = 0;
  uint8_t *p;
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
      /* improved version, takeover from ld7032 */
      /* assumes, that the args of a command is not longer than 31 bytes */
      /* speed improvement is about 4% compared to the classic version */
      if ( in_transfer != 0 )
	 ngfx_u8x8_byte_EndTransfer(u8x8); 
      
      ngfx_u8x8_byte_StartTransfer(u8x8);
      ngfx_u8x8_byte_SendByte(u8x8, 0x000);	/* cmd byte for ssd13xx controller */
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      in_transfer = 1;
      /* lightning version: can replace the improved version from above */
      /* the drawback of the lightning version is this: The complete init sequence */
      /* must fit into the 32 byte Arduino Wire buffer, which might not always be the case */
      /* speed improvement is about 6% compared to the classic version */
      // if ( in_transfer == 0 )
	// {
	//   ngfx_u8x8_byte_StartTransfer(u8x8);
	//   ngfx_u8x8_byte_SendByte(u8x8, 0x000);	/* cmd byte for ssd13xx controller */
	//   in_transfer = 1;
	// }
	//ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      break;      
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
      if ( in_transfer != 0 )
	ngfx_u8x8_byte_EndTransfer(u8x8); 
      
    
      /* the FeatherWing OLED with the 32u4 transfer of long byte */
      /* streams was not possible. This is broken down to */
      /* smaller streams, 32 seems to be the limit... */
      /* I guess this is related to the size of the Wire buffers in Arduino */
      /* Unfortunately, this can not be handled in the byte level drivers, */
      /* so this is done here. Even further, only 24 bytes will be sent, */
      /* because there will be another byte (DC) required during the transfer */
      p = (uint8_t *)arg_ptr;
       while( arg_int > 24 )
      {
	ngfx_u8x8_i2c_data_transfer(u8x8, 24, p);
	arg_int-=24;
	p+=24;
      }
      ngfx_u8x8_i2c_data_transfer(u8x8, arg_int, p);
      in_transfer = 0;
      break;
    case NGFX_U8X8_MSG_CAD_INIT:
      /* apply default i2c adr if required so that the start transfer msg can use this */
      if ( u8x8->i2c_address == 255 )
	u8x8->i2c_address = 0x078;
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
      in_transfer = 0;
      break;
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      if ( in_transfer != 0 )
	ngfx_u8x8_byte_EndTransfer(u8x8); 
      in_transfer = 0;
      break;
    default:
      return 0;
  }
  return 1;
}



/* the st75256 i2c driver is a copy of the ssd13xx driver, but with arg=1 */
/* modified from cad001 (ssd13xx) to cad011 */
uint8_t ngfx_u8x8_cad_st75256_i2c(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t *p;
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
      ngfx_u8x8_byte_StartTransfer(u8x8);
      ngfx_u8x8_byte_SendByte(u8x8, 0x000);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      ngfx_u8x8_byte_EndTransfer(u8x8);      
      break;
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      ngfx_u8x8_byte_StartTransfer(u8x8);
      ngfx_u8x8_byte_SendByte(u8x8, 0x040);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      ngfx_u8x8_byte_EndTransfer(u8x8);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
      /* see ssd13xx driver */
      p = (uint8_t *)arg_ptr;
       while( arg_int > 24 )
      {
	ngfx_u8x8_i2c_data_transfer(u8x8, 24, p);
	arg_int-=24;
	p+=24;
      }
      ngfx_u8x8_i2c_data_transfer(u8x8, arg_int, p);
      break;
    case NGFX_U8X8_MSG_CAD_INIT:
      /* apply default i2c adr if required so that the start transfer msg can use this */
      if ( u8x8->i2c_address == 255 )
	u8x8->i2c_address = 0x078;	/* ST75256, often this is 0x07e */
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      /* cad transfer commands are ignored */
      break;
    default:
      return 0;
  }
  return 1;
}

/* cad i2c procedure for the ld7032 controller */
/* Issue https://github.com/olikraus/u8g2/issues/865 mentiones, that I2C does not work */
/* Workaround is to remove the while loop (or increase the value in the condition) */
uint8_t ngfx_u8x8_cad_ld7032_i2c(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t in_transfer = 0;
  uint8_t *p;
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
      if ( in_transfer != 0 )
	ngfx_u8x8_byte_EndTransfer(u8x8); 
      ngfx_u8x8_byte_StartTransfer(u8x8);
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      in_transfer = 1;
      break;
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
      //ngfx_u8x8_byte_SetDC(u8x8, 1);
    
      /* the FeatherWing OLED with the 32u4 transfer of long byte */
      /* streams was not possible. This is broken down to */
      /* smaller streams, 32 seems to be the limit... */
      /* I guess this is related to the size of the Wire buffers in Arduino */
      /* Unfortunately, this can not be handled in the byte level drivers, */
      /* so this is done here. Even further, only 24 bytes will be sent, */
      /* because there will be another byte (DC) required during the transfer */
      p = (uint8_t *)arg_ptr;
       while( arg_int > 24 )
      {
	u8x8->byte_cb(u8x8, NGFX_U8X8_MSG_CAD_SEND_DATA, 24, p);
	arg_int-=24;
	p+=24;
	ngfx_u8x8_byte_EndTransfer(u8x8); 
	ngfx_u8x8_byte_StartTransfer(u8x8);
	ngfx_u8x8_byte_SendByte(u8x8, 0x08);	/* data write for LD7032 */
      }
      u8x8->byte_cb(u8x8, NGFX_U8X8_MSG_CAD_SEND_DATA, arg_int, p);
      break;
    case NGFX_U8X8_MSG_CAD_INIT:
      /* apply default i2c adr if required so that the start transfer msg can use this */
      if ( u8x8->i2c_address == 255 )
	u8x8->i2c_address = 0x060;
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
      in_transfer = 0;
      break;
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      if ( in_transfer != 0 )
	ngfx_u8x8_byte_EndTransfer(u8x8); 
      break;
    default:
      return 0;
  }
  return 1;
}

/* cad procedure for the UC16xx family in I2C mode */
/* ngfx_u8x8_byte_SetDC is not used */
/* DC bit is encoded into the adr byte, structure is CAD001 */
uint8_t ngfx_u8x8_cad_uc16xx_i2c(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t in_transfer = 0;	
  static uint8_t is_data = 0;
  uint8_t *p;
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      if ( in_transfer != 0 )
      {
	if ( is_data != 0 )
	{
	  /* transfer mode is active, but data transfer */
	  ngfx_u8x8_byte_EndTransfer(u8x8); 
	  /* clear the lowest two bits of the adr */
	  ngfx_u8x8_SetI2CAddress( u8x8, ngfx_u8x8_GetI2CAddress(u8x8)&0x0fc );
	  ngfx_u8x8_byte_StartTransfer(u8x8); 
	}
      }
      else
      {
	/* clear the lowest two bits of the adr */
	ngfx_u8x8_SetI2CAddress( u8x8, ngfx_u8x8_GetI2CAddress(u8x8)&0x0fc );
	ngfx_u8x8_byte_StartTransfer(u8x8);
      }
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      in_transfer = 1;
      // is_data = 0;  // 20 Jun 2021: I assume that this is missing here
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
      if ( in_transfer != 0 )
      {
	if ( is_data == 0 )
	{
	  /* transfer mode is active, but data transfer */
	  ngfx_u8x8_byte_EndTransfer(u8x8); 
	  /* clear the lowest two bits of the adr */
	  ngfx_u8x8_SetI2CAddress( u8x8, (ngfx_u8x8_GetI2CAddress(u8x8)&0x0fc)|2 );
	  ngfx_u8x8_byte_StartTransfer(u8x8); 
	}
      }
      else
      {
	/* clear the lowest two bits of the adr */
	ngfx_u8x8_SetI2CAddress( u8x8, (ngfx_u8x8_GetI2CAddress(u8x8)&0x0fc)|2 );
	ngfx_u8x8_byte_StartTransfer(u8x8);
      }
      in_transfer = 1;
      // is_data = 1;  // 20 Jun 2021: I assume that this is missing here
      
      p = (uint8_t *)arg_ptr;
      while( arg_int > 24 )
      {
	u8x8->byte_cb(u8x8, NGFX_U8X8_MSG_CAD_SEND_DATA, 24, p);
	arg_int-=24;
	p+=24;
	ngfx_u8x8_byte_EndTransfer(u8x8); 
	ngfx_u8x8_byte_StartTransfer(u8x8);
      }
      u8x8->byte_cb(u8x8, NGFX_U8X8_MSG_CAD_SEND_DATA, arg_int, p);
      
      break;
    case NGFX_U8X8_MSG_CAD_INIT:
      /* apply default i2c adr if required so that the start transfer msg can use this */
      if ( u8x8->i2c_address == 255 )
	u8x8->i2c_address = 0x070;
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
      in_transfer = 0;    
      /* actual start is delayed, because we do not whether this is data or cmd transfer */
      break;
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      if ( in_transfer != 0 )
	ngfx_u8x8_byte_EndTransfer(u8x8);
      in_transfer = 0;
      break;
    default:
      return 0;
  }
  return 1;
}


/* cad procedure for the UC1638 in I2C mode */
/* same as  ngfx_u8x8_cad_uc16xx_i2c but CAD structure is CAD011 */
uint8_t ngfx_u8x8_cad_uc1638_i2c(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t in_transfer = 0;	
  static uint8_t is_data = 0;
  uint8_t *p;
  switch(msg)
  {
    case NGFX_U8X8_MSG_CAD_SEND_CMD:
      if ( in_transfer != 0 )
      {
	if ( is_data != 0 )
	{
	  /* transfer mode is active, but data transfer */
	  ngfx_u8x8_byte_EndTransfer(u8x8); 
	  /* clear the lowest two bits of the adr */
	  ngfx_u8x8_SetI2CAddress( u8x8, ngfx_u8x8_GetI2CAddress(u8x8)&0x0fc );
	  ngfx_u8x8_byte_StartTransfer(u8x8); 
	}
      }
      else
      {
	/* clear the lowest two bits of the adr */
	ngfx_u8x8_SetI2CAddress( u8x8, ngfx_u8x8_GetI2CAddress(u8x8)&0x0fc );
	ngfx_u8x8_byte_StartTransfer(u8x8);
      }
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      in_transfer = 1;
      is_data = 0;
      break;
    case NGFX_U8X8_MSG_CAD_SEND_ARG:
      if ( in_transfer != 0 )
      {
	if ( is_data == 0 )
	{
	  /* transfer mode is active, but data transfer */
	  ngfx_u8x8_byte_EndTransfer(u8x8); 
	  /* clear the lowest two bits of the adr */
	  ngfx_u8x8_SetI2CAddress( u8x8, (ngfx_u8x8_GetI2CAddress(u8x8)&0x0fc)|2 );
	  ngfx_u8x8_byte_StartTransfer(u8x8); 
	}
      }
      else
      {
	/* clear the lowest two bits of the adr */
	ngfx_u8x8_SetI2CAddress( u8x8, (ngfx_u8x8_GetI2CAddress(u8x8)&0x0fc)|2 );
	ngfx_u8x8_byte_StartTransfer(u8x8);
      }
      ngfx_u8x8_byte_SendByte(u8x8, arg_int);
      in_transfer = 1;
      is_data = 1;
      break;
    case NGFX_U8X8_MSG_CAD_SEND_DATA:
      if ( in_transfer != 0 )
      {
	if ( is_data == 0 )
	{
	  /* transfer mode is active, but data transfer */
	  ngfx_u8x8_byte_EndTransfer(u8x8); 
	  /* clear the lowest two bits of the adr */
	  ngfx_u8x8_SetI2CAddress( u8x8, (ngfx_u8x8_GetI2CAddress(u8x8)&0x0fc)|2 );
	  ngfx_u8x8_byte_StartTransfer(u8x8); 
	}
      }
      else
      {
	/* clear the lowest two bits of the adr */
	ngfx_u8x8_SetI2CAddress( u8x8, (ngfx_u8x8_GetI2CAddress(u8x8)&0x0fc)|2 );
	ngfx_u8x8_byte_StartTransfer(u8x8);
      }
      in_transfer = 1;
      is_data = 1;
      
      p = (uint8_t *)arg_ptr;
      while( arg_int > 24 )
      {
	u8x8->byte_cb(u8x8, NGFX_U8X8_MSG_CAD_SEND_DATA, 24, p);
	arg_int-=24;
	p+=24;
	ngfx_u8x8_byte_EndTransfer(u8x8); 
	ngfx_u8x8_byte_StartTransfer(u8x8);
      }
      u8x8->byte_cb(u8x8, NGFX_U8X8_MSG_CAD_SEND_DATA, arg_int, p);
      
      break;
    case NGFX_U8X8_MSG_CAD_INIT:
      /* apply default i2c adr if required so that the start transfer msg can use this */
      if ( u8x8->i2c_address == 255 )
	u8x8->i2c_address = 0x078;  /* see also https://github.com/olikraus/u8g2/issues/371 for a discussion on this value */
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case NGFX_U8X8_MSG_CAD_START_TRANSFER:
      in_transfer = 0;    
      /* actual start is delayed, because we do not whether this is data or cmd transfer */
      break;
    case NGFX_U8X8_MSG_CAD_END_TRANSFER:
      if ( in_transfer != 0 )
	ngfx_u8x8_byte_EndTransfer(u8x8);
      in_transfer = 0;
      break;
    default:
      return 0;
  }
  return 1;
}
