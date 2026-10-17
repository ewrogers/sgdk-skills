#include <genesis.h>
#include "catalog.h"
static struct {u16 before[8];u16 data[2048];u16 after[8];} buf;
static void emit(u16 n){*(volatile u16 *)0xC00000=n;}
__attribute__((used,noipa,externally_visible))
int main(bool hard_reset) {
    (void)hard_reset;
    for(u16 i=0;i<8;i++)buf.before[i]=buf.after[i]=0xA55A;
    for(u16 sample=0;sample<2;sample++) {
        for(u16 codec=0;codec<3;codec++) {
            const CLAsset *a=&cl_assets[sample][codec];
            memset(buf.data,0,4096);
            if(cl_decode_asset(a,(u8 *)buf.data,4095,4096))return 1;
            for(u16 i=0;i<2048;i++)if(buf.data[i])return 2;
            if(!cl_decode(sample,codec,0,0,(u8 *)buf.data,4096,0))return 3;
            for(u16 i=0;i<4096;i++)if(((u8 *)buf.data)[i]!=cl_assets[sample][0].data[i])return 4;
            for(u16 i=0;i<8;i++)if(buf.before[i]!=0xA55A || buf.after[i]!=0xA55A)return 5;
            emit(a->stored);
        }
        for(u16 part=0;part<8;part++) {
            memset(buf.data,0,4096);
            if(!cl_decode(sample,2,part,1,(u8 *)buf.data,512,0))return 6;
            for(u16 i=0;i<512;i++)if(((u8 *)buf.data)[i]!=cl_assets[sample][0].data[part*512+i])return 7;
            for(u16 i=256;i<2048;i++)if(buf.data[i])return 8;
            for(u16 i=0;i<8;i++)if(buf.before[i]!=0xA55A || buf.after[i]!=0xA55A)return 12;
            emit(cl_chunks[sample][part].stored);
        }
    }
    if(cl_decode(2,0,0,0,(u8 *)buf.data,4096,0))return 9;
    if(cl_decode(0,2,8,1,(u8 *)buf.data,512,0))return 10;
    CLAsset invalid={cl_assets[0][0].data,4095,0};
    if(cl_decode_asset(&invalid,(u8 *)buf.data,4096,4096))return 11;
    emit(sizeof(CLLoader));
    return 0;
}
