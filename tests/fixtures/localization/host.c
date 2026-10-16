#include <stdio.h>
#include <string.h>
#include "checks.h"
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    int result;
    if(!strcmp(argv[1],"numbers"))result=check_numbers();
    else if(!strcmp(argv[1],"layout"))result=check_layout();
    else if(!strcmp(argv[1],"preferences"))result=check_preferences();
    else if(!strcmp(argv[1],"controller"))result=check_controller();
    else return 2;
    if(result)fprintf(stderr,"Failed line %d\n",result);
    return result ? 1 : 0;
}
