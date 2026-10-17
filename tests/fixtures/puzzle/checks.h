#include "puzzle.h"
#include <string.h>
static int bytecmp(const void *a,const void *b,u16 count)
{const u8 *x=a,*y=b;for(u16 i=0;i<count;i++)if(x[i]!=y[i])return 1;return 0;}
#define CHECK(x) do{if(!(x))return __LINE__;}while(0)
static int contracts(void)
{
    PzBoard b,before;const char *solutions[]={"RRRRRLLLLDDRRRRDRR","DRUURRRRRR","DRRUUULDLDRRRDURRRR"};
    for(u16 room=0;room<3;room++) {
        CHECK(pz_load(&b,room));CHECK(pz_valid(&b));CHECK(!pz_complete(&b));
        const char *s=solutions[room];
        for(u16 i=0;s[i];i++) {
            before=b;u16 d=s[i]=='L' ? PZ_LEFT : s[i]=='R' ? PZ_RIGHT : s[i]=='U' ? PZ_UP : PZ_DOWN;
            CHECK(pz_move(&b,d)!=PZ_BLOCKED);CHECK(pz_valid(&b));
            PzState after=b.state;CHECK(pz_undo(&b));CHECK(!bytecmp(&b.state,&before.state,sizeof(PzState)));
            CHECK(pz_move(&b,d)!=PZ_BLOCKED);CHECK(!bytecmp(&after,&b.state,sizeof(PzState)));
        }
        CHECK(pz_complete(&b));before=b;CHECK(pz_move(&b,PZ_LEFT)==PZ_BLOCKED);CHECK(!bytecmp(&b,&before,sizeof(b)));
        while(pz_undo(&b)){}CHECK(!bytecmp(&b.state,&b.initial,sizeof(PzState)));
    }
    CHECK(pz_load(&b,0));before=b;CHECK(!pz_load(&b,3));CHECK(!pz_parse(&b,NULL));CHECK(!pz_parse(&b,"bad"));CHECK(!bytecmp(&b,&before,sizeof(b)));
    CHECK(pz_move(&b,44)==PZ_BLOCKED);CHECK(!bytecmp(&b,&before,sizeof(b)));
    char map[PZ_CELLS+1];memset(map,'.',PZ_CELLS);map[PZ_CELLS]=0;map[13]='@';map[106]='E';
    CHECK(pz_parse(&b,map));
    for(u16 i=0;i<70;i++)CHECK(pz_move(&b,i&1 ? PZ_LEFT : PZ_RIGHT)==PZ_WALK);
    CHECK(b.count==64);for(u16 i=0;i<64;i++)CHECK(pz_undo(&b));CHECK(!pz_undo(&b));CHECK(b.state.moves==6);
    pz_restart(&b);CHECK(!b.count && !b.head && !b.state.moves);CHECK(pz_valid(&b));
    b.state.player=11;before=b;CHECK(pz_move(&b,PZ_RIGHT)==PZ_BLOCKED);CHECK(!bytecmp(&b,&before,sizeof(b)));
    b.state.player=0;before=b;CHECK(pz_move(&b,PZ_UP)==PZ_BLOCKED);CHECK(pz_move(&b,PZ_LEFT)==PZ_BLOCKED);CHECK(!bytecmp(&b,&before,sizeof(b)));
    map[14]='$';map[15]='$';CHECK(pz_parse(&b,map));before=b;CHECK(pz_move(&b,PZ_RIGHT)==PZ_BLOCKED);CHECK(!bytecmp(&b,&before,sizeof(b)));
    map[15]='#';CHECK(pz_parse(&b,map));before=b;CHECK(pz_move(&b,PZ_RIGHT)==PZ_BLOCKED);CHECK(!bytecmp(&b,&before,sizeof(b)));
    map[15]='~';CHECK(pz_parse(&b,map));CHECK(pz_move(&b,PZ_RIGHT)==PZ_BRIDGE);CHECK(b.state.crate[0]==PZ_NONE && b.state.filled==1);
    CHECK(pz_move(&b,PZ_RIGHT)==PZ_WALK);CHECK(pz_undo(&b));CHECK(pz_undo(&b));CHECK(!b.state.filled && b.state.crate[0]==14);
    map[14]='D';map[15]='.';map[13]='@';map[12]='o';CHECK(pz_parse(&b,map));before=b;
    CHECK(pz_move(&b,PZ_RIGHT)==PZ_BLOCKED);CHECK(!bytecmp(&b,&before,sizeof(b)));
    CHECK(pz_move(&b,PZ_LEFT)==PZ_WALK);CHECK(pz_gate_open(&b,&b.state));
    CHECK(pz_move(&b,PZ_RIGHT)==PZ_WALK);CHECK(!pz_gate_open(&b,&b.state));
    map[12]='.';map[13]='o';map[1]='@';CHECK(pz_parse(&b,map));
    CHECK(pz_move(&b,PZ_DOWN)==PZ_WALK);CHECK(pz_move(&b,PZ_RIGHT)==PZ_WALK);CHECK(pz_gate_open(&b,&b.state));
    CHECK(pz_move(&b,PZ_RIGHT)==PZ_WALK);CHECK(!pz_gate_open(&b,&b.state));CHECK(pz_undo(&b));CHECK(pz_gate_open(&b,&b.state));
    before=b;map[2]='@';CHECK(!pz_parse(&b,map));CHECK(!bytecmp(&b,&before,sizeof(b)));
    map[2]='?';CHECK(!pz_parse(&b,map));CHECK(!bytecmp(&b,&before,sizeof(b)));
    for(u16 i=2;i<11;i++)map[i]='~';
    CHECK(!pz_parse(&b,map));CHECK(!bytecmp(&b,&before,sizeof(b)));
    return 0;
}
static int soak(u16 frames,u32 *digest)
{
    PzBoard b;pz_load(&b,0);u32 rng=157u,h=2166136261u;
    for(u16 i=0;i<frames;i++) {
        rng=rng*1664525u+1013904223u;u16 action=(u16)(rng>>27);
        if(action<24) {
            PzBoard before=b;u16 result=pz_move(&b,action&3);
            if(result==PZ_BLOCKED)CHECK(!bytecmp(&b,&before,sizeof(b)));
        } else if(action<28)pz_undo(&b);
        else if(action==28)pz_restart(&b);
        else if(action==29)CHECK(pz_load(&b,(u16)((rng>>16)%3)));
        CHECK(pz_valid(&b));h=(h^pz_digest(&b))*16777619u;
    }
    *digest=h;return 0;
}
