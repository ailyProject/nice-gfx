/***************************************************************************************
** Function name:           drawLine
** Description:             draw a line between 2 arbitrary points
***************************************************************************************/
// Bresenham's algorithm - thx Wikipedia - speed enhanced by Bodmer to use
// an efficient FastH/V Line draw routine for line segments of 2 pixels or more
void NICE_GFX::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
  if (_vpOoB) return;
  WriteScope write(*this);
  nice_gfx::rasterLine(x0,y0,x1,y1,_xDatum,_yDatum,_vpX,_vpY,_vpW,_vpH,
    [this,color](int32_t x,int32_t y,int32_t w,int32_t h) {
      // The visible coordinates still lie between the original endpoints.
      x=int32_t(int64_t(x)-_xDatum);y=int32_t(int64_t(y)-_yDatum);
      if (h==1) drawFastHLine(x,y,w,color);
      else drawFastVLine(x,y,h,color);
    });
}


/***************************************************************************************
** Description:  Constants for anti-aliased line drawing on TFT and in Sprites
***************************************************************************************/
constexpr float PixelAlphaGain   = 255.0;
constexpr float LoAlphaTheshold  = 1.0/32.0;
constexpr float HiAlphaTheshold  = 1.0 - LoAlphaTheshold;
constexpr float deg2rad      = 3.14159265359/180.0;

/***************************************************************************************
** Function name:           drawPixel (alpha blended)
** Description:             Draw a pixel blended with the screen or bg pixel colour
***************************************************************************************/
uint16_t NICE_GFX::drawPixel(int32_t x, int32_t y, uint32_t color, uint8_t alpha, uint32_t bg_color)
{
  if (bg_color == 0x00FFFFFF) bg_color = readPixel(x, y);
  color = fastBlend(alpha, color, bg_color);
  drawPixel(x, y, color);
  return color;
}


/***************************************************************************************
** Function name:           drawSmoothArc
** Description:             Draw a smooth arc clockwise from 6 o'clock
***************************************************************************************/
void NICE_GFX::drawSmoothArc(int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle, uint32_t fg_color, uint32_t bg_color, bool roundEnds)
// Centre at x,y
// r = arc outer radius, ir = arc inner radius. Inclusive so arc thickness = r - ir + 1
// Angles in range 0-360
// Arc foreground colour anti-aliased with background colour at edges
// anti-aliased roundEnd is optional, default is anti-aliased straight end
// Note: rounded ends extend the arc angle so can overlap, user sketch to manage this.
{
  WriteScope write(*this);

  if (endAngle != startAngle && (startAngle != 0 || endAngle != 360))
  {
    float sx = -sinf(startAngle * deg2rad);
    float sy = +cosf(startAngle * deg2rad);
    float ex = -sinf(  endAngle * deg2rad);
    float ey = +cosf(  endAngle * deg2rad);

    if (roundEnds)
    { // Round ends
      sx = sx * (r + ir)/2.0 + x;
      sy = sy * (r + ir)/2.0 + y;
      drawSpot(sx, sy, (r - ir)/2.0, fg_color, bg_color);

      ex = ex * (r + ir)/2.0 + x;
      ey = ey * (r + ir)/2.0 + y;
      drawSpot(ex, ey, (r - ir)/2.0, fg_color, bg_color);
    }
    else
    { // Square ends
      float asx = sx * ir + x;
      float asy = sy * ir + y;
      float aex = sx *  r + x;
      float aey = sy *  r + y;
      drawWedgeLine(asx, asy, aex, aey, 0.3, 0.3, fg_color, bg_color);

      asx = ex * ir + x;
      asy = ey * ir + y;
      aex = ex *  r + x;
      aey = ey *  r + y;
      drawWedgeLine(asx, asy, aex, aey, 0.3, 0.3, fg_color, bg_color);
    }

    // Draw arc
    drawArc(x, y, r, ir, startAngle, endAngle, fg_color, bg_color);

  }
  else // Draw full 360
  {
    drawArc(x, y, r, ir, 0, 360, fg_color, bg_color);
  }


}

/***************************************************************************************
** Function name:           sqrt_fraction (private function)
** Description:             Smooth graphics support function for alpha derivation
***************************************************************************************/
// Compute the fixed point square root of an integer and
// return the 8 MS bits of fractional part.
// Quicker than sqrt() for processors that do not have an FPU (e.g. RP2040)
inline uint8_t NICE_GFX::sqrt_fraction(uint32_t num) {
  if (num > (0x40000000)) return 0;
  uint32_t bsh = 0x00004000;
  uint32_t fpr = 0;
  uint32_t osh = 0;

  // Auto adjust from U8:8 up to U15:16
  while (num>bsh) {bsh <<= 2; osh++;}

  do {
    uint32_t bod = bsh + fpr;
    if(num >= bod)
    {
      num -= bod;
      fpr = bsh + bod;
    }
    num <<= 1;
  } while(bsh >>= 1);

  return fpr>>osh;
}

/***************************************************************************************
** Function name:           drawArc
** Description:             Draw an arc clockwise from 6 o'clock position
***************************************************************************************/
// Centre at x,y
// r = arc outer radius, ir = arc inner radius. Inclusive, so arc thickness = r-ir+1
// Angles MUST be in range 0-360
// Arc foreground fg_color anti-aliased with background colour along sides
// smooth is optional, default is true, smooth=false means no antialiasing
// Note: Arc ends are not anti-aliased (use drawSmoothArc instead for that)
void NICE_GFX::drawArc(int32_t x, int32_t y, int32_t r, int32_t ir,
                       uint32_t startAngle, uint32_t endAngle,
                       uint32_t fg_color, uint32_t bg_color,
                       bool smooth)
{
  if (endAngle   > 360)   endAngle = 360;
  if (startAngle > 360) startAngle = 360;
  if (_vpOoB || startAngle == endAngle) return;
  if (r < ir) transpose(r, ir);  // Required that r > ir
  if (r <= 0 || ir < 0) return;  // Invalid r, ir can be zero (circle sector)

  if (endAngle < startAngle) {
    // Arc sweeps through 6 o'clock so draw in two parts
    if (startAngle < 360) drawArc(x, y, r, ir, startAngle, 360, fg_color, bg_color, smooth);
    if (endAngle == 0) return;
    startAngle = 0;
  }
  WriteScope write(*this);

  int32_t xs = 0;        // x start position for quadrant scan
  uint8_t alpha = 0;     // alpha value for blending pixels

  uint32_t r2 = r * r;   // Outer arc radius^2
  if (smooth) r++;       // Outer AA zone radius
  uint32_t r1 = r * r;   // Outer AA radius^2
  int16_t w  = r - ir;   // Width of arc (r - ir + 1)
  uint32_t r3 = ir * ir; // Inner arc radius^2
  if (smooth) ir--;      // Inner AA zone radius
  uint32_t r4 = ir * ir; // Inner AA radius^2

  //     1 | 2
  //    ---¦---    Arc quadrant index
  //     0 | 3
  // Fixed point U16.16 slope table for arc start/end in each quadrant
  uint32_t startSlope[4] = {0, 0, 0xFFFFFFFF, 0};
  uint32_t   endSlope[4] = {0, 0xFFFFFFFF, 0, 0};

  // Ensure maximum U16.16 slope of arc ends is ~ 0x8000 0000
  constexpr float minDivisor = 1.0f/0x8000;

  // Fill in start slope table and empty quadrants
  float fabscos = fabsf(cosf(startAngle * deg2rad));
  float fabssin = fabsf(sinf(startAngle * deg2rad));

  // U16.16 slope of arc start
  uint32_t slope = (fabscos/(fabssin + minDivisor)) * (float)(1UL<<16);

  // Update slope table, add slope for arc start
  if (startAngle <= 90) {
    startSlope[0] =  slope;
  }
  else if (startAngle <= 180) {
    startSlope[1] =  slope;
  }
  else if (startAngle <= 270) {
    startSlope[1] = 0xFFFFFFFF;
    startSlope[2] = slope;
  }
  else {
    startSlope[1] = 0xFFFFFFFF;
    startSlope[2] =  0;
    startSlope[3] = slope;
  }

  // Fill in end slope table and empty quadrants
  fabscos  = fabsf(cosf(endAngle * deg2rad));
  fabssin  = fabsf(sinf(endAngle * deg2rad));

  // U16.16 slope of arc end
  slope   = (uint32_t)((fabscos/(fabssin + minDivisor)) * (float)(1UL<<16));

  // Work out which quadrants will need to be drawn and add slope for arc end
  if (endAngle <= 90) {
    endSlope[0] = slope;
    endSlope[1] =  0;
    startSlope[2] =  0;
  }
  else if (endAngle <= 180) {
    endSlope[1] = slope;
    startSlope[2] =  0;
  }
  else if (endAngle <= 270) {
    endSlope[2] =  slope;
  }
  else {
    endSlope[3] =  slope;
  }

  // Scan quadrant
  for (int32_t cy = r - 1; cy > 0; cy--)
  {
    // Partial-buffer refresh often intersects only a few scanlines of a gauge.
    // Keep the edge tracker lazy: the next visible row advances xs as needed.
    const int64_t top = int64_t(y) + _yDatum + cy - r;
    const int64_t bottom = int64_t(y) + _yDatum - cy + r;
    if ((top < _vpY || top >= _vpH) && (bottom < _vpY || bottom >= _vpH)) continue;
    uint32_t len[4] = { 0,  0,  0,  0}; // Pixel run length
    int32_t  xst[4] = {-1, -1, -1, -1}; // Pixel run x start
    uint32_t dy2 = (r - cy) * (r - cy);

    // Find and track arc zone start point
    while ((r - xs) * (r - xs) + dy2 >= r1) xs++;

    for (int32_t cx = xs; cx < r; cx++)
    {
      // Calculate radius^2
      uint32_t hyp = (r - cx) * (r - cx) + dy2;

      // If in outer zone calculate alpha
      if (hyp > r2) {
        alpha = ~sqrt_fraction(hyp); // Outer AA zone
      }
      // If within arc fill zone, get line start and lengths for each quadrant
      else if (hyp >= r3) {
        // Calculate U16.16 slope
        slope = ((r - cy) << 16)/(r - cx);
        if (slope <= startSlope[0] && slope >= endSlope[0]) { // slope hi -> lo
          xst[0] = cx; // Bottom left line end
          len[0]++;
        }
        if (slope >= startSlope[1] && slope <= endSlope[1]) { // slope lo -> hi
          xst[1] = cx; // Top left line end
          len[1]++;
        }
        if (slope <= startSlope[2] && slope >= endSlope[2]) { // slope hi -> lo
          xst[2] = cx; // Bottom right line start
          len[2]++;
        }
        if (slope <= endSlope[3] && slope >= startSlope[3]) { // slope lo -> hi
          xst[3] = cx; // Top right line start
          len[3]++;
        }
        continue; // Next x
      }
      else {
        if (hyp <= r4) break;  // Skip inner pixels
        alpha = sqrt_fraction(hyp); // Inner AA zone
      }

      if (alpha < 16) continue;  // Skip low alpha pixels

      // If background is read it must be done in each quadrant
      uint16_t pcol = fastBlend(alpha, fg_color, bg_color);
      // Check if an AA pixels need to be drawn
      slope = ((r - cy)<<16)/(r - cx);
      if (slope <= startSlope[0] && slope >= endSlope[0]) // BL
        drawPixel(x + cx - r, y - cy + r, pcol);
      if (slope >= startSlope[1] && slope <= endSlope[1]) // TL
        drawPixel(x + cx - r, y + cy - r, pcol);
      if (slope <= startSlope[2] && slope >= endSlope[2]) // TR
        drawPixel(x - cx + r, y + cy - r, pcol);
      if (slope <= endSlope[3] && slope >= startSlope[3]) // BR
        drawPixel(x - cx + r, y - cy + r, pcol);
    }
    // Add line in inner zone
    if (len[0]) drawFastHLine(x + xst[0] - len[0] + 1 - r, y - cy + r, len[0], fg_color); // BL
    if (len[1]) drawFastHLine(x + xst[1] - len[1] + 1 - r, y + cy - r, len[1], fg_color); // TL
    if (len[2]) drawFastHLine(x - xst[2] + r, y + cy - r, len[2], fg_color); // TR
    if (len[3]) drawFastHLine(x - xst[3] + r, y - cy + r, len[3], fg_color); // BR
  }

  // Fill in centre lines
  if (startAngle ==   0 || endAngle == 360) drawFastVLine(x, y + r - w, w, fg_color); // Bottom
  if (startAngle <=  90 && endAngle >=  90) drawFastHLine(x - r + 1, y, w, fg_color); // Left
  if (startAngle <= 180 && endAngle >= 180) drawFastVLine(x, y - r + 1, w, fg_color); // Top
  if (startAngle <= 270 && endAngle >= 270) drawFastHLine(x + r - w, y, w, fg_color); // Right


}

/***************************************************************************************
** Function name:           drawSmoothCircle
** Description:             Draw a smooth circle
***************************************************************************************/
// To have effective anti-aliasing the circle will be 3 pixels thick
void NICE_GFX::drawSmoothCircle(int32_t x, int32_t y, int32_t r, uint32_t fg_color, uint32_t bg_color)
{
  drawSmoothRoundRect(x-r, y-r, r, r-1, 0, 0, fg_color, bg_color);
}

/***************************************************************************************
** Function name:           fillSmoothCircle
** Description:             Draw a filled anti-aliased circle
***************************************************************************************/
void NICE_GFX::fillSmoothCircle(int32_t x, int32_t y, int32_t r, uint32_t color, uint32_t bg_color)
{
  if (r <= 0) return;

  WriteScope write(*this);

  drawFastHLine(x - r, y, 2 * r + 1, color);
  int32_t xs = 1;
  int32_t cx = 0;

  int32_t r1 = r * r;
  r++;
  int32_t r2 = r * r;
  
  for (int32_t cy = r - 1; cy > 0; cy--)
  {
    int32_t dy2 = (r - cy) * (r - cy);
    for (cx = xs; cx < r; cx++)
    {
      int32_t hyp2 = (r - cx) * (r - cx) + dy2;
      if (hyp2 <= r1) break;
      if (hyp2 >= r2) continue;

      uint8_t alpha = ~sqrt_fraction(hyp2);
      if (alpha > 246) break;
      xs = cx;
      if (alpha < 9) continue;

      if (bg_color == 0x00FFFFFF) {
        drawPixel(x + cx - r, y + cy - r, color, alpha, bg_color);
        drawPixel(x - cx + r, y + cy - r, color, alpha, bg_color);
        drawPixel(x - cx + r, y - cy + r, color, alpha, bg_color);
        drawPixel(x + cx - r, y - cy + r, color, alpha, bg_color);
      }
      else {
        uint16_t pcol = drawPixel(x + cx - r, y + cy - r, color, alpha, bg_color);
        drawPixel(x - cx + r, y + cy - r, pcol);
        drawPixel(x - cx + r, y - cy + r, pcol);
        drawPixel(x + cx - r, y - cy + r, pcol);
      }
    }
    drawFastHLine(x + cx - r, y + cy - r, 2 * (r - cx) + 1, color);
    drawFastHLine(x + cx - r, y - cy + r, 2 * (r - cx) + 1, color);
  }

}


/***************************************************************************************
** Function name:           drawSmoothRoundRect
** Description:             Draw a rounded rectangle
***************************************************************************************/
// x,y is top left corner of bounding box for a complete rounded rectangle
// r = arc outer corner radius, ir = arc inner radius. Arc thickness = r-ir+1
// w and h are width and height of the bounding rectangle
// If w and h are < radius (e.g. 0,0) a circle will be drawn with centre at x+r,y+r
// Arc foreground fg_color anti-aliased with background colour at edges
// A subset of corners can be drawn by specifying a quadrants mask. A bit set in the
// mask means draw that quadrant (all are drawn if parameter missing):
//   0x1 | 0x2
//    ---¦---    Arc quadrant mask select bits (as in drawCircleHelper fn)
//   0x8 | 0x4
void NICE_GFX::drawSmoothRoundRect(int32_t x, int32_t y, int32_t r, int32_t ir, int32_t w, int32_t h, uint32_t fg_color, uint32_t bg_color, uint8_t quadrants)
{
  if (_vpOoB) return;
  if (r < ir) transpose(r, ir); // Required that r > ir
  if (r <= 0 || ir < 0) return; // Invalid

  w -= 2*r;
  h -= 2*r;

  if (w < 0) w = 0;
  if (h < 0) h = 0;

  WriteScope write(*this);

  x += r;
  y += r;

  uint16_t t = r - ir + 1;
  int32_t xs = 0;
  int32_t cx = 0;

  int32_t r2 = r * r;   // Outer arc radius^2
  r++;
  int32_t r1 = r * r;   // Outer AA zone radius^2

  int32_t r3 = ir * ir; // Inner arc radius^2
  ir--;
  int32_t r4 = ir * ir; // Inner AA zone radius^2

  uint8_t alpha = 0;

  // Scan top left quadrant x y r ir fg_color  bg_color
  for (int32_t cy = r - 1; cy > 0; cy--)
  {
    int32_t len = 0;  // Pixel run length
    int32_t lxst = 0; // Left side run x start
    int32_t rxst = 0; // Right side run x start
    int32_t dy2 = (r - cy) * (r - cy);

    // Find and track arc zone start point
    while ((r - xs) * (r - xs) + dy2 >= r1) xs++;

    for (cx = xs; cx < r; cx++)
    {
      // Calculate radius^2
      int32_t hyp = (r - cx) * (r - cx) + dy2;

      // If in outer zone calculate alpha
      if (hyp > r2) {
        alpha = ~sqrt_fraction(hyp); // Outer AA zone
      }
      // If within arc fill zone, get line lengths for each quadrant
      else if (hyp >= r3) {
        rxst = cx; // Right side start
        len++;     // Line segment length
        continue;  // Next x
      }
      else {
        if (hyp <= r4) break;  // Skip inner pixels
        alpha = sqrt_fraction(hyp); // Inner AA zone
      }

      if (alpha < 16) continue;  // Skip low alpha pixels

      if (bg_color == 0x00FFFFFF) {
        // Corners can cover different background pixels, including in Sprites.
        const auto blendPixel = [&](int32_t px, int32_t py) {
          drawPixel(px, py, fastBlend(alpha, fg_color, readPixel(px, py)));
        };
        if (quadrants & 0x8) blendPixel(x + cx - r, y - cy + r + h);
        if (quadrants & 0x1) blendPixel(x + cx - r, y + cy - r);
        if (quadrants & 0x2) blendPixel(x - cx + r + w, y + cy - r);
        if (quadrants & 0x4) blendPixel(x - cx + r + w, y - cy + r + h);
      } else {
        const uint16_t pcol = fastBlend(alpha, fg_color, bg_color);
        if (quadrants & 0x8) drawPixel(x + cx - r, y - cy + r + h, pcol);
        if (quadrants & 0x1) drawPixel(x + cx - r, y + cy - r, pcol);
        if (quadrants & 0x2) drawPixel(x - cx + r + w, y + cy - r, pcol);
        if (quadrants & 0x4) drawPixel(x - cx + r + w, y - cy + r + h, pcol);
      }
    }
    // Fill arc inner zone in each quadrant
    lxst = rxst - len + 1; // Calculate line segment start for left side
    if (quadrants & 0x8) drawFastHLine(x + lxst - r, y - cy + r + h, len, fg_color);     // BL
    if (quadrants & 0x1) drawFastHLine(x + lxst - r, y + cy - r, len, fg_color);         // TL
    if (quadrants & 0x2) drawFastHLine(x - rxst + r + w, y + cy - r, len, fg_color);     // TR
    if (quadrants & 0x4) drawFastHLine(x - rxst + r + w, y - cy + r + h, len, fg_color); // BR
  }

  // Draw sides
  if ((quadrants & 0xC) == 0xC) fillRect(x, y + r - t + h, w + 1, t, fg_color); // Bottom
  if ((quadrants & 0x9) == 0x9) fillRect(x - r + 1, y, t, h + 1, fg_color);     // Left
  if ((quadrants & 0x3) == 0x3) fillRect(x, y - r + 1, w + 1, t, fg_color);     // Top
  if ((quadrants & 0x6) == 0x6) fillRect(x + r - t + w, y, t, h + 1, fg_color); // Right


}

/***************************************************************************************
** Function name:           fillSmoothRoundRect
** Description:             Draw a filled anti-aliased rounded corner rectangle
***************************************************************************************/
void NICE_GFX::fillSmoothRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color, uint32_t bg_color)
{
  WriteScope write(*this);

  int32_t xs = 0;
  int32_t cx = 0;

  // Limit radius to half width or height
  if (r < 0)   r = 0;
  if (r > w/2) r = w/2;
  if (r > h/2) r = h/2;

  y += r;
  h -= 2*r;
  fillRect(x, y, w, h, color);

  h--;
  x += r;
  w -= 2*r+1;

  int32_t r1 = r * r;
  r++;
  int32_t r2 = r * r;

  for (int32_t cy = r - 1; cy > 0; cy--)
  {
    int32_t dy2 = (r - cy) * (r - cy);
    for (cx = xs; cx < r; cx++)
    {
      int32_t hyp2 = (r - cx) * (r - cx) + dy2;
      if (hyp2 <= r1) break;
      if (hyp2 >= r2) continue;

      uint8_t alpha = ~sqrt_fraction(hyp2);
      if (alpha > 246) break;
      xs = cx;
      if (alpha < 9) continue;

      drawPixel(x + cx - r, y + cy - r, color, alpha, bg_color);
      drawPixel(x - cx + r + w, y + cy - r, color, alpha, bg_color);
      drawPixel(x - cx + r + w, y - cy + r + h, color, alpha, bg_color);
      drawPixel(x + cx - r, y - cy + r + h, color, alpha, bg_color);
    }
    drawFastHLine(x + cx - r, y + cy - r, 2 * (r - cx) + 1 + w, color);
    drawFastHLine(x + cx - r, y - cy + r + h, 2 * (r - cx) + 1 + w, color);
  }

}

/***************************************************************************************
** Function name:           drawSpot - maths intensive, so for small filled circles
** Description:             Draw an anti-aliased filled circle at ax,ay with radius r
***************************************************************************************/
// Coordinates are floating point to achieve sub-pixel positioning
void NICE_GFX::drawSpot(float ax, float ay, float r, uint32_t fg_color, uint32_t bg_color)
{
  // Filled circle can be created by the wide line function with zero line length
  drawWedgeLine( ax, ay, ax, ay, r, r, fg_color, bg_color);
}

/***************************************************************************************
** Function name:           drawWideLine - background colour specified or pixel read
** Description:             draw an anti-aliased line with rounded ends, width wd
***************************************************************************************/
void NICE_GFX::drawWideLine(float ax, float ay, float bx, float by, float wd, uint32_t fg_color, uint32_t bg_color)
{
  drawWedgeLine( ax, ay, bx, by, wd/2.0, wd/2.0, fg_color, bg_color);
}

/***************************************************************************************
** Function name:           drawWedgeLine - background colour specified or pixel read
** Description:             draw an anti-aliased line with different width radiused ends
***************************************************************************************/
void NICE_GFX::drawWedgeLine(float ax, float ay, float bx, float by, float ar, float br, uint32_t fg_color, uint32_t bg_color)
{
  if ( (ar < 0.0) || (br < 0.0) )return;
  if ( (fabsf(ax - bx) < 0.01f) && (fabsf(ay - by) < 0.01f) ) bx += 0.01f;  // Avoid divide by zero

  // Find line bounding box
  int32_t x0 = (int32_t)floorf(fminf(ax-ar, bx-br));
  int32_t x1 = (int32_t) ceilf(fmaxf(ax+ar, bx+br));
  int32_t y0 = (int32_t)floorf(fminf(ay-ar, by-br));
  int32_t y1 = (int32_t) ceilf(fmaxf(ay+ar, by+br));

  if (!clipWindow(&x0, &y0, &x1, &y1)) return;

  // Establish x start and y start
  int32_t ys = ay;
  if ((ax-ar)>(bx-br)) ys = by;

  float rdt = ar - br; // Radius delta
  float alpha = 1.0f;
  ar += 0.5;

  uint16_t bg = bg_color;
  float xpax, ypay, bax = bx - ax, bay = by - ay;

  WriteScope write(*this);

  int32_t xs = x0;
  // Scan bounding box from ys down, calculate pixel intensity from distance to line
  for (int32_t yp = ys; yp <= y1; yp++) {
    bool swin = true;  // Flag to start new window area
    bool endX = false; // Flag to skip pixels
    ypay = yp - ay;
    for (int32_t xp = xs; xp <= x1; xp++) {
      if (endX) if (alpha <= LoAlphaTheshold) break;  // Skip right side
      xpax = xp - ax;
      alpha = ar - wedgeLineDistance(xpax, ypay, bax, bay, rdt);
      if (alpha <= LoAlphaTheshold ) continue;
      // Track edge to minimise calculations
      if (!endX) { endX = true; xs = xp; }
      if (alpha > HiAlphaTheshold) {
          if (swin) { setWindow(xp, yp, x1, yp); swin = false; }
          pushColor(fg_color);
        continue;
      }
      //Blend color with background and plot
      if (bg_color == 0x00FFFFFF) {
        bg = readPixel(xp, yp); swin = true;
      }
        if (swin) { setWindow(xp, yp, x1, yp); swin = false; }
        pushColor(fastBlend((uint8_t)(alpha * PixelAlphaGain), fg_color, bg));
    }
  }

  // Reset x start to left side of box
  xs = x0;
  // Scan bounding box from ys-1 up, calculate pixel intensity from distance to line
  for (int32_t yp = ys-1; yp >= y0; yp--) {
    bool swin = true;  // Flag to start new window area
    bool endX = false; // Flag to skip pixels
    ypay = yp - ay;
    for (int32_t xp = xs; xp <= x1; xp++) {
      if (endX) if (alpha <= LoAlphaTheshold) break;  // Skip right side of drawn line
      xpax = xp - ax;
      alpha = ar - wedgeLineDistance(xpax, ypay, bax, bay, rdt);
      if (alpha <= LoAlphaTheshold ) continue;
      // Track line boundary
      if (!endX) { endX = true; xs = xp; }
      if (alpha > HiAlphaTheshold) {
          if (swin) { setWindow(xp, yp, x1, yp); swin = false; }
          pushColor(fg_color);
        continue;
      }
      //Blend colour with background and plot
      if (bg_color == 0x00FFFFFF) {
        bg = readPixel(xp, yp); swin = true;
      }
        if (swin) { setWindow(xp, yp, x1, yp); swin = false; }
        pushColor(fastBlend((uint8_t)(alpha * PixelAlphaGain), fg_color, bg));
    }
  }


}


/***************************************************************************************
** Function name:           lineDistance - private helper function for drawWedgeLine
** Description:             returns distance of px,py to closest part of a to b wedge
***************************************************************************************/
inline float NICE_GFX::wedgeLineDistance(float xpax, float ypay, float bax, float bay, float dr)
{
  float h = fmaxf(fminf((xpax * bax + ypay * bay) / (bax * bax + bay * bay), 1.0f), 0.0f);
  float dx = xpax - bax * h, dy = ypay - bay * h;
  return sqrtf(dx * dx + dy * dy) + h * dr;
}


/***************************************************************************************
** Function name:           drawFastVLine
** Description:             draw a vertical line
***************************************************************************************/
void NICE_GFX::drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color)
{
  fillRect(x,y,1,h,color);
}


/***************************************************************************************
** Function name:           drawFastHLine
** Description:             draw a horizontal line
***************************************************************************************/
void NICE_GFX::drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color)
{
  fillRect(x,y,w,1,color);
}


/***************************************************************************************
** Function name:           fillRect
** Description:             draw a filled rectangle
***************************************************************************************/
void NICE_GFX::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
  if (_vpOoB) return;
  nice_gfx::ClippedRect r;
  if (!nice_gfx::clipRect(x,y,w,h,_xDatum,_yDatum,_vpX,_vpY,_vpW,_vpH,r)) return;
  beginPanelWrite();
  setWindow(r.x,r.y,r.x+r.width-1,r.y+r.height-1);
  if (_runtimeStatus==nice_gfx::Status::Ok) pushBlock(uint16_t(color),uint32_t(r.width)*uint32_t(r.height));
  endPanelWrite();
}


/***************************************************************************************
** Function name:           fillRectVGradient
** Description:             draw a filled rectangle with a vertical colour gradient
***************************************************************************************/
void NICE_GFX::fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2)
{
  nice_gfx::ClippedRect r;
  if (_vpOoB || !nice_gfx::clipRect(x,y,w,h,_xDatum,_yDatum,_vpX,_vpY,_vpW,_vpH,r)) return;
  WriteScope write(*this);
  const auto colorAt = [&](int32_t row) -> uint16_t {
    const uint8_t alpha=h==1?255:uint8_t(255-uint32_t(r.sourceY+row)*255/uint32_t(h-1));
    return alpha==255?uint16_t(color1):alpha==0?uint16_t(color2):fastBlend(alpha,color1,color2);
  };
  // RGB565 quantization gives adjacent rows the same color. Submit each run
  // once, retaining the original interpolation and clipped source positions.
  int32_t first=0;
  uint16_t color=colorAt(0);
  for (int32_t row=1;row<=r.height;++row) {
    const uint16_t next=row<r.height?colorAt(row):color;
    if (row<r.height && next==color) continue;
    fillRect(x+r.sourceX,y+r.sourceY+first,r.width,row-first,color);
    first=row;
    color=next;
  }
}


/***************************************************************************************
** Function name:           fillRectHGradient
** Description:             draw a filled rectangle with a horizontal colour gradient
***************************************************************************************/
void NICE_GFX::fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2)
{
  nice_gfx::ClippedRect r;
  if (_vpOoB || !nice_gfx::clipRect(x,y,w,h,_xDatum,_yDatum,_vpX,_vpY,_vpW,_vpH,r)) return;
  WriteScope write(*this);
  const auto colorAt = [&](int32_t col) -> uint16_t {
    const uint8_t alpha=w==1?255:uint8_t(255-uint32_t(r.sourceX+col)*255/uint32_t(w-1));
    return alpha==255?uint16_t(color1):alpha==0?uint16_t(color2):fastBlend(alpha,color1,color2);
  };
  // Group identical columns so a horizontal gradient does not degenerate
  // into one framebuffer/driver submission per column (or per pixel).
  int32_t first=0;
  uint16_t color=colorAt(0);
  for (int32_t col=1;col<=r.width;++col) {
    const uint16_t next=col<r.width?colorAt(col):color;
    if (col<r.width && next==color) continue;
    fillRect(x+r.sourceX+first,y+r.sourceY,col-first,r.height,color);
    first=col;
    color=next;
  }
}
