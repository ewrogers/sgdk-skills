#include "stream_world.h"

u16 sw_page(u16 world_column,u16 row)
{
    if(world_column>=160 || row>=24)return SC_PAGES;
    u16 room=world_column/40,x=world_column%40/4,y=row/4;
    u16 kind=1;
    if(y==5)kind=0;
    else if(y==4)kind=(x&1) ? 2 : 5;
    else if((x==1 || x==4 || x==7) && y==2)kind=4;
    else if((x==1 || x==4 || x==7) && y==3)kind=3;
    return room*6+kind;
}

u16 sw_columns(u16 camera)
{
    return camera<=960 ? (camera+319)/8-camera/8+1 : 0;
}

u16 sw_wanted(u16 camera,u16 future,bool covered,u16 pages[SC_SLOTS])
{
    if(camera>960 || future>960 || !pages)return 0;
    u16 count=0;
    for(u16 room=0;room<4;room++) {
        bool visible=!covered && room>=camera/320 && room<=(camera+319)/320;
        bool next=room>=future/320 && room<=(future+319)/320;
        if(visible || next)for(u16 p=0;p<6;p++) {
            if(count==SC_SLOTS)return 0; /* Caller must cover a distant teleport. */
            pages[count++]=room*6+p;
        }
    }
    return count;
}

bool sw_entering(u16 before,u16 after,u16 *column)
{
    if(before>960 || after>960 || !column)return false;
    if(after>before && after-before<=8 && (after+319)/8!=(before+319)/8) {
        *column=(after+319)/8;return true;
    }
    if(after<before && before-after<=8 && after/8!=before/8) {
        *column=after/8;return true;
    }
    return false;
}
