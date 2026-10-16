#include "dialogue.h"
#include "panel.h"
#include "lab.h"
#include "dialogue_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static const DlgToken first[]={ {65,DL_GLYPH,0},{100,DL_PAUSE,0},{66,DL_GLYPH,1},{0,DL_PAGE,0},{67,DL_GLYPH,5},{0,DL_FINISH,0} };
static const DlgToken second[]={ {68,DL_GLYPH,0},{0,DL_FINISH,0} };
static const DlgNode nodes[]={
 {.tokens=first,.speaker="ORRIN",.token_count=6,.next=1,.pane=1,.portrait=1,.flags=DL_NAMEPLATE},
 {.tokens=second,.speaker="MIRA",.token_count=2,.next=2,.pane=0,.portrait=0,.flags=DL_SPLIT|DL_NAMEPLATE},
 {.tokens=second,.speaker="",.token_count=2,.next=DL_END,.pane=0,.portrait=DL_NO_PORTRAIT}
};
static const DlgProgram program={nodes,3,1};

static void timing(void)
{
    Dialogue d; CHECK(dlg_start(&d,&program,0,0));
    dlg_step(&d,0,0); CHECK(d.panes[1].cells[0].glyph == 'A' && d.token == 1 && d.wait_ms == 30);
    dlg_step(&d,29,0); CHECK(d.token == 1 && d.wait_ms == 1);
    dlg_step(&d,1,0); CHECK(d.token == 2 && d.wait_ms == 100);
    dlg_step(&d,99,0); CHECK(d.token == 2 && d.wait_ms == 1 && !d.panes[1].cells[1].glyph);
    dlg_step(&d,1,0); CHECK(d.panes[1].cells[1].glyph == 'B' && d.panes[1].cells[1].style == 1);
    dlg_step(&d,30,0); CHECK(d.state == DL_PAGE_WAIT && dlg_has_more(&d));
    dlg_step(&d,1000,0); CHECK(d.state == DL_PAGE_WAIT && d.token == 4);
    dlg_step(&d,0,DL_ACCEPT); CHECK(d.state == DL_TYPING && !d.panes[1].cells[0].glyph);
    dlg_step(&d,0,DL_ACCEPT); CHECK(d.state == DL_NODE_WAIT && d.panes[1].cells[0].glyph == 'C' && d.panes[1].cells[0].style == 5);
    CHECK(dlg_has_more(&d));
    dlg_step(&d,0,DL_ACCEPT); CHECK(d.node == 1 && d.state == DL_TYPING && d.panes[0].node && d.panes[1].node);
    CHECK(d.panes[1].cells[0].glyph == 'C');
    dlg_step(&d,0,DL_ACCEPT); CHECK(d.node == 1 && d.state == DL_NODE_WAIT);
    dlg_step(&d,0,DL_ACCEPT); CHECK(d.node == 2 && !d.panes[1].node && d.panes[0].node);
    dlg_step(&d,0,DL_ACCEPT); CHECK(d.state == DL_NODE_WAIT && !dlg_has_more(&d));
    dlg_step(&d,0,DL_ACCEPT); CHECK(d.state == DL_DONE && (d.events & DL_DONE_EVENT));
    dlg_step(&d,1000,0); CHECK(d.state == DL_DONE && d.events == 0);
    CHECK(dlg_start(&d,&program,0,0)); dlg_step(&d,0,DL_ACCEPT);
    CHECK(d.state == DL_PAGE_WAIT && d.token == 4); /* Reveal cannot also advance. */
    CHECK(dlg_start(&d,&program,0,0)); dlg_step(&d,65535,0);
    CHECK(d.state == DL_PAGE_WAIT); /* Bounded elapsed time still stops at page. */
}

static void choices(void)
{
    const DlgNode branch[]={
      {.tokens=second,.speaker="Q",.token_count=2,.next=DL_END,.portrait=DL_NO_PORTRAIT,.choice_count=3,
       .choices={{"Borrow",1,0,1,1},{"Cross",2,1,0,0},{"Leave",2,0,0,0}}},
      {.tokens=second,.speaker="A",.token_count=2,.next=0,.portrait=DL_NO_PORTRAIT},
      {.tokens=second,.speaker="End",.token_count=2,.next=DL_END,.portrait=DL_NO_PORTRAIT}
    };
    const DlgProgram p={branch,3,1}; Dialogue d;
    CHECK(dlg_start(&d,&p,0,0)); dlg_step(&d,0,DL_ACCEPT);
    CHECK(d.state == DL_CHOOSE && d.visible_count == 2 && d.visible[0] == 0 && d.visible[1] == 2 && d.flags == 0);
    dlg_step(&d,0,DL_UP); CHECK(d.selected == 1);
    dlg_step(&d,0,DL_DOWN); CHECK(d.selected == 0);
    dlg_step(&d,0,DL_ACCEPT); CHECK(d.node == 1 && d.flags == 1 && d.state == DL_TYPING && (d.events & DL_CHOICE_EVENT));
    dlg_step(&d,0,DL_ACCEPT); dlg_step(&d,0,DL_ACCEPT); dlg_step(&d,0,DL_ACCEPT);
    CHECK(d.node == 0 && d.state == DL_CHOOSE && d.visible[0] == 1 && d.visible[1] == 2);
    dlg_step(&d,0,DL_ACCEPT); CHECK(d.node == 2 && d.flags == 1);
    DlgNode hidden=branch[0]; hidden.choice_count=1; hidden.choices[0].require=1; hidden.choices[0].forbid=0;
    const DlgProgram fallback={&hidden,1,1}; hidden.choices[0].target=0;
    CHECK(dlg_start(&d,&fallback,0,0)); dlg_step(&d,0,DL_ACCEPT);
    CHECK(d.state == DL_NODE_WAIT && !dlg_has_more(&d)); dlg_step(&d,0,DL_ACCEPT); CHECK(d.state == DL_DONE);
}

static void validation(void)
{
    Dialogue d; CHECK(!dlg_start(NULL,&program,0,0)); CHECK(!dlg_start(&d,NULL,0,0) && d.state == DL_ERROR);
    CHECK(!dlg_start(&d,&program,3,0)); CHECK(!dlg_start(&d,&program,0,2));
    DlgNode n=nodes[2]; DlgProgram p={&n,1,1}; CHECK(dlg_validate(&p));
    n.pane=2; CHECK(!dlg_validate(&p)); n=nodes[2];
    n.portrait=2; CHECK(!dlg_validate(&p)); n=nodes[2];
    n.next=1; CHECK(!dlg_validate(&p)); n=nodes[2];
    n.speaker="TOO LONG A SPEAKER"; CHECK(!dlg_validate(&p)); n=nodes[2];
    n.choice_count=4; CHECK(!dlg_validate(&p)); n=nodes[2];
    n.token_count=2049; CHECK(!dlg_validate(&p)); n=nodes[2];
    DlgToken broken[2]; memcpy(broken,second,sizeof second); n.tokens=broken;
    broken[0].style=6; CHECK(!dlg_validate(&p)); broken[0]=second[0];
    broken[0].value=31; CHECK(!dlg_validate(&p)); broken[0]=second[0];
    broken[0].op=99; CHECK(!dlg_validate(&p)); broken[0]=second[0];
    broken[1].op=DL_GLYPH; CHECK(!dlg_validate(&p)); broken[1]=second[1];
    n.choice_count=1; n.choices[0]=(DlgChoice){"Choice",0,1,1,0}; CHECK(!dlg_validate(&p));
    n.choices[0]=(DlgChoice){"Choice",0,0,0,2}; CHECK(!dlg_validate(&p));
    p.count=65; CHECK(!dlg_validate(&p));
    DlgToken long_page[257];
    for (u16 i=0; i<256; i++) long_page[i]=(DlgToken){1,DL_SPEED,0};
    long_page[256]=(DlgToken){0,DL_FINISH,0};
    n=nodes[2]; n.tokens=long_page; n.token_count=257; p.count=1; CHECK(!dlg_validate(&p));
    dlg_step(NULL,100,DL_ACCEPT);
}

static void panels(void)
{
    CHECK(!panel_fits(65535,0,3,3) && !panel_fits(0,65535,3,3));
    CHECK(!panel_fits(0,0,65535,3) && !panel_fits(0,0,1,3));
    CHECK(panel_fits(0,0,40,28) && panel_fits(38,26,2,2));
    CHECK(!panel_fits(39,26,2,2) && !panel_fits(38,27,2,2));
    CHECK(panel_piece(0,0,2,2) == 0 && panel_piece(1,0,2,2) == 2);
    CHECK(panel_piece(0,1,2,2) == 6 && panel_piece(1,1,2,2) == 8);
    CHECK(panel_piece(3,0,3,3) == 255 && panel_piece(0,0,1,3) == 255);
    for (u16 w=3; w<=40; w++) for (u16 h=3; h<=28; h++)
        for (u16 y=0; y<h; y++) for (u16 x=0; x<w; x++)
        {
            u8 expected=(y == 0 ? 0 : y == h-1 ? 6 : 3)+(x == 0 ? 0 : x == w-1 ? 2 : 1);
            CHECK(panel_piece(x,y,w,h) == expected);
        }
}

static void clocks(void)
{
    DialogueLab pal,ntsc; lab_init(&pal); lab_init(&ntsc);
    lab_frame(&pal,LAB_A,50); lab_frame(&ntsc,LAB_A,60);
    pal.clock_ms=ntsc.clock_ms=0;
    for (u16 i=0; i<50; i++) lab_frame(&pal,0,50);
    for (u16 i=0; i<60; i++) lab_frame(&ntsc,0,60);
    CHECK(pal.clock_ms == 1000 && ntsc.clock_ms == 1000);
    CHECK(pal.dialogue.token == ntsc.dialogue.token && pal.dialogue.wait_ms == ntsc.dialogue.wait_ms);
    CHECK(!memcmp(pal.dialogue.panes[1].cells,ntsc.dialogue.panes[1].cells,sizeof pal.dialogue.panes[1].cells));
    lab_frame(&pal,LAB_PAUSE,50); CHECK(pal.paused);
    const u16 token=pal.dialogue.token; const u32 time=pal.clock_ms;
    lab_frame(&pal,LAB_A,50); for (u16 i=0; i<100; i++) lab_frame(&pal,LAB_A,50);
    CHECK(pal.dialogue.token == token && pal.clock_ms == time);
    lab_frame(&pal,LAB_A|LAB_PAUSE,50); CHECK(!pal.paused);
    lab_frame(&pal,LAB_A,50); CHECK(pal.dialogue.state == DL_TYPING); /* Held input not replayed. */
    lab_frame(&pal,0,50); lab_frame(&pal,LAB_A,50); CHECK(pal.dialogue.state == DL_PAGE_WAIT);
    lab_frame(&pal,LAB_REPLAY,50); CHECK(pal.dialogue.token == 0 && pal.dialogue.flags == 0);
    lab_frame(&pal,LAB_BACK,50); CHECK(!pal.active);
    lab_frame(&pal,LAB_DOWN,50); CHECK(pal.story == 1);
    lab_frame(&pal,LAB_A,50); CHECK(pal.dialogue.node == lab_starts[1]);
    lab_frame(&pal,LAB_A,50); CHECK(pal.dialogue.state == DL_TYPING); /* Start edge not reused. */
}

static void authored(void)
{
    CHECK(dlg_validate(&lab_program));
    u32 seed=12345;
    for (u16 story=0; story<LAB_STORY_COUNT; story++)
    {
        Dialogue d; CHECK(dlg_start(&d,&lab_program,lab_starts[story],0));
        for (u32 step=0; step<10000; step++)
        {
            if (d.state == DL_DONE) CHECK(dlg_start(&d,&lab_program,lab_starts[story],0));
            seed=seed*1664525UL+1013904223UL;
            dlg_step(&d,(u16)(seed%101),(seed%3 == 0 ? DL_ACCEPT : 0)|(seed%7 == 0 ? DL_DOWN : 0));
            CHECK(d.state != DL_ERROR && d.node < lab_program.count && d.row < DL_ROWS);
            CHECK(d.token <= lab_program.nodes[d.node].token_count && d.column <= dlg_columns(&lab_program.nodes[d.node]));
            if (d.state == DL_CHOOSE) CHECK(d.visible_count && d.selected < d.visible_count);
            for (u16 pane=0; pane<2; pane++) for (u16 cell=0; cell<DL_CELLS; cell++)
                CHECK(d.panes[pane].cells[cell].style <= 5 && (!d.panes[pane].cells[cell].glyph ||
                    (d.panes[pane].cells[cell].glyph >= 32 && d.panes[pane].cells[cell].glyph <= 126)));
        }
    }
}

int main(int argc,char **argv)
{
    CHECK(argc == 2);
    if (!strcmp(argv[1],"timing")) timing();
    else if (!strcmp(argv[1],"choices")) choices();
    else if (!strcmp(argv[1],"validation")) validation();
    else if (!strcmp(argv[1],"panels")) panels();
    else if (!strcmp(argv[1],"clocks")) clocks();
    else if (!strcmp(argv[1],"authored")) authored();
    else return 2;
    puts(argv[1]); return 0;
}
