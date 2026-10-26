/*

  ngfx_u8x8_d_ssd1305.c

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2017, olikraus@gmail.com
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




static const uint8_t ngfx_u8x8_d_ssd1305_powersave0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0af),		                /* display on */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_ssd1305_powersave1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0ae),		                /* display off */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_ssd1305_128x32_flip0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_CA(0x0d3, 32),			/* display offset to 32 */
  NGFX_U8X8_C(0x0a1),				/* segment remap a0/a1*/
  NGFX_U8X8_C(0x0c8),				/* c0: scan dir normal, c8: reverse */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_ssd1305_128x32_flip1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_CA(0x0d3, 0),			/* display offset to  */
  NGFX_U8X8_C(0x0a0),				/* segment remap a0/a1*/
  NGFX_U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};



static uint8_t ngfx_u8x8_d_ssd1305_generic(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;
  switch(msg)
  {
    case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
      ngfx_u8x8_cad_StartTransfer(u8x8);
      x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;    
      x *= 8;
      x += u8x8->x_offset;
    
      ngfx_u8x8_cad_SendCmd(u8x8, 0x040 );	/* set line offset to 0 */
    
      ngfx_u8x8_cad_SendCmd(u8x8, 0x010 | (x>>4) );
      ngfx_u8x8_cad_SendArg(u8x8, 0x000 | ((x&15)));
      ngfx_u8x8_cad_SendArg(u8x8, 0x0b0 | (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos)   );

    
      do
      {
	c = ((ngfx_u8x8_tile_t *)arg_ptr)->cnt;
	ptr = ((ngfx_u8x8_tile_t *)arg_ptr)->tile_ptr;
	ngfx_u8x8_cad_SendData(u8x8, c*8, ptr); 	/* note: SendData can not handle more than 255 bytes */
	/*
	do
	{
	  ngfx_u8x8_cad_SendData(u8x8, 8, ptr);
	  ptr += 8;
	  c--;
	} while( c > 0 );
	*/
	arg_int--;
      } while( arg_int > 0 );
      
      ngfx_u8x8_cad_EndTransfer(u8x8);
      break;
    case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      if ( arg_int == 0 )
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_powersave0_seq);
      else
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_powersave1_seq);
      break;
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
    case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:
      ngfx_u8x8_cad_StartTransfer(u8x8);
      ngfx_u8x8_cad_SendCmd(u8x8, 0x081 );
      ngfx_u8x8_cad_SendArg(u8x8, arg_int );	/* ssd1305 has range from 0 to 255 */
      ngfx_u8x8_cad_EndTransfer(u8x8);
      break;
#endif
    default:
      return 0;
  }
  return 1;
}

/* timing from SSD1306 */
static const ngfx_u8x8_display_info_t ngfx_u8x8_ssd1305_128x32_noname_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 10,
  /* reset_pulse_width_ms = */ 100, 	/* SSD1306: 3 us */
  /* post_reset_wait_ms = */ 100, /* far east OLEDs need much longer setup time */
  /* sda_setup_time_ns = */ 50,		/* SSD1306: 15ns, but cycle time is 100ns, so use 100/2 */
  /* sck_pulse_width_ns = */ 50,	/* SSD1306: 20ns, but cycle time is 100ns, so use 100/2, AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,
  /* write_pulse_width_ns = */ 150,	/* SSD1306: cycle time is 300ns, so use 300/2 = 150 */
  /* tile_width = */ 16,
  /* tile_height = */ 4,
  /* default_x_offset = */ 2,
  /* flipmode_x_offset = */ 2,
  /* pixel_width = */ 128,
  /* pixel_height = */ 32
};


static const uint8_t ngfx_u8x8_d_ssd1305_128x32_noname_init_seq[] = {
    
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  
  NGFX_U8X8_C(0x0ae),		                /* display off */
  NGFX_U8X8_CA(0x0d5, 0x080),		/* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
  NGFX_U8X8_CA(0x0a8, 0x03f),		/* multiplex ratio */
  NGFX_U8X8_CA(0x0d3, 32),			/* display offset to 32 */
  NGFX_U8X8_C(0x040),		        	/* set display start line to 0 */
  NGFX_U8X8_CA(0x020, 0x000),		/* horizontal addressing mode */
  
  NGFX_U8X8_C(0x0a1),				/* segment remap a0/a1*/
  NGFX_U8X8_C(0x0c8),				/* c0: scan dir normal, c8: reverse */
  // Flipmode
  // NGFX_U8X8_C(0x0a0),				/* segment remap a0/a1*/
  // NGFX_U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  
  NGFX_U8X8_CA(0x0da, 0x012),		/* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */

  NGFX_U8X8_CA(0x081, 0x0cf), 		/* [2] set contrast control */
  NGFX_U8X8_CA(0x0d9, 0x0f1), 		/* [2] pre-charge period 0x022/f1*/
  NGFX_U8X8_CA(0x0db, 0x040), 		/* vcomh deselect level */  
  // if vcomh is 0, then this will give the biggest range for contrast control issue #98
  // restored the old values for the noname constructor, because vcomh=0 will not work for all OLEDs, #116
  
  NGFX_U8X8_C(0x02e),				/* Deactivate scroll */ 
  NGFX_U8X8_C(0x0a4),				/* output ram to display */
  NGFX_U8X8_C(0x0a6),				/* none inverted normal display mode */
    
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


uint8_t ngfx_u8x8_d_ssd1305_128x32_noname(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    
  if ( ngfx_u8x8_d_ssd1305_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;
  
  switch(msg)
  {
    case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      if ( arg_int == 0 )
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x32_flip0_seq);
	u8x8->x_offset = u8x8->display_info->default_x_offset;
      }
      else
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x32_flip1_seq);
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
      }
      break;
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x32_noname_init_seq);    
      break;
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_ssd1305_128x32_noname_display_info);
      break;
    default:
      return 0;
  }
  return 1;
}

/*================================================*/
/* issue 2002: Waveshare */

#ifdef SAME_AS_ADAFRUIT
/* timing from SSD1306 */
static const ngfx_u8x8_display_info_t ngfx_u8x8_ssd1305_128x32_waveshare_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 10,
  /* reset_pulse_width_ms = */ 100, 	/* SSD1306: 3 us */
  /* post_reset_wait_ms = */ 100, /* far east OLEDs need much longer setup time */
  /* sda_setup_time_ns = */ 50,		/* SSD1306: 15ns, but cycle time is 100ns, so use 100/2 */
  /* sck_pulse_width_ns = */ 50,	/* SSD1306: 20ns, but cycle time is 100ns, so use 100/2, AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,
  /* write_pulse_width_ns = */ 150,	/* SSD1306: cycle time is 300ns, so use 300/2 = 150 */
  /* tile_width = */ 16,
  /* tile_height = */ 4,
  /* default_x_offset = */ 4,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 128,
  /* pixel_height = */ 32
};

uint8_t ngfx_u8x8_d_ssd1305_128x32_waveshare(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    
  if ( ngfx_u8x8_d_ssd1305_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;
  
  switch(msg)
  {
    case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      if ( arg_int == 0 )
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x32_flip0_seq);
	u8x8->x_offset = u8x8->display_info->default_x_offset;
      }
      else
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x32_flip1_seq);
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
      }
      break;
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x32_noname_init_seq);    
      break;
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_ssd1305_128x32_waveshare_display_info);
      break;
    default:
      return 0;
  }
  return 1;
}
#endif


/*================================================*/
/* adafruit 128x32 SSD1305 OLED, https://www.adafruit.com/product/2675 */
/* issue 724 */

/* timing from SSD1306 */
static const ngfx_u8x8_display_info_t ngfx_u8x8_ssd1305_128x32_adafruit_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 10,
  /* reset_pulse_width_ms = */ 100, 	/* SSD1306: 3 us */
  /* post_reset_wait_ms = */ 100, /* far east OLEDs need much longer setup time */
  /* sda_setup_time_ns = */ 50,		/* SSD1306: 15ns, but cycle time is 100ns, so use 100/2 */
  /* sck_pulse_width_ns = */ 50,	/* SSD1306: 20ns, but cycle time is 100ns, so use 100/2, AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,
  /* write_pulse_width_ns = */ 150,	/* SSD1306: cycle time is 300ns, so use 300/2 = 150 */
  /* tile_width = */ 16,
  /* tile_height = */ 4,
  /* default_x_offset = */ 4,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 128,
  /* pixel_height = */ 32
};


uint8_t ngfx_u8x8_d_ssd1305_128x32_adafruit(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    
  if ( ngfx_u8x8_d_ssd1305_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;
  
  switch(msg)
  {
    case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      if ( arg_int == 0 )
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x32_flip0_seq);
	u8x8->x_offset = u8x8->display_info->default_x_offset;
      }
      else
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x32_flip1_seq);
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
      }
      break;
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x32_noname_init_seq);    
      break;
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_ssd1305_128x32_adafruit_display_info);
      break;
    default:
      return 0;
  }
  return 1;
}




/*================================================*/
/* adafruit SSD1305 OLED */

/* timing from SSD1306 */
static const ngfx_u8x8_display_info_t ngfx_u8x8_ssd1305_128x64_adafruit_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 10,
  /* reset_pulse_width_ms = */ 100, 	/* SSD1306: 3 us */
  /* post_reset_wait_ms = */ 100, /* far east OLEDs need much longer setup time */
  /* sda_setup_time_ns = */ 50,		/* SSD1306: 15ns, but cycle time is 100ns, so use 100/2 */
  /* sck_pulse_width_ns = */ 50,	/* SSD1306: 20ns, but cycle time is 100ns, so use 100/2, AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,
  /* write_pulse_width_ns = */ 150,	/* SSD1306: cycle time is 300ns, so use 300/2 = 150 */
  /* tile_width = */ 16,
  /* tile_height = */ 8,
  /* default_x_offset = */ 2,
  /* flipmode_x_offset = */ 2,
  /* pixel_width = */ 128,
  /* pixel_height = */ 64
};


static const uint8_t ngfx_u8x8_d_ssd1305_128x64_adafruit_init_seq[] = {
    
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  
  NGFX_U8X8_C(0x0ae),		                /* display off */
  NGFX_U8X8_CA(0x0d5, 0x0f0),		/* clock divide ratio (0x00=1) and oscillator frequency */
  NGFX_U8X8_CA(0x0a8, 0x03f),		/* multiplex ratio */
  NGFX_U8X8_CA(0x0d3, 0x040),		/* display offset to 32 */
  NGFX_U8X8_C(0x040),		        	/* set display start line to 0 */
  NGFX_U8X8_CA(0x020, 0x000),		/* horizontal addressing mode */
  
  NGFX_U8X8_C(0x0a1),				/* segment remap a0/a1*/
  NGFX_U8X8_C(0x0c8),				/* c0: scan dir normal, c8: reverse */
  // Flipmode
  // NGFX_U8X8_C(0x0a0),				/* segment remap a0/a1*/
  // NGFX_U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  
  NGFX_U8X8_CA(0x0da, 0x012),		/* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */

  NGFX_U8X8_CA(0x081, 0x032), 		/* [2] set contrast control */
  NGFX_U8X8_CA(0x082, 0x080), 		/* set area brightness (reset=0x080) */
  NGFX_U8X8_CA(0x0d9, 0x0f1), 		/* [2] pre-charge period 0x022/f1*/
  NGFX_U8X8_CA(0x0db, 0x040), 		/* vcomh deselect level */  
  // if vcomh is 0, then this will give the biggest range for contrast control issue #98
  // restored the old values for the noname constructor, because vcomh=0 will not work for all OLEDs, #116
  
  NGFX_U8X8_C(0x02e),				/* Deactivate scroll */ 
  NGFX_U8X8_C(0x0a4),				/* output ram to display */
  NGFX_U8X8_C(0x0a6),				/* none inverted normal display mode */
    
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


uint8_t ngfx_u8x8_d_ssd1305_128x64_adafruit(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    
  if ( ngfx_u8x8_d_ssd1305_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;
  
  switch(msg)
  {
    case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      if ( arg_int == 0 )
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x32_flip0_seq);
	u8x8->x_offset = u8x8->display_info->default_x_offset;
      }
      else
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x32_flip1_seq);
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
      }
      break;
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x64_adafruit_init_seq);
      break;
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_ssd1305_128x64_adafruit_display_info);
      break;
    default:
      return 0;
  }
  return 1;
}


/*================================================*/
/* Raystar RET012864 OLED, issue https://github.com/olikraus/u8g2/issues/1111 */

static const ngfx_u8x8_display_info_t ngfx_u8x8_ssd1305_128x64_raystar_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 10,
  /* reset_pulse_width_ms = */ 100, 	/* SSD1306: 3 us */
  /* post_reset_wait_ms = */ 100, /* far east OLEDs need much longer setup time */
  /* sda_setup_time_ns = */ 50,		/* SSD1306: 15ns, but cycle time is 100ns, so use 100/2 */
  /* sck_pulse_width_ns = */ 50,	/* SSD1306: 20ns, but cycle time is 100ns, so use 100/2, AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,
  /* write_pulse_width_ns = */ 150,	/* SSD1306: cycle time is 300ns, so use 300/2 = 150 */
  /* tile_width = */ 16,
  /* tile_height = */ 8,
  /* default_x_offset = */ 4,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 128,
  /* pixel_height = */ 64
};

uint8_t ngfx_u8x8_d_ssd1305_128x64_raystar(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    
  if ( ngfx_u8x8_d_ssd1305_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;
  
  switch(msg)
  {
    case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      if ( arg_int == 0 )
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x32_flip0_seq);
	u8x8->x_offset = u8x8->display_info->default_x_offset;
      }
      else
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x32_flip1_seq);
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
      }
      break;
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1305_128x64_adafruit_init_seq);
      break;
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_ssd1305_128x64_raystar_display_info);
      break;
    default:
      return 0;
  }
  return 1;
}
