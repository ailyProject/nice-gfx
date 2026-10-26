/*

  ngfx_u8x8_d_st75256.c

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

  0x030	ext 00
  0x031	ext 01
  0x038	ext 10
  0x039	ext 11
  
  cad 011
  
  
  code examples:
  http://www.it610.com/article/2601023.htm
  
  normal mode:
	0x00c	bit format
  NGFX_U8X8_CA( 0xbc, 0x00 ),	data scan dir 
  NGFX_U8X8_A( 0xa6 ),				
  y: 0 offset
  
  flip mode:
	0x008	bit format
  NGFX_U8X8_CA( 0xbc, 0x03 ),	data scan dir 
  NGFX_U8X8_A( 0xa6 ),				
  y: 5 offset
	
  
*/


#include "u8x8.h"


/* not a real power down for the st75256... just a display off */
static const uint8_t ngfx_u8x8_d_st75256_256x128_powersave0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */  
  NGFX_U8X8_C( 0x94 ),				/* sleep out */
  NGFX_U8X8_DLY(10),
  NGFX_U8X8_C( 0xaf ),				/* display on */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_st75256_256x128_powersave1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0xae ),				/* display off */
  NGFX_U8X8_C( 0x95 ),				/* sleep in */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_st75256_jlx256128_flip0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x00 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x00c ),				/* data format LSB top */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_st75256_jlx256128_flip1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x03 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x008 ),				/* data format MSB top */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_st75256_jlx172104_flip0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x02 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x00c ),				/* data format LSB top */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_st75256_jlx172104_flip1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x01 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x008 ),				/* data format MSB top */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_st75256_jlx256160_flip0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x00 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x00c ),				/* data format LSB top */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_st75256_jlx256160_flip1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x03 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x008 ),				/* data format MSB top */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


static uint8_t ngfx_u8x8_d_st75256_256x128_generic(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;
  switch(msg)
  {
    /* handled by the calling function
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_st75256_256x128_display_info);
      break;
    */
    /* handled by the calling function
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_init_seq);    
      break;
    */
    case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      if ( arg_int == 0 )
        ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave0_seq);
      else
        ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave1_seq);

      break;
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
    case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:

      ngfx_u8x8_cad_StartTransfer(u8x8);
      
      ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );
      ngfx_u8x8_cad_SendCmd(u8x8, 0x081 );  /* there are 9 bit for the volume control */
      ngfx_u8x8_cad_SendArg(u8x8, (arg_int & 0x1f)<<1 );	/* lower 6 bit */
      ngfx_u8x8_cad_SendArg(u8x8, (arg_int>>5));		/* upper 3 bit */
      
      ngfx_u8x8_cad_EndTransfer(u8x8);
      break;
#endif
    case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
      
      ngfx_u8x8_cad_StartTransfer(u8x8);
      x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;    
      x *= 8;
      
      ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );	/* select command set */
      ngfx_u8x8_cad_SendCmd(u8x8, 0x075 );	/* row */
      ngfx_u8x8_cad_SendArg(u8x8, u8x8->x_offset + (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));	/* x offset is used as y offset */
      ngfx_u8x8_cad_SendArg(u8x8, 0x04f);
      //ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
      ngfx_u8x8_cad_SendCmd(u8x8, 0x015 );	/* col */
      ngfx_u8x8_cad_SendArg(u8x8, x);
      ngfx_u8x8_cad_SendArg(u8x8, 255);
      ngfx_u8x8_cad_SendCmd(u8x8, 0x05c );	
          
      do
      {
        c = ((ngfx_u8x8_tile_t *)arg_ptr)->cnt;
        ptr = ((ngfx_u8x8_tile_t *)arg_ptr)->tile_ptr;
        /* SendData can not handle more than 255 bytes, treat c > 31 correctly  */
        if ( c > 31 )
        {
          ngfx_u8x8_cad_SendData(u8x8, 248, ptr); 	/* 31*8=248 */
          ptr+=248;
          c -= 31;
        }
        
        ngfx_u8x8_cad_SendData(u8x8, c*8, ptr); 	
        arg_int--;
      } while( arg_int > 0 );
      
      ngfx_u8x8_cad_EndTransfer(u8x8);
      break;
    default:
      return 0;
  }
  return 1;
}

/*=============================================*/
/* JLX256128 */

static const ngfx_u8x8_display_info_t ngfx_u8x8_st75256_256x128_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 20,
  /* reset_pulse_width_ms = */ 5, 	
  /* post_reset_wait_ms = */ 5, 		/**/
  /* sda_setup_time_ns = */ 20,		/* */
  /* sck_pulse_width_ns = */ 40,	/*  */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,	/* 400KHz */
  /* data_setup_time_ns = */ 15,
  /* write_pulse_width_ns = */ 70,	
  /* tile_width = */ 32,
  /* tile_height = */ 16,
  /* default_x_offset = */ 0,	/* must be 0, because this is checked also for normal mode */
  /* flipmode_x_offset = */ 5,		/* used as y offset */
  /* pixel_width = */ 256,
  /* pixel_height = */ 128
};


static const uint8_t ngfx_u8x8_d_st75256_256x128_init_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  NGFX_U8X8_DLY(20),

  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x094 ),				/* sleep out */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x0ae ),				/* display off */

  NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_CA( 0x0d7, 0x09f ),		/* disable auto read */  

  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x032 ),				/* analog circuit set */
  NGFX_U8X8_A( 0x000 ),				/* code example: OSC Frequency adjustment */
  NGFX_U8X8_A( 0x001 ),				/* Frequency on booster capacitors 1 = 6KHz? */
  NGFX_U8X8_A( 0x000 ),				/* Bias: 1: 1/13, 2: 1/12, 3: 1/11, 4:1/10, 5:1/9 */
    
  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x020 ),				/* gray levels */
  NGFX_U8X8_A( 0x01 ),
  NGFX_U8X8_A( 0x03 ),
  NGFX_U8X8_A( 0x05 ),
  NGFX_U8X8_A( 0x07 ),
  NGFX_U8X8_A( 0x09),
  NGFX_U8X8_A( 0x0b ),
  NGFX_U8X8_A( 0x0d ),
  NGFX_U8X8_A( 0x10 ),
  NGFX_U8X8_A( 0x11 ),
  NGFX_U8X8_A( 0x13 ),
  NGFX_U8X8_A( 0x15 ),
  NGFX_U8X8_A( 0x17 ),
  NGFX_U8X8_A( 0x19 ),
  NGFX_U8X8_A( 0x1b ),
  NGFX_U8X8_A( 0x1d ),
  NGFX_U8X8_A( 0x1f ),
 
  
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA(0x75, 0, 0x4f),		/* row range */
  NGFX_U8X8_CAA(0x15, 0, 255),		/* col range */
  
  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x00 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x00c ),				/* data format LSB top */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_C( 0xca ),				/* display control, 3 args follow  */
  NGFX_U8X8_A( 0x00 ),				/* 0x00: no clock division, 0x04: devide clock */
  NGFX_U8X8_A( 0x7f ),				/* 1/160 duty value from the DS example code */
  NGFX_U8X8_A( 0x20 ),				/* nline off */ 

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_CA( 0x0f0, 0x010 ),		/* monochrome mode  = 0x010*/

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA( 0x81, 0x36, 0x05 ),	/* Volume control */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0x020, 0x00b ),		/* Power control: Regulator, follower & booster on */
  NGFX_U8X8_DLY(100),

  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


uint8_t ngfx_u8x8_d_st75256_jlx256128(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  if ( ngfx_u8x8_d_st75256_256x128_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;
  if ( msg == NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY )
  {
    //ngfx_u8x8_SetI2CAddress(u8x8, 0x078);		/* lowest I2C adr of the ST75256 */
    ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_st75256_256x128_display_info);
    return 1;
  }
  else if ( msg == NGFX_U8X8_MSG_DISPLAY_INIT )
  {
    ngfx_u8x8_d_helper_display_init(u8x8);
    ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_init_seq);    
    return 1;
  }
  else if  ( msg == NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE )
  {
    if ( arg_int == 0 )
    {
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx256128_flip0_seq);
      u8x8->x_offset = u8x8->display_info->default_x_offset;
    }
    else
    {
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx256128_flip1_seq);
      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
    }
    return 1;
  }
  return 0;
}



/*=============================================*/
/* WO256X128, https://github.com/olikraus/u8g2/issues/891  */

static const ngfx_u8x8_display_info_t ngfx_u8x8_st75256_wo256x128_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 20,
  /* reset_pulse_width_ms = */ 5, 	
  /* post_reset_wait_ms = */ 5, 		/**/
  /* sda_setup_time_ns = */ 20,		/* */
  /* sck_pulse_width_ns = */ 40,	/*  */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,	/* 400KHz */
  /* data_setup_time_ns = */ 15,
  /* write_pulse_width_ns = */ 70,	
  /* tile_width = */ 32,
  /* tile_height = */ 16,
  /* default_x_offset = */ 5,	/* must be 0, because this is checked also for normal mode */
  /* flipmode_x_offset = */ 0,		/* used as y offset */
  /* pixel_width = */ 256,
  /* pixel_height = */ 128
};


static const uint8_t ngfx_u8x8_d_st75256_wo256x128_init_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  NGFX_U8X8_DLY(20),

  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x094 ),				/* sleep out */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x0ae ),				/* display off */

  NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_CA( 0x0d7, 0x09f ),		/* disable auto read */  

  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x032 ),				/* analog circuit set */
  NGFX_U8X8_A( 0x000 ),				/* code example: OSC Frequency adjustment */
  NGFX_U8X8_A( 0x001 ),				/* Frequency on booster capacitors 1 = 6KHz? */
  NGFX_U8X8_A( 0x000 ),				/* Bias: 1: 1/13, 2: 1/12, 3: 1/11, 4:1/10, 5:1/9 */
    
  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x020 ),				/* gray levels */
  NGFX_U8X8_A( 0x01 ),
  NGFX_U8X8_A( 0x03 ),
  NGFX_U8X8_A( 0x05 ),
  NGFX_U8X8_A( 0x07 ),
  NGFX_U8X8_A( 0x09),
  NGFX_U8X8_A( 0x0b ),
  NGFX_U8X8_A( 0x0d ),
  NGFX_U8X8_A( 0x10 ),
  NGFX_U8X8_A( 0x11 ),
  NGFX_U8X8_A( 0x13 ),
  NGFX_U8X8_A( 0x15 ),
  NGFX_U8X8_A( 0x17 ),
  NGFX_U8X8_A( 0x19 ),
  NGFX_U8X8_A( 0x1b ),
  NGFX_U8X8_A( 0x1d ),
  NGFX_U8X8_A( 0x1f ),
 
  
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA(0x75, 0, 0x4f),		/* row range */
  NGFX_U8X8_CAA(0x15, 0, 255),		/* col range */
  
  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x01 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x008 ),				/* data format LSB top */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_C( 0xca ),				/* display control, 3 args follow  */
  NGFX_U8X8_A( 0x00 ),				/* 0x00: no clock division, 0x04: devide clock */
  NGFX_U8X8_A( 0x7f ),				/* 1/160 duty value from the DS example code */
  NGFX_U8X8_A( 0x20 ),				/* nline off */ 

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_CA( 0x0f0, 0x010 ),		/* monochrome mode  = 0x010*/

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA( 0x81, 0x36, 0x05 ),	/* Volume control */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0x020, 0x00b ),		/* Power control: Regulator, follower & booster on */
  NGFX_U8X8_DLY(100),

  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


uint8_t ngfx_u8x8_d_st75256_wo256x128(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  if ( ngfx_u8x8_d_st75256_256x128_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;
  if ( msg == NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY )
  {
    //ngfx_u8x8_SetI2CAddress(u8x8, 0x078);		/* lowest I2C adr of the ST75256 */
    ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_st75256_wo256x128_display_info);
    return 1;
  }
  else if ( msg == NGFX_U8X8_MSG_DISPLAY_INIT )
  {
    ngfx_u8x8_d_helper_display_init(u8x8);
    ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_wo256x128_init_seq);    
    return 1;
  }
  else if  ( msg == NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE )
  {
    if ( arg_int == 0 )
    {
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip1_seq);	// this matches the init sequence
      u8x8->x_offset = u8x8->display_info->default_x_offset;
    }
    else
    {
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip0_seq);
      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
    }
    return 1;
  }
  return 0;
}


/*=============================================*/
/* JLX25664 */

static const ngfx_u8x8_display_info_t ngfx_u8x8_st75256_256x64_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 20,
  /* reset_pulse_width_ms = */ 5, 	
  /* post_reset_wait_ms = */ 5, 		/**/
  /* sda_setup_time_ns = */ 20,		/* */
  /* sck_pulse_width_ns = */ 40,	/*  */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,	/* 400KHz */
  /* data_setup_time_ns = */ 15,
  /* write_pulse_width_ns = */ 70,	
  /* tile_width = */ 32,
  /* tile_height = */ 8,
  /* default_x_offset = */ 0,	/* must be 0, because this is checked also for normal mode */
  /* flipmode_x_offset = */ 13,		/* used as y offset */
  /* pixel_width = */ 256,
  /* pixel_height = */ 64
};


static const uint8_t ngfx_u8x8_d_st75256_256x64_init_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  NGFX_U8X8_DLY(20),

  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x094 ),				/* sleep out */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x0ae ),				/* display off */

  NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_CA( 0x0d7, 0x09f ),		/* disable auto read */  

  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x032 ),				/* analog circuit set */
  NGFX_U8X8_A( 0x000 ),				/* code example: OSC Frequency adjustment */
  NGFX_U8X8_A( 0x001 ),				/* Frequency on booster capacitors 1 = 6KHz? */
  NGFX_U8X8_A( 0x005 ),				/* Bias: 1: 1/13, 2: 1/12, 3: 1/11, 4:1/10, 5:1/9 */
    
  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x020 ),				/* gray levels */
  NGFX_U8X8_A( 0x01 ),
  NGFX_U8X8_A( 0x03 ),
  NGFX_U8X8_A( 0x05 ),
  NGFX_U8X8_A( 0x07 ),
  NGFX_U8X8_A( 0x09),
  NGFX_U8X8_A( 0x0b ),
  NGFX_U8X8_A( 0x0d ),
  NGFX_U8X8_A( 0x10 ),
  NGFX_U8X8_A( 0x11 ),
  NGFX_U8X8_A( 0x13 ),
  NGFX_U8X8_A( 0x15 ),
  NGFX_U8X8_A( 0x17 ),
  NGFX_U8X8_A( 0x19 ),
  NGFX_U8X8_A( 0x1b ),
  NGFX_U8X8_A( 0x1d ),
  NGFX_U8X8_A( 0x1f ),
 
  
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA(0x75, 0, 0x1f),		/* row range */
  NGFX_U8X8_CAA(0x15, 0, 255),		/* col range */
  
  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x00 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x00c ),				/* data format LSB top */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_C( 0xca ),				/* display control, 3 args follow  */
  NGFX_U8X8_A( 0x00 ),				/* 0x00: no clock division, 0x04: devide clock */
  NGFX_U8X8_A( 0x3f ),				/* 64 duty value from the DS example code */
  NGFX_U8X8_A( 0x20 ),				/* nline off */ 

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_CA( 0x0f0, 0x010 ),		/* monochrome mode  = 0x010*/

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA( 0x81, 012, 0x02 ),	/* Volume control */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0x020, 0x00b ),		/* Power control: Regulator, follower & booster on */
  NGFX_U8X8_DLY(100),

  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


uint8_t ngfx_u8x8_d_st75256_jlx25664(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  if ( ngfx_u8x8_d_st75256_256x128_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;
  if ( msg == NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY )
  {
    //ngfx_u8x8_SetI2CAddress(u8x8, 0x078);		/* lowest I2C adr of the ST75256 */
    ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_st75256_256x64_display_info);
    return 1;
  }
  else if ( msg == NGFX_U8X8_MSG_DISPLAY_INIT )
  {
    ngfx_u8x8_d_helper_display_init(u8x8);
    ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x64_init_seq);    
    return 1;
  }
  else if  ( msg == NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE )
  {
    if ( arg_int == 0 )
    {
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx256128_flip0_seq);
      u8x8->x_offset = u8x8->display_info->default_x_offset;
    }
    else
    {
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx256128_flip1_seq);
      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
    }
    return 1;
  }
  return 0;
}


/*=============================================*/
/* JLX172104 LCD */

static const ngfx_u8x8_display_info_t ngfx_u8x8_st75256_172x104_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 20,
  /* reset_pulse_width_ms = */ 5, 	
  /* post_reset_wait_ms = */ 5, 		/**/
  /* sda_setup_time_ns = */ 20,		/* */
  /* sck_pulse_width_ns = */ 40,	/*  */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,	/* 400KHz */
  /* data_setup_time_ns = */ 15,
  /* write_pulse_width_ns = */ 70,	
  /* tile_width = */ 22,			/* 22=176 */
  /* tile_height = */ 13,
  /* default_x_offset = */ 84,	/*  */
  /* flipmode_x_offset = */ 0,		
  /* pixel_width = */ 172,
  /* pixel_height = */ 104
};

static const uint8_t ngfx_u8x8_d_st75256_jlx172104_init_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  NGFX_U8X8_DLY(20),

  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x094 ),				/* sleep out */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x0ae ),				/* display off */

  NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_CA( 0x0d7, 0x09f ),		/* disable auto read */  

  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x032 ),				/* analog circuit set */
  NGFX_U8X8_A( 0x000 ),				/* code example: OSC Frequency adjustment */
  NGFX_U8X8_A( 0x001 ),				/* Frequency on booster capacitors 1 = 6KHz? */
  NGFX_U8X8_A( 0x003 ),				/* Bias: 1: 1/13, 2: 1/12, 3: 1/11, 4:1/10, 5:1/9 */
    
  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x020 ),				/* gray levels */
  NGFX_U8X8_A( 0x01 ),
  NGFX_U8X8_A( 0x03 ),
  NGFX_U8X8_A( 0x05 ),
  NGFX_U8X8_A( 0x07 ),
  NGFX_U8X8_A( 0x09),
  NGFX_U8X8_A( 0x0b ),
  NGFX_U8X8_A( 0x0d ),
  NGFX_U8X8_A( 0x10 ),
  NGFX_U8X8_A( 0x11 ),
  NGFX_U8X8_A( 0x13 ),
  NGFX_U8X8_A( 0x15 ),
  NGFX_U8X8_A( 0x17 ),
  NGFX_U8X8_A( 0x19 ),
  NGFX_U8X8_A( 0x1b ),
  NGFX_U8X8_A( 0x1d ),
  NGFX_U8X8_A( 0x1f ),
 
  
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA(0x75, 0, 0x4f),		/* row range */
  NGFX_U8X8_CAA(0x15, 0, 255),		/* col range */
  
  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x02 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x00c ),				/* data format LSB top */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_C( 0xca ),				/* display control, 3 args follow  */
  NGFX_U8X8_A( 0x00 ),				/* 0x00: no clock division, 0x04: devide clock */
  NGFX_U8X8_A( 0x9f ),				/* 1/160 duty value from the DS example code */
  NGFX_U8X8_A( 0x20 ),				/* nline off */ 

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_CA( 0x0f0, 0x010 ),		/* monochrome mode  = 0x010*/

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA( 0x81, 0x08, 0x04 ),	/* Volume control */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0x020, 0x00b ),		/* Power control: Regulator, follower & booster on */
  NGFX_U8X8_DLY(100),

  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};    


uint8_t ngfx_u8x8_d_st75256_jlx172104(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;

  switch(msg)
  {
            case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
              
              ngfx_u8x8_cad_StartTransfer(u8x8);
              x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;    
              x *= 8;
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );	/* select command set */
              ngfx_u8x8_cad_SendCmd(u8x8, 0x075 );	/* row */
	      if ( u8x8->x_offset == 0 )		/* 0 means flip mode 1, then adjust y value */
		ngfx_u8x8_cad_SendArg(u8x8, 8+(((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
	      else
		ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              ngfx_u8x8_cad_SendArg(u8x8, 0x04f);
              //ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              ngfx_u8x8_cad_SendCmd(u8x8, 0x015 );	/* col */
              ngfx_u8x8_cad_SendArg(u8x8, x+u8x8->x_offset);
              ngfx_u8x8_cad_SendArg(u8x8, 255);
              ngfx_u8x8_cad_SendCmd(u8x8, 0x05c );	
            
              
              /* this procedure assumes, that the overall width is 172 */
              do
              {
                c = ((ngfx_u8x8_tile_t *)arg_ptr)->cnt;
                ptr = ((ngfx_u8x8_tile_t *)arg_ptr)->tile_ptr;
                c *= 8;

                if ( c + x > 172u )
                {
                        c = 172u;
                        c -= x;
                }
                      
                ngfx_u8x8_cad_SendData(u8x8, c, ptr); 	
                x += c;
                arg_int--;
              } while( arg_int > 0 );
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
        case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
            //ngfx_u8x8_SetI2CAddress(u8x8, 0x078);		/* lowest I2C adr of the ST75256 */
            ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_st75256_172x104_display_info);
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_INIT:
            ngfx_u8x8_d_helper_display_init(u8x8);
            ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_init_seq);
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
              if ( arg_int == 0 )
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave0_seq);
              else
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave1_seq);

              return 1;
	case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	    if ( arg_int == 0 )
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip0_seq);
	      u8x8->x_offset = u8x8->display_info->default_x_offset;
	    }
	    else
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip1_seq); 
	      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	    }
	    return 1;
		
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
        case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:

              ngfx_u8x8_cad_StartTransfer(u8x8);
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );
              ngfx_u8x8_cad_SendCmd(u8x8, 0x081 );  /* there are 9 bit for the volume control */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int & 0x1f)<<1 );	/* lower 6 bit */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int>>5));		/* upper 3 bit */
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
#endif
  }
  return 0;
}

/*=============================================*/
/* JLX240160 */

static const ngfx_u8x8_display_info_t ngfx_u8x8_st75256_240x160_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 20,
  /* reset_pulse_width_ms = */ 5, 	
  /* post_reset_wait_ms = */ 5, 		/**/
  /* sda_setup_time_ns = */ 20,		/* */
  /* sck_pulse_width_ns = */ 40,	/*  */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,	/* 400KHz */
  /* data_setup_time_ns = */ 15,
  /* write_pulse_width_ns = */ 70,	
  /* tile_width = */ 30,
  /* tile_height = */ 20,
  /* default_x_offset = */ 16,	/*  x offset in flipmode 0 */
  /* flipmode_x_offset = */ 0,		/* */
  /* pixel_width = */ 240,
  /* pixel_height = */ 160
};


static const uint8_t ngfx_u8x8_d_st75256_240x160_init_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  NGFX_U8X8_DLY(20),

  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x094 ),				/* sleep out */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x0ae ),				/* display off */

  NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_CA( 0x0d7, 0x09f ),		/* disable auto read */  

  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x032 ),				/* analog circuit set */
  NGFX_U8X8_A( 0x000 ),				/* code example: OSC Frequency adjustment */
  NGFX_U8X8_A( 0x001 ),				/* Frequency on booster capacitors 1 = 6KHz? */
  NGFX_U8X8_A( 0x000 ),				/* Bias: 1: 1/13, 2: 1/12, 3: 1/11, 4:1/10, 5:1/9 */
    
  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x020 ),				/* gray levels */
  NGFX_U8X8_A( 0x01 ),
  NGFX_U8X8_A( 0x03 ),
  NGFX_U8X8_A( 0x05 ),
  NGFX_U8X8_A( 0x07 ),
  NGFX_U8X8_A( 0x09),
  NGFX_U8X8_A( 0x0b ),
  NGFX_U8X8_A( 0x0d ),
  NGFX_U8X8_A( 0x10 ),
  NGFX_U8X8_A( 0x11 ),
  NGFX_U8X8_A( 0x13 ),
  NGFX_U8X8_A( 0x15 ),
  NGFX_U8X8_A( 0x17 ),
  NGFX_U8X8_A( 0x19 ),
  NGFX_U8X8_A( 0x1b ),
  NGFX_U8X8_A( 0x1d ),
  NGFX_U8X8_A( 0x1f ),
 
  
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA(0x75, 0, 0x4f),		/* row range */
  NGFX_U8X8_CAA(0x15, 0, 239),		/* col range */
  
  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x02 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x00c ),				/* data format LSB top */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_C( 0xca ),				/* display control, 3 args follow  */
  NGFX_U8X8_A( 0x00 ),				/* 0x00: no clock division, 0x04: devide clock */
  NGFX_U8X8_A( 159 ),				/* 1/160 duty value from the DS example code */
  NGFX_U8X8_A( 0x20 ),				/* nline off */ 

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_CA( 0x0f0, 0x010 ),		/* monochrome mode  = 0x010*/

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA( 0x81, 0x18, 0x04 ),	/* Volume control */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0x020, 0x00b ),		/* Power control: Regulator, follower & booster on */
  NGFX_U8X8_DLY(100),

  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


uint8_t ngfx_u8x8_d_st75256_jlx240160(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;

  switch(msg)
  {
            case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
              
              ngfx_u8x8_cad_StartTransfer(u8x8);
              x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;    
              x *= 8;
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );	/* select command set */
              ngfx_u8x8_cad_SendCmd(u8x8, 0x075 );	/* row */
	      if ( u8x8->x_offset == 0 )		/* 0 means flip mode 1 */
		ngfx_u8x8_cad_SendArg(u8x8, 1+(((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
	      else
		ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              ngfx_u8x8_cad_SendArg(u8x8, 0x04f);
              //ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              ngfx_u8x8_cad_SendCmd(u8x8, 0x015 );	/* col */
              ngfx_u8x8_cad_SendArg(u8x8, x+u8x8->x_offset);
              ngfx_u8x8_cad_SendArg(u8x8, 255);
              ngfx_u8x8_cad_SendCmd(u8x8, 0x05c );	
            
              
              do
              {
                c = ((ngfx_u8x8_tile_t *)arg_ptr)->cnt;
                ptr = ((ngfx_u8x8_tile_t *)arg_ptr)->tile_ptr;
                c *= 8;

                if ( c + x > 240u )
                {
                        c = 240u;
                        c -= x;
                }
                      
                ngfx_u8x8_cad_SendData(u8x8, c, ptr); 	
                x += c;
                arg_int--;
              } while( arg_int > 0 );
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
        case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
            //ngfx_u8x8_SetI2CAddress(u8x8, 0x078);		/* lowest I2C adr of the ST75256 */
	    ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_st75256_240x160_display_info);
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_INIT:
	    ngfx_u8x8_d_helper_display_init(u8x8);
	    ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_240x160_init_seq);    
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
              if ( arg_int == 0 )
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave0_seq);
              else
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave1_seq);

              return 1;
	case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	    if ( arg_int == 0 )
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip0_seq);
	      u8x8->x_offset = u8x8->display_info->default_x_offset;
	    }
	    else
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip1_seq);
	      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	    }
	    return 1;
		
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
        case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:

              ngfx_u8x8_cad_StartTransfer(u8x8);
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );
              ngfx_u8x8_cad_SendCmd(u8x8, 0x081 );  /* there are 9 bit for the volume control */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int & 0x1f)<<1 );	/* lower 6 bit */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int>>5));		/* upper 3 bit */
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
#endif
  }
  return 0;
  
  
  
}


/*=============================================*/
/* JLX256160 */

static const ngfx_u8x8_display_info_t ngfx_u8x8_st75256_256x160_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 20,
  /* reset_pulse_width_ms = */ 5, 	
  /* post_reset_wait_ms = */ 5, 		/**/
  /* sda_setup_time_ns = */ 20,		/* */
  /* sck_pulse_width_ns = */ 40,	/*  */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,	/* 400KHz */
  /* data_setup_time_ns = */ 15,
  /* write_pulse_width_ns = */ 70,	
  /* tile_width = */ 32,
  /* tile_height = */ 20,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 1,	/* x offset is used as y offset in flipmode */
  /* pixel_width = */ 256,
  /* pixel_height = */ 160
};


static const uint8_t ngfx_u8x8_d_st75256_256x160_init_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  NGFX_U8X8_DLY(20),

  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x094 ),				/* sleep out */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x0ae ),				/* display off */

  NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_CA( 0x0d7, 0x09f ),		/* disable auto read */  

  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x032 ),				/* analog circuit set */
  NGFX_U8X8_A( 0x000 ),				/* code example: OSC Frequency adjustment */
  NGFX_U8X8_A( 0x001 ),				/* Frequency on booster capacitors 1 = 6KHz? */
  NGFX_U8X8_A( 0x000 ),				/* Bias: 1: 1/13, 2: 1/12, 3: 1/11, 4:1/10, 5:1/9 */
    
  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x020 ),				/* gray levels */
  NGFX_U8X8_A( 0x01 ),
  NGFX_U8X8_A( 0x03 ),
  NGFX_U8X8_A( 0x05 ),
  NGFX_U8X8_A( 0x07 ),
  NGFX_U8X8_A( 0x09),
  NGFX_U8X8_A( 0x0b ),
  NGFX_U8X8_A( 0x0d ),
  NGFX_U8X8_A( 0x10 ),
  NGFX_U8X8_A( 0x11 ),
  NGFX_U8X8_A( 0x13 ),
  NGFX_U8X8_A( 0x15 ),
  NGFX_U8X8_A( 0x17 ),
  NGFX_U8X8_A( 0x19 ),
  NGFX_U8X8_A( 0x1b ),
  NGFX_U8X8_A( 0x1d ),
  NGFX_U8X8_A( 0x1f ),
 
  
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA(0x75, 0, 0x28),		/* row range */
  NGFX_U8X8_CAA(0x15, 0, 0xFF),		/* col range */
  
  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x00 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x00c ),				/* data format LSB top */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_C( 0xca ),				/* display control, 3 args follow  */
  NGFX_U8X8_A( 0x00 ),				/* 0x00: no clock division, 0x04: devide clock */
  NGFX_U8X8_A( 159 ),				/* 1/160 duty value from the DS example code */
  NGFX_U8X8_A( 0x20 ),				/* nline off */ 

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_CA( 0x0f0, 0x010 ),		/* monochrome mode  = 0x010*/

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA( 0x81, 0x18, 0x05 ),	/* Volume control */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0x020, 0x00b ),		/* Power control: Regulator, follower & booster on */
  NGFX_U8X8_DLY(100),

  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


uint8_t ngfx_u8x8_d_st75256_jlx256160(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;

  switch(msg)
  {
            case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
              
              ngfx_u8x8_cad_StartTransfer(u8x8);
              x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;    
              x *= 8;
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );	/* select command set */
              ngfx_u8x8_cad_SendCmd(u8x8, 0x075 );	/* row */
	      if ( u8x8->x_offset == 1 )		/* 1 means flip mode 1 */
		ngfx_u8x8_cad_SendArg(u8x8, 1+(((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
	      else
		ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              ngfx_u8x8_cad_SendArg(u8x8, 0x04f);
              //ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              ngfx_u8x8_cad_SendCmd(u8x8, 0x015 );	/* col */
              ngfx_u8x8_cad_SendArg(u8x8, x+u8x8->display_info->default_x_offset);
              ngfx_u8x8_cad_SendArg(u8x8, 255);
              ngfx_u8x8_cad_SendCmd(u8x8, 0x05c );	
            
              
              do
              {
                c = ((ngfx_u8x8_tile_t *)arg_ptr)->cnt;
                ptr = ((ngfx_u8x8_tile_t *)arg_ptr)->tile_ptr;
		/* SendData can not handle more than 255 bytes, treat c > 31 correctly  */
		if ( c > 31 )
		{
		  ngfx_u8x8_cad_SendData(u8x8, 248, ptr); 	/* 31*8=248 */
		  ptr+=248;
		  c -= 31;
		}
		
		ngfx_u8x8_cad_SendData(u8x8, c*8, ptr); 	
                arg_int--;
              } while( arg_int > 0 );
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
        case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
            //ngfx_u8x8_SetI2CAddress(u8x8, 0x078);		/* lowest I2C adr of the ST75256 */
	    ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_st75256_256x160_display_info);
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_INIT:
	    ngfx_u8x8_d_helper_display_init(u8x8);
	    ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x160_init_seq);    
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
              if ( arg_int == 0 )
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave0_seq);
              else
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave1_seq);

              return 1;
	case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	    if ( arg_int == 0 )
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx256160_flip0_seq);
	      u8x8->x_offset = u8x8->display_info->default_x_offset;
	    }
	    else
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx256160_flip1_seq);
	      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	    }
	    return 1;
		
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
        case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:

              ngfx_u8x8_cad_StartTransfer(u8x8);
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );
              ngfx_u8x8_cad_SendCmd(u8x8, 0x081 );  /* there are 9 bit for the volume control */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int & 0x1f)<<1 );	/* lower 6 bit */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int>>5));		/* upper 3 bit */
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
#endif
  }
  return 0;
}


/*=============================================*/
/* JLX256160 mirror version #930 */


static const uint8_t ngfx_u8x8_d_st75256_256x160m_init_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  NGFX_U8X8_DLY(20),

  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x094 ),				/* sleep out */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x0ae ),				/* display off */

  NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_CA( 0x0d7, 0x09f ),		/* disable auto read */  

  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x032 ),				/* analog circuit set */
  NGFX_U8X8_A( 0x000 ),				/* code example: OSC Frequency adjustment */
  NGFX_U8X8_A( 0x001 ),				/* Frequency on booster capacitors 1 = 6KHz? */
  NGFX_U8X8_A( 0x000 ),				/* Bias: 1: 1/13, 2: 1/12, 3: 1/11, 4:1/10, 5:1/9 */
    
  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x020 ),				/* gray levels */
  NGFX_U8X8_A( 0x01 ),
  NGFX_U8X8_A( 0x03 ),
  NGFX_U8X8_A( 0x05 ),
  NGFX_U8X8_A( 0x07 ),
  NGFX_U8X8_A( 0x09),
  NGFX_U8X8_A( 0x0b ),
  NGFX_U8X8_A( 0x0d ),
  NGFX_U8X8_A( 0x10 ),
  NGFX_U8X8_A( 0x11 ),
  NGFX_U8X8_A( 0x13 ),
  NGFX_U8X8_A( 0x15 ),
  NGFX_U8X8_A( 0x17 ),
  NGFX_U8X8_A( 0x19 ),
  NGFX_U8X8_A( 0x1b ),
  NGFX_U8X8_A( 0x1d ),
  NGFX_U8X8_A( 0x1f ),
 
  
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA(0x75, 0, 0x28),		/* row range */
  NGFX_U8X8_CAA(0x15, 0, 0xFF),		/* col range */
  
  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x02 ),			/* data scan dir  ( CHANGED FOR MIRROR VERSION ) */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x00c ),				/* data format LSB top */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_C( 0xca ),				/* display control, 3 args follow  */
  NGFX_U8X8_A( 0x00 ),				/* 0x00: no clock division, 0x04: devide clock */
  NGFX_U8X8_A( 159 ),				/* 1/160 duty value from the DS example code */
  NGFX_U8X8_A( 0x20 ),				/* nline off */ 

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_CA( 0x0f0, 0x010 ),		/* monochrome mode  = 0x010*/

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA( 0x81, 0x18, 0x05 ),	/* Volume control */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0x020, 0x00b ),		/* Power control: Regulator, follower & booster on */
  NGFX_U8X8_DLY(100),

  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

uint8_t ngfx_u8x8_d_st75256_jlx256160m(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;

  switch(msg)
  {
            case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
              
              ngfx_u8x8_cad_StartTransfer(u8x8);
              x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;    
              x *= 8;
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );	/* select command set */
              ngfx_u8x8_cad_SendCmd(u8x8, 0x075 );	/* row */
	      if ( u8x8->x_offset == 1 )		/* 1 means flip mode 1 */
		ngfx_u8x8_cad_SendArg(u8x8, 1+(((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
	      else
		ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              ngfx_u8x8_cad_SendArg(u8x8, 0x04f);
              //ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              ngfx_u8x8_cad_SendCmd(u8x8, 0x015 );	/* col */
              ngfx_u8x8_cad_SendArg(u8x8, x+u8x8->display_info->default_x_offset);
              ngfx_u8x8_cad_SendArg(u8x8, 255);
              ngfx_u8x8_cad_SendCmd(u8x8, 0x05c );	
            
              
              do
              {
                c = ((ngfx_u8x8_tile_t *)arg_ptr)->cnt;
                ptr = ((ngfx_u8x8_tile_t *)arg_ptr)->tile_ptr;
		/* SendData can not handle more than 255 bytes, treat c > 31 correctly  */
		if ( c > 31 )
		{
		  ngfx_u8x8_cad_SendData(u8x8, 248, ptr); 	/* 31*8=248 */
		  ptr+=248;
		  c -= 31;
		}
		
		ngfx_u8x8_cad_SendData(u8x8, c*8, ptr); 	
                arg_int--;
              } while( arg_int > 0 );
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
        case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
            //ngfx_u8x8_SetI2CAddress(u8x8, 0x078);		/* lowest I2C adr of the ST75256 */
	    ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_st75256_256x160_display_info);
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_INIT:
	    ngfx_u8x8_d_helper_display_init(u8x8);
	    ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x160m_init_seq);    
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
              if ( arg_int == 0 )
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave0_seq);
              else
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave1_seq);

              return 1;
	case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	    if ( arg_int == 0 )
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip0_seq);
	      u8x8->x_offset = u8x8->display_info->default_x_offset;
	    }
	    else
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip1_seq);
	      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	    }
	    return 1;
		
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
        case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:

              ngfx_u8x8_cad_StartTransfer(u8x8);
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );
              ngfx_u8x8_cad_SendCmd(u8x8, 0x081 );  /* there are 9 bit for the volume control */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int & 0x1f)<<1 );	/* lower 6 bit */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int>>5));		/* upper 3 bit */
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
#endif
  }
  return 0;
}




/*=============================================*/
/* JLX256160 alternative version from issue #561 */

static const ngfx_u8x8_display_info_t ngfx_u8x8_st75256_256x160_alt_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 20,
  /* reset_pulse_width_ms = */ 5, 	
  /* post_reset_wait_ms = */ 5, 		/**/
  /* sda_setup_time_ns = */ 20,		/* */
  /* sck_pulse_width_ns = */ 40,	/*  */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,	/* 400KHz */
  /* data_setup_time_ns = */ 15,
  /* write_pulse_width_ns = */ 70,	
  /* tile_width = */ 32,
  /* tile_height = */ 20,
  /* default_x_offset = */ 0,	/*  x offset in flipmode 0 */
  /* flipmode_x_offset = */ 0,		/* */
  /* pixel_width = */ 256,
  /* pixel_height = */ 160
};


static const uint8_t ngfx_u8x8_d_st75256_256x160_alt_init_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  NGFX_U8X8_DLY(20),

  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x094 ),				/* sleep out */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x0ae ),				/* display off */

  NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_CA( 0x0d7, 0x09f ),		/* disable auto read */  

  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x032 ),				/* analog circuit set */
  NGFX_U8X8_A( 0x000 ),				/* code example: OSC Frequency adjustment */
  NGFX_U8X8_A( 0x001 ),				/* Frequency on booster capacitors 1 = 6KHz? */
  NGFX_U8X8_A( 0x000 ),				/* Bias: 1: 1/13, 2: 1/12, 3: 1/11, 4:1/10, 5:1/9 */
    
  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x020 ),				/* gray levels */
  NGFX_U8X8_A( 0x01 ),
  NGFX_U8X8_A( 0x03 ),
  NGFX_U8X8_A( 0x05 ),
  NGFX_U8X8_A( 0x07 ),
  NGFX_U8X8_A( 0x09),
  NGFX_U8X8_A( 0x0b ),
  NGFX_U8X8_A( 0x0d ),
  NGFX_U8X8_A( 0x10 ),
  NGFX_U8X8_A( 0x11 ),
  NGFX_U8X8_A( 0x13 ),
  NGFX_U8X8_A( 0x15 ),
  NGFX_U8X8_A( 0x17 ),
  NGFX_U8X8_A( 0x19 ),
  NGFX_U8X8_A( 0x1b ),
  NGFX_U8X8_A( 0x1d ),
  NGFX_U8X8_A( 0x1f ),
 
  
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA(0x75, 0, 0x4f),		/* row range */
  NGFX_U8X8_CAA(0x15, 0, 255),		/* col range */
  
  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x02 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x00c ),				/* data format LSB top */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_C( 0xca ),				/* display control, 3 args follow  */
  NGFX_U8X8_A( 0x00 ),				/* 0x00: no clock division, 0x04: devide clock */
  NGFX_U8X8_A( 159 ),				/* 1/160 duty value from the DS example code */
  NGFX_U8X8_A( 0x20 ),				/* nline off */ 

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_CA( 0x0f0, 0x010 ),		/* monochrome mode  = 0x010*/

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA( 0x81, 0x18, 0x05 ),	/* Volume control */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0x020, 0x00b ),		/* Power control: Regulator, follower & booster on */
  NGFX_U8X8_DLY(100),

  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


uint8_t ngfx_u8x8_d_st75256_jlx256160_alt(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;

  switch(msg)
  {
            case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
              
              ngfx_u8x8_cad_StartTransfer(u8x8);
              x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;    
              x *= 8;
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );	/* select command set */
              ngfx_u8x8_cad_SendCmd(u8x8, 0x075 );	/* row */
	      if ( u8x8->x_offset == 0 )		/* 0 means flip mode 1 */
		ngfx_u8x8_cad_SendArg(u8x8, 1+(((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
	      else
		ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              ngfx_u8x8_cad_SendArg(u8x8, 0x04f);
              //ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              ngfx_u8x8_cad_SendCmd(u8x8, 0x015 );	/* col */
              ngfx_u8x8_cad_SendArg(u8x8, x+u8x8->x_offset);
              ngfx_u8x8_cad_SendArg(u8x8, 255);
              ngfx_u8x8_cad_SendCmd(u8x8, 0x05c );	
            
              
              do
              {
                c = ((ngfx_u8x8_tile_t *)arg_ptr)->cnt;
                ptr = ((ngfx_u8x8_tile_t *)arg_ptr)->tile_ptr;
		/* SendData can not handle more than 255 bytes, treat c > 31 correctly  */
		if ( c > 31 )
		{
		  ngfx_u8x8_cad_SendData(u8x8, 248, ptr); 	/* 31*8=248 */
		  ptr+=248;
		  c -= 31;
		}
		
		ngfx_u8x8_cad_SendData(u8x8, c*8, ptr); 	
                arg_int--;
              } while( arg_int > 0 );
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
        case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
            //ngfx_u8x8_SetI2CAddress(u8x8, 0x078);		/* lowest I2C adr of the ST75256 */
	    ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_st75256_256x160_alt_display_info);
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_INIT:
	    ngfx_u8x8_d_helper_display_init(u8x8);
	    ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x160_alt_init_seq);    
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
              if ( arg_int == 0 )
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave0_seq);
              else
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave1_seq);

              return 1;
	case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	    if ( arg_int == 0 )
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip0_seq);
	      u8x8->x_offset = u8x8->display_info->default_x_offset;
	    }
	    else
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip1_seq);
	      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	    }
	    return 1;
		
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
        case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:

              ngfx_u8x8_cad_StartTransfer(u8x8);
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );
              ngfx_u8x8_cad_SendCmd(u8x8, 0x081 );  /* there are 9 bit for the volume control */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int & 0x1f)<<1 );	/* lower 6 bit */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int>>5));		/* upper 3 bit */
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
#endif
  }
  return 0;

} 


/*=============================================*/
/* JLX19296 LCD */

static const ngfx_u8x8_display_info_t ngfx_u8x8_st75256_192x96_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 20,
  /* reset_pulse_width_ms = */ 5, 	
  /* post_reset_wait_ms = */ 5, 		/**/
  /* sda_setup_time_ns = */ 20,		/* */
  /* sck_pulse_width_ns = */ 40,	/*  */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,	/* 400KHz */
  /* data_setup_time_ns = */ 15,
  /* write_pulse_width_ns = */ 70,	
  /* tile_width = */ 24,	
  /* tile_height = */ 12,
  /* default_x_offset = */ 0,	/*  */
  /* flipmode_x_offset = */ 64,
  /* pixel_width = */ 192,
  /* pixel_height = */ 96
};

static const uint8_t ngfx_u8x8_d_st75256_jlx19296_init_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  NGFX_U8X8_DLY(20),

  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x094 ),				/* sleep out */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x0ae ),				/* display off */

  NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_CA( 0x0d7, 0x09f ),		/* disable auto read */  

  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x032 ),				/* analog circuit set */
  NGFX_U8X8_A( 0x000 ),				/* code example: OSC Frequency adjustment */
  NGFX_U8X8_A( 0x001 ),				/* Frequency on booster capacitors 1 = 6KHz? */
  NGFX_U8X8_A( 0x003 ),				/* Bias: 1: 1/13, 2: 1/12, 3: 1/11, 4:1/10, 5:1/9 */
    
  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x020 ),				/* gray levels */
  NGFX_U8X8_A( 0x01 ),
  NGFX_U8X8_A( 0x03 ),
  NGFX_U8X8_A( 0x05 ),
  NGFX_U8X8_A( 0x07 ),
  NGFX_U8X8_A( 0x09),
  NGFX_U8X8_A( 0x0b ),
  NGFX_U8X8_A( 0x0d ),
  NGFX_U8X8_A( 0x10 ),
  NGFX_U8X8_A( 0x11 ),
  NGFX_U8X8_A( 0x13 ),
  NGFX_U8X8_A( 0x15 ),
  NGFX_U8X8_A( 0x17 ),
  NGFX_U8X8_A( 0x19 ),
  NGFX_U8X8_A( 0x1b ),
  NGFX_U8X8_A( 0x1d ),
  NGFX_U8X8_A( 0x1f ),
 
  
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA(0x75, 0, 0x4f),		/* row range */
  NGFX_U8X8_CAA(0x15, 0, 255),		/* col range */
  
  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x00 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x00c ),				/* data format LSB top */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_C( 0xca ),				/* display control, 3 args follow  */
  NGFX_U8X8_A( 0x00 ),				/* 0x00: no clock division, 0x04: devide clock */
  NGFX_U8X8_A( 0x9f ),				/* 1/160 duty value from the DS example code */
  NGFX_U8X8_A( 0x20 ),				/* nline off */ 

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_CA( 0x0f0, 0x010 ),		/* monochrome mode  = 0x010*/

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA( 0x81, 0x2e, 0x03 ),	/* Volume control */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0x020, 0x00b ),		/* Power control: Regulator, follower & booster on */
  NGFX_U8X8_DLY(100),

  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};    


uint8_t ngfx_u8x8_d_st75256_jlx19296(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;

  switch(msg)
  {
            case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
              
              ngfx_u8x8_cad_StartTransfer(u8x8);
              x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;    
              x *= 8;
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );	/* select command set */
              ngfx_u8x8_cad_SendCmd(u8x8, 0x075 );	/* row */
	      if ( u8x8->x_offset == 0 )		/* 0 means flip mode 1, then adjust y value */
		ngfx_u8x8_cad_SendArg(u8x8, 8+(((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
	      else
		ngfx_u8x8_cad_SendArg(u8x8, 1+(((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              ngfx_u8x8_cad_SendArg(u8x8, 0x04f);
              //ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              ngfx_u8x8_cad_SendCmd(u8x8, 0x015 );	/* col */
              ngfx_u8x8_cad_SendArg(u8x8, x+u8x8->x_offset);
              ngfx_u8x8_cad_SendArg(u8x8, 255);
              ngfx_u8x8_cad_SendCmd(u8x8, 0x05c );	
            
              
              do
              {
                c = ((ngfx_u8x8_tile_t *)arg_ptr)->cnt;
                ptr = ((ngfx_u8x8_tile_t *)arg_ptr)->tile_ptr;
                c *= 8;

                if ( c + x > 192u )
                {
                        c = 192u;
                        c -= x;
                }
                      
                ngfx_u8x8_cad_SendData(u8x8, c, ptr); 	
                x += c;
                arg_int--;
              } while( arg_int > 0 );
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
        case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
            //ngfx_u8x8_SetI2CAddress(u8x8, 0x078);		/* lowest I2C adr of the ST75256 */
            ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_st75256_192x96_display_info);
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_INIT:
            ngfx_u8x8_d_helper_display_init(u8x8);
            ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx19296_init_seq);
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
              if ( arg_int == 0 )
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave0_seq);
              else
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave1_seq);
              return 1;
	case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	    if ( arg_int == 0 )
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx256160_flip0_seq);
	      u8x8->x_offset = u8x8->display_info->default_x_offset;
	    }
	    else
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx256160_flip1_seq);
	      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	    }
	    return 1;
	    /*
	    if ( arg_int == 0 )
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip0_seq);
	      u8x8->x_offset = u8x8->display_info->default_x_offset;
	    }
	    else
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip1_seq); 
	      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	    }
	    return 1;
	    */
		
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
        case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:

              ngfx_u8x8_cad_StartTransfer(u8x8);
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );
              ngfx_u8x8_cad_SendCmd(u8x8, 0x081 );  /* there are 9 bit for the volume control */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int & 0x1f)<<1 );	/* lower 6 bit */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int>>5));		/* upper 3 bit */
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
#endif
  }
  return 0;
}



/*=============================================*/
/* 
  JLX16080 LCD 

  https://github.com/olikraus/u8g2/issues/2058

  WARNING: NOT FULLY TESTED in issue 2058!
*/

static const ngfx_u8x8_display_info_t ngfx_u8x8_st75256_jlx16080_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 20,
  /* reset_pulse_width_ms = */ 5, 	
  /* post_reset_wait_ms = */ 5, 		/**/
  /* sda_setup_time_ns = */ 20,		/* */
  /* sck_pulse_width_ns = */ 40,	/*  */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,	/* 400KHz */
  /* data_setup_time_ns = */ 15,
  /* write_pulse_width_ns = */ 70,	
  /* tile_width = */ 20,	
  /* tile_height = */ 10,
  /* default_x_offset = */ 0,	/*  */
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 160,
  /* pixel_height = */ 80
};

static const uint8_t ngfx_u8x8_d_st75256_jlx16080_init_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  NGFX_U8X8_DLY(20),

  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x094 ),				/* sleep out */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x0ae ),				/* display off */

  NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_CA( 0x0d7, 0x09f ),		/* disable auto read */  

  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x032 ),				/* analog circuit set */
  NGFX_U8X8_A( 0x000 ),				/* code example: OSC Frequency adjustment */
  NGFX_U8X8_A( 0x001 ),				/* Frequency on booster capacitors 1 = 6KHz? */
  NGFX_U8X8_A( 0x003 ),				/* Bias: 1: 1/13, 2: 1/12, 3: 1/11, 4:1/10, 5:1/9 */
    
  //NGFX_U8X8_C( 0x031 ),				/* select 01 commands */
  NGFX_U8X8_C( 0x020 ),				/* gray levels */
  NGFX_U8X8_A( 0x01 ),
  NGFX_U8X8_A( 0x03 ),
  NGFX_U8X8_A( 0x05 ),
  NGFX_U8X8_A( 0x07 ),
  NGFX_U8X8_A( 0x09),
  NGFX_U8X8_A( 0x0b ),
  NGFX_U8X8_A( 0x0d ),
  NGFX_U8X8_A( 0x10 ),
  NGFX_U8X8_A( 0x11 ),
  NGFX_U8X8_A( 0x13 ),
  NGFX_U8X8_A( 0x15 ),
  NGFX_U8X8_A( 0x17 ),
  NGFX_U8X8_A( 0x19 ),
  NGFX_U8X8_A( 0x1b ),
  NGFX_U8X8_A( 0x1d ),
  NGFX_U8X8_A( 0x1f ),
 
  
  NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA(0x75, 0, 0x4f),		/* row range */
  NGFX_U8X8_CAA(0x15, 0, 255),		/* col range */
  
  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0xbc, 0x00 ),			/* data scan dir */
  NGFX_U8X8_A( 0xa6 ),				/* ??? */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_C( 0x00c ),				/* data format LSB top */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_C( 0xca ),				/* display control, 3 args follow  */
  NGFX_U8X8_A( 0x00 ),				/* 0x00: no clock division, 0x04: devide clock */
  NGFX_U8X8_A( 0x9f ),				/* 1/160 duty value from the DS example code */
  NGFX_U8X8_A( 0x20 ),				/* nline off */ 

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */ 
  NGFX_U8X8_CA( 0x0f0, 0x010 ),		/* monochrome mode  = 0x010*/

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CAA( 0x81, 0x2e, 0x03 ),	/* Volume control */

  //NGFX_U8X8_C( 0x030 ),				/* select 00 commands */
  NGFX_U8X8_CA( 0x020, 0x00b ),		/* Power control: Regulator, follower & booster on */
  NGFX_U8X8_DLY(100),

  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};    


uint8_t ngfx_u8x8_d_st75256_jlx16080(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;

  switch(msg)
  {
            case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
              
              ngfx_u8x8_cad_StartTransfer(u8x8);
              x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;    
              x *= 8;
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );	/* select command set */
              ngfx_u8x8_cad_SendCmd(u8x8, 0x075 );	/* row */
            
            
	      //if ( u8x8->x_offset == 0 )		/* 0 means flip mode 1, then adjust y value */
		//ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
	      //else
		ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              
              
              ngfx_u8x8_cad_SendArg(u8x8, 0x04f);
              //ngfx_u8x8_cad_SendArg(u8x8, (((ngfx_u8x8_tile_t *)arg_ptr)->y_pos));
              ngfx_u8x8_cad_SendCmd(u8x8, 0x015 );	/* col */
              ngfx_u8x8_cad_SendArg(u8x8, x+u8x8->x_offset);
              ngfx_u8x8_cad_SendArg(u8x8, 255);
              ngfx_u8x8_cad_SendCmd(u8x8, 0x05c );	
            
              
              do
              {
                c = ((ngfx_u8x8_tile_t *)arg_ptr)->cnt;
                ptr = ((ngfx_u8x8_tile_t *)arg_ptr)->tile_ptr;
                c *= 8;

                if ( c + x > 192u )
                {
                        c = 192u;
                        c -= x;
                }
                      
                ngfx_u8x8_cad_SendData(u8x8, c, ptr); 	
                x += c;
                arg_int--;
              } while( arg_int > 0 );
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
        case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
            //ngfx_u8x8_SetI2CAddress(u8x8, 0x078);		/* lowest I2C adr of the ST75256 */
            ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_st75256_jlx16080_display_info);
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_INIT:
            ngfx_u8x8_d_helper_display_init(u8x8);
            ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx16080_init_seq);
            return 1;
        case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
              if ( arg_int == 0 )
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave0_seq);
              else
                ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_256x128_powersave1_seq);
              return 1;
	case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	    if ( arg_int == 0 )
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx256160_flip0_seq);
	      u8x8->x_offset = u8x8->display_info->default_x_offset;
	    }
	    else
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx256160_flip1_seq);
	      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	    }
	    return 1;
	    /*
	    if ( arg_int == 0 )
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip0_seq);
	      u8x8->x_offset = u8x8->display_info->default_x_offset;
	    }
	    else
	    {
	      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx172104_flip1_seq); 
	      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	    }
	    return 1;
	    */
		
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
        case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:

              ngfx_u8x8_cad_StartTransfer(u8x8);
              
              ngfx_u8x8_cad_SendCmd(u8x8, 0x030 );
              ngfx_u8x8_cad_SendCmd(u8x8, 0x081 );  /* there are 9 bit for the volume control */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int & 0x1f)<<1 );	/* lower 6 bit */
              ngfx_u8x8_cad_SendArg(u8x8, (arg_int>>5));		/* upper 3 bit */
              
              ngfx_u8x8_cad_EndTransfer(u8x8);
              return 1;
#endif
  }
  return 0;
}

/*=============================================*/
/* 128x128  issue https://github.com/olikraus/u8g2/issues/2702*/

static const ngfx_u8x8_display_info_t ngfx_u8x8_st75256_128x128_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 20,
  /* pre_chip_disable_wait_ns = */ 20,
  /* reset_pulse_width_ms = */ 5, 	
  /* post_reset_wait_ms = */ 5, 		/**/
  /* sda_setup_time_ns = */ 20,		/* */
  /* sck_pulse_width_ns = */ 40,	/*  */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,	/* 400KHz */
  /* data_setup_time_ns = */ 15,
  /* write_pulse_width_ns = */ 70,	
  /* tile_width = */ 16,
  /* tile_height = */ 16,
  /* default_x_offset = */ 0,	/* must be 0, because this is checked also for normal mode */
  /* flipmode_x_offset = */ 0,		/* used as y offset */
  /* pixel_width = */ 128,
  /* pixel_height = */ 128
};


static const uint8_t ngfx_u8x8_d_st75256_128x128_init_seq[] = {
    NGFX_U8X8_START_TRANSFER(),
    NGFX_U8X8_DLY(10),               // 10ms delay

    NGFX_U8X8_C(0x30),               // Extension command 1
    NGFX_U8X8_C(0x6E),               // Enable Master

    NGFX_U8X8_C(0x31),               // Extension command 2
    NGFX_U8X8_C(0xD7), NGFX_U8X8_A(0x9F), // Disable Auto Read
    NGFX_U8X8_C(0xE0), NGFX_U8X8_A(0x00), // Enable OTP Read
    NGFX_U8X8_DLY(10),
    NGFX_U8X8_C(0xE3),               // OTP Up-Load
    NGFX_U8X8_DLY(20),               // 20ms delay
    NGFX_U8X8_C(0xE1),               // OTP Control End

    NGFX_U8X8_C(0x30),               // Extension command 1
    NGFX_U8X8_C(0x94),               // Sleep Out
    NGFX_U8X8_C(0xAE),               // Display Off
    NGFX_U8X8_DLY(50),               // 50ms delay

    NGFX_U8X8_C(0x20),               // Power Control  
    NGFX_U8X8_A(0x0B),               // VB, VR, VF All on
    NGFX_U8X8_C(0x81), NGFX_U8X8_A(0x1D), NGFX_U8X8_A(0x04), // Set Vop (15.0V)

    NGFX_U8X8_C(0x31),               // Extension command 2
    NGFX_U8X8_C(0x32), NGFX_U8X8_A(0x00), // Analog Set
    NGFX_U8X8_A(0x01),               // Booster efficiency level 1
    NGFX_U8X8_A(0x02),               // Bias 1/12
    NGFX_U8X8_C(0x51), NGFX_U8X8_A(0xFA), // Booster Level x 8

    NGFX_U8X8_C(0x30),               // Extension command 1
    NGFX_U8X8_C(0xF0), NGFX_U8X8_A(0x10), // Display Mode = Monochrome
    NGFX_U8X8_C(0xCA),               // Display Control
    NGFX_U8X8_A(0x00),               // CL Dividing Ratio = Not divided
    NGFX_U8X8_A(0x7F),               // Duty Set = 128
    NGFX_U8X8_A(0x3F),               
    NGFX_U8X8_C(0xBC), NGFX_U8X8_A(0x00), // Data Scan Direction
    NGFX_U8X8_C(0xA6),               // Normal Display
    NGFX_U8X8_C(0x0C),               // Data Format = LSB on Top
    NGFX_U8X8_CAA(0x75, 0x00, 0x7F), // row range for 128 rows
    NGFX_U8X8_CAA(0x15, 0x00, 0x7F), // col range for 128 cols

    NGFX_U8X8_C(0x31),               // Extension command 2
    NGFX_U8X8_C(0x40),               // Internal Power Supply

    NGFX_U8X8_C(0x30),               // Extension command 1
    NGFX_U8X8_C(0xaf),               // Display On
    NGFX_U8X8_END_TRANSFER(),
    NGFX_U8X8_END()
};


uint8_t ngfx_u8x8_d_st75256_128x128(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  if ( ngfx_u8x8_d_st75256_256x128_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;
  if ( msg == NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY )
  {
    //ngfx_u8x8_SetI2CAddress(u8x8, 0x078);		/* lowest I2C adr of the ST75256 */
    ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_st75256_128x128_display_info);
    return 1;
  }
  else if ( msg == NGFX_U8X8_MSG_DISPLAY_INIT )
  {
    ngfx_u8x8_d_helper_display_init(u8x8);
    ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_128x128_init_seq);    
    return 1;
  }
  else if  ( msg == NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE )
  {
    if ( arg_int == 0 )
    {
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx256128_flip0_seq);
      u8x8->x_offset = u8x8->display_info->default_x_offset;
    }
    else
    {
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_st75256_jlx256128_flip1_seq);
      u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
    }
    return 1;
  }
  return 0;
}


