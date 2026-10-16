/* Simple conservative-band oracle with deliberately straightforward traversal. */
#include "effects.h"
static s16 clamp(s16 x,s16 lo,s16 hi) { return x<lo ? lo : x>hi ? hi : x; }
void fx_reference_select(const FxLab *s,FxRender *out,u16 budget)
{
    *out=(FxRender){0};
    u8 bands[28]={0};
    if (budget>FX_DRAW_MAX) budget=FX_DRAW_MAX;
    s16 base=clamp(s->player_x-152,0,320);
    static const s8 jitter[]={0,2,-2,1,-1,2,-1,0};
    s16 shake=s->shake_enabled && s->shake_left ? jitter[s->shake_left&7] : 0;
    out->camera_x=clamp(base+shake,0,320); out->camera_y=shake;
    /* Critical impact feedback before trails before weather. Rotate ties so
     * a dense cluster does not permanently starve the same particles. */
    u16 start=(u16)(s->ticks%FX_CAPACITY);
    for (s16 rank=2;rank>=0;rank--) for (u16 n=0;n<FX_CAPACITY;n++) {
        u16 slot=(start+n)%FX_CAPACITY;
        const FxParticle *p=&s->particles[slot];
        if (!p->life || p->priority!=rank) continue;
        s16 x=(s16)(p->x/FX_SCALE-out->camera_x-4);
        s16 y=(s16)(p->y/FX_SCALE-out->camera_y-4);
        if (x<0 || x>312 || y<FX_TOP || y>FX_BOTTOM-8) continue;
        out->visible++;
        bool fits=out->count<budget;
        for (u16 band=(u16)y/8;band<=((u16)y+7)/8;band++) if (bands[band]>=FX_LINE_MAX) fits=false;
        if (!fits) { out->skipped++; continue; }
        for (u16 band=(u16)y/8;band<=((u16)y+7)/8;band++) bands[band]++;
        u8 frame;
        if (p->kind==FX_SPARK) frame=p->age<12 ? 0 : 1;
        else if (p->kind==FX_SMOKE) frame=p->age<32 ? 2 : 3;
        else if (p->kind==FX_RAIN) frame=4;
        else if (p->kind==FX_SNOW) frame=5;
        else frame=p->age<12 ? 6 : 7;
        out->items[out->count++]=(FxDraw){x,y,slot,frame};
    }
    for (u16 band=0;band<28;band++) out->bands[band]=bands[band];
}
