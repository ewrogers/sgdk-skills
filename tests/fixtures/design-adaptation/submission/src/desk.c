#include "desk.h"
const unsigned short timetable[SERVICES] = {0,240,540,840,1140,1440,1740,2040,2340,2640,2940,3240,3540,3840,4140,4440,4800,5100};
unsigned char desk_count(const Desk *d, unsigned char state) {
 unsigned char i,n=0; for(i=0;i<SERVICES;i++) if(d->service[i].state==state)n++; return n;
}
unsigned char desk_backlog(const Desk *d) {
 unsigned char i,n=0; for(i=0;i<SERVICES;i++) if(d->service[i].state==FUTURE && timetable[i]<=d->tick)n++; return n;
}
unsigned char desk_mask(unsigned char origin, unsigned char bay) {
 if(origin>2 || bay>3)return 0;
 return (origin==0?1:origin==2?2:3) | (bay<2?1:2);
}
unsigned char desk_busy(const Desk *d) {
 unsigned char i,m=0;for(i=0;i<SERVICES;i++)if(d->service[i].state==TRANSIT)m|=desk_mask(d->service[i].origin,d->service[i].bay);return m;
}
unsigned char desk_route_ok(const Desk *d,unsigned char id,unsigned char bay) {
 unsigned char i;
 if(id>=SERVICES || bay>3 || d->service[id].state!=WAITING)return 0;
 if(desk_mask(d->service[id].origin,bay)&desk_busy(d))return 0;
 for(i=0;i<SERVICES;i++)if(d->service[i].state==TRANSIT && d->service[i].bay==bay)return 0;
 return 1;
}
static void admit(Desk *d) {
 unsigned char i,n=desk_count(d,WAITING);
 for(i=0;i<SERVICES && n<WAIT_CAP;i++)if(d->service[i].state==FUTURE && timetable[i]<=d->tick){d->service[i].state=WAITING;n++;}
 if(d->selected>=SERVICES || d->service[d->selected].state!=WAITING){
  d->selected=NONE;for(i=0;i<SERVICES;i++)if(d->service[i].state==WAITING){d->selected=i;break;}
 }
}
void desk_reset(Desk *d) {
 unsigned char i;
 unsigned int z;
 /* Clear padding too: byte-for-byte checkpoints and reset are deterministic. */
 for(z=0;z<sizeof(*d);z++)((unsigned char *)d)[z]=0;
 d->tick=0;d->phase=0;d->wait_ticks=0;d->selected=NONE;d->bay=0;d->held=0;d->help=0;d->finished=0;d->sound=0;
 for(i=0;i<SERVICES;i++){d->service[i].state=FUTURE;d->service[i].origin=i%3;d->service[i].bay=NONE;d->service[i].remaining=0;}admit(d);
}
void desk_input(Desk *d,unsigned short edges) {
 unsigned char i,list[WAIT_CAP],n=0,pos=0;
 d->sound=0;
 if(d->finished)return;
 if(edges&K_START){d->help=!d->help;return;}
 if(d->help)return;
 if(edges&K_A)d->held=!d->held;
 for(i=0;i<SERVICES;i++)if(d->service[i].state==WAITING){if(i==d->selected)pos=n;list[n++]=i;}
 if(n && (edges&(K_UP|K_DOWN))){pos=(edges&K_UP)?(pos+n-1)%n:(pos+1)%n;d->selected=list[pos];}
 if(edges&K_B)d->bay=NONE;
 else if(edges&K_LEFT)d->bay=d->bay>3?3:(d->bay+3)%4;
 else if(edges&K_RIGHT)d->bay=d->bay>3?0:(d->bay+1)%4;
 if(edges&K_C){
  if(desk_route_ok(d,d->selected,d->bay)){
   Service *s=&d->service[d->selected];s->state=TRANSIT;s->bay=d->bay;s->remaining=JOURNEY_TICKS;d->sound=1;admit(d);
  }else d->sound=2;
 }
}
void desk_tick(Desk *d) {
 unsigned char i;
 if(d->held || d->help || d->finished)return;
 for(i=0;i<SERVICES;i++){
  Service *s=&d->service[i];
  if(s->state==WAITING || (s->state==FUTURE && timetable[i]<=d->tick))d->wait_ticks++;
  if(s->state==TRANSIT && --s->remaining==0)s->state=DELIVERED;
 }
 d->tick++;admit(d);
 if(d->tick==SHIFT_TICKS){d->finished=1;d->sound=3;}
}
void desk_frame(Desk *d,unsigned char fps) {
 if(d->held || d->help || d->finished)return;
 d->phase+=60;
 while(d->phase>=fps && !d->finished){d->phase-=fps;desk_tick(d);}
}
