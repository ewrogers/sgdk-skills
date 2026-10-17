#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "checks.h"
int main(int argc,char **argv)
{
    if(argc==4 && !strcmp(argv[1],"swing")) {
        CcGame g;setup(&g);CcActor *a=&g.actor[0];a->state=CC_STRIKE;
        a->facing=(u16)strtoul(argv[2],NULL,10);a->time=6+2*(u16)strtoul(argv[3],NULL,10);
        if(a->facing>3 || a->time>=14)return 2;
        CcBox b=cc_attack_box(a),o=cc_swing_origin(a);
        printf("%d %d %d %d %d %d\n",b.x-o.x,b.y-o.y,b.w,b.h,o.x,o.y);return 0;
    }
    int e=contracts();u32 trace=0;if(!e)e=check_soak(argc>1 && !strcmp(argv[1],"trace") ? 256 : 20000,&trace);
    if(e){fprintf(stderr,"Combat contract failed at line %d\n",e);return 1;}
    printf("%lu\n",(unsigned long)trace);return 0;
}
