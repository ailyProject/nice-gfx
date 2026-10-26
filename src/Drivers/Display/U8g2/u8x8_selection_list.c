/*

  ngfx_u8x8_selection_list.c
  
  selection list with scroll option
  
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
  increase the cursor position
*/
void ngfx_u8sl_Next(ngfx_u8sl_t *u8sl)
{
  u8sl->current_pos++;
  if ( u8sl->current_pos >= u8sl->total )
  {
    u8sl->current_pos = 0;
    u8sl->first_pos = 0;
  }
  else
  {
    if ( u8sl->first_pos + u8sl->visible <= u8sl->current_pos + 1 )
    {
      u8sl->first_pos = u8sl->current_pos - u8sl->visible + 1;
    }
  }
}

void ngfx_u8sl_Prev(ngfx_u8sl_t *u8sl)
{
  if ( u8sl->current_pos == 0 )
  {
    u8sl->current_pos = u8sl->total - 1;
    u8sl->first_pos = 0;
    if ( u8sl->total > u8sl->visible )
      u8sl->first_pos = u8sl->total - u8sl->visible;
  }
  else
  {
    u8sl->current_pos--;
    if ( u8sl->first_pos > u8sl->current_pos )
      u8sl->first_pos = u8sl->current_pos;
  }
}

void ngfx_u8x8_DrawSelectionList(ngfx_u8x8_t *u8x8, ngfx_u8sl_t *u8sl, ngfx_u8x8_sl_cb sl_cb, const void *aux)
{
  uint8_t i;
  for( i = 0; i < u8sl->visible; i++ )
  {
    sl_cb(u8x8, u8sl, i+u8sl->first_pos, aux);
  }
}

/* selection list with string line */
void ngfx_u8x8_sl_string_line_cb(ngfx_u8x8_t *u8x8, ngfx_u8sl_t *u8sl, uint8_t idx, const void *aux)
{
  const char *s;
  uint8_t row;
  /* calculate offset from display upper border */
  row = u8sl->y;
  
  /* calculate target pos */
  row += idx;
  row -= u8sl->first_pos;
  
  /* check whether this is the current cursor line */
  if ( idx == u8sl->current_pos )
    ngfx_u8x8_SetInverseFont(u8x8, 1);
  else
    ngfx_u8x8_SetInverseFont(u8x8, 0);
  
  /* get the line from the array */
  s = ngfx_u8x8_GetStringLineStart(idx, (const char *)aux);
  
  /* draw the line */
  if ( s == NULL )
    s = "";
  ngfx_u8x8_DrawUTF8Line(u8x8, u8sl->x, row, ngfx_u8x8_GetCols(u8x8), s);  
  ngfx_u8x8_SetInverseFont(u8x8, 0);
}

/*
  title: 		NULL for no title, valid str for title line. Can contain mutliple lines, separated by '\n'
  start_pos: 	default position for the cursor (starts with 1)
  sl:			string list (list of strings separated by \n)
  returns 0 if user has pressed the home key
  returns the selected line+1 if user has pressed the select key (e.g. 1 for the first line)
*/
uint8_t ngfx_u8x8_UserInterfaceSelectionList(ngfx_u8x8_t *u8x8, const char *title, uint8_t start_pos, const char *sl)
{
  ngfx_u8sl_t u8sl;
  uint8_t event;
  uint8_t title_lines;
  
  if ( start_pos > 0 )
    start_pos--;
  
  u8sl.visible = ngfx_u8x8_GetRows(u8x8);
  u8sl.total = ngfx_u8x8_GetStringLineCnt(sl);
  u8sl.first_pos = 0;
  u8sl.current_pos = start_pos;
  u8sl.x = 0;
  u8sl.y = 0;
  

  //ngfx_u8x8_ClearDisplay(u8x8);   /* not required because all is 100% filled */
  ngfx_u8x8_SetInverseFont(u8x8, 0);
  
  if ( title != NULL )
  {
    title_lines = ngfx_u8x8_DrawUTF8Lines(u8x8, u8sl.x, u8sl.y, ngfx_u8x8_GetCols(u8x8), title);
    u8sl.y+=title_lines;
    u8sl.visible-=title_lines;
  }
  
  if ( u8sl.current_pos >= u8sl.total )
    u8sl.current_pos = u8sl.total-1;

  
  ngfx_u8x8_DrawSelectionList(u8x8, &u8sl, ngfx_u8x8_sl_string_line_cb, sl);

  for(;;)
  {
    event = ngfx_u8x8_GetMenuEvent(u8x8);
    if ( event == NGFX_U8X8_MSG_GPIO_MENU_SELECT )
      return u8sl.current_pos+1;
    else if ( event == NGFX_U8X8_MSG_GPIO_MENU_HOME )
      return 0;
    else if ( event == NGFX_U8X8_MSG_GPIO_MENU_NEXT || event == NGFX_U8X8_MSG_GPIO_MENU_DOWN )
    {
      ngfx_u8sl_Next(&u8sl);
      ngfx_u8x8_DrawSelectionList(u8x8, &u8sl, ngfx_u8x8_sl_string_line_cb, sl);      
    }
    else if ( event == NGFX_U8X8_MSG_GPIO_MENU_PREV || event == NGFX_U8X8_MSG_GPIO_MENU_UP  )
    {
      ngfx_u8sl_Prev(&u8sl);
      ngfx_u8x8_DrawSelectionList(u8x8, &u8sl, ngfx_u8x8_sl_string_line_cb, sl);      
    }
  }
}

