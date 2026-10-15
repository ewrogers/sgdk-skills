#include <stdio.h>
#include <string.h>
void test_routes(void);
void test_boss(void);
void test_lasers(void);
void test_levels(void);
void test_boss_path(void);
void test_radial_scatter(void);
void test_start_checkpoints(void);
void test_join(void);
void test_skip_and_edges(void);
void test_movement_pause_and_coins(void);
void test_switching(void);
void test_text(void);
void test_checkpoint_format(void);
void test_checkpoint_recovery(void);
void test_video(void);
void test_storage(void);
void test_view(void);
void test_rollback(void);
int main(int argc,char **argv)
{
    if (argc!=2) return 2;
    if (!strcmp(argv[1],"routes")) test_routes();
    else if (!strcmp(argv[1],"join")) test_join();
    else if (!strcmp(argv[1],"skip")) test_skip_and_edges();
    else if (!strcmp(argv[1],"play")) test_movement_pause_and_coins();
    else if (!strcmp(argv[1],"boss")) test_boss();
    else if (!strcmp(argv[1],"path")) test_boss_path();
    else if (!strcmp(argv[1],"levels")) test_levels();
    else if (!strcmp(argv[1],"scatter")) test_radial_scatter();
    else if (!strcmp(argv[1],"lasers")) test_lasers();
    else if (!strcmp(argv[1],"start-checkpoints")) test_start_checkpoints();
    else if (!strcmp(argv[1],"switch")) test_switching();
    else if (!strcmp(argv[1],"text")) test_text();
    else if (!strcmp(argv[1],"checkpoint")) test_checkpoint_format();
    else if (!strcmp(argv[1],"recovery")) test_checkpoint_recovery();
    else if (!strcmp(argv[1],"video")) test_video();
    else if (!strcmp(argv[1],"storage")) test_storage();
    else if (!strcmp(argv[1],"view")) test_view();
    else if (!strcmp(argv[1],"rollback")) test_rollback();
    else return 2;
    puts(argv[1]); return 0;
}
