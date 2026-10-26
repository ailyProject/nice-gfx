/*

  ngfx_u8x8_d_ssd1326.c

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


/* ER OLED */
static const uint8_t ngfx_u8x8_d_ssd1326_er_256x32_init_seq[] = {
    
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  NGFX_U8X8_CA(0x0fd, 0x012),		/* unlock (not required, this is default by reset) */
  
  
  NGFX_U8X8_C(0x0ae),		                /* display off */
  NGFX_U8X8_CA(0x0a8, 0x01f),		/* multiplex ratio: 0x03f * 1/64 duty - changed by CREESOO, acc. to datasheet, 100317*/ 
  NGFX_U8X8_CA(0x0a1, 0x000),		/* display start line */
  NGFX_U8X8_CA(0x0a2, 0x000),		/* display offset, shift mapping ram counter */
  NGFX_U8X8_CA(0x0ad, 0x002),		/* master configuration: disable embedded DC-DC, enable internal VCOMH */
  /*
    a0 command: 0x0a0 ***abcde
      a: 1: mono mode
      b: 0: horizontal (1: vertical) address increment
      c: 1: enable bit remap
      d: 1: COM remap
      e: 1: Column remap
  */
  NGFX_U8X8_CA(0x0a0, 0x006),		/* remap configuration, see above */
  NGFX_U8X8_C(0x086),				/* full current range (0x084, 0x085, 0x086) */

  NGFX_U8X8_C(0x0b7),				/* set default gray scale table */
    
  NGFX_U8X8_CA(0x081, 0x027),		/* contrast, brightness, 0..128 */
  NGFX_U8X8_CA(0x0b1, 0x071),                    /* phase length */
  //NGFX_U8X8_CA(0x0b2, 0x051),		/* frame frequency (row period) */
  NGFX_U8X8_CA(0x0b3, 0x0f0),		/* set display clock divide ratio/oscillator frequency (set clock as 135 frames/sec) */			
  //NGFX_U8X8_CA(0x0b4, 0x002),                    /* set pre-charge compensation level (not documented in the SDD1325 datasheet, but used in the NHD init seq.) */
  //NGFX_U8X8_CA(0x0b0, 0x028),                    /* enable pre-charge compensation (not documented in the SDD1325 datasheet, but used in the NHD init seq.) */
  NGFX_U8X8_CAA(0x0bb, 0x035, 0x0ff),                     /* set precharge */
  NGFX_U8X8_CA(0x0bc, 0x01f),                    /* pre-charge voltage level */
  NGFX_U8X8_CA(0x0be, 0x00f),                     /* VCOMH voltage */
  NGFX_U8X8_CA(0x0bf, 0x002|0x00d),           /* VSL voltage level (not documented in the SDD1325 datasheet, but used in the NHD init seq.) */
  NGFX_U8X8_C(0x0a4),				/* normal display mode */
    
  //NGFX_U8X8_CA(0x023, 0x003),		/* graphics accelleration: fill pixel */
    
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_ssd1326_256x32_nhd_powersave0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0af),		                /* display on */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_ssd1326_256x32_nhd_powersave1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0ae),		                /* display off */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_ssd1326_256x32_nhd_flip0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_CA(0x0a0, 0x006),		/* remap 00110 */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_ssd1326_256x32_nhd_flip1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  //NGFX_U8X8_CA(0x0a0, 0x005),		/* remap 00101 */
  NGFX_U8X8_CA(0x0a0, 0x001),		/* remap 00001 */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


/*
  input:
    one tile (8 Bytes)
  output:
    Tile for ssd1326 (32 Bytes)
*/

static uint8_t ngfx_u8x8_ssd1326_8to32_dest_buf[32];

static uint8_t *ngfx_u8x8_ssd1326_8to32(NGFX_U8X8_UNUSED ngfx_u8x8_t *u8x8, uint8_t *ptr)
{
  uint8_t v;
  uint8_t a,b;
  uint8_t i, j;
  uint8_t *dest;
  
  for( j = 0; j < 4; j++ )
  {
    dest = ngfx_u8x8_ssd1326_8to32_dest_buf;
    dest += j;
    a =*ptr;
    ptr++;
    b = *ptr;
    ptr++;
    for( i = 0; i < 8; i++ )
    {
      v = 0;
      if ( a&1 ) v |= 0xf0;
      if ( b&1 ) v |= 0x0f;
      *dest = v;
      dest+=4;
      a >>= 1;
      b >>= 1;
    }
  }
  
  return ngfx_u8x8_ssd1326_8to32_dest_buf;
}




static uint8_t ngfx_u8x8_d_ssd1326_256x32_generic(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, y, c;
  uint8_t *ptr;
  switch(msg)
  {
    /* handled by the calling function
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_ssd1326_256x32_nhd_display_info);
      break;
    */
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1326_er_256x32_init_seq);    
      break;
    case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      if ( arg_int == 0 )
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1326_256x32_nhd_powersave0_seq);
      else
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1326_256x32_nhd_powersave1_seq);
      break;
    case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      if ( arg_int == 0 )
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1326_256x32_nhd_flip0_seq);
	u8x8->x_offset = u8x8->display_info->default_x_offset;
      }
      else
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_ssd1326_256x32_nhd_flip1_seq);
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
      }
      break;
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
    case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:
      ngfx_u8x8_cad_StartTransfer(u8x8);
      ngfx_u8x8_cad_SendCmd(u8x8, 0x081 );
      ngfx_u8x8_cad_SendArg(u8x8, arg_int );	/* ssd1326 has range from 0 to 255 */
      ngfx_u8x8_cad_EndTransfer(u8x8);
      break;
#endif
    case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
      ngfx_u8x8_cad_StartTransfer(u8x8);
      x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;
      x *= 4;
      
      y = (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos);
      
      y *= 8;
      y += u8x8->x_offset;		/* x_offset is used as y offset for the ssd1326 */
    
      
      do
      {
	c = ((ngfx_u8x8_tile_t *)arg_ptr)->cnt;
	ptr = ((ngfx_u8x8_tile_t *)arg_ptr)->tile_ptr;

	do
	{
          ngfx_u8x8_cad_SendCmd(u8x8, 0x015 );	/* set column address */
          ngfx_u8x8_cad_SendArg(u8x8, x );	/* start */
          ngfx_u8x8_cad_SendArg(u8x8, x+3 );	/* end */

          ngfx_u8x8_cad_SendCmd(u8x8, 0x075 );	/* set row address */
          ngfx_u8x8_cad_SendArg(u8x8, y);
          ngfx_u8x8_cad_SendArg(u8x8, y+7);
          
          ngfx_u8x8_cad_SendData(u8x8, 32, ngfx_u8x8_ssd1326_8to32(u8x8, ptr));
          
	  ptr += 8;
	  x += 4;
	  c--;
	} while( c > 0 );
	
	//x += 4;
	arg_int--;
      } while( arg_int > 0 );
      
      ngfx_u8x8_cad_EndTransfer(u8x8);
      break;
    default:
      return 0;
  }
  return 1;
}


static const ngfx_u8x8_display_info_t ngfx_u8x8_ssd1326_256x32_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 15,
  /* reset_pulse_width_ms = */ 100, 	
  /* post_reset_wait_ms = */ 100, 		/**/
  /* sda_setup_time_ns = */ 100,		/* ssd1326  */
  /* sck_pulse_width_ns = */ 100,	/* ssd1326  */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,
  /* write_pulse_width_ns = */ 60,	/* ssd1326 */
  /* tile_width = */ 32,
  /* tile_height = */ 4,
  /* default_x_offset = */ 0,		/* x_offset is used as y offset for the ssd1326 */
  /* flipmode_x_offset = */ 0,		/* x_offset is used as y offset for the ssd1326 */
  /* pixel_width = */ 256,
  /* pixel_height = */ 32
};

uint8_t ngfx_u8x8_d_ssd1326_er_256x32(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    if ( msg == NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY )
    {
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_ssd1326_256x32_display_info);
      return 1;
    }
    return ngfx_u8x8_d_ssd1326_256x32_generic(u8x8, msg, arg_int, arg_ptr);
}


