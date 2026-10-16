#include <stdio.h>
#include <string.h>
#include "checks.h"
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"digest")){printf("%lu\n",(unsigned long)trace_digest());return 0;}
    if(!strcmp(argv[1],"query")) {
        int ax,ay,aw,ah,dx,dy,bx,by,bw,bh,vx,vy,point;
        while(scanf("%d%d%d%d%d%d%d%d%d%d%d%d%d",&ax,&ay,&aw,&ah,&dx,&dy,&bx,&by,&bw,&bh,&vx,&vy,&point)==13) {
            CcHit h=point ? cc_ray((CcVec){ax,ay},(CcVec){dx,dy},(CcBox){bx,by,bw,bh}) :
                cc_sweep((CcBox){ax,ay,aw,ah},(CcVec){dx,dy},(CcBox){bx,by,bw,bh},(CcVec){vx,vy});
            printf("%u %u %d %d %ld %ld\n",h.kind,h.time,h.nx,h.ny,(long)h.num,(long)h.den);
        }
        return 0;
    }
    int error=!strcmp(argv[1],"sweep") ? check_sweep() : !strcmp(argv[1],"ray") ? check_ray() :
        !strcmp(argv[1],"slide") ? check_cast_slide() : !strcmp(argv[1],"invalid") ? check_invalid() : check_demo();
    if(error)fprintf(stderr,"Collision contract failed at %d\n",error);
    return error ? 1 : 0;
}
