#include <stdio.h>
#include <string.h>
#include "checks.h"
int main(int argc,char **argv)
{
    if(argc==2 && !strcmp(argv[1],"record")) {
        u8 record[HL_RECORD_BYTES];hl_encode(record,0x12345678);
        for(u16 i=0;i<HL_RECORD_BYTES;i++)printf("%02x",record[i]);
        puts("");return 0;
    }
    int e=check_clock();if(!e)e=check_budgets();if(!e)e=check_records();if(!e)e=check_fields();if(!e)e=check_ed_link();
    if(e)fprintf(stderr,"Hardware helper contract failed at %d\n",e);
    return e ? 1 : 0;
}
