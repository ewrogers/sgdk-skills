#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "desk.h"
static void ticks(Desk *d,int n){while(n--)desk_tick(d);}
static void invariant(const Desk *d){
 int i,j;unsigned mask=0;
 assert(desk_count(d,WAITING)<=WAIT_CAP);
 assert(desk_count(d,TRANSIT)<=2);
 for(i=0;i<SERVICES;i++){
  assert(d->service[i].state<=DELIVERED);
  if(d->service[i].state==TRANSIT){
   unsigned m=desk_mask(d->service[i].origin,d->service[i].bay);
   assert(m && !(mask&m));mask|=m;assert(d->service[i].remaining>0);
   for(j=i+1;j<SERVICES;j++)if(d->service[j].state==TRANSIT)assert(d->service[j].bay!=d->service[i].bay);
  }
 }
 if(desk_count(d,WAITING))assert(d->selected<SERVICES && d->service[d->selected].state==WAITING);
}
static void conflicts(void){
 Desk d;int a,b,o,p;
 for(o=0;o<3;o++)for(a=0;a<4;a++)for(p=0;p<3;p++)for(b=0;b<4;b++){
  Service before;
  desk_reset(&d);d.service[0].origin=o;d.bay=a;desk_input(&d,K_C);assert(d.sound==1);
  d.service[1].state=WAITING;d.service[1].origin=p;d.selected=1;d.bay=b;before=d.service[1];
  if(desk_mask(o,a)&desk_mask(p,b)){
   desk_input(&d,K_C);assert(d.sound==2);assert(memcmp(&before,&d.service[1],sizeof before)==0);assert(d.selected==1 && d.bay==b);
  }else{desk_input(&d,K_C);assert(d.sound==1);assert(d.service[1].state==TRANSIT);}
  invariant(&d);
 }
 puts("PASS all 144 ordered route pairs: conflicts, occupied bays, independent routes");
}
static void release_reuse(void){
 Desk d;desk_reset(&d);desk_input(&d,K_C);ticks(&d,359);assert(d.service[0].state==TRANSIT && desk_busy(&d));
 desk_tick(&d);assert(d.service[0].state==DELIVERED && !desk_busy(&d));
 assert(d.selected==1);assert(desk_route_ok(&d,1,0));desk_input(&d,K_C);assert(d.service[1].state==TRANSIT);
 puts("PASS exact journey release and bay/interlock reuse");
}
static void full_queue(void){
 Desk d;int i;desk_reset(&d);ticks(&d,1800);assert(desk_count(&d,WAITING)==3);assert(desk_backlog(&d)==4);
 assert(d.selected==0);assert(d.wait_ticks>1800*3UL);
 desk_input(&d,K_C);assert(desk_count(&d,WAITING)==3);assert(d.service[3].state==WAITING);assert(d.selected==1);
 desk_input(&d,K_DOWN);assert(d.selected==2);ticks(&d,400);assert(d.selected==2);
 for(i=0;i<5400;i++){desk_tick(&d);invariant(&d);}
 assert(d.tick==SHIFT_TICKS && d.finished);assert(desk_count(&d,WAITING)+desk_backlog(&d)+desk_count(&d,TRANSIT)+desk_count(&d,DELIVERED)==SERVICES);
 puts("PASS full queue, FIFO backlog, no loss, stable selected identity");
}
static void hold_help(void){
 Desk d;unsigned short time,left,phase;unsigned long wait;int i;
 desk_reset(&d);desk_input(&d,K_C);desk_frame(&d,50);desk_input(&d,K_A);
 time=d.tick;left=d.service[0].remaining;wait=d.wait_ticks;phase=d.phase;
 for(i=0;i<600;i++)desk_frame(&d,50);
 assert(d.tick==time && d.service[0].remaining==left && d.wait_ticks==wait && d.phase==phase);
 desk_input(&d,K_B);assert(d.bay==NONE);desk_input(&d,K_RIGHT);assert(d.bay==0);
 desk_input(&d,K_START);assert(d.help && d.held);desk_input(&d,K_A);assert(d.held);
 for(i=0;i<600;i++)desk_frame(&d,60);
 assert(d.tick==time);
 desk_input(&d,K_START);assert(!d.help && d.held);desk_input(&d,K_A);assert(!d.held);
 desk_input(&d,K_START);for(i=0;i<100;i++)desk_frame(&d,60);assert(d.tick==time);
 desk_input(&d,K_START);assert(!d.held);desk_frame(&d,60);assert(d.tick==time+1);
 puts("PASS hold/help freeze arrivals, journeys, waiting and phase; prior state restored; B clears");
}
static void timing(void){
 Desk d;int fps,frame;
 for(fps=50;fps<=60;fps+=10){desk_reset(&d);for(frame=1;frame<fps*90;frame++){desk_frame(&d,fps);assert(!d.finished);}desk_frame(&d,fps);assert(d.finished && d.tick==SHIFT_TICKS);printf("PASS %d Hz: %d frames = 90 seconds\n",fps,fps*90);}
}
static void deterministic(void){
 Desk a,b;int i;desk_reset(&a);desk_reset(&b);
 for(i=0;i<5400;i++){
  unsigned k=i%73==0?K_C:i%41==0?K_RIGHT:i%101==0?K_DOWN:0;
  desk_input(&a,k);desk_tick(&a);desk_input(&b,k);desk_tick(&b);assert(memcmp(&a,&b,sizeof a)==0);invariant(&a);
 }
 desk_reset(&a);desk_reset(&b);assert(memcmp(&a,&b,sizeof a)==0);
 puts("PASS deterministic full-shift replay and complete retry reset");
}
static void region_replay(void){
 Desk a,b;int sec,f;desk_reset(&a);desk_reset(&b);
 for(sec=0;sec<90;sec++){
  unsigned k=sec%3==0?K_C:sec%3==1?K_RIGHT:K_DOWN;
  desk_input(&a,k);desk_input(&b,k);
  for(f=0;f<50;f++)desk_frame(&a,50);
  for(f=0;f<60;f++)desk_frame(&b,60);
  assert(memcmp(&a,&b,sizeof a)==0);invariant(&a);
 }
 puts("PASS PAL/NTSC identical second-boundary input replay");
}
int main(void){conflicts();release_reuse();full_queue();hold_help();timing();deterministic();region_replay();puts("ALL TESTS PASSED");return 0;}
