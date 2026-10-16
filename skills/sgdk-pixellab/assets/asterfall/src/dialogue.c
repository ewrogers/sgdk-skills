#include "view.h"
#include "resources.h"

void dialogue_draw(const Game *g,bool setup)
{
    static const char *lines[4][2][2][3]={
        {{{"Welcome to Willowmere!","The road is yours to explore.","Take a moment. B opens your menu."},
          {"The cartographer marked four","destinations on your map.","Press C when you are ready."}},
         {{"I tend the lanterns here.","Each carries a little memory.","What story will yours hold?"},
          {"The crypt holds old secrets.","The wood keeps older ones.","And the sea never stops talking."}}},
        {{{"These stones remember us.","Listen beyond the torchlight.","No battles in this old tale."},
          {"That crystal is still warm.","Someone left it burning.","Perhaps they knew you would come."}},
         {{"Careful on the broken floor.","The lantern makes a fine guide.","Keep its light close."},
          {"Your journal holds the route.","There is always a way home.","The map will show you."}}},
        {{{"The path bends with the trees.","Let it lead you for a while.","There is no hurry here."},
          {"A mossbell only rings at dusk.","Have you ever heard one?","I am still waiting."}},
         {{"Look at all those colors!","Even the shadows are green.","This forest has its own palette."},
          {"The coast is just a map away.","Or stay and watch the leaves.","Both are good adventures."}}},
        {{{"Welcome to Tideglass Quay.","Every sail brings a new story.","The next ship leaves at sunrise."},
          {"See the tide beneath the dock?","A fine place to pause.","Then choose your next road."}},
         {{"I used to live across the sea.","Now every road feels like home.","Travel does that to a person."},
          {"Your lantern is shining.","Take that light wherever you go.","The world is not finished yet."}}}
    };
    if(setup)
    {
        ui_panel(0,17,40,11);ui_image(g->speaker?&art_portrait_sage:&art_portrait_guide,PAL2,2,19);
        ui_text(g->speaker?"Lyra / Lantern Sage":"Bram / Harbor Guide",12,19,true);
    }
    u32 remaining=g->screen_age/(g->text_speed+1);
    const char *const *page=lines[g->zone][g->speaker][g->dialogue_page];
    char paragraph[128];u16 length=0;
    for(u16 row=0;row<3;row++)
    {
        for(u16 i=0;page[row][i]&&length<126;i++)paragraph[length++]=page[row][i];
        if(row<2&&length<126)paragraph[length++]=' ';
    }
    paragraph[length]=0;u16 start=0;
    for(u16 row=0;row<4&&start<length;row++)
    {
        u16 end=start+26;if(end>length)end=length;
        if(end<length)
        {u16 space=end;while(space>start&&paragraph[space]!=' ')space--;if(space>start)end=space;}
        char visible[27];u16 n=0;
        for(u16 at=start;at<end;at++)
        {if(remaining){visible[n++]=paragraph[at];remaining--;}}
        visible[n]=0;ui_text(visible,12,21+row,false);
        start=end;while(start<length&&paragraph[start]==' ')start++;
    }
    ui_text(g->dialogue_page?"A Close":"A More",30,26,true);
    if((g->ticks/25)%2==0)ui_text("v",37,25,true);else ui_text(" ",37,25,true);
}
