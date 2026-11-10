#pragma once
#include "NativeTypes.h"

namespace nice_gfx { namespace it8951 {

// Original protocol implementation from ITE's I80/SPI/I2C Programming Guide
// v2.7 (2017-09-04), chapters 1-2 and Appendix II. Official document archive:
// https://files.waveshare.com/upload/b/b0/IT8951_I80%2BProgrammingGuide_16bits_20170904_v2.7_common_CXDX.zip
// The reTerminal E1003 product cookbook identifies GC16 as waveform mode 2:
// https://wiki.seeedstudio.com/reterminal_e10xx_with_arduino/
// Firmware and panel waveforms must already be installed on the TCON board.

struct Area {
  uint16_t x=0,y=0,width=0,height=0;
  Area()=default;
  Area(uint16_t x0,uint16_t y0,uint16_t w,uint16_t h):x(x0),y(y0),width(w),height(h){}
};

// Four monochrome bits become four grayscale nibbles, first pixel in bits
// 15..12. A host monochrome 1 is white (gray 15), and 0 is black (gray 0).
inline uint16_t monoNibbleToGrayWord(uint8_t value){
  const uint16_t lanes=uint16_t((value&8)<<9)|uint16_t((value&4)<<6)|
    uint16_t((value&2)<<3)|uint16_t(value&1);
  return uint16_t(lanes*15);
}
inline void expandMono4(const uint8_t *source,uint16_t *target,size_t sourceBytes){
  for(size_t i=0;i<sourceBytes;++i){
    target[i*2]=monoNibbleToGrayWord(uint8_t(source[i]>>4));
    target[i*2+1]=monoNibbleToGrayWord(source[i]);
  }
}

// Each call performs at most one framed SPI operation. Busy is progress, not
// failure: retry the SAME method and arguments until Ok. The caller owns every
// pixel buffer; neither this controller nor NativeEpaperIo retains its address.
// NativeEpaperIo must preserve CS and packet phase across Busy, including the
// HRDY checks after the preamble and read dummy. Word values are host integers;
// the physical transport serializes each word most-significant byte first.
class Controller {
 public:
  Controller(){resetState();}
  void resetState(){
    for(size_t i=0;i<20;++i)info_[i]=0;
    operation_=Operation::None;stage_=0;commandPart_=0;registerPart_=0;
    remainingWords_=0;pendingWords_=0;initialized_=false;uploadOpen_=false;sleeping_=false;
  }
  bool initialized() const {return initialized_;}
  uint16_t width() const {return info_[0];}
  uint16_t height() const {return info_[1];}
  uint32_t imageAddress() const {return uint32_t(info_[2])|(uint32_t(info_[3])<<16);}
  const uint16_t *info() const {return info_;} // width, height, address L/H, FW[8], LUT[8]
  size_t remainingWords() const {return remainingWords_;}

  Status initialize(NativeEpaperIo &io,uint16_t expectedWidth,uint16_t expectedHeight){
    if(!expectedWidth||!expectedHeight)return Status::InvalidArgument;
    if(!enter(Operation::Initialize))return Status::Busy;
    if(stage_==0)return advance(send(io,0x0001,nullptr,0));
    if(stage_==1)return advance(send(io,0x0302,nullptr,0));
    if(stage_==2){
      // This reply MUST be one 20-word burst, with one dummy for the whole CS.
      const Status s=io.readWords(info_,20);
      if(s!=Status::Ok)return s;
      if(width()!=expectedWidth||height()!=expectedHeight||width()>=2048||height()>=2048||
         (imageAddress()&0xfc000000u))return Status::IoError;
      ++stage_;return Status::Busy;
    }
    const Status s=writeRegister(io,0x0004,1); // enable packed command parameters
    if(s==Status::Ok){initialized_=true;sleeping_=false;leave();}
    return s;
  }

  Status startUpload(NativeEpaperIo &io,Area area){
    if(!initialized_||sleeping_)return Status::NotInitialized;
    if(uploadOpen_||!valid(area)||(area.x&3)||(area.width&3))return Status::InvalidArgument;
    if(!enter(Operation::Upload))return Status::Busy;
    if(stage_==0)return advance(lutReady(io));
    if(stage_==1)return advance(writeRegister(io,0x020a,uint16_t(imageAddress()>>16)));
    if(stage_==2)return advance(writeRegister(io,0x0208,uint16_t(imageAddress())));
    // Bit 8: big endian; bits 5..4: 4bpp; bits 1..0: no rotation.
    const uint16_t parameters[]={0x0120,area.x,area.y,area.width,area.height};
    const Status s=send(io,0x0021,parameters,5);
    if(s==Status::Ok){
      remainingWords_=size_t(area.width)*area.height/4;uploadOpen_=true;leave();
    }
    return s;
  }

  Status writePixels(NativeEpaperIo &io,const uint16_t *pixels,size_t words){
    if(!initialized_||sleeping_)return Status::NotInitialized;
    if(!uploadOpen_||!pixels||!words||words>1024||words>remainingWords_)return Status::InvalidArgument;
    if(operation_==Operation::None)pendingWords_=words;
    if(!enter(Operation::Pixels))return Status::Busy;
    if(words!=pendingWords_)return Status::InvalidArgument;
    const Status s=io.writeWords(0x0000,pixels,words);
    if(s==Status::Ok){remainingWords_-=words;pendingWords_=0;leave();}
    return s;
  }

  Status finishUpload(NativeEpaperIo &io){
    if(!initialized_||sleeping_)return Status::NotInitialized;
    if(!uploadOpen_||remainingWords_)return Status::InvalidArgument;
    if(!enter(Operation::FinishUpload))return Status::Busy;
    const Status s=send(io,0x0022,nullptr,0);
    if(s==Status::Ok){uploadOpen_=false;leave();}
    return s;
  }

  Status refresh(NativeEpaperIo &io,Area area,uint16_t mode=2){
    if(!initialized_||sleeping_)return Status::NotInitialized;
    if(uploadOpen_||!valid(area)||mode>4)return Status::InvalidArgument;
    if(!enter(Operation::Refresh))return Status::Busy;
    if(stage_==0)return advance(lutReady(io));
    if(stage_==1){
      const uint16_t parameters[]={area.x,area.y,area.width,area.height,mode};
      return advance(send(io,0x0034,parameters,5));
    }
    // HRDY only describes the host interface. LUTAFSR is the refresh-complete
    // indication; Display Area itself does not wait for the display engine.
    const Status s=lutReady(io);if(s==Status::Ok)leave();return s;
  }

  Status powerOff(NativeEpaperIo &io){
    if(!initialized_||sleeping_)return Status::NotInitialized;
    if(uploadOpen_)return Status::InvalidArgument;
    if(!enter(Operation::PowerOff))return Status::Busy;
    if(stage_==0)return advance(lutReady(io));
    const uint16_t off=0;
    const Status s=send(io,0x0038,&off,1);if(s==Status::Ok)leave();return s;
  }
  Status sleep(NativeEpaperIo &io){
    if(!initialized_)return Status::NotInitialized;
    if(sleeping_)return Status::Ok;
    if(uploadOpen_)return Status::InvalidArgument;
    if(!enter(Operation::Sleep))return Status::Busy;
    if(stage_==0)return advance(lutReady(io));
    const Status s=send(io,0x0003,nullptr,0);
    if(s==Status::Ok){sleeping_=true;leave();}return s;
  }
  Status wake(NativeEpaperIo &io){
    if(!initialized_)return Status::NotInitialized;
    if(uploadOpen_)return Status::InvalidArgument;
    if(!enter(Operation::Wake))return Status::Busy;
    const Status s=send(io,0x0001,nullptr,0);
    if(s==Status::Ok){sleeping_=false;leave();}return s;
  }

 private:
  enum class Operation:uint8_t {None,Initialize,Upload,Pixels,FinishUpload,Refresh,PowerOff,Sleep,Wake};
  bool enter(Operation operation){
    if(operation_==Operation::None){operation_=operation;stage_=0;commandPart_=0;registerPart_=0;}
    return operation_==operation;
  }
  void leave(){operation_=Operation::None;stage_=0;commandPart_=0;registerPart_=0;}
  Status advance(Status s){if(s==Status::Ok){++stage_;return Status::Busy;}return s;}
  bool valid(Area a) const {
    return a.width&&a.height&&uint32_t(a.x)+a.width<=width()&&uint32_t(a.y)+a.height<=height();
  }
  Status send(NativeEpaperIo &io,uint16_t command,const uint16_t *parameters,uint8_t count){
    if(commandPart_==0){
      const Status s=io.writeWords(0x6000,&command,1);
      if(s!=Status::Ok)return s;
      if(!count)return Status::Ok;
      commandPart_=1;return Status::Busy;
    }
    // Packed parameters are enabled during initialization. The complete
    // short argument list fits one FIFO burst, with one preamble/HRDY gate.
    const Status s=io.writeWords(0x0000,parameters,count);
    if(s==Status::Ok)commandPart_=0;return s;
  }
  Status writeRegister(NativeEpaperIo &io,uint16_t address,uint16_t value){
    const uint16_t parameters[]={address,value};return send(io,0x0011,parameters,2);
  }
  Status lutReady(NativeEpaperIo &io){
    if(registerPart_==0){
      const uint16_t address=0x1224;
      const Status s=send(io,0x0010,&address,1);
      if(s!=Status::Ok)return s;
      registerPart_=1;return Status::Busy;
    }
    uint16_t flags=0;
    const Status s=io.readWords(&flags,1);
    if(s!=Status::Ok)return s;
    registerPart_=0;return flags?Status::Busy:Status::Ok;
  }
  uint16_t info_[20];
  size_t remainingWords_,pendingWords_;
  Operation operation_;
  uint8_t stage_,commandPart_,registerPart_;
  bool initialized_,uploadOpen_,sleeping_;
};

} } // namespace nice_gfx::it8951
