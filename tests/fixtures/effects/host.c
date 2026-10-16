#include <stdio.h>
#include <string.h>
#include "checks.h"

static int clocks(void)
{
    FxLab pal,ntsc; fx_init(&pal,0x1234); fx_init(&ntsc,0x1234);
    for (u16 i=0;i<300;i++) fx_frame(&pal,0,50);
    for (u16 i=0;i<360;i++) fx_frame(&ntsc,0,60);
    CHECK(pal.ticks==360 && state_digest(&pal)==state_digest(&ntsc));
    CHECK(!pal.phase && !ntsc.phase);
    fx_frame(&pal,FX_C,50); u8 density=pal.density;
    for (u16 i=0;i<100;i++) fx_frame(&pal,FX_C,50);
    CHECK(pal.density==density);
    pal.ticks=0xFFFFFFFFUL; fx_frame(&pal,0,60); CHECK(pal.ticks<2);
    return 0;
}

static int fairness(void)
{
    FxLab s; FxRender r; bool seen[96]={0}; fx_init(&s,1);
    for (u16 i=0;i<96;i++) CHECK(fx_spawn(&s,FX_TRAIL,100,100,0,0,180));
    for (u16 i=0;i<96;i++) {
        s.ticks=i; FxLab copy=s;
        fx_select(&s,&r,64); CHECK(memcmp(&s,&copy,sizeof(s))==0);
        for (u16 j=0;j<r.count;j++) seen[r.items[j].slot]=true;
    }
    for (u16 i=0;i<96;i++) CHECK(seen[i]);
    fx_select(&s,&r,64); u32 before=state_digest(&s);
    fx_select(&s,&r,0); CHECK(state_digest(&s)==before);
    return 0;
}

static int soak(void)
{
    FxLab s;FxRender r;fx_init(&s,0xFFFF);
    u32 random=817;
    for (u32 i=0;i<100000;i++) {
        random=random*1664525UL+1013904223UL;
        u16 keys=(u16)(random>>16)&0x7F;
        fx_frame(&s,keys,i&1 ? 50 : 60);
        fx_select(&s,&r,(u16)(i%80));
        CHECK(fx_active(&s)<=96 && r.count<=64 && r.count+r.skipped==r.visible);
        CHECK(s.player_x>=16 && s.player_x<=624 && s.player_y>=56 && s.player_y<=176);
        CHECK(r.camera_x>=0 && r.camera_x<=320);
        u8 oracle[224]={0};
        for (u16 n=0;n<r.count;n++) {
            const FxDraw *d=&r.items[n]; CHECK(d->frame<8);
            for (u16 y=(u16)d->y;y<(u16)d->y+8;y++) oracle[y]++;
        }
        for (u16 y=0;y<224;y++) CHECK(oracle[y]==fx_line_count(&r,y) && oracle[y]<=FX_LINE_MAX);
        u16 counted=0,priorities[3]={0};
        for (u16 n=0;n<96;n++) {
            const FxParticle *p=&s.particles[n]; if (!p->life) continue;
            counted++;priorities[p->priority]++;
            CHECK(p->x>=0 && p->x<640*16 && p->y>=40*16 && p->y<192*16 && p->age<p->life);
        }
        CHECK(counted==fx_active(&s) && priorities[0]<=FX_WEATHER_MAX && s.free_count+counted==96);
        bool free_seen[96]={0};
        for(u16 n=0;n<s.free_count;n++) {
            u8 slot=s.free_slots[n];CHECK(slot<96 && !free_seen[slot] && !s.particles[slot].life);
            free_seen[slot]=true;
        }
        for(u16 rank=0;rank<3;rank++) CHECK(priorities[rank]==s.priorities[rank]);
    }
    fx_clear(&s); fx_select(&s,&r,64); CHECK(!r.count);
    return 0;
}

static int reference(void)
{
    FxLab s;FxRender a,b;fx_init(&s,123);u32 random=77;
    for(u16 i=0;i<5000;i++) {
        random=random*1664525UL+1013904223UL;
        fx_frame(&s,(u16)(random>>16)&127,i&1 ? 50 : 60);
        fx_select(&s,&a,i%80);fx_reference_select(&s,&b,i%80);
        CHECK(render_equal(&a,&b));
    }
    return 0;
}

static int scanlines(void)
{
    FxLab s;FxRender r;fx_init(&s,1);
    for(u16 y=40;y<=184;y++) {
        fx_clear(&s);
        for(u16 i=0;i<9;i++)CHECK(fx_spawn(&s,FX_TRAIL,100,(s16)y+4,0,0,100));
        fx_select(&s,&r,64);CHECK(r.count==8 && r.skipped==1);
        for(u16 line=0;line<224;line++)CHECK(fx_line_count(&r,line)==(line>=y && line<y+8 ? 8 : 0));
    }
    return 0;
}

int main(int argc,char **argv)
{
    if (argc!=2) return 2;
    int result;
    if (!strcmp(argv[1],"pool")) result=check_pool();
    else if (!strcmp(argv[1],"motion")) result=check_motion();
    else if (!strcmp(argv[1],"render")) result=check_render();
    else if (!strcmp(argv[1],"clocks")) result=clocks();
    else if (!strcmp(argv[1],"fairness")) result=fairness();
    else if (!strcmp(argv[1],"scanlines")) result=scanlines();
    else if (!strcmp(argv[1],"reference")) result=reference();
    else if (!strcmp(argv[1],"soak")) result=soak();
    else if (!strcmp(argv[1],"digest")) {
        FxLab s;fx_init(&s,0xCAFE);
        for (u16 i=0;i<60;i++) fx_frame(&s,i<30 ? FX_RIGHT : 0,60);
        printf("%lu\n",(unsigned long)state_digest(&s));return 0;
    } else return 2;
    if (result) fprintf(stderr,"Effect contract failed at line %d\n",result);
    return result ? 1 : 0;
}
