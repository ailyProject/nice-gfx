/*

  ngfx_u8x8_d_uc1608.c
  
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





static const uint8_t ngfx_u8x8_d_uc1608_powersave0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0af),		                /* display on */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_uc1608_powersave1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0ae),		                /* display off, enter sleep mode */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_uc1608_flip0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0c8),            			/* LCD Mapping */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_uc1608_flip1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0c4),            			/* LCD Mapping */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


uint8_t ngfx_u8x8_d_uc1608_common(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, y, c;
  uint8_t *ptr;
  switch(msg)
  {
    case NGFX_U8X8_MSG_DISPLAY_DRAW_TILE:
      ngfx_u8x8_cad_StartTransfer(u8x8);
    
      x = ((ngfx_u8x8_tile_t *)arg_ptr)->x_pos;
      x *= 8;
   
      ngfx_u8x8_cad_SendCmd(u8x8, 0x000 | ((x&15)));
      ngfx_u8x8_cad_SendCmd(u8x8, 0x010 | (x>>4) );
    
      y = ((ngfx_u8x8_tile_t *)arg_ptr)->y_pos;
      y += u8x8->x_offset;
      ngfx_u8x8_cad_SendCmd(u8x8, 0x0b0 | (y&15));
    
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
    /*	handled in the calling procedure 
    case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
      ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_uc1608_128x64_display_info);
      break;
    case NGFX_U8X8_MSG_DISPLAY_INIT:
      ngfx_u8x8_d_helper_display_init(u8x8);
      ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_uc1701_dogs102_init_seq);
      break;
    */
    case NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      if ( arg_int == 0 )
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_uc1608_powersave0_seq);
      else
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_uc1608_powersave1_seq);
      break;
    case NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      if ( arg_int == 0 )
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_uc1608_flip0_seq);
	u8x8->x_offset = u8x8->display_info->default_x_offset;
      }
      else
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_uc1608_flip1_seq);
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
      }	
      break;
#ifdef NGFX_U8X8_WITH_SET_CONTRAST
    case NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST:
      ngfx_u8x8_cad_StartTransfer(u8x8);
      ngfx_u8x8_cad_SendCmd(u8x8, 0x081 );
      ngfx_u8x8_cad_SendArg(u8x8, arg_int  );	/* uc1608 has range from 0 to 255 */
      ngfx_u8x8_cad_EndTransfer(u8x8);
      break;
#endif
    default:
      return 0;
  }
  return 1;
}

/*================================================*/
/* ERC24064-1 */

/*
  The UC1608 has only one high active chip select input.
  UC1604, UC1610 and UC1611 have two chip select inputs.
*/

static const ngfx_u8x8_display_info_t ngfx_u8x8_uc1608_240x64_display_info =
{
  /* chip_enable_level = */ 1,	/* uc1608 has high active CS */
  /* chip_disable_level = */ 0,
  
  /* post_chip_enable_wait_ns = */ 10,	/* uc1608 datasheet, page 39, actually 0 */
  /* pre_chip_disable_wait_ns = */ 20,	/* uc1608 datasheet, page 39 */
  /* reset_pulse_width_ms = */ 1, 	/* uc1608 datasheet, page 42 */
  /* post_reset_wait_ms = */ 10, 	
  /* sda_setup_time_ns = */ 30,		/* uc1608 datasheet, page 41 */
  /* sck_pulse_width_ns = */ 65,	/* half of cycle time  */
  /* sck_clock_hz = */ 8000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 30,	/* uc1608 datasheet, page 39 */
  /* write_pulse_width_ns = */ 35,	/* uc1608 datasheet, page 39 */
  /* tile_width = */ 30,		/* width of 30*8=240 pixel */
  /* tile_height = */ 8,
  /* default_x_offset = */ 0,	/* reused as y page offset */
  /* flipmode_x_offset = */ 4,	/* reused as y page offset */
  /* pixel_width = */ 240,
  /* pixel_height = */ 64
};

static const uint8_t ngfx_u8x8_d_uc1608_erc24064_init_seq[] = {
    
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  NGFX_U8X8_C(0x0e2),            			/* soft reset */
  NGFX_U8X8_DLY(200),

  NGFX_U8X8_C(0x023),            			/* Bit 0/1: Temp compenstation, Bit 2: Multiplex Rate 0=96, 1=128 */
  //NGFX_U8X8_C(0x027),            			/* Bit 0/1: Temp compenstation, Bit 2: Multiplex Rate 0=96, 1=128 */

  NGFX_U8X8_C(0x0c8),            			/* Map control, Bit 3: MY=1, Bit 2: MX=0, Bit 0: MSF =0 */
  NGFX_U8X8_C(0x0e8),            			/* LCD bias Bits 0/1: 00=10.7 01=10.3, 10=12.0, 11=12.7 */
  
  NGFX_U8X8_C(0x02f),            			/* power on, Bit 2 PC2=1 (internal charge pump), Bits 0/1: cap of panel */
  NGFX_U8X8_DLY(50),
  
  NGFX_U8X8_C(0x040),            			/* set display start line to 0 */
  NGFX_U8X8_C(0x090),            			/* no fixed lines */
  NGFX_U8X8_C(0x089),            			/* RAM access control */
  
  NGFX_U8X8_CA(0x081, 0x014),		/* set contrast, ERC24064-1 default: 0x040 */
  
  NGFX_U8X8_C(0x000),		                /* column low nibble */
  NGFX_U8X8_C(0x010),		                /* column high nibble */  
  NGFX_U8X8_C(0x0b0),		                /* page adr  */
  
  
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

uint8_t ngfx_u8x8_d_uc1608_erc24064(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( ngfx_u8x8_d_uc1608_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
	ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_uc1608_240x64_display_info);
	break;
      case NGFX_U8X8_MSG_DISPLAY_INIT:
	ngfx_u8x8_d_helper_display_init(u8x8);
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_uc1608_erc24064_init_seq);
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}


/*================================================*/
/* experimental implementation for the uc1608 240x128, not referenced in codebuild */

static const ngfx_u8x8_display_info_t ngfx_u8x8_uc1608_240x128_display_info =
{
  /* chip_enable_level = */ 1,	/* uc1608 has high active CS */
  /* chip_disable_level = */ 0,
  
  /* post_chip_enable_wait_ns = */ 10,	/* uc1608 datasheet, page 39, actually 0 */
  /* pre_chip_disable_wait_ns = */ 20,	/* uc1608 datasheet, page 39 */
  /* reset_pulse_width_ms = */ 1, 	/* uc1608 datasheet, page 42 */
  /* post_reset_wait_ms = */ 10, 	
  /* sda_setup_time_ns = */ 30,		/* uc1608 datasheet, page 41 */
  /* sck_pulse_width_ns = */ 65,	/* half of cycle time  */
  /* sck_clock_hz = */ 8000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 30,	/* uc1608 datasheet, page 39 */
  /* write_pulse_width_ns = */ 35,	/* uc1608 datasheet, page 39 */
  /* tile_width = */ 30,		/* width of 30*8=240 pixel */
  /* tile_height = */ 16,
  /* default_x_offset = */ 0,	/* reused as y page offset */
  /* flipmode_x_offset = */ 0,	/* reused as y page offset */
  /* pixel_width = */ 240,
  /* pixel_height = */ 128
};

static const uint8_t ngfx_u8x8_d_uc1608_240x128_init_seq[] = {
    
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  NGFX_U8X8_C(0x0e2),            			/* soft reset */
  NGFX_U8X8_DLY(200),

  //NGFX_U8X8_C(0x023),            			/* Bit 0/1: Temp compenstation, Bit 2: Multiplex Rate 0=96, 1=128 */
  NGFX_U8X8_C(0x026),            			/* Bit 0/1: Temp compenstation, Bit 2: Multiplex Rate 0=96, 1=128 */

  NGFX_U8X8_C(0x0c8),            			/* Map control, Bit 3: MY=1, Bit 2: MX=0, Bit 0: MSF =0 */
  NGFX_U8X8_C(0x0ea),            			/* LCD bias Bits 0/1: 00=10.7 01=10.3, 10=12.0, 11=12.7 */
							/* maybe 0x0eb??? */
  
  NGFX_U8X8_C(0x02f),            			/* power on, Bit 2 PC2=1 (internal charge pump), Bits 0/1: cap of panel */
  NGFX_U8X8_DLY(50),
  
  NGFX_U8X8_C(0x040),            			/* set display start line to 0 */
  NGFX_U8X8_C(0x090),            			/* no fixed lines */
  NGFX_U8X8_C(0x089),            			/* RAM access control */
  
  NGFX_U8X8_CA(0x081, 0x072),		/* set contrast  */
  
  NGFX_U8X8_C(0x000),		                /* column low nibble */
  NGFX_U8X8_C(0x010),		                /* column high nibble */  
  NGFX_U8X8_C(0x0b0),		                /* page adr  */
  
  
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

uint8_t ngfx_u8x8_d_uc1608_240x128(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( ngfx_u8x8_d_uc1608_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
	ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_uc1608_240x128_display_info);
	break;
      case NGFX_U8X8_MSG_DISPLAY_INIT:
	ngfx_u8x8_d_helper_display_init(u8x8);
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_uc1608_240x128_init_seq);
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}

/*================================================*/
/* experimental implementation for the uc1608 erc240x120 */

static const ngfx_u8x8_display_info_t ngfx_u8x8_uc1608_erc240120_display_info =
{
  /* chip_enable_level = */ 1,	/* uc1608 has high active CS */
  /* chip_disable_level = */ 0,
  
  /* post_chip_enable_wait_ns = */ 10,	/* uc1608 datasheet, page 39, actually 0 */
  /* pre_chip_disable_wait_ns = */ 20,	/* uc1608 datasheet, page 39 */
  /* reset_pulse_width_ms = */ 1, 	/* uc1608 datasheet, page 42 */
  /* post_reset_wait_ms = */ 10, 	
  /* sda_setup_time_ns = */ 30,		/* uc1608 datasheet, page 41 */
  /* sck_pulse_width_ns = */ 65,	/* half of cycle time  */
  /* sck_clock_hz = */ 8000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 30,	/* uc1608 datasheet, page 39 */
  /* write_pulse_width_ns = */ 35,	/* uc1608 datasheet, page 39 */
  /* tile_width = */ 30,		/* width of 30*8=240 pixel */
  /* tile_height = */ 15,
  /* default_x_offset = */ 1,	/* reused as y page offset */
  /* flipmode_x_offset = */ 0,	/* reused as y page offset */
  /* pixel_width = */ 240,
  /* pixel_height = */ 120
};

/* http://www.buydisplay.com/download/democode/ERC240120-1_DemoCode.txt */
static const uint8_t ngfx_u8x8_d_uc1608_erc240120_init_seq[] = {
    
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  NGFX_U8X8_C(0x0e2),            			/* soft reset */
  NGFX_U8X8_DLY(200),

  //NGFX_U8X8_C(0x023),            			/* Bit 0/1: Temp compenstation, Bit 2: Multiplex Rate 0=96, 1=128 */
  NGFX_U8X8_C(0x026),            			/* Bit 0/1: Temp compenstation, Bit 2: Multiplex Rate 0=96, 1=128 */

  NGFX_U8X8_C(0x0c8),            			/* Map control, Bit 3: MY=1, Bit 2: MX=0, Bit 0: MSF =0 */
  NGFX_U8X8_C(0x0ea),            			/* LCD bias Bits 0/1: 00=10.7 01=10.3, 10=12.0, 11=12.7 */
							/* according to DemoCode.txt */
  
  NGFX_U8X8_C(0x02f),            			/* power on, Bit 2 PC2=1 (internal charge pump), Bits 0/1: cap of panel */
  NGFX_U8X8_DLY(50),
  
  NGFX_U8X8_C(0x040),            			/* set display start line to 0 */
  NGFX_U8X8_C(0x090),            			/* no fixed lines */
  NGFX_U8X8_C(0x089),            			/* RAM access control */
  
  //NGFX_U8X8_CA(0x081, 46),			/* set contrast, 46 according to  DemoCode.txt */
  NGFX_U8X8_CA(0x081, 80),			/* */
  
  NGFX_U8X8_C(0x000),		                /* column low nibble */
  NGFX_U8X8_C(0x010),		                /* column high nibble */  
  NGFX_U8X8_C(0x0b0),		                /* page adr  */
  
  
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

uint8_t ngfx_u8x8_d_uc1608_erc240120(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( ngfx_u8x8_d_uc1608_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
	ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_uc1608_erc240120_display_info);
	break;
      case NGFX_U8X8_MSG_DISPLAY_INIT:
	ngfx_u8x8_d_helper_display_init(u8x8);
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_uc1608_erc240120_init_seq);
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}

/*================================================*/
/* DEM 240064, issue 1164 */


static const ngfx_u8x8_display_info_t ngfx_u8x8_uc1608_dem240064_display_info =
{
  /* chip_enable_level = */ 1,	/* uc1608 has high active CS */
  /* chip_disable_level = */ 0,
  
  /* post_chip_enable_wait_ns = */ 10,	/* uc1608 datasheet, page 39, actually 0 */
  /* pre_chip_disable_wait_ns = */ 20,	/* uc1608 datasheet, page 39 */
  /* reset_pulse_width_ms = */ 1, 	/* uc1608 datasheet, page 42 */
  /* post_reset_wait_ms = */ 10, 	
  /* sda_setup_time_ns = */ 30,		/* uc1608 datasheet, page 41 */
  /* sck_pulse_width_ns = */ 65,	/* half of cycle time  */
  /* sck_clock_hz = */ 8000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 30,	/* uc1608 datasheet, page 39 */
  /* write_pulse_width_ns = */ 35,	/* uc1608 datasheet, page 39 */
  /* tile_width = */ 30,		/* width of 30*8=240 pixel */
  /* tile_height = */ 8,
  /* default_x_offset = */ 1,	/* reused as y page offset */
  /* flipmode_x_offset = */ 0,	/* reused as y page offset */
  /* pixel_width = */ 240,
  /* pixel_height = */ 64
};

/* http://www.buydisplay.com/download/democode/ERC240120-1_DemoCode.txt */
static const uint8_t ngfx_u8x8_d_uc1608_dem240064_init_seq[] = {
    
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  NGFX_U8X8_C(0x0e2),            			/* soft reset */
  NGFX_U8X8_DLY(200),

  //NGFX_U8X8_C(0x023),            			/* Bit 0/1: Temp compenstation, Bit 2: Multiplex Rate 0=96, 1=128 */
  NGFX_U8X8_C(0x026),            			/* Bit 0/1: Temp compenstation, Bit 2: Multiplex Rate 0=96, 1=128 */

  NGFX_U8X8_C(0x0c8),            			/* Map control, Bit 3: MY=1, Bit 2: MX=0, Bit 0: MSF =0 */
  NGFX_U8X8_C(0x0ea),            			/* LCD bias Bits 0/1: 00=10.7 01=10.3, 10=12.0, 11=12.7 */
							/* according to DemoCode.txt */
  
  NGFX_U8X8_C(0x02f),            			/* power on, Bit 2 PC2=1 (internal charge pump), Bits 0/1: cap of panel */
  NGFX_U8X8_DLY(50),
  
  NGFX_U8X8_C(0x07f),            			/* set display start line*/
  NGFX_U8X8_C(0x094),            			/* fixed lines */
  NGFX_U8X8_C(0x089),            			/* RAM access control */
  
  NGFX_U8X8_CA(0x081, 160),			/* issue 1164 */
  
  NGFX_U8X8_C(0x000),		                /* column low nibble */
  NGFX_U8X8_C(0x010),		                /* column high nibble */  
  NGFX_U8X8_C(0x0b0),		                /* page adr  */
  
  
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_uc1608_dem240064_flip0_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0c8),            			/* LCD Mapping */
  NGFX_U8X8_C(0x07f),            			/* set display start line*/
  NGFX_U8X8_C(0x094),            			/* fixed lines */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};

static const uint8_t ngfx_u8x8_d_uc1608_dem240064_flip1_seq[] = {
  NGFX_U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  NGFX_U8X8_C(0x0c4),            			/* LCD Mapping */
  NGFX_U8X8_C(0x040),            			/* set display start line*/
  NGFX_U8X8_C(0x090),            			/* fixed lines */
  NGFX_U8X8_END_TRANSFER(),             	/* disable chip */
  NGFX_U8X8_END()             			/* end of sequence */
};


uint8_t ngfx_u8x8_d_uc1608_dem240064(ngfx_u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  
  /* checking for the flip mode cmd first */
  if ( msg == NGFX_U8X8_MSG_DISPLAY_SET_FLIP_MODE )
  {
      if ( arg_int == 0 )
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_uc1608_dem240064_flip0_seq);
	u8x8->x_offset = u8x8->display_info->default_x_offset;
      }
      else
      {
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_uc1608_dem240064_flip1_seq);
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
      }	
      return 1;
  }
  /* call the common procedure, this now leads to the effect, that the flip code is executed again */
  /* maybe we should paste the common code here to avoid this */
  
  
  if ( ngfx_u8x8_d_uc1608_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case NGFX_U8X8_MSG_DISPLAY_SETUP_MEMORY:
	ngfx_u8x8_d_helper_display_setup_memory(u8x8, &ngfx_u8x8_uc1608_dem240064_display_info);
	break;
      case NGFX_U8X8_MSG_DISPLAY_INIT:
	ngfx_u8x8_d_helper_display_init(u8x8);
	ngfx_u8x8_cad_SendSequence(u8x8, ngfx_u8x8_d_uc1608_dem240064_init_seq);
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}
