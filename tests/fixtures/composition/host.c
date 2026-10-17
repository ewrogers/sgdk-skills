#include "checks.h"
#include <stdio.h>
int main(void)
{
    int code=courier_checks();
    if(code) {fprintf(stderr,"Courier contract failed at line %d\n",code);return 1;}
    printf("%lu\n",(unsigned long)courier_trace());return 0;
}
