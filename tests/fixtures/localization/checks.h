#ifndef LANGUAGE_LAB_CHECKS_H
#define LANGUAGE_LAB_CHECKS_H
#include "loc.h"
#include "catalog.h"
#include "lab.h"
#define CHECK(value) do { if (!(value)) return __LINE__; } while (0)

static int check_numbers(void)
{
    static const u16 values[]={0,1,999,1000,12345,65535};
    for (u8 language=0;language<LOC_LANGUAGE_COUNT;language++) {
        for (u16 j=0;j<6;j++) {
            u8 out[9];for(u16 i=0;i<9;i++)out[i]=0xEE;
            CHECK(loc_number(language,values[j],out,7));
            u16 n=0;while(out[n])n++;
            CHECK(n<=6 && out[7]==0xEE && out[8]==0xEE);
            u32 decoded=0;
            for (u16 i=0;i<n;i++) {
                if(out[i]==loc_languages[language].separator)continue;
                CHECK(out[i]>=('0'-31) && out[i]<=('9'-31));
                decoded=decoded*10+(out[i]-('0'-31));
            }
            CHECK(decoded==values[j]);
        }
    }
    u8 exact[8];for(u16 i=0;i<8;i++)exact[i]=77;
    CHECK(!loc_number(0,65535,exact,6));
    for(u16 i=0;i<8;i++)CHECK(exact[i]==77);
    CHECK(loc_number(0,12345,exact,7));
    CHECK(exact[0]=='1'-31 && exact[1]=='2'-31 && exact[2]==','-31 && exact[6]==0);
    CHECK(loc_number(1,12345,exact,7) && exact[2]==LOC_NBSP);
    CHECK(loc_number(2,12345,exact,7) && exact[2]=='.'-31);
    CHECK(loc_number(3,12345,exact,6) && exact[2]=='3'-31 && exact[5]==0);
    return 0;
}

static int check_layout(void)
{
    u8 name[9];for(u16 i=0;i<8;i++)name[i]='W'-31;name[8]=0;
    for(u8 language=0;language<LOC_LANGUAGE_COUNT;language++) {
        for(u16 id=0;id<LOC_MESSAGE_COUNT;id++) {
            u8 text[LOC_TEXT_MAX];LocLayout layout;
            CHECK(loc_expand(language,id,name,65535,text,sizeof(text)));
            CHECK(loc_layout(text,loc_boxes[id],&layout));
            CHECK(layout.lines<=loc_boxes[id].rows*loc_boxes[id].pages);
            for(u16 y=0;y<LOC_LINES_MAX;y++)for(u16 x=0;x<LOC_WIDTH_MAX;x++) {
                CHECK(layout.cells[y][x]<LOC_GLYPH_COUNT);
                if(y>=layout.lines || x>=loc_boxes[id].width)CHECK(layout.cells[y][x]==0);
            }
        }
    }
    const u8 words[]={'A'-31,'A'-31,LOC_SPACE,'B'-31,'B'-31,0};
    LocLayout layout;layout.lines=77;layout.cells[0][0]=99;
    CHECK(!loc_layout(words,(LocBox){2,1,1},&layout));
    CHECK(layout.lines==77 && layout.cells[0][0]==99);
    CHECK(loc_layout(words,(LocBox){2,2,1},&layout));
    CHECK(layout.lines==2 && layout.cells[0][0]=='A'-31 && layout.cells[1][0]=='B'-31);
    const u8 long_word[]={'A'-31,'B'-31,'C'-31,0};
    CHECK(!loc_layout(long_word,(LocBox){2,2,1},&layout));
    const u8 nonbreak[]={'1'-31,LOC_NBSP,'2'-31,0};
    CHECK(!loc_layout(nonbreak,(LocBox){2,2,1},&layout));
    CHECK(loc_layout(nonbreak,(LocBox){3,1,1},&layout) && layout.cells[0][1]==LOC_NBSP);
    const u8 explicit_lines[]={'A'-31,LOC_NEWLINE,'B'-31,0};
    CHECK(loc_layout(explicit_lines,(LocBox){2,1,2},&layout) && layout.lines==2);
    CHECK(!loc_layout(words,(LocBox){37,1,1},&layout));
    u8 out[2]={77,78};
    CHECK(!loc_expand(0,TXT_DIALOGUE,name,1,out,2) && out[0]==77 && out[1]==78);
    CHECK(!loc_expand(0,LOC_MESSAGE_COUNT,name,1,out,2));
    u8 bad_name[10];for(u16 i=0;i<10;i++)bad_name[i]='A'-31;
    CHECK(!loc_expand(0,TXT_DIALOGUE,bad_name,1,out,2));
    CHECK(loc_language_index(255)==0);
    return 0;
}

static int check_preferences(void)
{
    for(u8 language=0;language<LOC_LANGUAGE_COUNT;language++) {
        u8 bytes[6],id=255;
        CHECK(loc_pref_encode(loc_languages[language].id,bytes));
        CHECK(loc_pref_decode(bytes,&id) && id==loc_languages[language].id);
        for(u16 byte=0;byte<6;byte++)for(u16 bit=0;bit<8;bit++) {
            bytes[byte]^=(u8)(1u<<bit);id=255;
            CHECK(!loc_pref_decode(bytes,&id) && id==255);
            bytes[byte]^=(u8)(1u<<bit);
        }
    }
    u8 output[6];for(u16 i=0;i<6;i++)output[i]=77;
    CHECK(!loc_pref_encode(255,output));
    for(u16 i=0;i<6;i++)CHECK(output[i]==77);
    return 0;
}

static void press(LanguageLab *s,u16 held) {lab_input(s,0);lab_input(s,held);}

static int check_controller(void)
{
    LanguageLab s;lab_init(&s,255);
    CHECK(s.language==0 && s.screen==HOME && s.selected==0);
    press(&s,KEY_DOWN);press(&s,KEY_A);
    CHECK(s.screen==BAG && s.selected==1);
    for(u16 i=0;i<10;i++)lab_input(&s,KEY_A);
    CHECK(s.quantity==0); /* The held entry button cannot change inventory. */
    press(&s,KEY_A);CHECK(lab_quantity(&s)==2);
    press(&s,KEY_C);CHECK(s.language==1 && s.quantity==1 && s.selected==1 && s.screen==BAG);
    press(&s,KEY_B);CHECK(s.screen==HOME && s.selected==1);
    press(&s,KEY_DOWN);press(&s,KEY_A);CHECK(s.screen==NAME);
    for(u16 i=0;i<4;i++)press(&s,KEY_B);
    CHECK(!s.name[0]);
    s.cursor=26;
    for(u16 i=0;i<9;i++)press(&s,KEY_A);
    CHECK(s.name[0]==loc_name_chars[26] && s.name[7]==loc_name_chars[26] && s.name[8]==0);
    press(&s,KEY_C);CHECK(s.language==2 && s.name[7]==loc_name_chars[26] && s.cursor==26);
    press(&s,KEY_START);CHECK(s.screen==HOME && s.selected==2);
    s.selected=0;press(&s,KEY_A);CHECK(s.screen==TALK);
    for(u16 i=0;i<4 && s.screen==TALK;i++)press(&s,KEY_A);
    CHECK(s.screen==HOME && s.page==0);
    for(u16 i=0;i<1000;i++)press(&s,KEY_C);
    CHECK(s.language<LOC_LANGUAGE_COUNT && s.name[8]==0);
    return 0;
}
#endif
