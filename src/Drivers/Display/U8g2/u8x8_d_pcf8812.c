/*

  ngfx_u8x8_d_pcf8812.c
  
  pcf8812: 65x102
  pcf8814: 65x96

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



static const uint8_t ngfx_u8x8_d_pcf8812_96x65_init_seq[] = {
    
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  NGFX_U8X8_C(0x020),		                /* activate chip (PD=0), horizontal increment (V=0), enter normal command set (H=0) */
  NGFX_U8X8_C(0x008),		                /* blank display */
  
  NGFX_U8X8_C(0x021),		                /* activate chip (PD=0), horizontal increment (V=0), enter extended command set (H=1) */
  NGFX_U8X8_C(0x006),		                /* temp. control: b10 = 2 */
  NGFX_U8X8_C(0x013),		                /* bias system, 0x010..0x07 1:48 */
  NGFX_U8X8_C(0x09f),				/* contrast setting, 0..127 */
  //NGFX_U8X8_CA(0x020 | 2, 0x080 | 0),				/* contrast setting, pcf8814 */

  NGFX_U8X8_C(0x024),		                /* deactivate chip (PD=1), horizontal increment (V=0), enter normal command set (H=0) */
    
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


static const uint8_t ngfx_u8x8_d_pcf8812_101x64_init_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x21), // PowerON, ExtCommandSet
  NGFX_U8X8_C(0x09), // Internal HV-gen x3
  NGFX_U8X8_C(0xB7), // Set Vop
  NGFX_U8X8_C(0x16), // Bias n=2    //15
  NGFX_U8X8_C(0x06), // Temperature coeff 2
  NGFX_U8X8_C(0x20), // StandartCommandSet
  NGFX_U8X8_C(0x0C), // normal mode, display non-inverted
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};



static const uint8_t ngfx_u8x8_d_pcf8812_96x65_powersave0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x020),		                /* power on */
  NGFX_U8X8_C(0x00c),		                /* display on */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_pcf8812_96x65_powersave1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x020),		                /* power on */
  NGFX_U8X8_C(0x008),		                /* blank display */
  NGFX_U8X8_C(0x024),		                /* power down */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


static uint8_t ngfx_u8x8_d_pcf8812_96x65_generic(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;
  switch(msg)
  {
    /* handled by the calling function
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_pcf8812_96x65_display_info);
      break;
    */
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_pcf8812_96x65_init_seq);    
      break;
    case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      if ( arg_int == 0 )
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_pcf8812_96x65_powersave0_seq);
      else
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_pcf8812_96x65_powersave1_seq);
      break;
/*
    case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      if ( arg_int == 0 )
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_pcf8812_96x65_flip0_seq);
	u8x8->x_offset = u8x8->display_info->default_x_offset;
      }
      else
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_pcf8812_96x65_flip1_seq);
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
      }
      break;
*/
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
    case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:
      ngfx_u8x8_cad_StartTransfer(u8x8);
      ngfx_u8x8_cad_SendCmd(u8x8, 0x021 );    /* command mode, extended function set */
      ngfx_u8x8_cad_SendArg(u8x8, (arg_int>>1)|0x80 );	/* 0..127 for contrast */
      ngfx_u8x8_cad_EndTransfer(u8x8);
      break;
#endif
    case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
      ngfx_u8x8_cad_StartTransfer(u8x8);
      x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;    
      x *= 8;
      x += u8x8->x_offset;
    
      ngfx_u8x8_cad_SendCmd(u8x8, 0x020 );	/* activate chip (PD=0), horizontal increment (V=0), enter normal command set (H=0) */
      ngfx_u8x8_cad_SendCmd(u8x8, 0x080 | x);
      ngfx_u8x8_cad_SendCmd(u8x8, 0x040 | ((ngfx_u8x8_tile_t *)arg_ptr)->y_pos);
      
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
    default:
      return 0;
  }
  return 1;
}


static uint8_t ngfx_u8x8_d_pcf8812_101x64_generic(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;
  switch(msg)
  {
    /* handled by the calling function
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_pcf8812_96x65_display_info);
      break;
    */
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_pcf8812_101x64_init_seq);    
      break;
    case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      if ( arg_int == 0 )
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_pcf8812_96x65_powersave0_seq);
      else
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_pcf8812_96x65_powersave1_seq);
      break;
/*
    case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      if ( arg_int == 0 )
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_pcf8812_96x65_flip0_seq);
	u8x8->x_offset = u8x8->display_info->default_x_offset;
      }
      else
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_pcf8812_96x65_flip1_seq);
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
      }
      break;
*/
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
    case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:
      ngfx_u8x8_cad_StartTransfer(u8x8);
      ngfx_u8x8_cad_SendCmd(u8x8, 0x021 );    /* command mode, extended function set */
      ngfx_u8x8_cad_SendArg(u8x8, (arg_int>>1)|0x80 );	/* 0..127 for contrast */
      ngfx_u8x8_cad_EndTransfer(u8x8);
      break;
#endif
    case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
      ngfx_u8x8_cad_StartTransfer(u8x8);
      x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;    
      x *= 8;
      x += u8x8->x_offset;
    
      ngfx_u8x8_cad_SendCmd(u8x8, 0x020 );	/* activate chip (PD=0), horizontal increment (V=0), enter normal command set (H=0) */
      ngfx_u8x8_cad_SendCmd(u8x8, 0x080 | x);
      ngfx_u8x8_cad_SendCmd(u8x8, 0x040 | ((ngfx_u8x8_tile_t *)arg_ptr)->y_pos);
      
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
    default:
      return 0;
  }
  return 1;
}




static const ngfx_u8x8_display_info_t ngfx_u8x8_pcf8812_96x65_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 100,
  /* pre_chip_disable_wait_ns = */ 100,
  /* reset_pulse_width_ms = */ 100, 
  /* post_reset_wait_ms = */ 100, 
  /* sda_setup_time_ns = */ 100,	
  /* sck_pulse_width_ns = */ 100,	
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,
  /* write_pulse_width_ns = */ 150,	
  /* tile_width = */ 12,
  /* tile_height = */ 9,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 96,
  /* pixel_height = */ 65
};

uint8_t ngfx_u8x8_d_pcf8812_96x65(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    if ( msg == NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY )
    {
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_pcf8812_96x65_display_info);
      return 1;
    }
    return ngfx_u8x8_d_pcf8812_96x65_generic(u8x8, msg, arg_int, arg_ptr);
}

/* https://github.com/olikraus/u8g2/issues/2421 */
static const ngfx_u8x8_display_info_t ngfx_u8x8_pcf8812_101x64_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 100,
  /* pre_chip_disable_wait_ns = */ 100,
  /* reset_pulse_width_ms = */ 100, 
  /* post_reset_wait_ms = */ 100, 
  /* sda_setup_time_ns = */ 100,	
  /* sck_pulse_width_ns = */ 100,	
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,
  /* write_pulse_width_ns = */ 150,	
  /* tile_width = */ 13,
  /* tile_height = */ 8,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 101,
  /* pixel_height = */ 64
};




uint8_t ngfx_u8x8_d_pcf8812_101x64(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    if ( msg == NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY )
    {
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_pcf8812_101x64_display_info);
      return 1;
    }
    else if ( msg == NGFX_U8X8_MSG_DISPLAY_INIT )
    {
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_pcf8812_101x64_init_seq);    
    }    
    return ngfx_u8x8_d_pcf8812_101x64_generic(u8x8, msg, arg_int, arg_ptr);
}

