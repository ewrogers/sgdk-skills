#include "checks.h"
#include <stdio.h>
int main(void)
{
    u32 pal=0,ntsc=0;int e=route_checks(50,&pal);
    if(!e)e=route_checks(60,&ntsc);
    if(e) { fprintf(stderr,"Stream/audio contract failed at line %d\n",e);return 1; }
    printf("%lu %lu\n",(unsigned long)pal,(unsigned long)ntsc);return 0;
}
