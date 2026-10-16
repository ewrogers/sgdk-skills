#ifndef EFFECTS_CHECKS_H
#define EFFECTS_CHECKS_H
#include "effects.h"

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (0)

static int check_pool(void)
{
    FxLab s; fx_init(&s,0);
    CHECK(s.random!=0 && fx_active(&s)==0);
    for (u16 i=0;i<FX_CAPACITY;i++) CHECK(fx_spawn(&s,i<FX_WEATHER_MAX ? FX_RAIN : FX_TRAIL,100,80,0,0,180));
    CHECK(fx_active(&s)==FX_CAPACITY);
    s.particles[27].age=40;
    CHECK(!fx_spawn(&s,FX_SNOW,100,80,0,0,50) && s.rejected==1);
    CHECK(fx_spawn(&s,FX_TRAIL,100,80,0,0,20));
    CHECK(s.particles[27].kind==FX_TRAIL && s.replaced==1);
    CHECK(fx_spawn(&s,FX_SPARK,100,80,0,0,20) && s.particles[0].kind==FX_SPARK);
    s.rejected=0xFFFFFFFFUL;
    CHECK(!fx_spawn(&s,FX_RAIN,100,80,0,0,50) && s.rejected==0xFFFFFFFFUL);
    CHECK(!fx_spawn(&s,FX_KIND_COUNT,100,80,0,0,10));
    CHECK(!fx_spawn(&s,FX_SPARK,-1,80,0,0,10));
    CHECK(!fx_spawn(&s,FX_SPARK,100,80,97,0,10));
    CHECK(!fx_spawn(&s,FX_SPARK,100,80,0,0,181));
    fx_clear(&s); CHECK(fx_active(&s)==0 && !s.shake_left);
    CHECK(fx_spawn(&s,FX_SPARK,100,80,0,0,10));
    return 0;
}

static int check_motion(void)
{
    FxLab s; fx_init(&s,7); s.weather=0;
    CHECK(fx_spawn(&s,FX_SPARK,100,100,16,-16,3));
    fx_frame(&s,0,60);
    CHECK(s.particles[0].x==101*16 && s.particles[0].y==99*16 && s.particles[0].vy==-14);
    fx_frame(&s,0,60); CHECK(s.particles[0].life);
    fx_frame(&s,0,60); CHECK(!s.particles[0].life);
    CHECK(fx_spawn(&s,FX_RAIN,1,80,-96,0,10));
    fx_frame(&s,0,60); CHECK(!s.particles[0].life);
    fx_clear(&s);
    CHECK(fx_spawn(&s,FX_TRAIL,100,100,0,0,24));
    fx_frame(&s,FX_RIGHT,60);
    CHECK(s.player_x==162 && s.particles[0].x==1600);
    fx_frame(&s,FX_A,60); u16 random=s.random;
    fx_frame(&s,FX_A,60); CHECK(s.random==random); /* One press, not each held frame. */
    CHECK(s.shake_left>0);
    fx_frame(&s,FX_START,60); u32 ticks=s.ticks;
    for (u16 i=0;i<30;i++) fx_frame(&s,FX_RIGHT|FX_A,60);
    CHECK(s.ticks==ticks && s.paused);
    fx_frame(&s,FX_START|FX_A,60); CHECK(!s.paused && s.random==random);
    fx_frame(&s,0,60); fx_frame(&s,FX_UP|FX_START,60);
    CHECK(!s.shake_enabled && !s.shake_left && !s.paused);
    return 0;
}

static int check_bursts(void)
{
    FxLab s;fx_init(&s,7);s.weather=0;
    CHECK(fx_spawn(&s,FX_SNOW,100,80,0,0,180));
    CHECK(fx_spawn(&s,FX_TRAIL,100,80,0,0,24));
    CHECK(fx_spawn(&s,FX_SPARK,100,80,0,0,45)); /* Separate fountain. */
    for (u16 n=0;n<100;n++) {
        fx_burst(&s,160,112);
        u16 bursts=0,sparks=0,smoke=0;
        for (u16 i=0;i<FX_CAPACITY;i++) {
            const FxParticle *p=&s.particles[i];
            if (!p->life || !p->burst) continue;
            bursts++;sparks+=p->kind==FX_SPARK;smoke+=p->kind==FX_SMOKE;
        }
        CHECK(bursts==12 && sparks==8 && smoke==4 && s.occupied==15);
        CHECK(s.particles[0].life && s.particles[1].life && s.particles[2].life);
        CHECK(s.free_count+s.occupied==FX_CAPACITY && !s.rejected && !s.replaced);
    }
    u16 random=s.random;fx_burst(&s,-1,112);CHECK(s.random==random && s.occupied==15);
    fx_clear(&s);fx_burst(&s,160,112);CHECK(s.occupied==12);
    return 0;
}

static int check_render(void)
{
    FxLab s; FxRender r; fx_init(&s,1); s.weather=0;
    for (u16 i=0;i<FX_CAPACITY;i++) CHECK(fx_spawn(&s,FX_TRAIL,100,100,0,0,180));
    fx_select(&s,&r,FX_DRAW_MAX);
    CHECK(r.visible==FX_CAPACITY && r.count==8 && r.skipped==FX_CAPACITY-8);
    CHECK(fx_spawn(&s,FX_SPARK,100,100,0,0,30));
    fx_select(&s,&r,FX_DRAW_MAX); CHECK(r.items[0].slot==0);
    for (u16 i=0;i<224;i++) CHECK(fx_line_count(&r,i)<=FX_LINE_MAX);
    fx_select(&s,&r,0); CHECK(!r.count && r.skipped==FX_CAPACITY);
    fx_clear(&s);
    /* Distributed rectangles can fill the whole frame budget. */
    for (u16 i=0;i<FX_CAPACITY;i++) CHECK(fx_spawn(&s,FX_TRAIL,12+(i%8)*36,48+(i/8)*16,0,0,180));
    fx_select(&s,&r,0xFFFF); CHECK(r.count==FX_DRAW_MAX);
    for (u16 i=0;i<r.count;i++) {
        CHECK(r.items[i].x>=0 && r.items[i].x<=312 && r.items[i].y>=FX_TOP && r.items[i].y<=FX_BOTTOM-8);
        for (u16 j=0;j<i;j++) CHECK(r.items[i].slot!=r.items[j].slot);
    }
    s.player_x=16; s.shake_left=18; fx_select(&s,&r,64); CHECK(r.camera_x>=0);
    s.player_x=624; fx_select(&s,&r,64); CHECK(r.camera_x<=320);
    s.shake_enabled=false; fx_select(&s,&r,64); CHECK(r.camera_x==320 && !r.camera_y);
    return 0;
}

void fx_reference_select(const FxLab *s,FxRender *out,u16 budget);
static bool render_equal(const FxRender *a,const FxRender *b)
{
    if(a->camera_x!=b->camera_x || a->camera_y!=b->camera_y || a->count!=b->count ||
       a->visible!=b->visible || a->skipped!=b->skipped)return false;
    for(u16 i=0;i<28;i++)if(a->bands[i]!=b->bands[i])return false;
    for(u16 i=0;i<a->count;i++)if(a->items[i].x!=b->items[i].x || a->items[i].y!=b->items[i].y ||
       a->items[i].slot!=b->items[i].slot || a->items[i].frame!=b->items[i].frame)return false;
    return true;
}

static u32 state_digest(const FxLab *s)
{
    u32 hash=2166136261UL;
    for (u16 i=0;i<FX_CAPACITY;i++) {
        const FxParticle *p=&s->particles[i];
        const u16 words[]={(u16)p->x,(u16)p->y,(u16)p->vx,(u16)p->vy,p->age,p->life,p->kind,p->priority};
        for (u16 n=0;n<8;n++) hash=(hash^words[n])*16777619UL;
    }
    return hash ^ s->ticks ^ s->random ^ (u16)s->player_x ^ (u16)s->player_y;
}
#endif
