#include <stdio.h>
#include <stdlib.h>
#include "checks.h"
#include "blocks_checks.h"
int main(int argc,char **argv)
{
    if(argc==2 && !strcmp(argv[1],"shapes")) {
        for(u16 p=0;p<7;p++)for(u16 r=0;r<4;r++)printf("%u\n",bl_shape(p,r));
        return 0;
    }
    if(argc==3 && !strcmp(argv[1],"rows")) {
        if(strlen(argv[2])!=BL_W*BL_H)return 2;
        BlGame g;bl_init(&g,1);
        for(u16 i=0;i<BL_W*BL_H;i++){if(argv[2][i]<'0' || argv[2][i]>'7')return 2;g.cells[i]=(u8)(argv[2][i]-'0');}
        printf("%u\n",bl_clear(&g));
        for(u16 i=0;i<BL_W*BL_H;i++)putchar('0'+g.cells[i]);
        putchar('\n');return 0;
    }
    if(argc==3 && !strcmp(argv[1],"room")) {
        u16 room=(u16)strtoul(argv[2],NULL,10);if(room>=3)return 2;puts(pz_rooms[room]);return 0;
    }
    if(argc==4 && !strcmp(argv[1],"replay")) {
        PzBoard b;if(!pz_load(&b,(u16)strtoul(argv[2],NULL,10)))return 2;
        for(const char *s=argv[3];*s;s++) {
            u16 result=*s=='Z' ? (pz_undo(&b) ? 6 : 0) : *s=='X' ? (pz_restart(&b),7) :
                pz_move(&b,*s=='L' ? PZ_LEFT : *s=='R' ? PZ_RIGHT : *s=='U' ? PZ_UP : PZ_DOWN);
            printf("%u %u %u %u",result,b.state.player,b.state.filled,b.state.moves);
            for(u16 i=0;i<PZ_CRATES;i++)printf(" %u",b.state.crate[i]);
            printf("\n");
        }
        return 0;
    }
    int e=contracts();if(!e)e=block_contracts();u32 h=0,bh=0;if(!e)e=soak(argc>1 ? 256 : 20000,&h);
    if(!e)e=block_soak(argc>1 ? 256 : 20000,&bh);
    h^=bh;
    if(e){fprintf(stderr,"Puzzle contract failed at line %d\n",e);return 1;}
    printf("%lu\n",(unsigned long)h);return 0;
}
