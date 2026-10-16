#ifndef LANGUAGE_LAB_CONTROLLER_H
#define LANGUAGE_LAB_CONTROLLER_H
#include "loc.h"

enum { KEY_UP=1,KEY_DOWN=2,KEY_LEFT=4,KEY_RIGHT=8,KEY_B=16,KEY_C=32,KEY_A=64,KEY_START=128 };
enum { HOME,TALK,BAG,NAME };
typedef struct {
    u8 language, screen, selected, cursor, page, quantity;
    u8 name[LOC_NAME_MAX+1];
    u16 held;
} LanguageLab;

void lab_init(LanguageLab *lab,u8 language_id);
bool lab_input(LanguageLab *lab,u16 held);
u16 lab_quantity(const LanguageLab *lab);

#endif
