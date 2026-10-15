#include "text.h"
bool text_field(char *out,unsigned capacity,const char *text,unsigned width)
{
    if (!out || !text || width>=capacity) return false;
    bool ended=false;
    for (unsigned i=0;i<width;++i) {
        unsigned char c=ended ? 0 : (unsigned char)text[i];
        if (!c) ended=true;
        out[i]=ended ? ' ' : c>=32 && c<=126 ? c : '?';
    }
    out[width]=0; return true;
}
bool text_number(char *out,unsigned capacity,u32 value,unsigned width)
{
    if (!out || !width || width>=capacity) return false;
    for (unsigned i=width;i>0;--i) {
        out[i-1]=value || i==width ? '0'+value%10 : ' ';
        value/=10;
    }
    if (value) for (unsigned i=0;i<width;++i) out[i]='#';
    out[width]=0; return true;
}
