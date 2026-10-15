#include <assert.h>
#include "sprite_list.h"
static struct { s16 x,y; u8 size,link; u16 attribute; } sat[80];
static u16 writes,count;
void VDP_setSpriteFull(u16 index,s16 x,s16 y,u8 size,u16 attribute,u8 link)
{ assert(index<80); sat[index].x=x; sat[index].y=y; sat[index].size=size; sat[index].attribute=attribute; sat[index].link=link; writes++; }
void VDP_setSpriteLink(u16 index,u8 link) { assert(index<80); sat[index].link=link; }
void VDP_updateSprites(u16 number,u16 transfer) { assert(transfer==DMA_QUEUE); count=number; }
int main(void)
{
    AnimationFrame frame={.numSprite=1,.frameVDPSprites={{.offsetX=3,.offsetY=2,.size=5}}};
    AnimationFrame *frames[]={&frame}; Animation animation={.numFrame=1,.frames=frames};
    Animation *animations[]={&animation}; SpriteDefinition definition={.animations=animations};
    Actor actor={&definition,2}; u16 tiles[]={123};
    sprite_list_begin(240);
    sprite_list_place(&actor,16*100,16*80,16,true,0,tiles);
    assert(writes==1 && sat[0].x==95 && sat[0].y==74 && sat[0].size==5);
    assert(sat[0].attribute==((2<<13)|123));
    sprite_list_place(&actor,16*240+8*16,0,16,true,0,tiles);
    sprite_list_place(&actor,0,0,16,false,0,tiles); assert(writes==1);
    sprite_list_place(&actor,-16*7,16*10,16,true,0,tiles);
    sprite_list_end(); assert(count==2 && sat[0].link==1 && sat[1].link==0);
    sprite_list_begin(320);
    for (u16 i=0;i<100;i++) sprite_list_place(&actor,16*250,16*80,16,true,0,tiles);
    sprite_list_end(); assert(count==80 && sat[79].link==0);
    sprite_list_begin(240); sprite_list_end(); assert(count==0);
    return 0;
}
