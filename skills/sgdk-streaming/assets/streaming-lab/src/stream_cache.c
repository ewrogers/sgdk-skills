#include "stream_cache.h"

static bool wanted(const StreamCache *c,u16 page)
{
    for(u16 i=0;i<c->wanted_count;i++)if(c->wanted[i]==page)return true;
    return false;
}

void sc_init(StreamCache *c)
{
    *c=(StreamCache){0};c->pending=SC_SLOTS;
    for(u16 i=0;i<SC_PAGES;i++)c->slot_for[i]=SC_SLOTS;
}

bool sc_pin(StreamCache *c,const u16 *pages,u16 count)
{
    if(count>SC_SLOTS || (count && !pages))return false;
    for(u16 i=0;i<count;i++) {
        if(pages[i]>=SC_PAGES)return false;
        for(u16 j=0;j<i;j++)if(pages[i]==pages[j])return false;
    }
    c->wanted_count=count;
    for(u16 i=0;i<count;i++)c->wanted[i]=pages[i];
    for(u16 i=0;i<SC_SLOTS;i++)c->slots[i].pinned=c->slots[i].state && wanted(c,c->slots[i].page);
    return true;
}

u16 sc_require(StreamCache *c,u16 page)
{
    if(page>=SC_PAGES)return SC_INVALID;
    u16 index=c->slot_for[page];
    if(index<SC_SLOTS) {
        if(c->slots[index].state!=SC_READY)return SC_WAIT;
        c->slots[index].touched=++c->clock;c->hits++;return SC_AVAILABLE;
    }
    if(c->pending!=SC_SLOTS)return SC_WAIT;
    u16 victim=SC_SLOTS;u32 age=0;
    for(u16 i=0;i<SC_SLOTS;i++) {
        const ScSlot *s=&c->slots[i];
        if(!s->state) { victim=i;break; }
        if(!s->pinned && (victim==SC_SLOTS || c->clock-s->touched>age)) {
            victim=i;age=c->clock-s->touched;
        }
    }
    if(victim==SC_SLOTS)return SC_FULL;
    if(c->slots[victim].state) {
        c->slot_for[c->slots[victim].page]=SC_SLOTS;c->evictions++;
    }
    c->slot_for[page]=(u8)victim;
    c->slots[victim]=(ScSlot){page,SC_LOADING,wanted(c,page),++c->clock};
    c->pending=victim;c->completed=0;c->misses++;
    return SC_WAIT;
}

u16 sc_step(StreamCache *c,u16 budget,ScRead read,ScQueue queue,void *context)
{
    if(c->pending==SC_SLOTS || c->flight || !budget || !read || !queue)return 0;
    u16 count=SC_PAGE_TILES-c->completed;
    if(count>SC_CHUNK_TILES)count=SC_CHUNK_TILES;
    if(count>budget)count=budget;
    const ScSlot *slot=&c->slots[c->pending];
    if(!read(context,slot->page,c->completed*SC_TILE_BYTES,c->staging,count*SC_TILE_BYTES)) {
        c->read_errors++;return 0;
    }
    u16 tile=SC_CACHE_BASE+c->pending*SC_PAGE_TILES+c->completed;
    if(!queue(context,c->staging,tile,count)) { c->queue_errors++;return 0; }
    c->flight=count;return count;
}

void sc_complete(StreamCache *c,bool drained)
{
    if(!drained || !c->flight)return;
    c->completed+=c->flight;c->flight=0;
    if(c->completed==SC_PAGE_TILES) {
        ScSlot *slot=&c->slots[c->pending];
        slot->state=SC_READY;slot->touched=++c->clock;
        c->pending=SC_SLOTS;
    }
}

bool sc_tile(const StreamCache *c,u16 page,u16 within,u16 *tile)
{
    if(page>=SC_PAGES || within>=SC_PAGE_TILES || !tile)return false;
    u16 index=c->slot_for[page];
    if(index<SC_SLOTS && c->slots[index].state==SC_READY) {
        *tile=SC_CACHE_BASE+index*SC_PAGE_TILES+within;return true;
    }
    return false;
}

bool sc_reset(StreamCache *c)
{
    if(c->flight)return false;
    sc_init(c);return true;
}
