#include "view.h"
#include "resources.h"
static const SpriteDefinition *idle[]={&art_hero_idle,&art_guide_idle,&art_sage_idle};
static const SpriteDefinition *walk[]={&art_hero_walk,&art_guide_walk,&art_sage_walk};
static u16 cached[3];
void actors_reset(void){for(u16 n=0;n<3;n++)cached[n]=65535;}

void actors_frame(const Game *g)
{
    const Actor *actors[]={&g->hero,&g->npc[0],&g->npc[1]};
    u16 order[]={0,1,2};
    for(u16 a=0;a<3;a++)for(u16 b=a+1;b<3;b++)if(actors[order[a]]->y<actors[order[b]]->y)
    {u16 t=order[a];order[a]=order[b];order[b]=t;}
    u16 count=0;
    for(u16 p=0;p<3;p++)
    {
        u16 n=order[p];const Actor *actor=actors[n];
        const SpriteDefinition *definition=actor->moving?walk[n]:idle[n];
        const AnimationFrame *frame=actor->moving?definition->animations[actor->direction]->frames[actor->frame]:definition->animations[0]->frames[actor->direction];
        u16 key=actor->direction*8+(actor->moving?actor->frame+4:0),tile=actor_base+n*36;
        if(cached[n]!=key){VDP_loadTileSet(frame->tileset,tile,DMA);cached[n]=key;}
        for(u16 piece_index=0;piece_index<(frame->numSprite&0x7F);piece_index++)
        {
            const FrameVDPSprite *piece=&frame->frameVDPSprites[piece_index];
            VDP_setSpriteFull(count,actor->x-24+piece->offsetX,16+actor->y-42+piece->offsetY,piece->size,
                TILE_ATTR_FULL(PAL2,g->screen!=DIALOGUE,FALSE,FALSE,tile),count+1);
            count++;tile+=piece->numTile;
        }
    }
    if(count)VDP_setSpriteLink(count-1,0);
    VDP_updateSprites(count,DMA);
}
