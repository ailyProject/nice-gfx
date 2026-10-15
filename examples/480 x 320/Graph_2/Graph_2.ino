/*

  This program provides cartesian type graph function

  Revisions
  rev     date        author      description
  1       12-24-2015  kasprzak    initial creation

  Updated by Bodmer to be an example for the library here:
*/

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>
#include <SPI.h>

ExampleDisplay gfx;                   // Invoke custom library with default width and height


#define LTBLUE    0xB6DF
#define LTTEAL    0xBF5F
#define LTGREEN   0xBFF7
#define LTCYAN    0xC7FF
#define LTRED     0xFD34
#define LTMAGENTA 0xFD5F
#define LTYELLOW  0xFFF8
#define LTORANGE  0xFE73
#define LTPINK    0xFDDF
#define LTPURPLE  0xCCFF
#define LTGREY    0xE71C

#define BLUE      0x001F
#define TEAL      0x0438
#define GREEN     0x07E0
#define CYAN      0x07FF
#define RED       0xF800
#define MAGENTA   0xF81F
#define YELLOW    0xFFE0
#define ORANGE    0xFC00
#define PINK      0xF81F
#define PURPLE    0x8010
#define GREY      0xC618
#define WHITE     0xFFFF
#define BLACK     0x0000

#define DKBLUE    0x000D
#define DKTEAL    0x020C
#define DKGREEN   0x03E0
#define DKCYAN    0x03EF
#define DKRED     0x6000
#define DKMAGENTA 0x8008
#define DKYELLOW  0x8400
#define DKORANGE  0x8200
#define DKPINK    0x9009
#define DKPURPLE  0x4010
#define DKGREY    0x4A49

// these are the only external variables used by the graph function
// it's a flag to draw the coordinate system only on the first call to the Graph() function
// and will mimize flicker
// also create some variables to store the old x and y, if you draw 2 graphs on the same display
// you will need to store ox and oy per each display
bool display1 = true;
bool update1 = true;

double ox = -999, oy = -999; // Force them to be off screen

/*

  function to draw a cartesian coordinate system and plot whatever data you want
  just pass x and y and the graph will be drawn

  huge arguement list
  &d name of your display object
  x = x data point
  y = y datapont
  gx = x graph location (lower left)
  gy = y graph location (lower left)
  w = width of graph
  h = height of graph
  xlo = lower bound of x axis
  xhi = upper bound of x asis
  xinc = division of x axis (distance not count)
  ylo = lower bound of y axis
  yhi = upper bound of y asis
  yinc = division of y axis (distance not count)
  title = title of graph
  xlabel = x asis label
  ylabel = y asis label
  &redraw = flag to redraw graph on first call only
  color = plotted trace colour
*/


void Graph(NICE_GFX &gfx, double x, double y, byte dp,
                           double gx, double gy, double w, double h,
                           double xlo, double xhi, double xinc,
                           double ylo, double yhi, double yinc,
                           char *title, char *xlabel, char *ylabel,
                           bool &redraw, unsigned int color) {

  double ydiv, xdiv;
  double i;
  double temp;
  int rot, newrot;

  // gcolor = graph grid colors
  // acolor = axes line colors
  // pcolor = color of your plotted data
  // tcolor = text color
  // bcolor = background color
  unsigned int gcolor = DKBLUE;
  unsigned int acolor = RED;
  unsigned int pcolor = color;
  unsigned int tcolor = WHITE;
  unsigned int bcolor = BLACK;

  if (redraw == true) {

    redraw = false;
    // initialize old x and old y in order to draw the first point of the graph
    // but save the transformed value
    // note my transform funcition is the same as the map function, except the map uses long and we need doubles
    //ox = (x - xlo) * ( w) / (xhi - xlo) + gx;
    //oy = (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;

    gfx.setTextDatum(MR_DATUM);

    // draw y scale
    for ( i = ylo; i <= yhi; i += yinc) {
      // compute the transform
      temp =  (i - ylo) * (gy - h - gy) / (yhi - ylo) + gy;

      if (i == 0) {
        gfx.drawLine(gx, temp, gx + w, temp, acolor);
        gfx.setTextColor(acolor, bcolor);
        nice_gfx::text::draw(gfx, xlabel, (int)(gx + w), (int)temp, nice_gfx::fonts::GfxTerm8x16, 1);
      }
      else {
        gfx.drawLine(gx, temp, gx + w, temp, gcolor);
      }
      // draw the axis labels
      gfx.setTextColor(tcolor, bcolor);
      // precision is default Arduino--this could really use some format control
      nice_gfx::text::decimal(gfx, i, dp, gx - 4, temp, nice_gfx::fonts::TomThumb4x6, 1);
    }

    // draw x scale
    for (i = xlo; i <= xhi; i += xinc) {

      // compute the transform
      temp =  (i - xlo) * ( w) / (xhi - xlo) + gx;
      if (i == 0) {
        gfx.drawLine(temp, gy, temp, gy - h, acolor);
        gfx.setTextColor(acolor, bcolor);
        gfx.setTextDatum(BC_DATUM);
        nice_gfx::text::draw(gfx, ylabel, (int)temp, (int)(gy - h - 8), nice_gfx::fonts::GfxTerm8x16, 1);
      }
      else {
        gfx.drawLine(temp, gy, temp, gy - h, gcolor);
      }
      // draw the axis labels
      gfx.setTextColor(tcolor, bcolor);
      gfx.setTextDatum(TC_DATUM);
      // precision is default Arduino--this could really use some format control
      nice_gfx::text::decimal(gfx, i, dp, temp, gy + 7, nice_gfx::fonts::TomThumb4x6, 1);
    }

    //now draw the graph labels
    gfx.setTextColor(tcolor, bcolor);
    nice_gfx::text::draw(gfx, title, (int)(gx + w / 2), (int)(gy - h - 30), nice_gfx::fonts::Spleen12x24, 1);
  }

  // the coordinates are now drawn, plot the data
  // the entire plotting code are these few lines...
  // recall that ox and oy are initialized above
  //x =  (x - xlo) * ( w) / (xhi - xlo) + gx;
  //y =  (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
  //gfx.drawLine(ox, oy, x, y, pcolor);
  // it's up to you but drawing 2 more lines to give the graph some thickness
  //gfx.drawLine(ox, oy + 1, x, y + 1, pcolor);
  //gfx.drawLine(ox, oy - 1, x, y - 1, pcolor);
  //ox = x;
  //oy = y;

}

void Trace(NICE_GFX &gfx, double x,  double y,  byte dp,
           double gx, double gy,
           double w, double h,
           double xlo, double xhi, double xinc,
           double ylo, double yhi, double yinc,
           char *title, char *xlabel, char *ylabel,
           bool &update1, unsigned int color)
{
  double ydiv, xdiv;
  double i;
  double temp;
  int rot, newrot;

  //unsigned int gcolor = DKBLUE;   // gcolor = graph grid color
  unsigned int acolor = RED;        // acolor = main axes and label color
  unsigned int pcolor = color;      // pcolor = color of your plotted data
  unsigned int tcolor = WHITE;      // tcolor = text color
  unsigned int bcolor = BLACK;      // bcolor = background color

  // initialize old x and old y in order to draw the first point of the graph
  // but save the transformed value
  // note my transform funcition is the same as the map function, except the map uses long and we need doubles
  if (update1) {
    update1 = false;
    
    ox = (x - xlo) * ( w) / (xhi - xlo) + gx;
    oy = (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;

    if ((ox < gx) || (ox > gx+w)) {update1 = true; return;}
    if ((oy < gy-h) || (oy > gy)) {update1 = true; return;}
    

    gfx.setTextDatum(MR_DATUM);

    // draw y scale
    for ( i = ylo; i <= yhi; i += yinc) {
      // compute the transform
      temp =  (i - ylo) * (gy - h - gy) / (yhi - ylo) + gy;

      if (i == 0) {
        gfx.setTextColor(acolor, bcolor);
        nice_gfx::text::draw(gfx, xlabel, (int)(gx + w), (int)temp, nice_gfx::fonts::GfxTerm8x16, 1);
      }
      // draw the axis labels
      gfx.setTextColor(tcolor, bcolor);
      // precision is default Arduino--this could really use some format control
      nice_gfx::text::decimal(gfx, i, dp, gx - 4, temp, nice_gfx::fonts::TomThumb4x6, 1);
    }

    // draw x scale
    for (i = xlo; i <= xhi; i += xinc) {

      // compute the transform
      temp =  (i - xlo) * ( w) / (xhi - xlo) + gx;
      if (i == 0) {
        gfx.setTextColor(acolor, bcolor);
        gfx.setTextDatum(BC_DATUM);
        nice_gfx::text::draw(gfx, ylabel, (int)temp, (int)(gy - h - 8), nice_gfx::fonts::GfxTerm8x16, 1);
      }

      // draw the axis labels
      gfx.setTextColor(tcolor, bcolor);
      gfx.setTextDatum(TC_DATUM);
      // precision is default Arduino--this could really use some format control
      nice_gfx::text::decimal(gfx, i, dp, temp, gy + 7, nice_gfx::fonts::TomThumb4x6, 1);
    }

    //now draw the graph labels
    gfx.setTextColor(tcolor, bcolor);
    nice_gfx::text::draw(gfx, title, (int)(gx + w / 2), (int)(gy - h - 30), nice_gfx::fonts::Spleen12x24, 1);
  }

  // the coordinates are now drawn, plot the data
  // the entire plotting code are these few lines...
  // recall that ox and oy are initialized above
  x =  (x - xlo) * ( w) / (xhi - xlo) + gx;
  y =  (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;

  if ((x < gx) || (x > gx+w)) {update1 = true; return;}
  if ((y < gy-h) || (y > gy)) {update1 = true; return;}
    
    
  gfx.drawLine(ox, oy, x, y, pcolor);
  // it's up to you but drawing 2 more lines to give the graph some thickness
  //gfx.drawLine(ox, oy + 1, x, y + 1, pcolor);
  //gfx.drawLine(ox, oy - 1, x, y - 1, pcolor);
  ox = x;
  oy = y;

}

/*

  End of graphing function

*/



void setup() {
  double x, y;

  gfx.begin();


  gfx.setFont(nice_gfx::fonts::TomThumb4x6);
  gfx.fillScreen(BLACK);
  gfx.setRotation(1);

  Graph(gfx, x, y, 1, 60, 290, 390, 260, 0, 6.5, 1, -1, 1, .25, "", "", "", display1, YELLOW);

  //delay(1000);
  
  update1 = true;
  for (x = 0; x <= 6.3; x += .1) {
    y = sin(x);
    Trace(gfx, x, y, 1, 60, 290, 390, 260, 0, 6.5, 1, -1, 1, .25, "Sin(x)", "x", "fn(x)", update1, YELLOW);
  }

  delay(2000);

  update1 = true;
  for (x = 0; x <= 6.3; x += .1) {
    y = cos(x);
    Trace(gfx, x, y, 1, 60, 290, 390, 260, 0, 6.5, 1, -1, 1, .25, "Sin(x) + Cos(x)", " x", "fn(x)", update1, GFX_PINK);
  }

  delay(2000);
  
  update1 = true;
  for (x = 0; x <= 6.3; x += .02) {
    y = tan(x);
    Trace(gfx, x, y, 1, 60, 290, 390, 260, 0, 6.5, 1, -1, 1, .25, "Sin(x) + Cos(x) + Tan(x)", " x", "fn(x)", update1, CYAN);
  }
}


void loop(void) {

}




