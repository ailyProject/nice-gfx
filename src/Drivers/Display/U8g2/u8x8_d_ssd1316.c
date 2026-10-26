/*

  ngfx_u8x8_d_ssd1316.c

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2019, olikraus@gmail.com
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

SSD1316: 128x39

  SSD1316: 128x32 OLED          https://github.com/olikraus/u8g2/issues/919
  SSD1316: 96x32 OLED          https://github.com/olikraus/u8g2/issues/1938
  SSD1307 128x32 OLED           https://github.com/olikraus/u8g2/issues/2783



*/


#include "u8x8.h"

static const uint8_t ngfx_u8x8_d_ssd1316_128x32_powersave0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0af),		                /* display on */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_ssd1316_128x32_powersave1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0ae),		                /* display off */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_ssd1316_128x32_flip0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0a1),				/* segment remap a0/a1*/
  NGFX_U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_ssd1316_128x32_flip1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0a0),				/* segment remap a0/a1*/
  NGFX_U8X8_C(0x0c8),				/* c0: scan dir normal, c8: reverse */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};




/*===================================================*/

static uint8_t ngfx_u8x8_d_ssd1316_generic(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;
  switch(msg)
  {
    /* handled by the calling function
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_ssd1316_128x32_display_info);
      break;
    */
    /* handled by the calling function
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1316_128x32_init_seq);    
      break;
    */
    case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      if ( arg_int == 0 )
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1316_128x32_powersave0_seq);
      else
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1316_128x32_powersave1_seq);
      break;
    case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      if ( arg_int == 0 )
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1316_128x32_flip0_seq);
	u8x8->x_offset = u8x8->display_info->default_x_offset;
      }
      else
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1316_128x32_flip1_seq);
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
      }
      break;
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
    case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:
      ngfx_u8x8_cad_StartTransfer(u8x8);
      ngfx_u8x8_cad_SendCmd(u8x8, 0x081 );
      ngfx_u8x8_cad_SendArg(u8x8, arg_int );	/* ssd1306 has range from 0 to 255 */
      ngfx_u8x8_cad_EndTransfer(u8x8);
      break;
#endif
    case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
      x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;    
      x *= 8;
      x += u8x8->x_offset;

      ngfx_u8x8_cad_StartTransfer(u8x8);
    
      ngfx_u8x8_cad_SendCmd(u8x8, 0x010 | (x>>4) );
      ngfx_u8x8_cad_SendCmd(u8x8, 0x000 | ((x&15)));	/* probably wrong, should be SendCmd */
      ngfx_u8x8_cad_SendCmd(u8x8, 0x0b0 | (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos)); /* probably wrong, should be SendCmd */

      do
      {
	c = ((ngfx_u8x8_tile_t *)arg_ptr)->cnt;
	ptr = ((ngfx_u8x8_tile_t *)arg_ptr)->tile_ptr;
	ngfx_u8x8_cad_SendData(u8x8, c*8, ptr); 	/* note: SendData can not handle more than 255 bytes */
	arg_int--;
      } while( arg_int > 0 );
      
      ngfx_u8x8_cad_EndTransfer(u8x8);
      break;
    default:
      return 0;
  }
  return 1;
}

/*===================================================*/


/* QT-2832TSWUG02/ZJY-2832TSWZG02 */
static const uint8_t ngfx_u8x8_d_ssd1316_128x32_init_seq[] = {
    
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  
  NGFX_U8X8_C(0x0ae),		        /* display off */
  NGFX_U8X8_C(0x040),		        /* start line */
  NGFX_U8X8_CA(0x081, 0x045), 		/* QG-2832TSWZG02 datasheet */

  NGFX_U8X8_C(0x0a6),			/* none inverted normal display mode */
  NGFX_U8X8_CA(0x0a8, 0x01f),		/* multiplex ratio, duty = 1/32 */

  NGFX_U8X8_C(0x0a1),			/* segment remap a0/a1*/
  NGFX_U8X8_C(0x0c0),			/* c0: scan dir normal, c8: reverse */

  NGFX_U8X8_CA(0x0d3, 0x000),		/* display offset */
  NGFX_U8X8_CA(0x0d5, 0x080),		/* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
  NGFX_U8X8_CA(0x0d9, 0x022), 		/* [2] pre-charge period 0x022/f1*/
  NGFX_U8X8_CA(0x0da, 0x012),		/* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */
  NGFX_U8X8_CA(0x0db, 0x020), 		/* vcomh deselect level */  
  NGFX_U8X8_CA(0x08d, 0x015),		/* [2] charge pump setting (p62): 0x014 enable, 0x010 disable, */
  
  //NGFX_U8X8_CA(0x0a2, 0x000),		/* set display start line to 0 */
  //NGFX_U8X8_CA(0x020, 0x000),		/* horizontal addressing mode */
  
  
  // Flipmode
  //NGFX_U8X8_C(0x0a1),			/* segment remap a0/a1*/
  //NGFX_U8X8_C(0x0c0),			/* c0: scan dir normal, c8: reverse */
  
  NGFX_U8X8_C(0x02e),			/* Deactivate scroll */ 
  //NGFX_U8X8_C(0x0a4),			/* output ram to display */
    
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()           			/* end of sequence */
};




static const ngfx_u8x8_display_info_t ngfx_u8x8_ssd1316_128x32_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 10,
  /* reset_pulse_width_ms = */ 100, 	/* reset time */
  /* post_reset_wait_ms = */ 100, /* reset delay */
  /* sda_setup_time_ns = */ 50,		/* SSD1306: 15ns, but cycle time is 100ns, so use 100/2 */
  /* sck_pulse_width_ns = */ 50,	/* SSD1306: 20ns, but cycle time is 100ns, so use 100/2, AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 8000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,
  /* write_pulse_width_ns = */ 150,	/* SSD1306: cycle time is 300ns, so use 300/2 = 150 */
  /* tile_width = */ 16,
  /* tile_height = */ 4,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 128,
  /* pixel_height = */ 32
};

uint8_t ngfx_u8x8_d_ssd1316_128x32(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    
  if ( ngfx_u8x8_d_ssd1316_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;
  
  switch(msg)
  {
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1316_128x32_init_seq);    
      break;
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_ssd1316_128x32_display_info);
      break;
    default:
      return 0;
  }
  return 1;
}


/*===================================================*/
/*
QG-9632TLBZG02
https://datasheet.lcsc.com/lcsc/1804162033_Shenzhen-Allvision-Tech-QG-9632TLBZG02_C91216.pdf
https://github.com/olikraus/u8g2/issues/1938

96x32
*/

static const uint8_t ngfx_u8x8_d_ssd1316_96x32_init_seq[] = {
    
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  
  NGFX_U8X8_C(0x0ae),		        /* display off */
  NGFX_U8X8_C(0x040),		        /* start line */
  NGFX_U8X8_CA(0x081, 0x045), 		/* QG-2832TSWZG02 datasheet: 0x081, 0x45 */
                                                        /* QG-9632: 0x81, 0x60 */

  NGFX_U8X8_C(0x0a6),			/* none inverted normal display mode */
  NGFX_U8X8_CA(0x0a8, 0x01f),		/* multiplex ratio, duty = 1/32 */

  NGFX_U8X8_C(0x0a1),			/* segment remap a0/a1*/
  NGFX_U8X8_C(0x0c0),			/* c0: scan dir normal, c8: reverse */

  NGFX_U8X8_CA(0x0d3, 0x000),		/* display offset */
  NGFX_U8X8_CA(0x0d5, 0x080),		/* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
  NGFX_U8X8_CA(0x0d9, 0x022), 		/* [2] pre-charge period 0x022/f1*/
  NGFX_U8X8_CA(0x0da, 0x012),		/* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */
  NGFX_U8X8_CA(0x0db, 0x020), 		/* vcomh deselect level */  
  NGFX_U8X8_CA(0x08d, 0x015),		/* [2] charge pump setting (p62): 0x014 enable, 0x010 disable, */
                                                        /* QG-9632: 0x8d, 0x10 */
  
  //NGFX_U8X8_CA(0x0a2, 0x000),		/* set display start line to 0 */
  //NGFX_U8X8_CA(0x020, 0x000),		/* horizontal addressing mode */
  
  
  // Flipmode
  //NGFX_U8X8_C(0x0a1),			/* segment remap a0/a1*/
  //NGFX_U8X8_C(0x0c0),			/* c0: scan dir normal, c8: reverse */
  
  NGFX_U8X8_C(0x02e),			/* Deactivate scroll */ 
  //NGFX_U8X8_C(0x0a4),			/* output ram to display */
    
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()           			/* end of sequence */
};


static const ngfx_u8x8_display_info_t ngfx_u8x8_ssd1316_96x32_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 10,
  /* reset_pulse_width_ms = */ 100, 	/* reset time */
  /* post_reset_wait_ms = */ 100, /* reset delay */
  /* sda_setup_time_ns = */ 50,		/* SSD1306: 15ns, but cycle time is 100ns, so use 100/2 */
  /* sck_pulse_width_ns = */ 50,	/* SSD1306: 20ns, but cycle time is 100ns, so use 100/2, AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 8000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,
  /* write_pulse_width_ns = */ 150,	/* SSD1306: cycle time is 300ns, so use 300/2 = 150 */
  /* tile_width = */ 12,
  /* tile_height = */ 4,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 96,
  /* pixel_height = */ 32
};

uint8_t ngfx_u8x8_d_ssd1316_96x32(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    
  if ( ngfx_u8x8_d_ssd1316_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;
  
  switch(msg)
  {
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1316_96x32_init_seq);    
      break;
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_ssd1316_96x32_display_info);
      break;
    default:
      return 0;
  }
  return 1;
}

