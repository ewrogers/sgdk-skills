#include "view.h"
bool rt_column(const Route *r,u16 column,u16 words[24])
{
    if(column>=160 || !words)return false;
    for(u16 row=0;row<24;row++) {
        u16 tile;
        if(!sc_tile(&r->cache,sw_page(column,row),(row&3)*4+(column&3),&tile))return false;
        words[row]=(u16)(((1+(column/40&1))<<13)|tile);
    }
    return true;
}
