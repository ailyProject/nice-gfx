#pragma once
#include "PackedMonoPanel.h"

namespace nice_gfx {

enum class MonoController : uint8_t { SSD1306, SH1106 };
struct MonoPanelConfig {
  MonoController controller=MonoController::SSD1306;
  uint16_t width=128,height=64;
  uint8_t columnOffset=0;
  bool externalVcc=false;
};

class MonoPanel : public PackedMonoPanel {
 public:
  MonoPanel(Transport &bus,MonoPanelConfig config=MonoPanelConfig(),int reset=-1)
      :PackedMonoPanel(bus,config.width,config.height,MonoPacking::PageLsb,reset),config_(config){}
  ~MonoPanel() override {end();}
  Status begin() override {
    if(ready_)return finish();
    if(config_.width!=128||(config_.height!=32&&config_.height!=64)||
       (config_.controller==MonoController::SH1106&&config_.height!=64)||
       unsigned(config_.width)+config_.columnOffset>(config_.controller==MonoController::SH1106?132U:128U)||
       !validReset())return record(Status::InvalidArgument);
    Status s=allocate(0);if(s!=Status::Ok)return s;
    addressValid_=false;
    s=bus_.begin();if(s!=Status::Ok)return record(s);
    if(reset_>=0){bus_.output(reset_,true);bus_.delayMs(1);bus_.output(reset_,false);bus_.delayMs(10);bus_.output(reset_,true);bus_.delayMs(10);}
    const uint8_t start[]={0xAE,0xD5,0x80,0xA8,uint8_t(nativeHeight_-1),0xD3,0x00,0x40};
    s=bus_.commands(start,sizeof(start));
    if(s==Status::Ok){const uint8_t power[]={uint8_t(config_.controller==MonoController::SH1106?0xAD:0x8D),
      uint8_t(config_.controller==MonoController::SH1106?(config_.externalVcc?0x8A:0x8B):(config_.externalVcc?0x10:0x14))};s=bus_.commands(power,sizeof(power));}
    if(s==Status::Ok&&config_.controller==MonoController::SSD1306){const uint8_t mode[]={0x20,0x00};s=bus_.commands(mode,sizeof(mode));}
    if(s==Status::Ok){const uint8_t finish[]={0xA1,0xC8,0xDA,uint8_t(nativeHeight_==64?0x12:0x02),0x81,
      uint8_t(nativeHeight_==32?0x8F:config_.externalVcc?0x9F:0xCF),0xD9,uint8_t(config_.externalVcc?0x22:0xF1),
      0xDB,uint8_t(config_.controller==MonoController::SH1106?0x40:0x40),0xA4,0xA6,0xAF};s=bus_.commands(finish,sizeof(finish));}
    if(s!=Status::Ok){bus_.end();return record(s);}ready_=true;return record(Status::Ok);
  }
  void end() override {if(ready_)bus_.wait();bus_.end();ready_=false;frameOpen_=false;addressValid_=false;}
  uint32_t id()const override{return config_.controller==MonoController::SH1106?0x1106:0x1306;}
  DisplayCapabilities capabilities()const override{
    DisplayCapabilities c;c.colorMode=DisplayColorMode::Monochrome;c.bitsPerPixel=1;
    c.nativeWidth=nativeWidth_;c.nativeHeight=nativeHeight_;c.recommendedFrameMs=33;return c;
  }
  Status invert(bool enabled) override {
    if(!ready_)return record(Status::NotInitialized);
    const uint8_t value=enabled?0xA7:0xA6;return record(bus_.commands(&value,1));
  }
  Status setContrast(uint8_t contrast){if(!ready_)return record(Status::NotInitialized);
    const uint8_t values[]={0x81,contrast};return record(bus_.commands(values,2));}
  Status finish() override {
    if(!ready_)return record(Status::NotInitialized);
    if(frameOpen_||!dirty_)return record(Status::Ok);
    Status s=bus_.wait();if(s!=Status::Ok){addressValid_=false;return record(s);}
    // SSD1306 full-screen horizontal mode uses one address window + one data
    // span. SH1106 intentionally retains separate page/column addressing.
    size_t pageCost=0;
    if(sentValid_&&config_.controller==MonoController::SSD1306){
      for(unsigned page=0;page<nativeHeight_/8;++page){const size_t base=size_t(page)*nativeWidth_;unsigned first=0;
        while(first<nativeWidth_){while(first<nativeWidth_&&frame_[base+first]==sent_[base+first])++first;
          if(first==nativeWidth_)break;unsigned end,next;run(base,first,end,next);pageCost+=end-first+6;first=next;}}
    }
    if(config_.controller==MonoController::SSD1306&&(!sentValid_||pageCost>=bytes_+6)){
      s=ssdAddress(0,uint8_t(nativeWidth_-1),0,uint8_t(nativeHeight_/8-1));if(s==Status::Ok)s=bus_.data(frame_,bytes_);
      if(s==Status::Ok){memcpy(sent_,frame_,bytes_);sentValid_=true;dirty_=false;}else addressValid_=false;return record(s);
    }
    for(unsigned page=0;page<nativeHeight_/8;++page){const size_t base=size_t(page)*nativeWidth_;
      unsigned first=0,last=nativeWidth_;
      if(sentValid_){while(first<last&&frame_[base+first]==sent_[base+first])++first;
        while(last>first&&frame_[base+last-1]==sent_[base+last-1])--last;}
      while(first<last){unsigned end=last,next=last;if(sentValid_)run(base,first,end,next);
        if(config_.controller==MonoController::SSD1306)s=ssdAddress(uint8_t(first),uint8_t(end-1),uint8_t(page),uint8_t(page));
        else{const unsigned column=first+config_.columnOffset;const uint8_t address[]={uint8_t(0xB0|page),uint8_t(column&15),uint8_t(0x10|(column>>4))};s=bus_.commands(address,sizeof(address));}
        if(s==Status::Ok)s=bus_.data(frame_+base+first,end-first);
        if(s!=Status::Ok){addressValid_=false;return record(s);}
        // Only acknowledge bytes actually accepted. Failed runs retry intact.
        memcpy(sent_+base+first,frame_+base+first,end-first);first=next;
      }
    }
    sentValid_=true;dirty_=false;return record(Status::Ok);
  }
 private:
  Status ssdAddress(uint8_t x0,uint8_t x1,uint8_t page0,uint8_t page1){
    if(addressValid_&&x0==address_[0]&&x1==address_[1]&&page0==address_[2]&&page1==address_[3])return Status::Ok;
    const uint8_t bytes[]={0x21,x0,x1,0x22,page0,page1};const Status s=bus_.commands(bytes,sizeof(bytes));
    addressValid_=s==Status::Ok;address_[0]=x0;address_[1]=x1;address_[2]=page0;address_[3]=page1;return s;
  }
  // Splitting far-apart changes saves payload; merge small gaps whose bytes
  // cost less than another command window plus I2C control-byte framing.
  void run(size_t base,unsigned start,unsigned &end,unsigned &next)const{
    const unsigned gap=config_.controller==MonoController::SSD1306?8:5;
    end=start+1;next=end;
    while(next<nativeWidth_){if(frame_[base+next]!=sent_[base+next]){
        if(next-end>gap)break;end=next+1;}++next;}
  }
  MonoPanelConfig config_;
  // SSD1306 section10.1.3: a complete horizontal window wraps both pointers
  // back to its start. Repeated identical windows need only pixel data. Every
  // call above sends the complete window; errors/reinitialization invalidate.
  bool addressValid_=false;
  uint8_t address_[4]={};
};

} // namespace nice_gfx
