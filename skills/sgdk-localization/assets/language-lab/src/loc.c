#include "loc.h"
#include "catalog.h"

static u8 ascii(char c) { return (u8)(c-31); }

u8 loc_language_index(u8 id)
{
    for (u8 i=0;i<LOC_LANGUAGE_COUNT;i++) if (loc_languages[i].id==id) return i;
    return 0;
}

bool loc_number(u8 language, u16 number, u8 *out, u16 capacity)
{
    if (!out) return false;
    if (language>=LOC_LANGUAGE_COUNT) language=0;
    u8 reverse[5], text[7];
    u16 digits=0, length=0;
    do { reverse[digits++]=ascii((char)('0'+number%10)); number/=10; } while (number);
    for (u16 i=digits;i>0;i--) {
        text[length++]=reverse[i-1];
        if (i==4 && loc_languages[language].separator) text[length++]=loc_languages[language].separator;
    }
    if (capacity<=length) return false;
    text[length]=0;
    for (u16 i=0;i<=length;i++) out[i]=text[i];
    return true;
}

bool loc_expand(u8 language,u16 message,const u8 *name,u16 count,u8 *out,u16 capacity)
{
    if (!name || !out || message>=LOC_MESSAGE_COUNT) return false;
    if (language>=LOC_LANGUAGE_COUNT) language=0;
    u16 name_length=0;
    while (name_length<=LOC_NAME_MAX && name[name_length]) {
        if (name[name_length]>=LOC_GLYPH_COUNT) return false;
        name_length++;
    }
    if (name_length>LOC_NAME_MAX) return false;
    u8 number[7], text[LOC_TEXT_MAX];
    if (!loc_number(language,count,number,sizeof(number))) return false;
    const u8 *program=loc_catalog[language][message];
    u16 length=0;
    for (u16 i=0;i<LOC_TEXT_MAX;i++) {
        u8 code=program[i];
        if (!code) {
            if (capacity<=length) return false;
            text[length]=0;
            for (u16 j=0;j<=length;j++) out[j]=text[j];
            return true;
        }
        const u8 *insert=code==LOC_NAME_TOKEN ? name : code==LOC_COUNT_TOKEN ? number : 0;
        if (insert) {
            for (u16 j=0;insert[j];j++) {
                if (length>=LOC_TEXT_MAX-1) return false;
                text[length++]=insert[j];
            }
        } else {
            if ((code>=LOC_GLYPH_COUNT && code!=LOC_NEWLINE) || length>=LOC_TEXT_MAX-1) return false;
            text[length++]=code;
        }
    }
    return false;
}

bool loc_layout(const u8 *text,LocBox box,LocLayout *out)
{
    if (!text || !out || !box.width || box.width>LOC_WIDTH_MAX || !box.rows ||
        !box.pages || (u16)box.rows*box.pages>LOC_LINES_MAX) return false;
    LocLayout result;
    for (u16 y=0;y<LOC_LINES_MAX;y++) for (u16 x=0;x<LOC_WIDTH_MAX;x++) result.cells[y][x]=0;
    u16 index=0,row=0,column=0;
    bool space=false;
    while (index<LOC_TEXT_MAX && text[index]) {
        u8 code=text[index];
        if (code==LOC_NEWLINE) { row++; column=0; space=false; index++; }
        else if (code==LOC_SPACE) { space=true; index++; }
        else {
            u16 start=index;
            while (index<LOC_TEXT_MAX && text[index] && text[index]!=LOC_SPACE && text[index]!=LOC_NEWLINE) {
                if (text[index]>=LOC_GLYPH_COUNT) return false;
                index++;
            }
            u16 length=index-start;
            if (length>box.width) return false;
            if (column && column+length+(space ? 1 : 0)>box.width) { row++; column=0; }
            if (row>=(u16)box.rows*box.pages) return false;
            if (column && space) result.cells[row][column++]=LOC_SPACE;
            for (u16 i=0;i<length;i++) result.cells[row][column++]=text[start+i];
            space=false;
        }
        if (row>=(u16)box.rows*box.pages) return false;
    }
    if (index==LOC_TEXT_MAX) return false;
    result.lines=(u8)(row+1);
    *out=result;
    return true;
}

static u8 crc(const u8 *bytes)
{
    u8 value=0;
    for (u16 i=0;i<4;i++) {
        value^=bytes[i];
        for (u16 bit=0;bit<8;bit++) value=(u8)((value<<1) ^ ((value & 128) ? 7 : 0));
    }
    return value;
}

bool loc_pref_encode(u8 id,u8 out[6])
{
    if (!out || loc_languages[loc_language_index(id)].id!=id) return false;
    out[0]='L'; out[1]='C'; out[2]=1; out[3]=id; out[4]=crc(out); out[5]=0xA5;
    return true;
}

bool loc_pref_decode(const u8 in[6],u8 *id)
{
    if (!in || !id || in[0]!='L' || in[1]!='C' || in[2]!=1 || in[5]!=0xA5 ||
        in[4]!=crc(in) || loc_languages[loc_language_index(in[3])].id!=in[3]) return false;
    *id=in[3]; return true;
}
