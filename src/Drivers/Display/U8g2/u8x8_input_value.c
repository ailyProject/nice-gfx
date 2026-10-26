/*

  ngfx_u8x8_input_value.c
  
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

/*
  return:
    0: value is not changed (HOME/Break Button pressed)
    1: value has been updated
*/

uint8_t ngfx_u8x8_UserInterfaceInputValue(ngfx_u8x8_t *u8x8, const char *title, const char *pre, uint8_t *value, uint8_t lo, uint8_t hi, uint8_t digits, const char *post)
{
  uint8_t height;
  uint8_t y;
  uint8_t width;
  uint8_t x;
  uint8_t local_value = *value;
  uint8_t r;
  uint8_t event;

  /* calculate overall height of the input value box */
  height = 1;	/* button line */
  height += ngfx_u8x8_GetStringLineCnt(title);
  
  /* calculate offset from top */
  y = 0;
  if ( height < ngfx_u8x8_GetRows(u8x8)  )
  {
    y = ngfx_u8x8_GetRows(u8x8);
    y -= height;
    y /= 2;
  }
  
  /* calculate offset from left for the label */
  x = 0;
  width = ngfx_u8x8_GetUTF8Len(u8x8, pre);
  width += digits;
  width += ngfx_u8x8_GetUTF8Len(u8x8, post);
  if ( width < ngfx_u8x8_GetCols(u8x8) )
  {
    x = ngfx_u8x8_GetCols(u8x8);
    x -= width;
    x /= 2;
  }
  
  /* render */
  ngfx_u8x8_ClearDisplay(u8x8);   /* required, because not everything is filled */
  ngfx_u8x8_SetInverseFont(u8x8, 0);  
  y += ngfx_u8x8_DrawUTF8Lines(u8x8, 0, y, ngfx_u8x8_GetCols(u8x8), title);
  x += ngfx_u8x8_DrawUTF8(u8x8, x, y, pre);
  ngfx_u8x8_DrawUTF8(u8x8, x+digits, y, post);
  ngfx_u8x8_SetInverseFont(u8x8, 1);
  
  /* event loop */
  ngfx_u8x8_DrawUTF8(u8x8, x, y, ngfx_u8x8_u8toa(local_value, digits));
  for(;;)
  {
    event = ngfx_u8x8_GetMenuEvent(u8x8);
    if ( event == NGFX_U8X8_MSG_GPIO_MENU_SELECT )
    {
      *value = local_value;
      r = 1;
      break;
    }
    else if ( event == NGFX_U8X8_MSG_GPIO_MENU_HOME )
    {
      r = 0;
      break;
    }
    else if ( event == NGFX_U8X8_MSG_GPIO_MENU_NEXT || event == NGFX_U8X8_MSG_GPIO_MENU_UP )
    {
      if ( local_value >= hi )
	local_value = lo;
      else
	local_value++;
      ngfx_u8x8_DrawUTF8(u8x8, x, y, ngfx_u8x8_u8toa(local_value, digits));
    }
    else if ( event == NGFX_U8X8_MSG_GPIO_MENU_PREV || event == NGFX_U8X8_MSG_GPIO_MENU_DOWN )
    {
      if ( local_value <= lo )
	local_value = hi;
      else
	local_value--;
      ngfx_u8x8_DrawUTF8(u8x8, x, y, ngfx_u8x8_u8toa(local_value, digits));
    }        
  }
  
  ngfx_u8x8_SetInverseFont(u8x8, 0);
  return r;  
}
