#include <stdio.h>
#include "checks.h"
int main(int argc,char **argv)
{
    (void)argv;u32 trace=0;int error=contracts();if(!error)error=soak(argc>1 ? 256 : 20000,&trace);
    if(error){fprintf(stderr,"Audio director contract failed at %d\n",error);return 1;}
    printf("%lu\n",(unsigned long)trace);return 0;
}
