#include <stdio.h>
#include <string.h>
#include "checks.h"
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"digest")){printf("%lu\n",(unsigned long)trace_digest());return 0;}
    int error=!strcmp(argv[1],"clock") ? check_clock() : !strcmp(argv[1],"actions") ? check_actions() :
        !strcmp(argv[1],"interrupts") ? check_interrupts() : !strcmp(argv[1],"timing") ? check_timing() : check_soak(100000);
    if(error)fprintf(stderr,"Animation contract failed at %d\n",error);
    return error ? 1 : 0;
}
