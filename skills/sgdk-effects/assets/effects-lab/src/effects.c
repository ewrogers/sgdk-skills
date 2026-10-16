#include "effects.h"

static u16 next_random(FxLab *s)
{
    u16 x=s->random;
    x^=(u16)(x<<7); x^=(u16)(x>>9); x^=(u16)(x<<8);
    s->random=x;
    return x;
}

static void increment(u32 *value) { if (*value<0xFFFFFFFFUL) ++*value; }
static s16 clamp(s16 x,s16 lo,s16 hi) { return x<lo ? lo : x>hi ? hi : x; }
static u8 priority(u8 kind) { return kind==FX_SPARK || kind==FX_SMOKE ? 2 : kind==FX_TRAIL ? 1 : 0; }

void fx_clear(FxLab *s)
{
    for (u16 i=0;i<FX_CAPACITY;i++) {
        s->particles[i]=(FxParticle){0};
        s->free_slots[FX_CAPACITY-1-i]=(u8)i;
    }
    s->free_count=FX_CAPACITY;
    s->shake_left=0; s->occupied=0;
    for (u16 i=0;i<3;i++) s->priorities[i]=0;
}

void fx_init(FxLab *s,u16 seed)
{
    *s=(FxLab){0};
    fx_clear(s);
    s->random=seed ? seed : 1;
    s->player_x=160; s->player_y=112;
    s->density=1; s->weather=1; s->shake_enabled=true;
}

static void retire(FxLab *s,u16 slot)
{
    FxParticle *p=&s->particles[slot];
    p->life=0;s->occupied--;s->priorities[p->priority]--;
    s->free_slots[s->free_count++]=(u8)slot;
}

static u16 acquire(FxLab *s,u8 incoming)
{
    if (!incoming && (s->occupied==FX_CAPACITY || s->priorities[0]>=FX_WEATHER_MAX)) {
        increment(&s->rejected); return FX_CAPACITY;
    }
    u16 slot;
    if (s->free_count) {
        slot=s->free_slots[--s->free_count];
        s->occupied++;
    } else {
        u8 lowest=0;
        while (!s->priorities[lowest]) lowest++;
        if (incoming<=lowest) { increment(&s->rejected); return FX_CAPACITY; }
        u16 oldest=0;
        slot=FX_CAPACITY;
        for (u16 i=0;i<FX_CAPACITY;i++) {
            const FxParticle *p=&s->particles[i];
            if (p->priority==lowest && (slot==FX_CAPACITY || p->age>oldest)) { slot=i;oldest=p->age; }
        }
        s->priorities[lowest]--;
        increment(&s->replaced);
    }
    s->priorities[incoming]++;
    return slot;
}

bool fx_spawn(FxLab *s,u8 kind,s16 x,s16 y,s16 vx,s16 vy,u16 life)
{
    if (kind>=FX_KIND_COUNT || !life || life>180 || x<0 || x>=FX_WORLD_WIDTH ||
        y<FX_TOP || y>=FX_BOTTOM || vx<-96 || vx>96 || vy<-96 || vy>96) return false;
    u8 incoming=priority(kind);
    u16 slot=acquire(s,incoming);
    if (slot==FX_CAPACITY) return false;
    static const u8 first[]={0,2,4,5,6};
    s->particles[slot]=(FxParticle){(s16)(x*FX_SCALE),(s16)(y*FX_SCALE),vx,vy,0,life,kind,incoming,first[kind],0};
    return true;
}

void fx_burst(FxLab *s,s16 x,s16 y)
{
    static const s16 directions[16][2]={
        {64,0},{59,24},{45,45},{24,59},{0,64},{-24,59},{-45,45},{-59,24},
        {-64,0},{-59,-24},{-45,-45},{-24,-59},{0,-64},{24,-59},{45,-45},{59,-24}};
    if (x<0 || x>=FX_WORLD_WIDTH || y<FX_TOP || y>=FX_BOTTOM) return;
    /* One lantern emitter: restarting it releases its previous burst, keeping
     * rapid button presses from accumulating unbounded overlapping explosions. */
    for (u16 slot=0;slot<FX_CAPACITY;slot++)
        if (s->particles[slot].life && s->particles[slot].burst) retire(s,slot);
    s16 qx=(s16)(x*FX_SCALE),qy=(s16)(y*FX_SCALE);
    for (u16 i=0;i<12;i++) {
        u16 n=next_random(s);
        const s16 *d=directions[i<8 ? i*2 : (i-8)*4+1];
        s16 speed=(s16)(8+(n&7));
        u16 slot=acquire(s,2);
        if (slot==FX_CAPACITY) continue;
        s->particles[slot]=(FxParticle){qx,qy,(s16)(d[0]*speed/16),(s16)(d[1]*speed/16),
            0,i<8 ? 40 : 70,i<8 ? FX_SPARK : FX_SMOKE,2,i<8 ? 0 : 2,1};
    }
    if (s->shake_enabled) s->shake_left=18;
}

static void tick(FxLab *s,u16 held)
{
    s->ticks++;
    if (s->shake_left) s->shake_left--;
    s16 dx=(held&FX_RIGHT ? 2 : 0)-(held&FX_LEFT ? 2 : 0);
    s16 dy=(held&FX_DOWN ? 2 : 0)-(held&FX_UP ? 2 : 0);
    s->player_x=clamp((s16)(s->player_x+dx),16,FX_WORLD_WIDTH-16);
    s->player_y=clamp((s16)(s->player_y+dy),56,176);
    for (u16 i=0;i<FX_CAPACITY;i++) {
        FxParticle *p=&s->particles[i];
        if (!p->life) continue;
        if (++p->age>=p->life) { retire(s,i); continue; }
        static const u8 first[]={0,2,4,5,6},threshold[]={12,32,255,255,12};
        p->frame=first[p->kind]+(p->age>=threshold[p->kind]);
        p->x+=p->vx; p->y+=p->vy;
        if (p->kind==FX_SPARK && p->vy<96) p->vy+=2;
        if (p->kind==FX_SMOKE && !(p->age&3)) { p->vx=p->vx*7/8; p->vy=p->vy*7/8-1; }
        if (p->kind==FX_SNOW && !(p->age&7)) p->vx=(p->age&16) ? 4 : -4;
        if (p->x<0 || p->x>=FX_WORLD_WIDTH*FX_SCALE || p->y<FX_TOP*FX_SCALE || p->y>=FX_BOTTOM*FX_SCALE)
            { retire(s,i); }
    }
    static const u8 rates[]={12,8,5,3};
    if (!(s->ticks%rates[s->density])) {
        /* Trails are world-space snapshots, not later copies of the emitter. */
        if (dx || dy) fx_spawn(s,FX_TRAIL,s->player_x-6,s->player_y,0,0,24);
        if (s->weather) {
            s16 camera=clamp(s->player_x-152,0,320);
            s16 x=(s16)(camera+(next_random(s)%312));
            fx_spawn(s,s->weather==1 ? FX_RAIN : FX_SNOW,x,FX_TOP, s->weather==1 ? -8 : 4,
                     s->weather==1 ? 64 : 12,s->weather==1 ? 42 : 180);
        }
    }
    if (!(s->ticks%6)) fx_spawn(s,FX_SPARK,440,148,(s16)(next_random(s)%33)-16,-48,45);
}

__attribute__((noinline)) void fx_frame(FxLab *s,u16 held,u16 hz)
{
    u16 pressed=held & (u16)~s->held;
    s->held=held;
    if (pressed&FX_START) {
        if (held&FX_UP) { s->shake_enabled=!s->shake_enabled; s->shake_left=0; }
        else s->paused=!s->paused;
    }
    if (s->paused) return;
    if (pressed&FX_B) s->weather=(s->weather+1)%3;
    if (pressed&FX_C) s->density=(s->density+1)%4;
    if (pressed&FX_A) fx_burst(s,s->player_x,s->player_y);
    if (hz!=50 && hz!=60) hz=60;
    s->phase+=60;
    while (s->phase>=hz) { s->phase-=hz; tick(s,held); }
}

u16 fx_active(const FxLab *s) { return s->occupied; }

__attribute__((noinline)) void fx_select(const FxLab *restrict s,FxRender *restrict out,u16 budget)
{
    u16 count=0,visible=0,skipped=0;
    for (u16 band=0;band<28;band++) out->bands[band]=0;
    if (budget>FX_DRAW_MAX) budget=FX_DRAW_MAX;
    s16 base=clamp(s->player_x-152,0,320);
    static const s8 jitter[]={0,2,-2,1,-1,2,-1,0};
    s16 shake=s->shake_enabled && s->shake_left ? jitter[s->shake_left&7] : 0;
    s16 camera_x=clamp(base+shake,0,320),camera_y=shake;
    out->camera_x=camera_x;out->camera_y=camera_y;
    /* Critical impact feedback before trails before weather. Rotate ties so
     * a dense cluster does not permanently starve the same particles. */
    u16 start=(u16)(s->ticks%FX_CAPACITY);
    for (u16 rank=3;rank--;) {
        if (!s->priorities[rank]) continue;
        const FxParticle *p=&s->particles[start];
        u16 index=start;
        for (u16 n=0;n<FX_CAPACITY;n++,p++,index++) {
            if (index==FX_CAPACITY) { index=0; p=s->particles; }
            if (!p->life || p->priority!=(u8)rank) continue;
            s16 x=(s16)((u16)p->x/FX_SCALE-camera_x-4);
            s16 y=(s16)((u16)p->y/FX_SCALE-camera_y-4);
            if (x<0 || x>312 || y<FX_TOP || y>FX_BOTTOM-8) continue;
            visible++;
            if (count>=budget) { skipped++; continue; }
            /* Conservative broad phase: a sprite occupies one or two 8px
             * bands. Limiting every touched band also bounds each real line. */
            u16 first=(u16)y/8,last=((u16)y+7)/8;
            if (out->bands[first]>=FX_LINE_MAX || out->bands[last]>=FX_LINE_MAX) { skipped++; continue; }
            out->bands[first]++;
            if (last!=first) out->bands[last]++;
            FxDraw *draw=&out->items[count++];
            draw->x=x;draw->y=y;draw->slot=index;draw->frame=(u8)p->frame;
        }
    }
    out->count=count;out->visible=visible;out->skipped=skipped;
}
