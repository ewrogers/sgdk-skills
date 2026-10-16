#include "view.h"
#include "resources.h"
#include "ui_tiles.h"

u16 image_base,panel_base,solid_base,gold_font_base,title_font_base,actor_base;
static u16 cursor,pin_base,mask_base[2];
static UITiles ui_tiles;
static struct {const Image *image;u16 base;} images[32];
static u16 image_count;
static void ui_put(u16 tile,u16 x,u16 y){ui_tiles_set(&ui_tiles,x,y,tile);}
static void ui_flush(TransferMethod method)
{
    for(u16 y=0;y<UI_HEIGHT;y++)if(ui_tiles.dirty[y])
    {
        VDP_setTileMapDataRow(BG_A,ui_tiles.cells+y*UI_WIDTH,y,0,UI_WIDTH,method);
        ui_tiles.dirty[y]=false;
    }
}

static Screen previous=255;
static const Image *zone_art[]={&art_town,&art_dungeon,&art_forest,&art_coast};
static bool fits=true,lighting=false;

static u16 reserve(u16 tiles)
{
    u16 start=cursor;
    if((u32)cursor+tiles>(u32)TILE_FONT_INDEX){fits=false;return 0;}
    cursor+=tiles;return start;
}

u16 art_load(const Image *image,u16 palette,bool load_palette)
{
    for(u16 i=0;i<image_count;i++)if(images[i].image==image)return images[i].base;
    if(image_count>=32){fits=false;return 0;}
    u16 base=reserve(image->tileset->numTile);
    if(!fits)return 0;
    images[image_count].image=image;images[image_count++].base=base;
    VDP_loadTileSet(image->tileset,base,DMA);
    if(load_palette)PAL_setPalette(palette,image->palette->data,DMA);
    return base;
}

static void draw_image(const Image *image,u16 palette,u16 x,u16 y)
{
    image_base=art_load(image,palette,false);
    VDP_setTileMapEx(BG_B,image->tilemap,TILE_ATTR_FULL(palette,lighting,FALSE,FALSE,image_base),x,y,0,0,image->tilemap->w,image->tilemap->h,DMA);
    PAL_setPalette(PAL0,image->palette->data,DMA);PAL_setPalette(PAL1,image->palette->data+16,DMA);
}

void ui_image(const Image *image,u16 palette,u16 x,u16 y)
{
    const u16 base=art_load(image,palette,false);
    if(!fits)return;
    for(u16 dy=0;dy<image->tilemap->h;dy++)for(u16 dx=0;dx<image->tilemap->w;dx++)
    {
        u16 source=image->tilemap->tilemap[dy*image->tilemap->w+dx];
        ui_put((source&0xF800)|((source&0x07FF)+base)|TILE_ATTR(palette,TRUE,FALSE,FALSE),x+dx,y+dy);
    }
}

void ui_text(const char *text,u16 x,u16 y,bool gold)
{
    u16 base=gold?gold_font_base:TILE_FONT_INDEX;
    for(u16 i=0;text[i]&&x+i<40;i++)
    {
        unsigned char c=(unsigned char)text[i];
        ui_put(TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,base+((c>=32&&c<128)?c-32:0)),x+i,y);
    }
}
void ui_number(u16 number,u16 x,u16 y,bool gold)
{
    char digits[6];u16 n=5;digits[n]=0;
    do{digits[--n]='0'+number%10;number/=10;}while(number);
    ui_text(digits+n,x,y,gold);
}
void ui_fill(u16 x,u16 y,u16 w,u16 h,u16 color)
{
    for(u16 dy=0;dy<h;dy++)for(u16 dx=0;dx<w;dx++)
        ui_put(TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,solid_base+color),x+dx,y+dy);
}
void ui_panel(u16 x,u16 y,u16 w,u16 h)
{
    if(w<4||h<4)return;
    ui_fill(x,y,w,h,1);
    for(u16 dy=0;dy<h;dy++)for(u16 dx=0;dx<w;dx++)
    {
        if(dx>=2&&dx<w-2&&dy>=2&&dy<h-2)continue;
        u16 sx=dx<2?dx:dx>=w-2?8-(w-dx):2+(dx-2)%4;
        u16 sy=dy<2?dy:dy>=h-2?8-(h-dy):2+(dy-2)%4;
        u16 source=art_panel.tilemap->tilemap[sy*8+sx];
        u16 attr=(source&0xF800)|((source&0x07FF)+panel_base)|TILE_ATTR(PAL3,TRUE,FALSE,FALSE);
        ui_put(attr,x+dx,y+dy);
    }
}
void ui_bar(u16 x,u16 y,u16 w,u16 value,u16 maximum,u16 color)
{
    ui_fill(x,y,w,1,2);u16 full=maximum?(u32)value*w/maximum:0;
    if(full>w)full=w;
    if(full)ui_fill(x,y,full,1,color);
}
void ui_large(const char *text,u16 x,u16 y)
{
    for(u16 i=0;text[i];i++)if(text[i]>='A'&&text[i]<='Z')
    {
        u16 tile=(text[i]-'A')*2+title_font_base;
        for(u16 dy=0;dy<2;dy++)for(u16 dx=0;dx<2;dx++)
            ui_put(TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,tile+dx+dy*52),x+i*2+dx,y+dy);
    }
}

void view_init(void)
{
    VDP_setScreenWidth320();VDP_setScreenHeight224();VDP_setPlaneSize(64,32,TRUE);
    VDP_setWindowHPos(FALSE,0);VDP_setWindowVPos(FALSE,0);VDP_setHilightShadow(FALSE);
}

static void begin(const Game *g)
{
    SYS_disableInts();VDP_setEnable(FALSE);fits=true;image_count=0;
    ui_tiles_clear(&ui_tiles);
    lighting=g->screen==TITLE;VDP_setHilightShadow(lighting);
    VDP_setHorizontalScroll(BG_A,0);VDP_setHorizontalScroll(BG_B,0);
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_clearSprites();VDP_updateSprites(1,DMA);
    VDP_loadTileSet(&art_font_ivory,TILE_FONT_INDEX,DMA);
    cursor=TILE_USER_INDEX;
    gold_font_base=reserve(art_font_gold.numTile);VDP_loadTileSet(&art_font_gold,gold_font_base,DMA);
    title_font_base=0;
    if(g->screen==TITLE)
    {title_font_base=reserve(art_font_title.numTile);VDP_loadTileSet(&art_font_title,title_font_base,DMA);}
    solid_base=reserve(16);
    u32 solid[128];for(u16 color=0;color<16;color++)for(u16 row=0;row<8;row++)solid[color*8+row]=0x11111111UL*color;
    VDP_loadTileData(solid,solid_base,16,DMA);
    PAL_setPalette(PAL3,art_panel.palette->data,DMA);
    panel_base=0;
    if(g->screen!=FIELD&&g->screen!=INTRO)panel_base=art_load(&art_panel,PAL3,false);
    VDP_setBackgroundColor(49);
    if(g->screen==FIELD||g->screen==DIALOGUE)
    {
        draw_image(zone_art[g->zone],PAL0,0,2);PAL_setPalette(PAL2,art_hero_idle.palette->data,DMA);
        actor_base=reserve(108);actors_reset();
        ui_text(zone_name(g->zone),2,0,true);
        ui_text("A Talk   B Menu   C Map",5,27,false);
    }
    else if(g->screen==TITLE)
    {
        draw_image(&art_title,PAL0,4,1);
        const SpriteDefinition *masks[]={&art_title_mask0,&art_title_mask1};
        for(u16 i=0;i<2;i++)
        {
            const TileSet *tiles=masks[i]->animations[0]->frames[0]->tileset;
            mask_base[i]=reserve(tiles->numTile);VDP_loadTileSet(tiles,mask_base[i],DMA);
        }
    }
    else if(g->screen==WORLD)
    {
        draw_image(&art_world,PAL0,4,3);
        const TileSet *tiles=art_travel_pin.animations[0]->frames[0]->tileset;
        pin_base=reserve(tiles->numTile);VDP_loadTileSet(tiles,pin_base,DMA);
    }
    else if(g->screen==INTRO)
    {
        draw_image(&art_intro_sky,PAL0,0,2);
        ui_image(&art_intro_front,PAL2,0,18);PAL_setPalette(PAL2,art_intro_front.palette->data,DMA);
        /* Narrow solid side bars conceal the small parallax crop at the edges. */
        ui_fill(0,0,2,28,1);ui_fill(38,0,2,28,1);
    }
    else
    {
        ui_panel(0,0,40,28);
        PAL_setPalette(PAL2,art_hero_idle.palette->data,DMA);
        PAL_setPalette(PAL1,art_icon_sword.palette->data,DMA);
    }
    ui_draw(g);
    if(!fits){VDP_clearPlane(BG_A,TRUE);ui_text("Art reservation exceeds VRAM",2,12,false);}
    ui_flush(DMA);VDP_setEnable(TRUE);SYS_enableInts();
}

void view_frame(Game *g)
{
    if(g->dirty||previous!=g->screen)
    {begin(g);g->dirty=false;g->ui_dirty=false;previous=g->screen;}
    else if(g->ui_dirty)
    {
        /* Input only changes cached tilemap cells. No blanking, palette writes
         * or resource uploads: changed rows are copied during VBlank below. */
        if(g->screen!=DIALOGUE)
        {
            ui_tiles_clear(&ui_tiles);
            if(g->screen!=TITLE&&g->screen!=WORLD)ui_panel(0,0,40,28);
        }
        ui_draw(g);g->ui_dirty=false;
    }
    if(g->screen==FIELD||g->screen==DIALOGUE)actors_frame(g);
    if(g->screen==WORLD)
    {
        static const u16 pins[4][2]={{11,12},{31,7},{19,6},{32,16}};
        u16 phase=(g->ticks/8)%8,bob=phase<4?phase:7-phase;
        VDP_setSpriteFull(0,pins[g->selection][0]*8,pins[g->selection][1]*8-14-bob,
            SPRITE_SIZE(2,2),TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,pin_base),0);
        VDP_updateSprites(1,DMA_QUEUE);
    }
    else if(g->screen==TITLE)
    {
        const SpriteDefinition *masks[]={&art_title_mask0,&art_title_mask1};u16 count=0;
        for(u16 i=0;i<2;i++)
        {
            const AnimationFrame *frame=masks[i]->animations[0]->frames[0];u16 tile=mask_base[i];
            for(u16 j=0;j<(frame->numSprite&0x7F);j++)
            {
                const FrameVDPSprite *piece=&frame->frameVDPSprites[j];
                VDP_setSpriteFull(count,32+i*128+piece->offsetX,64+piece->offsetY,piece->size,
                    TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,tile),count+1);
                count++;tile+=piece->numTile;
            }
        }
        if(count)VDP_setSpriteLink(count-1,0);
        VDP_updateSprites(count,DMA_QUEUE);
    }
    ui_tick(g);
    if(g->screen==INTRO)
    {
        u16 travel=(g->screen_age/12)%16;
        VDP_setHorizontalScroll(BG_B,-(s16)(travel<=8?travel:16-travel));
        if(g->screen_age%240==0)ui_draw(g);
        u16 phase=g->screen_age%240,level=phase<32?phase*7/32:phase>208?(240-phase)*7/32:7;
        u16 blue=1+level*6/7;
        u16 color=(level<<1)|(level<<5)|(blue<<9);
        PAL_setColors(63,&color,1,DMA_QUEUE_COPY);
    }
    ui_flush(DMA_QUEUE_COPY);
}
