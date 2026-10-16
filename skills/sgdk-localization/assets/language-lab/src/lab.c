#include "lab.h"
#include "catalog.h"

static void default_name(LanguageLab *s)
{
    static const char text[]="Mira";
    for (u16 i=0;i<LOC_NAME_MAX+1;i++) s->name[i]=0;
    for (u16 i=0;text[i];i++) s->name[i]=(u8)(text[i]-31);
}

void lab_init(LanguageLab *s,u8 id)
{
    s->language=loc_language_index(id); s->screen=HOME; s->selected=0;
    s->cursor=0; s->page=0; s->quantity=0; s->held=0;
    default_name(s);
}

u16 lab_quantity(const LanguageLab *s)
{
    static const u16 quantities[]={1,2,999,12345,65535};
    return quantities[s->quantity%5];
}

bool lab_input(LanguageLab *s,u16 held)
{
    u16 pressed=held & (u16)~s->held;
    s->held=held;
    if (!pressed) return false;
    u8 previous=s->language;
    if ((pressed & KEY_C) || (s->screen==HOME && (pressed & (KEY_LEFT|KEY_RIGHT)))) {
        s->language=(s->language+((pressed & KEY_LEFT) ? LOC_LANGUAGE_COUNT-1 : 1))%LOC_LANGUAGE_COUNT;
        /* Keep selected menu/keyboard/item, but restart reflowed dialogue at page one. */
        s->page=0;
    } else if (s->screen==HOME) {
        if (pressed & KEY_UP) s->selected=(s->selected+2)%3;
        if (pressed & KEY_DOWN) s->selected=(s->selected+1)%3;
        if (pressed & (KEY_A|KEY_START)) { s->screen=s->selected+1; s->page=0; }
    } else if (s->screen==NAME) {
        if (pressed & KEY_LEFT) s->cursor=(s->cursor/8)*8+(s->cursor+7)%8;
        if (pressed & KEY_RIGHT) s->cursor=(s->cursor/8)*8+(s->cursor+1)%8;
        if (pressed & KEY_UP) s->cursor=(s->cursor+24)%32;
        if (pressed & KEY_DOWN) s->cursor=(s->cursor+8)%32;
        u16 length=0;
        while (length<LOC_NAME_MAX && s->name[length]) length++;
        if ((pressed & KEY_A) && length<LOC_NAME_MAX) { s->name[length]=loc_name_chars[s->cursor]; s->name[length+1]=0; }
        if ((pressed & KEY_B) && length) s->name[length-1]=0;
        if (pressed & KEY_START) { if (!s->name[0]) default_name(s); s->screen=HOME; }
    } else {
        if (pressed & KEY_B) s->screen=HOME;
        else if (pressed & (KEY_A|KEY_START)) {
            if (s->screen==BAG) s->quantity=(s->quantity+1)%5;
            else {
                u8 text[LOC_TEXT_MAX]; LocLayout layout;
                if (loc_expand(s->language,TXT_DIALOGUE,s->name,0,text,sizeof(text)) &&
                    loc_layout(text,loc_boxes[TXT_DIALOGUE],&layout)) {
                    u8 pages=(layout.lines+loc_boxes[TXT_DIALOGUE].rows-1)/loc_boxes[TXT_DIALOGUE].rows;
                    if (++s->page>=pages) { s->page=0; s->screen=HOME; }
                }
            }
        }
    }
    return previous!=s->language;
}
