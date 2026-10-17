#include "desk_view.h"

static unsigned short attr(unsigned short tile, unsigned char palette) {
    return (unsigned short)(tile | ((unsigned short)palette << 13));
}

static void cell(DeskView *v, unsigned char plane, unsigned short x,
                 unsigned short y, unsigned short value) {
    if (x >= VIEW_W || y >= VIEW_H) { ++v->clipped; return; }
    v->next[plane][y * VIEW_W + x] = value;
}

static void text(DeskView *v, unsigned short x, unsigned short y,
                 const char *s, unsigned char palette) {
    while (*s) {
        unsigned char ch = (unsigned char)*s++;
        if (ch < 32 || ch > 127) { ch = '?'; ++v->clipped; }
        cell(v, 0, x++, y, attr(v->font + ch - 32, palette));
    }
}

static void number(DeskView *v, unsigned short x, unsigned short y,
                   unsigned long value, unsigned char width) {
    char digits[11];
    unsigned char n = 0, i;
    do { digits[n++] = (char)('0' + value % 10); value /= 10; } while (value);
    while (width > n) { text(v, x++, y, "0", 0); --width; }
    for (i = n; i; --i)
        cell(v, 0, x++, y, attr(v->font + digits[i - 1] - 32, 0));
}

static void fill(DeskView *v, unsigned short x, unsigned short y,
                 unsigned short w, unsigned short h, unsigned char palette) {
    unsigned short xx, yy;
    for (yy = y; yy < y + h; ++yy)
        for (xx = x; xx < x + w; ++xx)
            cell(v, 1, xx, yy, attr(v->art + ART_SOLID, palette));
}

static void panel(DeskView *v, unsigned short x, unsigned short y,
                  unsigned short w, unsigned short h) {
    unsigned short i;
    fill(v, x, y, w, h, 1);
    for (i = x; i < x + w; ++i) {
        cell(v, 0, i, y, attr(v->art + ART_H, 1));
        cell(v, 0, i, y + h - 1, attr(v->art + ART_H, 1));
    }
    for (i = y + 1; i < y + h - 1; ++i) {
        cell(v, 0, x, i, attr(v->art + ART_V, 1));
        cell(v, 0, x + w - 1, i, attr(v->art + ART_V, 1));
    }
}

static void header(DeskView *v, const char *subtitle) {
    text(v, 2, 1, "LAST SERVICE", 0);
    text(v, 2, 2, subtitle, 1);
}

static void title(DeskView *v, const DeskUI *ui) {
    header(v, "A quiet night at the tram depot.");
    panel(v, 1, 4, 38, 12);
    text(v, 4, 6, "BRING THE LAST TRAMS HOME", 0);
    text(v, 4, 9, "You choose which tram goes next", 0);
    text(v, 4, 11, "and which free bay it goes to.", 0);
    text(v, 4, 13, "One shift. 90 seconds.", 0);
    text(v, 4, 14, "Press A to hold the clock.", 1);
    fill(v, 3, ui->choice ? 21 : 18, 34, 2, 2);
    text(v, 5, 18, ui->choice ? "  BEGIN SHIFT" : "> BEGIN SHIFT", 0);
    text(v, 5, 21, ui->choice ? "> DESK GUIDE" : "  DESK GUIDE", 0);
    text(v, 3, 25, "UP/DOWN Choose     C Select", 0);
}

static void guide(DeskView *v, const DeskUI *ui) {
    header(v, "Desk guide - the clock is held.");
    panel(v, 1, 4, 38, 19);
    if (ui->guide_page == 0) {
        text(v, 4, 6, "YOUR JOB", 0);
        text(v, 4, 9, "Get waiting trams into free bays.", 0);
        text(v, 4, 12, "1  Choose a waiting tram.", 0);
        text(v, 4, 14, "2  Choose a bay marked OPEN.", 0);
        text(v, 4, 16, "3  Press C to send it home.", 0);
        text(v, 4, 19, "Each journey takes six seconds.", 1);
        text(v, 4, 21, "After 90 seconds: your report.", 1);
    } else if (ui->guide_page == 1) {
        text(v, 4, 6, "AT THE DESK", 0);
        text(v, 4, 9, "UP/DOWN     Choose a tram", 0);
        text(v, 4, 11, "LEFT/RIGHT  Choose a bay", 0);
        text(v, 4, 13, "C           Dispatch the tram", 0);
        text(v, 4, 15, "B           Clear your bay choice", 0);
        text(v, 4, 18, "A holds or resumes the clock.", 0);
        text(v, 4, 20, "Take a breath. Choose while held.", 1);
    } else {
        text(v, 4, 6, "WHICH LINE DOES MY TRAM NEED?", 0);
        text(v, 4, 9, "FROM       BAY 1/2    BAY 3/4", 1);
        text(v, 4, 11, "NORTH      UPPER      BOTH", 0);
        text(v, 4, 13, "CANAL      BOTH       BOTH", 0);
        text(v, 4, 15, "MARKET     BOTH       LOWER", 0);
        text(v, 4, 18, "U = upper line. L = lower line.", 1);
        text(v, 4, 20, "Crossing uses both: all bays wait.", 0);
    }
    text(v, 3, 24, "LEFT/RIGHT", 0);
    number(v, 21, 24, ui->guide_page + 1, 1);
    text(v, 23, 24, "OF 3", 0);
    text(v, 3, 26, ui->guide_page == 2 ? "C Return        START Close" :
                                        "C Next          START Close", 0);
}

static void desk(DeskView *v, const DeskUI *ui) {
    const Desk *d = &ui->desk;
    UIRoute selected = ui_route(d, d->bay);
    unsigned char i, row = 0, lane, busy = desk_busy(d);
    unsigned short seconds = (SHIFT_TICKS - d->tick + 59) / 60;
    header(v, "Get each tram into an open bay.");
    number(v, 26, 1, seconds / 60, 2);
    text(v, 28, 1, ":", 0); number(v, 29, 1, seconds % 60, 2);
    text(v, 32, 1, d->held ? "HELD" : "RUN ", 0);
    panel(v, 1, 4, 18, 10); panel(v, 21, 4, 18, 10);
    text(v, 3, 5, "1 WAITING TRAMS", 0);
    text(v, 23, 5, "2 CHOOSE A BAY", 0);
    text(v, 23, 6, "BAY", 1); text(v, 28, 6, "STATE", 1);
    text(v, 35, 6, "U", 1); text(v, 36, 6, "L", 1);
    for (i = 0; i < SERVICES; ++i) if (d->service[i].state == WAITING) {
        unsigned short y = 7 + 2 * row++;
        if (i == d->selected) fill(v, 2, y, 16, 1, 2);
        text(v, 3, y, i == d->selected ? ">" : " ", 0);
        number(v, 5, y, i + 101, 3);
        text(v, 9, y, d->service[i].origin == 0 ? "NORTH" :
                      d->service[i].origin == 1 ? "CANAL" : "MARKET", 0);
    }
    if (!row) text(v, 3, 8, "No waiting tram", 1);
    for (i = 0; i < 4; ++i) {
        UIRoute route = ui_route(d, i);
        unsigned char ok = desk_route_ok(d, d->selected, i);
        unsigned short y = 7 + i;
        if (d->bay == i) fill(v, 22, y, 16, 1, 2);
        text(v, 23, y, d->bay == i ? ">" : " ", 0);
        number(v, 25, y, i + 1, 1);
        text(v, 28, y, route.occupied ? "IN USE" : ok ? "OPEN" : "WAIT", ok ? 0 : 3);
        text(v, 35, y, (route.required & 1) ? "U" : "-",
             (route.blocked & 1) ? 3 : (route.required & 1) ? 0 : 1);
        text(v, 36, y, (route.required & 2) ? "L" : "-",
             (route.blocked & 2) ? 3 : (route.required & 2) ? 0 : 1);
    }
    text(v, 3, 12, "UP/DOWN: tram", 1);
    text(v, 23, 12, "LEFT/RIGHT: bay", 1);
    panel(v, 1, 14, 38, 9);
    text(v, 3, 15, "3 TRACKS", 0);
    text(v, 13, 15, "DOTS: ROUTE", 2);
    text(v, 27, 15, "HOME", 1);
    number(v, 33, 15, desk_count(d, DELIVERED), 2);
    for (lane = 0; lane < 2; ++lane) {
        unsigned short x, y = 17 + 3 * lane;
        unsigned char mask = (unsigned char)(1 << lane), wait = 0;
        text(v, 3, y, lane ? "LOWER" : "UPPER", 1);
        for (x = 10; x < 27; ++x)
            cell(v, 0, x, y, attr(v->art + ((busy & mask) ? ART_BUSY :
                                          (selected.required & mask) ? ART_PLANNED : ART_RAIL), 0));
        for (i = 0; i < SERVICES; ++i)
            if (d->service[i].state == TRANSIT &&
                (desk_mask(d->service[i].origin, d->service[i].bay) & (1 << lane))) {
                unsigned short progress = JOURNEY_TICKS - d->service[i].remaining;
                wait = (unsigned char)((d->service[i].remaining + 59) / 60);
                unsigned short tx = 10 + (progress * 15) / JOURNEY_TICKS;
                number(v, 12, y - 1, i + 101, 3);
                text(v, 16, y - 1, "TO BAY", 1);
                number(v, 23, y - 1, d->service[i].bay + 1, 1);
                cell(v, 0, tx, y, attr(v->art + ART_TRAM, 0));
                cell(v, 0, tx + 1, y, attr(v->art + ART_TRAM, 0));
            }
        text(v, 29, y, wait ? "BUSY" : "FREE", wait ? 3 : 1);
        if (wait) { number(v, 35, y, wait, 1); text(v, 36, y, "s", 0); }
    }
    fill(v, 1, 23, 38, 2, 2);
    if (d->held) text(v, 3, 23, "Clock held. Press A when ready.", 0);
    else if (d->selected == NONE) text(v, 3, 23, "Next tram arrives soon. Watch here.", 0);
    else if (d->bay == NONE) text(v, 3, 23, "LEFT/RIGHT: choose an OPEN bay.", 0);
    else if (desk_route_ok(d, d->selected, d->bay)) {
        text(v, 3, 23, "C SEND TRAM", 0); number(v, 15, 23, d->selected + 101, 3);
        text(v, 19, 23, "TO BAY", 0); number(v, 26, 23, d->bay + 1, 1);
    } else {
        text(v, 3, 23, selected.blocked == 3 ? "Both lines busy." :
                      selected.blocked == 1 ? "Upper line busy." : "Lower line busy.", 3);
        text(v, 21, 23, "Wait", 0); number(v, 26, 23, selected.wait_seconds, 1);
        text(v, 27, 23, "s.", 0);
    }
    if (!selected.required) text(v, 3, 24, "Choose a tram and bay to see a route.", 0);
    else {
        unsigned char origin = d->service[d->selected].origin;
        text(v, 3, 24, origin == 0 ? "NORTH" : origin == 1 ? "CANAL" : "MARKET", 0);
        text(v, 10, 24, "-> BAY", 1); number(v, 17, 24, d->bay + 1, 1);
        text(v, 19, 24, "USES", 1);
        text(v, 24, 24, selected.required == 3 ? "BOTH LINES" :
                       selected.required == 1 ? "UPPER LINE" : "LOWER LINE", 0);
    }
    text(v, 3, 26, "A Hold   B Clear   START Guide", 0);
}

static void report(DeskView *v, const DeskUI *ui) {
    const Desk *d = &ui->desk;
    header(v, "Shift complete. The rain goes on.");
    panel(v, 1, 4, 38, 14);
    text(v, 4, 6, "YOUR SHIFT", 0);
    text(v, 4, 9, "Trams home", 0); number(v, 32, 9, desk_count(d, DELIVERED), 2);
    text(v, 4, 11, "Still waiting", 0);
    number(v, 32, 11, desk_count(d, WAITING) + desk_backlog(d), 2);
    text(v, 4, 13, "Still on the track", 0); number(v, 32, 13, desk_count(d, TRANSIT), 2);
    text(v, 4, 15, "Total wait (tram-seconds)", 1); number(v, 29, 15, d->wait_ticks / 60, 5);
    fill(v, 3, ui->choice ? 22 : 19, 34, 2, 2);
    text(v, 5, 19, ui->choice ? "  RETRY SAME SHIFT" : "> RETRY SAME SHIFT", 0);
    text(v, 5, 22, ui->choice ? "> TITLE" : "  TITLE", 0);
    text(v, 3, 26, "UP/DOWN Choose     C Select", 0);
}

void view_init(DeskView *v) {
    unsigned short p, i;
    for (p = 0; p < VIEW_PLANES; ++p) {
        for (i = 0; i < VIEW_CELLS; ++i) v->shown[p][i] = 0xFFFF;
        v->dirty[p] = 0;
    }
    v->pending = v->clipped = 0;
}

unsigned char view_compose(DeskView *v, const DeskUI *ui,
                           unsigned short font, unsigned short art) {
    unsigned short i;
    if (v->pending) return 0;
    v->font = font; v->art = art; v->clipped = 0;
    for (i = 0; i < VIEW_CELLS; ++i) {
        v->next[0][i] = 0;
        v->next[1][i] = attr(art + ART_SOLID, 0);
    }
    if (ui->guide) guide(v, ui);
    else if (ui->screen == UI_TITLE) title(v, ui);
    else if (ui->screen == UI_REPORT) report(v, ui);
    else desk(v, ui);
    return 1;
}

unsigned char view_queue(DeskView *v, ViewRow sink, void *context) {
    unsigned char p, row, count = 0;
    unsigned short x;
    if (v->pending) return 0;
    for (p = 0; p < VIEW_PLANES; ++p) {
        v->dirty[p] = 0;
        for (row = 0; row < VIEW_H; ++row) {
            unsigned short start = row * VIEW_W;
            for (x = 0; x < VIEW_W; ++x)
                if (v->next[p][start + x] != v->shown[p][start + x]) break;
            if (x == VIEW_W) continue;
            v->dirty[p] |= 1UL << row;
            v->pending = 1;
            sink(context, p, row, v->next[p] + start);
            ++count;
        }
    }
    v->pending = count != 0;
    return count;
}

void view_complete(DeskView *v) {
    unsigned char p, row;
    unsigned short x;
    for (p = 0; p < VIEW_PLANES; ++p) {
        for (row = 0; row < VIEW_H; ++row) if (v->dirty[p] & (1UL << row))
            for (x = 0; x < VIEW_W; ++x)
                v->shown[p][row * VIEW_W + x] = v->next[p][row * VIEW_W + x];
        v->dirty[p] = 0;
    }
    v->pending = 0;
}
