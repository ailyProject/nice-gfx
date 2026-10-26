/*

  ngfx_u8x8_byte.c 
  
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
  
  
*/

#include "u8x8.h"

uint8_t ngfx_u8x8_byte_SetDC(ngfx_u8x8_t *u8x8, uint8_t dc)
{
  return u8x8->byte_cb(u8x8, NGFX_U8X8_MSG_BYTE_SET_DC, dc, NULL);
}

uint8_t ngfx_u8x8_byte_SendBytes(ngfx_u8x8_t *u8x8, uint8_t cnt, uint8_t *data)
{
  return u8x8->byte_cb(u8x8, NGFX_U8X8_MSG_BYTE_SEND, cnt, (void *)data);
}

uint8_t ngfx_u8x8_byte_SendByte(ngfx_u8x8_t *u8x8, uint8_t byte)
{
  return ngfx_u8x8_byte_SendBytes(u8x8, 1, &byte);
}

uint8_t ngfx_u8x8_byte_StartTransfer(ngfx_u8x8_t *u8x8)
{
  return u8x8->byte_cb(u8x8, NGFX_U8X8_MSG_BYTE_START_TRANSFER, 0, NULL);
}

uint8_t ngfx_u8x8_byte_EndTransfer(ngfx_u8x8_t *u8x8)
{
  return u8x8->byte_cb(u8x8, NGFX_U8X8_MSG_BYTE_END_TRANSFER, 0, NULL);
}

/*=========================================*/

uint8_t ngfx_u8x8_byte_empty(NGFX_U8X8_UNUSED ngfx_u8x8_t *u8x8, uint8_t msg, NGFX_U8X8_UNUSED uint8_t arg_int, NGFX_U8X8_UNUSED void *arg_ptr)
{
  switch(msg)
  {
    case NGFX_U8X8_MSG_BYTE_SEND:
    case NGFX_U8X8_MSG_BYTE_INIT:
    case NGFX_U8X8_MSG_BYTE_SET_DC:
    case NGFX_U8X8_MSG_BYTE_START_TRANSFER:
    case NGFX_U8X8_MSG_BYTE_END_TRANSFER:
      break;	/* do nothing */
  }
  return 1;	/* always succeed */
}


/*=========================================*/


/*
  Uses:
    u8x8->display_info->sda_setup_time_ns
    u8x8->display_info->sck_pulse_width_ns
    u8x8->display_info->spi_mode
    u8x8->display_info->chip_disable_level
    u8x8->display_info->chip_enable_level
    u8x8->display_info->post_chip_enable_wait_ns
    u8x8->display_info->pre_chip_disable_wait_ns
  Calls to GPIO and DELAY:
    NGFX_U8X8_MSG_DELAY_NANO
    NGFX_U8X8_MSG_GPIO_DC
    NGFX_U8X8_MSG_GPIO_CS
    NGFX_U8X8_MSG_GPIO_CLOCK
    NGFX_U8X8_MSG_GPIO_DATA
  Handles:
    NGFX_U8X8_MSG_BYTE_INIT
    NGFX_U8X8_MSG_BYTE_SEND
    NGFX_U8X8_MSG_BYTE_SET_DC
    NGFX_U8X8_MSG_BYTE_START_TRANSFER
    NGFX_U8X8_MSG_BYTE_END_TRANSFER
*/

uint8_t ngfx_u8x8_byte_4wire_sw_spi(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t i, b;
  uint8_t *data;
  uint8_t takeover_edge = ngfx_u8x8_GetSPIClockPhase(u8x8);
  uint8_t not_takeover_edge = 1 - takeover_edge;
 
  switch(msg)
  {
    case NGFX_U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
	b = *data;
	data++;
	arg_int--;
	for( i = 0; i < 8; i++ )
	{
	  if ( b & 128 )
	    ngfx_u8x8_gpio_SetSPIData(u8x8, 1);
	  else
	    ngfx_u8x8_gpio_SetSPIData(u8x8, 0);
	  b <<= 1;
	  
	  ngfx_u8x8_gpio_SetSPIClock(u8x8, not_takeover_edge);
	  ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->sda_setup_time_ns);
	  ngfx_u8x8_gpio_SetSPIClock(u8x8, takeover_edge);
	  ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->sck_pulse_width_ns);
	}    
      }
      break;
      
    case NGFX_U8X8_MSG_BYTE_INIT:
      /* disable chipselect */
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      /* no wait required here */
      
      /* for SPI: setup correct level of the clock signal */
      ngfx_u8x8_gpio_SetSPIClock(u8x8, ngfx_u8x8_GetSPIClockPhase(u8x8));
      break;
    case NGFX_U8X8_MSG_BYTE_SET_DC:
      ngfx_u8x8_gpio_SetDC(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_BYTE_START_TRANSFER:
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);  
      u8x8->gpio_and_delay_cb(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
      break;
    case NGFX_U8X8_MSG_BYTE_END_TRANSFER:
      u8x8->gpio_and_delay_cb(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      break;
    default:
      return 0;
  }
  return 1;
}

/*=========================================*/

uint8_t ngfx_u8x8_byte_8bit_6800mode(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t i, b;
  uint8_t *data;
 
  switch(msg)
  {
    case NGFX_U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
	b = *data;
	data++;
	arg_int--;
	for( i = NGFX_U8X8_MSG_GPIO_D0; i <= NGFX_U8X8_MSG_GPIO_D7; i++ )
	{
	  ngfx_u8x8_gpio_call(u8x8, i, b&1);
	  b >>= 1;
	}    
	
	ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->data_setup_time_ns);
	ngfx_u8x8_gpio_call(u8x8, NGFX_U8X8_MSG_GPIO_E, 1);
	ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->write_pulse_width_ns);
	ngfx_u8x8_gpio_call(u8x8, NGFX_U8X8_MSG_GPIO_E, 0);
      }
      break;
      
    case NGFX_U8X8_MSG_BYTE_INIT:
      /* disable chipselect */
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);    
      /* ensure that the enable signal is high */
      ngfx_u8x8_gpio_call(u8x8, NGFX_U8X8_MSG_GPIO_E, 0);
      break;
    case NGFX_U8X8_MSG_BYTE_SET_DC:
      ngfx_u8x8_gpio_SetDC(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_BYTE_START_TRANSFER:
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);  
      u8x8->gpio_and_delay_cb(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
      break;
    case NGFX_U8X8_MSG_BYTE_END_TRANSFER:
      u8x8->gpio_and_delay_cb(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      break;
    default:
      return 0;
  }
  return 1;
}

uint8_t ngfx_u8x8_byte_8bit_8080mode(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t i, b;
  uint8_t *data;
 
  switch(msg)
  {
    case NGFX_U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
	b = *data;
	data++;
	arg_int--;
	for( i = NGFX_U8X8_MSG_GPIO_D0; i <= NGFX_U8X8_MSG_GPIO_D7; i++ )
	{
	  ngfx_u8x8_gpio_call(u8x8, i, b&1);
	  b >>= 1;
	}    
	
	ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->data_setup_time_ns);
	ngfx_u8x8_gpio_call(u8x8, NGFX_U8X8_MSG_GPIO_E, 0);
	ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->write_pulse_width_ns);
	ngfx_u8x8_gpio_call(u8x8, NGFX_U8X8_MSG_GPIO_E, 1);
      }
      break;
      
    case NGFX_U8X8_MSG_BYTE_INIT:
      /* disable chipselect */
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);    
      /* ensure that the enable signal is high */
      ngfx_u8x8_gpio_call(u8x8, NGFX_U8X8_MSG_GPIO_E, 1);
      break;
    case NGFX_U8X8_MSG_BYTE_SET_DC:
      ngfx_u8x8_gpio_SetDC(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_BYTE_START_TRANSFER:
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);  
      u8x8->gpio_and_delay_cb(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
      break;
    case NGFX_U8X8_MSG_BYTE_END_TRANSFER:
      u8x8->gpio_and_delay_cb(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      break;
    default:
      return 0;
  }
  return 1;
}

/*=========================================*/

uint8_t ngfx_u8x8_byte_3wire_sw_spi(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t i;
  uint8_t *data;
  uint8_t takeover_edge = ngfx_u8x8_GetSPIClockPhase(u8x8);
  uint8_t not_takeover_edge = 1 - takeover_edge;
  uint16_t b;
  static uint8_t last_dc;
 
  switch(msg)
  {
    case NGFX_U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
	b = *data;
	if ( last_dc != 0 )
	  b |= 256;
	data++;
	arg_int--;
	for( i = 0; i < 9; i++ )
	{
	  if ( b & 256 )
	    ngfx_u8x8_gpio_SetSPIData(u8x8, 1);
	  else
	    ngfx_u8x8_gpio_SetSPIData(u8x8, 0);
	  b <<= 1;
	  
	  ngfx_u8x8_gpio_SetSPIClock(u8x8, not_takeover_edge);
	  ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->sda_setup_time_ns);
	  ngfx_u8x8_gpio_SetSPIClock(u8x8, takeover_edge);
	  ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->sck_pulse_width_ns);
	}    
      }
      break;
      
    case NGFX_U8X8_MSG_BYTE_INIT:
      /* disable chipselect */
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      /* no wait required here */
      
      /* for SPI: setup correct level of the clock signal */
      ngfx_u8x8_gpio_SetSPIClock(u8x8, ngfx_u8x8_GetSPIClockPhase(u8x8));
      break;
    case NGFX_U8X8_MSG_BYTE_SET_DC:
      last_dc = arg_int;
      break;
    case NGFX_U8X8_MSG_BYTE_START_TRANSFER:
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);  
      u8x8->gpio_and_delay_cb(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
      break;
    case NGFX_U8X8_MSG_BYTE_END_TRANSFER:
      u8x8->gpio_and_delay_cb(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      break;
    default:
      return 0;
  }
  return 1;
}

/*=========================================*/

void ngfx_u8x8_byte_set_ks0108_cs(ngfx_u8x8_t *u8x8, uint8_t arg)
{
  ngfx_u8x8_gpio_SetCS(u8x8, arg&1);
  arg = arg >> 1;
  ngfx_u8x8_gpio_call(u8x8, NGFX_U8X8_MSG_GPIO_CS1, arg&1);
  arg = arg >> 1;
  ngfx_u8x8_gpio_call(u8x8, NGFX_U8X8_MSG_GPIO_CS2, arg&1);
}

/* 6800 mode */
uint8_t ngfx_u8x8_byte_ks0108(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t i, b;
  uint8_t *data;
 
  switch(msg)
  {
    case NGFX_U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
	b = *data;
	data++;
	arg_int--;
	for( i = NGFX_U8X8_MSG_GPIO_D0; i <= NGFX_U8X8_MSG_GPIO_D7; i++ )
	{
	  ngfx_u8x8_gpio_call(u8x8, i, b&1);
	  b >>= 1;
	}    
	
	ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->data_setup_time_ns);
	ngfx_u8x8_gpio_call(u8x8, NGFX_U8X8_MSG_GPIO_E, 1);
	ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->write_pulse_width_ns);
	ngfx_u8x8_gpio_call(u8x8, NGFX_U8X8_MSG_GPIO_E, 0);
      }
      break;
      
    case NGFX_U8X8_MSG_BYTE_INIT:
      /* disable chipselect */
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);    
      /* ensure that the enable signal is low */
      ngfx_u8x8_gpio_call(u8x8, NGFX_U8X8_MSG_GPIO_E, 0);
      break;
    case NGFX_U8X8_MSG_BYTE_SET_DC:
      ngfx_u8x8_gpio_SetDC(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_BYTE_START_TRANSFER:
      /* expects 3 bits in arg_int for the chip select lines */ 
      ngfx_u8x8_byte_set_ks0108_cs(u8x8, arg_int);
      u8x8->gpio_and_delay_cb(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
      break;
    case NGFX_U8X8_MSG_BYTE_END_TRANSFER:
      u8x8->gpio_and_delay_cb(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
      ngfx_u8x8_byte_set_ks0108_cs(u8x8, arg_int);
      break;
    default:
      return 0;
  }
  return 1;
}


/* sed1520 or sbn1661 
  NGFX_U8X8_MSG_GPIO_E --> E1
  NGFX_U8X8_MSG_GPIO_CS --> E2
*/
uint8_t ngfx_u8x8_byte_sed1520(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t i, b;
  uint8_t *data;
  static uint8_t enable_pin;
 
  switch(msg)
  {
    case NGFX_U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
	b = *data;
	data++;
	arg_int--;
	for( i = NGFX_U8X8_MSG_GPIO_D0; i <= NGFX_U8X8_MSG_GPIO_D7; i++ )
	{
	  ngfx_u8x8_gpio_call(u8x8, i, b&1);
	  b >>= 1;
	}    
	
	ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->data_setup_time_ns);
	ngfx_u8x8_gpio_call(u8x8, enable_pin, 1);
	ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, 200);		/* KS0108 requires 450 ns, use 200 here */
	ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_NANO, u8x8->display_info->write_pulse_width_ns);  /* expect 250 here */
	ngfx_u8x8_gpio_call(u8x8, enable_pin, 0);
      }
      break;
      
    case NGFX_U8X8_MSG_BYTE_INIT:
      /* disable chipselect */
      ngfx_u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);    
      /* ensure that the enable signals are low */
      ngfx_u8x8_gpio_call(u8x8, NGFX_U8X8_MSG_GPIO_E, 0);
      ngfx_u8x8_gpio_call(u8x8, NGFX_U8X8_MSG_GPIO_CS, 0);
      enable_pin = NGFX_U8X8_MSG_GPIO_E;
      break;
    case NGFX_U8X8_MSG_BYTE_SET_DC:
      ngfx_u8x8_gpio_SetDC(u8x8, arg_int);
      break;
    case NGFX_U8X8_MSG_BYTE_START_TRANSFER:
      /* cs lines are not supported for the SED1520/SBN1661 */
      /* instead, this will select the E1 or E2 line */ 
      /* arg_int is set by ngfx_u8x8_d_sbn1661_122x32() function */
      enable_pin = NGFX_U8X8_MSG_GPIO_E;
      if ( arg_int != 0 )
	enable_pin = NGFX_U8X8_MSG_GPIO_CS;
      break;
    case NGFX_U8X8_MSG_BYTE_END_TRANSFER:
      break;
    default:
      return 0;
  }
  return 1;
}

/*=========================================*/


/*
  software i2c,
  ignores ACK response (which is anyway not provided by some displays)
  also does not allow reading from the device
*/
static void i2c_delay(ngfx_u8x8_t *u8x8) NGFX_U8X8_NOINLINE;
static void i2c_delay(ngfx_u8x8_t *u8x8)
{
  //ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_10MICRO, u8x8->display_info->i2c_bus_clock_100kHz);
  ngfx_u8x8_gpio_Delay(u8x8, NGFX_U8X8_MSG_DELAY_I2C, u8x8->display_info->i2c_bus_clock_100kHz);
}

static void i2c_init(ngfx_u8x8_t *u8x8)
{
  ngfx_u8x8_gpio_SetI2CClock(u8x8, 1);
  ngfx_u8x8_gpio_SetI2CData(u8x8, 1);
  
  i2c_delay(u8x8);
}

/* actually, the scl line is not observed, so this procedure does not return a value */

static void i2c_read_scl_and_delay(ngfx_u8x8_t *u8x8)
{
  /* set as input (line will be high) */
  ngfx_u8x8_gpio_SetI2CClock(u8x8, 1);

  i2c_delay(u8x8);
}

static void i2c_clear_scl(ngfx_u8x8_t *u8x8)
{
  ngfx_u8x8_gpio_SetI2CClock(u8x8, 0);
}

static void i2c_read_sda(ngfx_u8x8_t *u8x8)
{
  /* set as input (line will be high) */
  ngfx_u8x8_gpio_SetI2CData(u8x8, 1);
}

static void i2c_clear_sda(ngfx_u8x8_t *u8x8)
{
  /* set open collector and drive low */
  ngfx_u8x8_gpio_SetI2CData(u8x8, 0);
}

static void i2c_start(ngfx_u8x8_t *u8x8)
{
  if ( u8x8->i2c_started != 0 )
  {
    /* if already started: do restart */
    i2c_read_sda(u8x8);     /* SDA = 1 */
    i2c_delay(u8x8);
    i2c_read_scl_and_delay(u8x8);
  }
  i2c_read_sda(u8x8);
  /* send the start condition, both lines go from 1 to 0 */
  i2c_clear_sda(u8x8);
  i2c_delay(u8x8);
  i2c_clear_scl(u8x8);
  u8x8->i2c_started = 1;
}


static void i2c_stop(ngfx_u8x8_t *u8x8)
{
  /* set SDA to 0 */
  i2c_clear_sda(u8x8);  
  i2c_delay(u8x8);
 
  /* now release all lines */
  i2c_read_scl_and_delay(u8x8);
 
  /* set SDA to 1 */
  i2c_read_sda(u8x8);
  i2c_delay(u8x8);
  u8x8->i2c_started = 0;
}

static void i2c_write_bit(ngfx_u8x8_t *u8x8, uint8_t val)
{
  if (val)
    i2c_read_sda(u8x8);
  else
    i2c_clear_sda(u8x8);
 
  i2c_delay(u8x8);
  i2c_read_scl_and_delay(u8x8);
  i2c_clear_scl(u8x8);
}

static void i2c_read_bit(ngfx_u8x8_t *u8x8)
{
  //uint8_t val;
  /* do not drive SDA */
  i2c_read_sda(u8x8);
  i2c_delay(u8x8);
  i2c_read_scl_and_delay(u8x8);
  i2c_read_sda(u8x8);
  i2c_delay(u8x8);
  i2c_clear_scl(u8x8);
  //return val;
}

static void i2c_write_byte(ngfx_u8x8_t *u8x8, uint8_t b)
{
  i2c_write_bit(u8x8, b & 128);
  i2c_write_bit(u8x8, b & 64);
  i2c_write_bit(u8x8, b & 32);
  i2c_write_bit(u8x8, b & 16);
  i2c_write_bit(u8x8, b & 8);
  i2c_write_bit(u8x8, b & 4);
  i2c_write_bit(u8x8, b & 2);
  i2c_write_bit(u8x8, b & 1);
    
  /* read ack from client */
  /* 0: ack was given by client */
  /* 1: nothing happend during ack cycle */  
  i2c_read_bit(u8x8);
}

uint8_t ngfx_u8x8_byte_sw_i2c(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t *data;

  switch(msg)
  {
    case NGFX_U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
    
      while( arg_int > 0 )
      {
	i2c_write_byte(u8x8, *data);
	data++;
	arg_int--;
      }
      
      break;
      
    case NGFX_U8X8_MSG_BYTE_INIT:
      i2c_init(u8x8);
      break;
    case NGFX_U8X8_MSG_BYTE_SET_DC:
      break;
    case NGFX_U8X8_MSG_BYTE_START_TRANSFER:
      i2c_start(u8x8);
      i2c_write_byte(u8x8, ngfx_u8x8_GetI2CAddress(u8x8));
      //i2c_write_byte(u8x8, 0x078);
      break;
    case NGFX_U8X8_MSG_BYTE_END_TRANSFER:
      i2c_stop(u8x8);
      break;
    default:
      return 0;
  }
  return 1;
}

/*=========================================*/

/* alternative i2c byte procedure */
#ifdef ALTERNATIVE_I2C_BYTE_PROCEDURE


void i2c_transfer(ngfx_u8x8_t *u8x8, uint8_t adr, uint8_t cnt, uint8_t *data)
{
  uint8_t i;
  i2c_start(u8x8);
  i2c_write_byte(u8x8, adr);
  for( i = 0; i < cnt; i++ )
    i2c_write_byte(u8x8, data[i]);
  i2c_stop(u8x8);  
}


uint8_t ngfx_u8x8_byte_sw_i2c(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t buffer[32];		/* u8g2/u8x8 will never send more than 32 bytes */
  static uint8_t buf_idx;
  uint8_t *data;
 
  switch(msg)
  {
    case NGFX_U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;      
      while( arg_int > 0 )
      {
	buffer[buf_idx++] = *data;
	data++;
	arg_int--;
      }      
      break;
    case NGFX_U8X8_MSG_BYTE_INIT:
      i2c_init(u8x8);			/* init i2c communication */
      break;
    case NGFX_U8X8_MSG_BYTE_SET_DC:
      /* ignored for i2c */
      break;
    case NGFX_U8X8_MSG_BYTE_START_TRANSFER:
      buf_idx = 0;
      break;
    case NGFX_U8X8_MSG_BYTE_END_TRANSFER:
      i2c_transfer(u8x8, ngfx_u8x8_GetI2CAddress(u8x8), buf_idx, buffer);
      break;
    default:
      return 0;
  }
  return 1;
}

#endif
