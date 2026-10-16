#ifndef DL_DIALOGUE_H
#define DL_DIALOGUE_H
#include "dl_types.h"
#define DL_END 65535u
#define DL_NO_PORTRAIT 255u
#define DL_COLUMNS 34
#define DL_ROWS 4
#define DL_CELLS (DL_COLUMNS * DL_ROWS)
#define DL_CHOICES 3
#define DL_MAX_NODES 64
#define DL_MAX_TOKENS 2048
#define DL_PAGE_ACTIONS 256

enum { DL_GLYPH, DL_NEWLINE, DL_PAGE, DL_PAUSE, DL_SPEED, DL_FINISH };
enum { DL_SPLIT=1, DL_NAMEPLATE=2 };
enum { DL_UP=1, DL_DOWN=2, DL_ACCEPT=4 };
enum { DL_REVEAL_EVENT=1, DL_PAGE_EVENT=2, DL_CHOICE_EVENT=4, DL_ENTER_EVENT=8, DL_DONE_EVENT=16 };
typedef enum { DL_TYPING, DL_PAGE_WAIT, DL_NODE_WAIT, DL_CHOOSE, DL_DONE, DL_ERROR } DlgState;
typedef struct { u16 value; u8 op, style; } DlgToken;
typedef struct { u8 glyph, style; } DlgCell;
typedef struct {
    const char *label;
    u16 target, require, forbid, set;
} DlgChoice;
typedef struct {
    const DlgToken *tokens;
    const char *speaker;
    DlgChoice choices[DL_CHOICES];
    u16 token_count, next;
    u8 pane, portrait, flags, choice_count;
} DlgNode;
typedef struct { const DlgNode *nodes; u16 count, flag_mask; } DlgProgram;
typedef struct { DlgCell cells[DL_CELLS]; const DlgNode *node; } DlgPane;
typedef struct {
    const DlgProgram *program;
    DlgPane panes[2];
    u16 node, token, wait_ms, speed_ms, flags, revision, events;
    u8 row, column, selected, visible[DL_CHOICES], visible_count;
    DlgState state;
} Dialogue;

/* Program/token/string storage is borrowed for the whole conversation.
 * Only compiled, immutable program data is supported. Validate once at start. */
bool dlg_validate(const DlgProgram *program);
bool dlg_start(Dialogue *dialogue, const DlgProgram *program, u16 start, u16 flags);
/* Elapsed milliseconds: 0..1000. Input is press edges, not held buttons.
 * No page, choice or node transition consumes the same edge twice. */
void dlg_step(Dialogue *dialogue, u16 elapsed_ms, u16 pressed);
bool dlg_has_more(const Dialogue *dialogue);
u8 dlg_columns(const DlgNode *node);
#endif
