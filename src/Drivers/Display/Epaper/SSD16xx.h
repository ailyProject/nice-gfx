#pragma once
#include "../../../Display/Transport.h"
#include "EpaperTypes.h"

namespace nice_gfx { namespace ssd16xx {
// SSD1680/81 register protocol only. GPIO timing, BUSY state, scheduling, dirty
// calculation and cleanup policy belong to the display backend.
inline Status area(Transport &bus,EpaperWindow r){
  if(!r.width||!r.height||(r.x&7)||(r.width&7))return Status::InvalidArgument;
  const uint16_t right=r.x+r.width-1,bottom=r.y+r.height-1;
  const uint8_t x[]={uint8_t(r.x/8),uint8_t(right/8)};
  const uint8_t y[]={uint8_t(r.y),uint8_t(r.y>>8),uint8_t(bottom),uint8_t(bottom>>8)};
  Status s=bus.command(0x44,x,2);return s==Status::Ok?bus.command(0x45,y,4):s;
}
inline Status pointer(Transport &bus,uint16_t x=0,uint16_t y=0){
  const uint8_t column=uint8_t(x/8),row[]={uint8_t(y),uint8_t(y>>8)};
  Status s=bus.command(0x4E,&column,1);return s==Status::Ok?bus.command(0x4F,row,2):s;
}
inline Status trigger(Transport &bus,uint8_t mode){Status s=bus.command(0x22,&mode,1);return s==Status::Ok?bus.command(0x20,nullptr,0):s;}
inline Status configure(Transport &bus,const EpaperProfile &p){
  const uint16_t last=p.height-1;const uint8_t gate[]={uint8_t(last),uint8_t(last>>8),0},mode=3;
  Status s=bus.command(0x01,gate,3);if(s==Status::Ok)s=bus.command(0x11,&mode,1);
  if(s==Status::Ok)s=area(bus,EpaperWindow(0,0,p.width,p.height));
  if(s==Status::Ok&&p.controller==EpaperController::SSD1680){const uint8_t update[]={0,0x80};s=bus.command(0x21,update,2);}
  if(s==Status::Ok&&p.temperatureLoad){const uint8_t border=1,temp=0x80;
    s=bus.command(0x3C,&border,1);if(s==Status::Ok)s=bus.command(0x18,&temp,1);if(s==Status::Ok)s=trigger(bus,0xB1);}
  return s;
}
inline Status loadLut(Transport &bus,const uint8_t *lut){return lut?bus.command(0x32,lut,153):Status::InvalidArgument;}
inline Status voltages(Transport &bus,const uint8_t *lut){
  if(!lut)return Status::InvalidArgument;const uint8_t *v=lut+153;Status s=bus.command(0x3F,v,1);
  if(s==Status::Ok)s=bus.command(0x03,v+1,1);if(s==Status::Ok)s=bus.command(0x04,v+2,3);
  if(s==Status::Ok)s=bus.command(0x2C,v+5,1);return s;
}
inline Status preparePartial(Transport &bus){
  const uint8_t control[]={0,0,0,0,0,0x40,0,0,0,0},border=0x80;
  Status s=bus.command(0x37,control,sizeof(control));if(s==Status::Ok)s=bus.command(0x3C,&border,1);
  return s==Status::Ok?trigger(bus,0xC0):s;
}
inline Status writePlane(Transport &bus,uint8_t ram,const uint8_t *pixels,size_t stride,EpaperWindow r){
  Status s=pointer(bus,r.x,r.y);if(s!=Status::Ok)return s;
  const size_t n=r.width/8;const uint8_t *source=pixels+size_t(r.y)*stride+r.x/8;
  // A full-width region is contiguous and needs just one bulk transfer.
  if(n==stride)return bus.command(ram,source,n*r.height);
  s=bus.command(ram,nullptr,0);
  // Gather several narrow rows into one bounded span. A one-byte-wide 200-row
  // field now costs one data transaction instead of 200 transactions.
  uint8_t staging[256];const size_t rowsPerChunk=sizeof(staging)/n;
  for(unsigned y=0;y<r.height&&s==Status::Ok;){const size_t rows=r.height-y<rowsPerChunk?r.height-y:rowsPerChunk;
    for(size_t i=0;i<rows;++i)memcpy(staging+i*n,source+size_t(y+i)*stride,n);
    s=bus.data(staging,n*rows);y+=unsigned(rows);}
  return s;
}
inline Status full(Transport &bus,const EpaperProfile &p,const uint8_t *pixels){
  const EpaperWindow r(0,0,p.width,p.height);Status s=area(bus,r);
  if(s==Status::Ok)s=writePlane(bus,0x24,pixels,p.width/8,r);
  if(s==Status::Ok)s=writePlane(bus,0x26,pixels,p.width/8,r);
  return s==Status::Ok?trigger(bus,0xC7):s;
}
inline Status partial(Transport &bus,const EpaperProfile &p,const uint8_t *pixels,EpaperWindow r){
  Status s=area(bus,r);if(s==Status::Ok)s=writePlane(bus,0x24,pixels,p.width/8,r);
  return s==Status::Ok?trigger(bus,p.controller==EpaperController::SSD1681?0xCF:0x0F):s;
}
} } // namespace nice_gfx::ssd16xx
