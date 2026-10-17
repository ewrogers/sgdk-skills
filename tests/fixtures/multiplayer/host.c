#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "checks.h"
int main(int argc,char **argv)
{
    if(argc==2 && !strcmp(argv[1],"camera")) {
        char line[256];
        while(fgets(line,sizeof(line),stdin)) {
            long values[11];char *cursor=line;
            for(u16 i=0;i<11;i++){char *end;values[i]=strtol(cursor,&end,10);if(end==cursor || values[i]<0 || values[i]>512)return 2;cursor=end;}
            CpGame g;cp_init(&g);g.camera_x=(s16)values[1];g.camera_y=(s16)values[2];
            for(u16 i=0;i<4;i++){g.player[i].device=i;g.player[i].hp=(values[0]&(1u<<i)) ? 3 : 0;g.player[i].respawn=g.player[i].hp ? 0 : 1;g.player[i].x=(s16)values[3+i*2];g.player[i].y=(s16)values[4+i*2];}
            cp_camera(&g);printf("%d %d\n",g.camera_x,g.camera_y);
        }
        return 0;
    }
    int e=check_routes();if(!e)e=check_session();if(!e)e=check_respawn();if(!e)e=check_camera();
    if(!e)e=check_pickups_and_pause();
    if(!e)e=check_stun();
    if(!e)e=check_timing();
    u32 trace=0;
    if(!e)e=check_soak(argc>1 && !strcmp(argv[1],"trace") ? 256 : 20000,&trace);
    if(e){fprintf(stderr,"Co-op contract failed at line %d\n",e);return 1;}
    printf("%lu\n",(unsigned long)trace);return 0;
}
