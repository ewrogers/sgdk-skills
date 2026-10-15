#include "world.h"

/* Each character is a 16-pixel cell: # is a wall, . is floor.
   Alternating baffles block perimeter shortcuts. Sheltered entries have
   two-cell doorways; the visible outer frame matches the solid arena bounds. */
static const char walls[LEVEL_COUNT][GRID_H][GRID_W+1]={
    {
        ".....#.........#..",
        ".....#.........#..",
        ".....#.....#...#..",
        ".....#..##.#...#..",
        ".....#..##.#...#..",
        ".....#.....#...#..",
        "#..#.#.....#......",
        "...#.......#......",
        "...#.......#......"
    },
    {
        "....#........##...",
        "....#........##...",
        "....#...#....#.#..",
        "....#...#....#....",
        "....#...#....#....",
        "....#...#....#....",
        "........#....#....",
        "........#.........",
        "........#........."
    },
    {
        "...##.........#...",
        "...##.........#...",
        "#...#....#....#...",
        "....#....#....#...",
        "....#....#....#...",
        "....#....#....#...",
        "....#....#........",
        ".........#........",
        ".........#........"
    }
};
Point world_exit(u16 level)
{
    static const Point exits[LEVEL_COUNT]={{288,64},{32,64},{272,64}};
    return exits[level];
}
Point world_entry(u16 level,u16 player)
{
    if (!level) return (Point){player ? 48 : 32,192};
    Point entry=world_exit(level-1);
    if (player) entry.x+=entry.x>160 ? -16 : 16;
    return entry;
}
bool world_entry_pad(u16 level,Point position)
{
    Point entry=world_entry(level,0);
    s16 left=entry.x>160 ? 256 : 16,top=entry.y<128 ? 64 : 176;
    return position.x>=left && position.x+16<=left+48 && position.y>=top && position.y+16<=top+32;
}
bool world_wall(u16 level,s16 column,s16 row)
{
    return level>=LEVEL_COUNT || column<0 || column>=GRID_W || row<0 || row>=GRID_H || walls[level][row][column]=='#';
}
bool world_solid(u16 level,s16 x,s16 y,s16 width,s16 height)
{
    if (width<=0 || height<=0 || x<16 || y<64 || x+width>304 || y+height>208) return true;
    for (s16 row=(y-64)/16;row<=(y+height-65)/16;++row)
        for (s16 column=(x-16)/16;column<=(x+width-17)/16;++column)
            if (world_wall(level,column,row)) return true;
    return false;
}
u16 world_cell(Point position)
{
    s16 x=(position.x-16+8)/16,y=(position.y-64+8)/16;
    if (position.x<16 || position.y<64 || x>=GRID_W || y>=GRID_H) return NO_CELL;
    return y*GRID_W+x;
}
Point world_point(u16 cell) { return (Point){16+(cell%GRID_W)*16,64+(cell/GRID_W)*16}; }
u16 world_goal(u16 level,Point target)
{
    u16 best=NO_CELL,distance=0xFFFF;
    for (u16 cell=0;cell<GRID_CELLS;++cell) {
        if (world_wall(level,cell%GRID_W,cell/GRID_W)) continue;
        Point point=world_point(cell);
        s16 dx=point.x-target.x,dy=point.y-target.y;
        u16 d=(dx<0 ? -dx : dx)+(dy<0 ? -dy : dy);
        if (d<distance) { distance=d; best=cell; if (!d) break; }
    }
    return best;
}
bool world_move(u16 level,Point *position,s16 dx,s16 dy)
{
    Point before=*position;
    if (!world_actor_solid(level,(Point){position->x+dx,position->y})) position->x+=dx;
    if (!world_actor_solid(level,(Point){position->x,position->y+dy})) position->y+=dy;
    return before.x!=position->x || before.y!=position->y;
}
bool world_actor_solid(u16 level,Point position)
{
    /* Keep the artwork inside the arena, but allow two pixels of clearance
       around the body against interior walls and doorframes. */
    return position.x<16 || position.y<64 || position.x>288 || position.y>192 ||
        world_solid(level,position.x+ACTOR_INSET,position.y+ACTOR_INSET,ACTOR_SIZE,ACTOR_SIZE);
}
static bool trace_line(u16 level,Point from,Point to,u16 allowance)
{
    s16 x=(from.x+8-16)/16,y=(from.y+8-64)/16;
    s16 end_x=(to.x+8-16)/16,end_y=(to.y+8-64)/16;
    if (from.x<16 || from.y<64 || to.x<16 || to.y<64 || world_wall(level,x,y) || world_wall(level,end_x,end_y)) return false;
    s16 dx=end_x-x,dy=end_y-y,sx=dx<0 ? -1 : 1,sy=dy<0 ? -1 : 1;
    u16 ax=dx<0 ? -dx : dx,ay=dy<0 ? -dy : dy,ix=0,iy=0;
    /* Supercover of the cell-center ray. A corner cannot see through either solid side. */
    while (ix<ax || iy<ay) {
        u16 cross_x=(1+2*ix)*ay,cross_y=(1+2*iy)*ax;
        if (cross_x==cross_y) {
            u16 sides=world_wall(level,x+sx,y)+world_wall(level,x,y+sy);
            if (sides>allowance) return false;
            allowance-=sides;
            x+=sx; y+=sy; ++ix; ++iy;
        } else if (cross_x<cross_y) { x+=sx; ++ix; }
        else { y+=sy; ++iy; }
        if (world_wall(level,x,y)) { if (!allowance) return false; --allowance; }
    }
    return true;
}
bool world_line(u16 level,Point from,Point to) { return trace_line(level,from,to,0); }
bool world_lob_line(u16 level,Point from,Point to) { return trace_line(level,from,to,1); }
