#include <genesis.h>
#include "desk.h"
static Desk desk;
static u16 previous;
static u8 page, choice, fps, toneLeft, toneKind;
static char message[39]="Select a service and a free route.";
/* Original 8x8 assets: rails, tram, lamp, rain, vertical rails, junctions. */
static const u32 tiles[10*8]={
0,0,0x22222222,0x22222222,0,0x22222222,0,0,
0,0,0x33333333,0x33333333,0,0x33333333,0,0,
0,0,0x44444444,0x44444444,0,0x44444444,0,0,
0,0x22222222,0x23333332,0x23223232,0x23333332,0x22222222,0x02000020,0,
0,0x00333000,0x03333300,0x03333300,0x00333000,0,0,0,
0x00002000,0x00020000,0,0,0x00000002,0x00000020,0,0,
0x00202000,0x00202000,0x00202000,0x00202000,0x00202000,0x00202000,0x00202000,0x00202000,
0x00303000,0x00303000,0x00303000,0x00303000,0x00303000,0x00303000,0x00303000,0x00303000,
0x00202000,0x00202000,0x22222222,0x22222222,0x00202000,0x22222222,0x00202000,0x00202000,
0x00303000,0x00303000,0x33333333,0x33333333,0x00303000,0x33333333,0x00303000,0x00303000};
static void text(u16 x,u16 y,const char *s){VDP_drawText(s,x,y);}
static void number(u16 x,u16 y,u32 n,u16 width){char b[12];intToStr((s32)n,b,width);text(x,y,b);}
static void tile(u16 x,u16 y,u16 id){VDP_setTileMapXY(BG_B,TILE_ATTR_FULL(PAL0,0,0,0,TILE_USER_INDEX+id),x,y);}
static void sound(u8 kind){
 toneKind=kind;toneLeft=kind==3?fps:fps/8;
 PSG_setFrequency(0,kind==1?660:kind==2?155:440);PSG_setEnvelope(0,kind==2?6:9);
}
static void clear(void){VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);}
static void base(void){
 clear();text(2,0,"LAST SERVICE / MUNICIPAL TRAMWAYS");text(2,2,"RAIN / 23:58 / OCTOBER 1998");
}
static void title(void){
 u16 x;base();text(5,6,"THE LAST CARS ARE COMING HOME.");
 text(5,8,"A quiet night at the dispatch desk.");
 for(x=4;x<36;x++)tile(x,12,0);
 tile(11,11,3);tile(12,11,3);tile(27,11,4);
 for(x=5;x<36;x+=5)tile(x,16,5);
 text(6,19,choice==0?"> BEGIN SHIFT":"  BEGIN SHIFT");
 text(6,21,choice==1?"> DESK GUIDE":"  DESK GUIDE");
 text(6,24,"UP/DOWN choose   C select");text(6,26,"90 seconds. Keep the lines clear.");
}
static void guide(void){
 base();text(4,5,"DESK GUIDE");text(3,7,"UP/DOWN  Select waiting service");
 text(3,9,"LEFT/RIGHT  Select terminal bay");text(3,11,"C  Dispatch / rejects unsafe routes");
 text(3,13,"B  Clear bay selection");text(3,15,"A  Hold / run the shift clock");
 text(3,17,"START  Open / close this guide");text(3,19,"Guide holds time; keeps prior hold.");
 text(3,21,"X / Y are shared interlocks.");text(3,22,"Amber: occupied. Red: unavailable.");
 text(3,24,"Journeys take six seconds.");text(3,26,page==0?"C or START returns to title":"START returns to your desk");
}
static void desk_static(void){
 base();text(2,3,"WAITING SERVICE       DESTINATION");
 text(2,8,"INCOMING       INTERLOCKS      BAYS");
 text(2,21,"C DISPATCH  B CLEAR  A HOLD/RUN");text(2,23,"UP/DOWN service   LEFT/RIGHT bay");
 text(2,25,"START desk guide / holds the clock");
}
static void desk_draw(void){
 u8 i,j=0,busy=desk_busy(&desk);u16 x,y;
 text(2,1,desk.held?"HELD   REMAINING":"RUN    REMAINING");number(18,1,(SHIFT_TICKS-desk.tick+59)/60,2);text(21,1,"sec");
 text(26,1,"DONE");number(32,1,desk_count(&desk,DELIVERED),2);
 for(i=0;i<3;i++){text(2,4+i,"                  ");}
 for(i=0;i<SERVICES;i++)if(desk.service[i].state==WAITING){
  text(2,4+j,desk.selected==i?">":" ");text(4,4+j,"CAR");number(8,4+j,i+101,3);
  text(12,4+j,desk.service[i].origin==0?"NORTH":desk.service[i].origin==1?"CANAL":"MARKET");j++;
 }
 for(i=0;i<4;i++){
  u8 ok=desk_route_ok(&desk,desk.selected,i);VDP_setTextPalette(ok?PAL0:PAL1);
  text(23,4+i,desk.bay==i?"> BAY":"  BAY");number(29,4+i,i+1,1);text(31,4+i,ok?"FREE":"LOCK");
 }VDP_setTextPalette(PAL0);
 text(2,7,"BACKLOG");number(10,7,desk_backlog(&desk),2);
 /* BG_B artwork restoration is bounded to 40x11 cells, ten times/sec. */
 VDP_clearTileMapRect(BG_B,0,9,40,11);
 for(i=0;i<3;i++){y=10+i*3;text(2,y,i==0?"N":i==1?"C":"M");for(x=4;x<18;x++)tile(x,y,0);}
 for(x=17;x<29;x++){tile(x,10,(busy&1)?1:0);tile(x,16,(busy&2)?1:0);}
 for(y=10;y<=16;y++)tile(17,y,busy==3?7:6);
 tile(17,10,(busy&1)?9:8);tile(17,13,busy==3?9:8);tile(17,16,(busy&2)?9:8);
 for(y=9;y<=12;y++)tile(28,y,(busy&1)?7:6);
 for(y=15;y<=18;y++)tile(28,y,(busy&2)?7:6);
 text(19,11,(busy&1)?"X BUSY":"X FREE");text(19,17,(busy&2)?"Y BUSY":"Y FREE");
 for(i=0;i<4;i++){
  y=9+i*3;text(30,y+1,"    ");for(x=28;x<34;x++)tile(x,y,0);text(35,y,"B");number(37,y,i+1,1);
  for(j=0;j<SERVICES;j++)if(desk.service[j].state==TRANSIT && desk.service[j].bay==i){
   u16 progress=JOURNEY_TICKS-desk.service[j].remaining;
   u16 tx,ty;
   u16 fromY=10+desk.service[j].origin*3;
   u16 trunkY=i<2?10:16;
   if(progress<90){tx=4+progress*13/90;ty=fromY;}
   else if(progress<180){tx=17;ty=fromY+((s16)trunkY-(s16)fromY)*(s16)(progress-90)/90;}
   else if(progress<270){tx=17+(progress-180)*11/90;ty=trunkY;}
   else if(progress<315){tx=28;ty=trunkY+((s16)y-(s16)trunkY)*(s16)(progress-270)/45;}
   else{tx=28+(progress-315)*5/45;ty=y;}
   tile(tx,ty,3);tile(tx+1,ty,3);tile(33,y,4);
   number(30,y+1,(desk.service[j].remaining+59)/60,1);text(32,y+1,"s");
  }
 }
 text(2,20,"                                    ");text(2,20,message);
 text(2,26,"TOTAL WAIT");number(13,26,desk.wait_ticks/60,4);text(18,26,"service seconds");
}
static void report(void){
 base();text(5,5,"SHIFT CLOSED / THE RAIN GOES ON");text(5,8,"Delivered services");number(29,8,desk_count(&desk,DELIVERED),2);
 text(5,10,"Waiting / backlog");number(29,10,desk_count(&desk,WAITING)+desk_backlog(&desk),2);
 text(5,12,"Still in transit");number(29,12,desk_count(&desk,TRANSIT),2);
 text(5,15,"Accumulated waiting time");number(8,17,desk.wait_ticks/60,5);text(15,17,"service seconds");
 text(5,21,choice==0?"> RETRY SAME SHIFT":"  RETRY SAME SHIFT");text(5,23,choice==1?"> TITLE":"  TITLE");text(5,26,"UP/DOWN choose   C select");
}
static u16 edges(u16 p){u16 e=0;
 if(p&BUTTON_UP)e|=K_UP;
 if(p&BUTTON_DOWN)e|=K_DOWN;
 if(p&BUTTON_LEFT)e|=K_LEFT;
 if(p&BUTTON_RIGHT)e|=K_RIGHT;
 if(p&BUTTON_A)e|=K_A;
 if(p&BUTTON_B)e|=K_B;
 if(p&BUTTON_C)e|=K_C;
 if(p&BUTTON_START)e|=K_START;
 return e;
}
int main(bool hardReset){
 u16 now,pressed,refresh=0;u8 oldHelp;
 (void)hardReset;VDP_setScreenWidth320();fps=SYS_isPAL()?50:60;
 PAL_setColor(0,0xACE);PAL_setColor(1,0xACE);PAL_setColor(2,0x222);PAL_setColor(3,0x26A);PAL_setColor(4,0x226);PAL_setColor(15,0x222);
 PAL_setColor(16,0xACE);PAL_setColor(31,0x226);
 VDP_setBackgroundColor(0);VDP_setTextPalette(PAL0);VDP_loadTileData(tiles,TILE_USER_INDEX,10,CPU);
 Z80_loadDriver(Z80_DRIVER_NULL,TRUE);
 for(now=0;now<4;now++)PSG_setEnvelope(now,15);
 page=0;choice=0;title();
 while(TRUE){
  now=JOY_readJoypad(JOY_1);pressed=edges(now&~previous);previous=now;
  if(toneLeft && --toneLeft==0)PSG_setEnvelope(0,15);
  if(toneKind==3 && toneLeft==fps/2)PSG_setFrequency(0,330);
  if(page==0){
   if(desk.help){if(pressed&(K_C|K_START)){desk.help=0;title();}}
   else if(pressed&(K_UP|K_DOWN)){choice^=1;title();}
   else if(pressed&K_C){if(choice){desk.help=1;guide();}else{desk_reset(&desk);page=1;strcpy(message,"Select a service and a free route.");desk_static();desk_draw();}}
  }else if(page==1){
   oldHelp=desk.help;desk_input(&desk,pressed);
   if(desk.sound){sound(desk.sound);strcpy(message,desk.sound==1?"Route accepted. Tram in transit.":"Route locked / choose a free bay.");}
   desk_frame(&desk,fps);
   if(desk.finished){page=2;choice=0;sound(3);report();}
   else if(oldHelp!=desk.help){if(desk.help)guide();else{desk_static();desk_draw();}}
   else if(!desk.help && (pressed || ++refresh>=fps/10)){refresh=0;desk_draw();}
  }else{
   if(pressed&(K_UP|K_DOWN)){choice^=1;report();}
   else if(pressed&K_C){if(choice){page=0;choice=0;desk.help=0;title();}else{desk_reset(&desk);page=1;strcpy(message,"Select a service and a free route.");desk_static();desk_draw();}}
  }
  SYS_doVBlankProcess();
 }
 return 0;
}
