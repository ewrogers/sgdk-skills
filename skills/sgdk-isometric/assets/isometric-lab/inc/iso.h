#ifndef ISOMETRIC_LAB_H
#define ISOMETRIC_LAB_H
#ifdef IL_HOST
#include <stdint.h>
#include <stdbool.h>
typedef int16_t s16;typedef uint16_t u16;typedef int32_t s32;typedef uint32_t u32;
#else
#include <genesis.h>
#endif
#define IL_UNIT 256
#define IL_LIMIT 8192
#define IL_MAX_ITEMS 24
#define IL_MAX_LINKS 24
#define IL_STATIC 15
#define IL_ITEMS (IL_STATIC+1)
enum {IL_NONE,IL_BEFORE,IL_AFTER,IL_AMBIGUOUS,IL_INVALID};
enum {IL_SORT_OK,IL_SORT_AMBIGUOUS,IL_SORT_CYCLE,IL_SORT_INVALID};
enum {IL_LEFT=1,IL_RIGHT=2,IL_UP=4,IL_DOWN=8};
enum {IL_DECK,IL_POST,IL_RAMP_L,IL_RAMP_R,IL_TOWER,IL_TREE,IL_FOUNTAIN,IL_HERO};
typedef struct {s16 x,y;} IlPoint;
typedef struct {s16 x0,y0,z0,x1,y1,z1;} IlBox;
typedef struct {IlBox box;u16 id;} IlItem;
typedef struct {u16 before,after;} IlLink;
typedef struct {IlItem fixed[IL_MAX_ITEMS];u32 edges[IL_MAX_ITEMS];u16 count;bool valid;} IlCache;
typedef struct {s16 x,y,z;u16 kind;} IlProp;
typedef struct {s16 x,y,z,camera_x,camera_y;u16 layer,facing,walk,phase,ticks;bool moving;} IlGame;
extern const IlProp il_props[IL_STATIC];
IlPoint il_project(s16 x,s16 y,s16 z);
bool il_unproject(IlPoint screen,s16 z,IlPoint *world);
u16 il_relation(IlBox a,IlBox b);
u16 il_sort(const IlItem *items,u16 count,const IlLink *links,u16 link_count,u16 *order);
u16 il_cache_init(IlCache *cache,const IlItem *fixed,u16 count);
u16 il_sort_cached(const IlCache *cache,const IlItem *dynamic,u16 count,const IlLink *links,u16 link_count,u16 *order);
IlBox il_prop_box(u16 index);
void il_init(IlGame *g);
void il_tour(IlGame *g,u16 stop);
void il_tick(IlGame *g,u16 held);
u16 il_frame(IlGame *g,u16 held,u16 hz,bool paused);
u16 il_scene(const IlGame *g,IlItem *items,IlLink *support,u16 *link_count);
#endif
