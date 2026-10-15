#ifndef TEST_GENESIS_H
#define TEST_GENESIS_H
#include <stdint.h>
#include <stdbool.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef int32_t s32;
#define TRUE true
#define FALSE false
#define PORT_1 0
#define PORT_2 1
#define JOY_1 0
#define JOY_2 1
#define JOY_3 2
#define JOY_4 3
#define JOY_5 4
#define JOY_6 5
#define JOY_7 6
#define JOY_8 7
#define JOY_NUM 8
#define BUTTON_UP 0x0001
#define BUTTON_DOWN 0x0002
#define BUTTON_LEFT 0x0004
#define BUTTON_RIGHT 0x0008
#define BUTTON_A 0x0040
#define BUTTON_B 0x0010
#define BUTTON_C 0x0020
#define BUTTON_START 0x0080
#define BUTTON_X 0x0400
#define BUTTON_Y 0x0200
#define BUTTON_Z 0x0100
#define BUTTON_MODE 0x0800
#define BUTTON_LMB BUTTON_A
#define BUTTON_MMB BUTTON_B
#define BUTTON_RMB BUTTON_C
#define JOY_TYPE_PAD3 0
#define JOY_TYPE_PAD6 1
#define JOY_TYPE_MOUSE 2
#define JOY_TYPE_UNKNOWN 15
#define PORT_TYPE_MOUSE 3
#define PORT_TYPE_TEAMPLAYER 7
#define PORT_TYPE_PAD 13
#define PORT_TYPE_UNKNOWN 15
#define PORT_TYPE_EA4WAYPLAY 16
#define JOY_SUPPORT_OFF 0
#define JOY_SUPPORT_3BTN 1
#define JOY_SUPPORT_6BTN 2
#define JOY_SUPPORT_MOUSE 3
#define JOY_SUPPORT_TEAMPLAYER 5
#define JOY_SUPPORT_EA4WAYPLAY 6
typedef void JoyEventCallback(u16 joy, u16 changed, u16 state);
void JOY_setEventHandler(JoyEventCallback *callback);
void JOY_setSupport(u16 port, u16 support);
u8 JOY_getPortType(u16 port);
u8 JOY_getJoypadType(u16 joy);
u16 JOY_readJoypad(u16 joy);
s16 JOY_readJoypadX(u16 joy);
s16 JOY_readJoypadY(u16 joy);
s16 JOY_writeJoypadX(u16 joy, u16 pos);
s16 JOY_writeJoypadY(u16 joy, u16 pos);
void JOY_reset(void);
#endif
