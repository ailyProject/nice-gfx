#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace fieldlab { namespace wifi_survey {

struct Entry {
  char ssid[33];
  int32_t rssi;
  bool valid;
};

// A fixed-size scan summary. Named access points precede hidden ones, then
// each group is ordered by signal strength. Equal names remain separate BSSIDs.
class TopNetworks {
 public:
  TopNetworks() { clear(); }

  void clear() {
    for (size_t i=0;i<3;++i) {
      rows_[i].ssid[0]=0;rows_[i].rssi=0;rows_[i].valid=false;
    }
  }

  void add(const char *ssid,int32_t rssi) {
    Entry candidate={};candidate.rssi=rssi;candidate.valid=true;
    copyUtf8(ssid,candidate.ssid,sizeof(candidate.ssid));
    for (size_t i=0;i<3;++i) {
      if (!before(candidate,rows_[i])) continue;
      for (size_t j=2;j>i;--j) rows_[j]=rows_[j-1];
      rows_[i]=candidate;return;
    }
  }

  // The caller supplies an index in [0,3).
  const Entry &entry(size_t index) const { return rows_[index]; }

  void format(size_t index,char *out,size_t size) const {
    if (!out||!size) return;
    out[0]=0;
    if (index>=3||!rows_[index].valid) return;
    char line[64];
    snprintf(line,sizeof(line),"%s %lddBm",rows_[index].ssid[0]?rows_[index].ssid:"(hidden)",
             static_cast<long>(rows_[index].rssi));
    copyUtf8(line,out,size);
  }

 private:
  Entry rows_[3];

  static bool before(const Entry &a,const Entry &b) {
    if (!b.valid) return true;
    const bool namedA=a.ssid[0]!=0,namedB=b.ssid[0]!=0;
    if (namedA!=namedB) return namedA;
    return a.rssi>b.rssi;
  }

  // Preserve complete, valid UTF-8 scalars. Replace controls or malformed
  // bytes with '?'; an incomplete terminal sequence becomes one '?'. A valid
  // scalar crossing the destination limit is omitted in its entirety.
  static void copyUtf8(const char *source,char *out,size_t capacity) {
    if (!capacity) return;
    size_t used=0;out[0]=0;
    if (!source) return;
    const unsigned char *p=reinterpret_cast<const unsigned char *>(source);
    while (*p&&used+1<capacity) {
      const unsigned char first=*p;
      if (first<0x80) {
        out[used++]=(first<0x20||first==0x7f)?'?':static_cast<char>(first);
        ++p;continue;
      }
      const size_t length=first>=0xc2&&first<=0xdf?2:
                          first>=0xe0&&first<=0xef?3:
                          first>=0xf0&&first<=0xf4?4:0;
      if (!length) { out[used++]='?';++p;continue; }
      uint32_t scalar=first&(length==2?0x1f:length==3?0x0f:0x07);
      bool valid=true,incomplete=false;
      for (size_t i=1;i<length;++i) {
        if (!p[i]) { valid=false;incomplete=true;break; }
        if ((p[i]&0xc0)!=0x80) { valid=false;break; }
        scalar=(scalar<<6)|(p[i]&0x3f);
      }
      if (valid) valid=(length==2?scalar>=0x80:length==3?scalar>=0x800:scalar>=0x10000)&&
                       scalar<=0x10ffff&&!(scalar>=0xd800&&scalar<=0xdfff);
      if (!valid) {
        out[used++]='?';
        if (incomplete) break;
        ++p;continue;
      }
      if (scalar>=0x80&&scalar<=0x9f) { out[used++]='?';p+=length;continue; }
      if (length>capacity-1-used) break;
      memcpy(out+used,p,length);used+=length;p+=length;
    }
    out[used]=0;
  }
};

} } // namespace fieldlab::wifi_survey
