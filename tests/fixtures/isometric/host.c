#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "checks.h"
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"digest")){printf("%lu\n",(unsigned long)trace_digest(128));return 0;}
    if(!strcmp(argv[1],"pairs")) {
        char line[256];
        while(fgets(line,sizeof(line),stdin)) {
            s16 q[12];char *cursor=line;
            for(u16 i=0;i<12;i++){char *end;long v=strtol(cursor,&end,10);if(end==cursor || v < -32768 || v>32767)return 3;q[i]=(s16)v;cursor=end;}
            printf("%u\n",il_relation((IlBox){q[0],q[1],q[2],q[3],q[4],q[5]},(IlBox){q[6],q[7],q[8],q[9],q[10],q[11]}));
        }
        return 0;
    }
    int e=!strcmp(argv[1],"projection") ? check_projection() : !strcmp(argv[1],"relations") ? check_relations() :
        !strcmp(argv[1],"sort") ? check_sort() : !strcmp(argv[1],"world") ? check_world(1000) : !strcmp(argv[1],"soak") ? check_soak(20000) : check_cache_and_timing();
    if(e)fprintf(stderr,"Isometric contract failed at %d\n",e);
    return e ? 1 : 0;
}
