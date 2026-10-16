#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "checks.h"
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"digest")){printf("%lu\n",(unsigned long)trace_digest());return 0;}
    if(!strcmp(argv[1],"query")) {
        char line[256];
        while(fgets(line,sizeof(line),stdin)) {
            s16 q[13];char *cursor=line;
            for(u16 i=0;i<13;i++) {
                char *end;long value=strtol(cursor,&end,10);
                if(end==cursor || value < -32768 || value > 32767)return 3;
                q[i]=(s16)value;cursor=end;
            }
            CcHit h=q[12] ? cc_ray((CcVec){q[0],q[1]},(CcVec){q[4],q[5]},(CcBox){q[6],q[7],q[8],q[9]}) :
                cc_sweep((CcBox){q[0],q[1],q[2],q[3]},(CcVec){q[4],q[5]},(CcBox){q[6],q[7],q[8],q[9]},(CcVec){q[10],q[11]});
            printf("%u %u %d %d %ld %ld\n",h.kind,h.time,h.nx,h.ny,(long)h.num,(long)h.den);
        }
        return 0;
    }
    int error=!strcmp(argv[1],"sweep") ? check_sweep() : !strcmp(argv[1],"ray") ? check_ray() :
        !strcmp(argv[1],"slide") ? check_cast_slide() : !strcmp(argv[1],"invalid") ? check_invalid() : check_demo();
    if(error)fprintf(stderr,"Collision contract failed at %d\n",error);
    return error ? 1 : 0;
}
