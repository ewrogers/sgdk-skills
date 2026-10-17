#ifndef DESK_H
#define DESK_H
/* No SDK types: this is the exact simulation compiled by host tests and ROM. */
#define SERVICES 18
#define WAIT_CAP 3
#define JOURNEY_TICKS 360
#define SHIFT_TICKS 5400
#define NONE 255
#define K_UP 1
#define K_DOWN 2
#define K_LEFT 4
#define K_RIGHT 8
#define K_A 16
#define K_B 32
#define K_C 64
#define K_START 128
#define FUTURE 0
#define WAITING 1
#define TRANSIT 2
#define DELIVERED 3
typedef struct { unsigned char state, origin, bay; unsigned short remaining; } Service;
typedef struct {
 Service service[SERVICES];
 unsigned short tick, phase;
 unsigned long wait_ticks;
 unsigned char selected, bay, held, help, finished, sound;
} Desk;
extern const unsigned short timetable[SERVICES];
void desk_reset(Desk *d);
unsigned char desk_mask(unsigned char origin, unsigned char bay);
unsigned char desk_busy(const Desk *d);
unsigned char desk_route_ok(const Desk *d, unsigned char id, unsigned char bay);
unsigned char desk_count(const Desk *d, unsigned char state);
unsigned char desk_backlog(const Desk *d);
void desk_input(Desk *d, unsigned short edges);
void desk_tick(Desk *d);
void desk_frame(Desk *d, unsigned char fps);
#endif
