/*

  ngfx_u8x8_d_ist7920.c
  
  this is NOT ST7920!
  
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

  
*/
#include "u8x8.h"




static const uint8_t ngfx_u8x8_d_ist7920_128x128_powersave0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x03d),		                /* display on */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_ist7920_128x128_powersave1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x03c),		                /* display off */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_ist7920_128x128_flip0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x064),				/* Display Ctrl: Bit3: SHL 2:ADC 1:EON, 0:REV */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_ist7920_128x128_flip1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x068),				/* Display Ctrl: Bit3: SHL 2:ADC 1:EON, 0:REV */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


static const ngfx_u8x8_display_info_t ngfx_u8x8_ist7920_128x128_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 90,	/* IST7920 datasheet, page 48 */
  /* pre_chip_disable_wait_ns = */ 90,	/* IST7920 datasheet, page 48 */
  /* reset_pulse_width_ms = */ 10,		 
  /* post_reset_wait_ms = */ 20, 		/* IST7920 Startup Seq.. */
  /* sda_setup_time_ns = */ 45,		/* IST7920 datasheet, page 48 */
  /* sck_pulse_width_ns = */ 130,	/* IST7920 datasheet, page 48 */
  /* sck_clock_hz = */ 3000000UL,	/* IST7920 datasheet: 260ns */
  /* spi_mode = */ 0,		/* active high, rising edge (not verified) */
  /* i2c_bus_clock_100kHz = */ 4,	/* 400kHz according to IST7920 datasheet */
  /* data_setup_time_ns = */ 60,	/* IST7920 datasheet, page 47 */
  /* write_pulse_width_ns = */ 150,	/* IST7920 datasheet, page 47 */
  /* tile_width = */ 16,		/* width of 16*8=128 pixel */
  /* tile_height = */ 16,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 128,
  /* pixel_height = */ 128
};

/* 1/128 Duty, 1/10 Bias, 128x128 round display */
static const uint8_t ngfx_u8x8_d_ist7920_128x128_init_seq[] = {
    
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  NGFX_U8X8_C(0x076),				/* Software Reset */
  NGFX_U8X8_DLY(50),
  NGFX_U8X8_C(0x03c),				/* display off */
  
  
  NGFX_U8X8_CA(0x090, 128),			/* Set Duty */
  //NGFX_U8X8_CAA(0x0b2, 0x011, 0x00),	/* Set Frame Control */  

  NGFX_U8X8_CA(0x030, 16),			/* Set Bias 0: 1/8, 8: 1/9, 16: 1/10, 24: 1/11, 48: 1/12 ... */  
  NGFX_U8X8_CA(0x031, 0x03f),		/* Set voltage generate clock(31H/11H) */  
  //NGFX_U8X8_CA(0x032, 0x015),		/* Temperature compensation */  
  NGFX_U8X8_CA(0x033, 0x020),		/* Power Control */
  NGFX_U8X8_DLY(100),
  NGFX_U8X8_CA(0x033, 0x02c),		/* Power Control */
  NGFX_U8X8_DLY(100),
  NGFX_U8X8_C(0xfd),				/* set booster */
  NGFX_U8X8_DLY(100),
  NGFX_U8X8_CA(0x033, 0x02f),		/* Power Control */
  NGFX_U8X8_DLY(200),
  
  NGFX_U8X8_C(0x064),				/* Display Ctrl: Bit3: SHL 2:ADC 1:EON, 0:REV */

  NGFX_U8X8_CAA(0x074, 0x000, 0x00f),	/* AY Window */  
  NGFX_U8X8_CAA(0x075, 0x000, 0x07f),	/* AX Window */  
  
  NGFX_U8X8_CA(0x040, 64),			/* Start line at 64 */

  NGFX_U8X8_CA(0x0b1, 100),			/* electronic volume */  


  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

uint8_t ngfx_u8x8_d_ist7920_128x128(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;
  switch(msg)
  {
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_ist7920_128x128_display_info);
      break;
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ist7920_128x128_init_seq);
      break;
    case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      if ( arg_int == 0 )
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ist7920_128x128_powersave0_seq);
      else
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ist7920_128x128_powersave1_seq);
      break;
    case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      if ( arg_int == 0 )
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ist7920_128x128_flip0_seq);
	u8x8->x_offset = u8x8->display_info->default_x_offset;
      }
      else
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ist7920_128x128_flip1_seq);
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
      }	
      break;
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
    case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:
      ngfx_u8x8_cad_StartTransfer(u8x8);
      ngfx_u8x8_cad_SendCmd(u8x8, 0x0b1 );
      ngfx_u8x8_cad_SendArg(u8x8, arg_int );	/* st7920 has range from 0 to 255 */
      ngfx_u8x8_cad_EndTransfer(u8x8);
      break;
#endif
    case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
      ngfx_u8x8_cad_StartTransfer(u8x8);
    
      x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;
      x *= 8;
      x += u8x8->x_offset;
      ngfx_u8x8_cad_SendCmd(u8x8, 0x0c0 );
      ngfx_u8x8_cad_SendArg(u8x8, x );
      ngfx_u8x8_cad_SendCmd(u8x8, 0x001 );
      ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos) );
    
      c = ((ngfx_u8x8_tile_t *)arg_ptr)->cnt;
      c *= 8;
      ptr = ((ngfx_u8x8_tile_t *)arg_ptr)->tile_ptr;
      do
      {
	ngfx_u8x8_cad_SendData(u8x8, c, ptr);	/* note: SendData can not handle more than 255 bytes */
	arg_int--;
      } while( arg_int > 0 );
      
      ngfx_u8x8_cad_EndTransfer(u8x8);
      break;
    default:
      return 0;
  }
  return 1;
}


