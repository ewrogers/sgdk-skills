#include "dialogue.h"

u8 dlg_columns(const DlgNode *node)
{
    return node->portrait == DL_NO_PORTRAIT ? DL_COLUMNS : 29;
}

static bool short_text(const char *text, u16 limit)
{
    if (!text) return false;
    for (u16 i=0; i<=limit; i++)
    {
        if (!text[i]) return true;
        if ((u8)text[i] < 32 || (u8)text[i] > 126) return false;
    }
    return false;
}

bool dlg_validate(const DlgProgram *program)
{
    if (!program || !program->nodes || !program->count || program->count > DL_MAX_NODES) return false;
    for (u16 i=0; i<program->count; i++)
    {
        const DlgNode *node=&program->nodes[i];
        if (!node->tokens || !node->token_count || node->token_count > DL_MAX_TOKENS ||
            node->pane > 1 || (node->portrait > 1 && node->portrait != DL_NO_PORTRAIT) ||
            node->flags > (DL_SPLIT|DL_NAMEPLATE) || node->choice_count > DL_CHOICES ||
            !short_text(node->speaker,12) || (node->next != DL_END && node->next >= program->count)) return false;
        for (u16 c=0; c<node->choice_count; c++)
        {
            const DlgChoice *choice=&node->choices[c];
            if (!short_text(choice->label,26) || !*choice->label || choice->target >= program->count ||
                ((choice->require|choice->forbid|choice->set) & ~program->flag_mask) ||
                (choice->require & choice->forbid)) return false;
        }
        u16 actions=0; u8 column=0,row=0; bool content=false;
        for (u16 t=0; t<node->token_count; t++)
        {
            const DlgToken token=node->tokens[t];
            if (++actions > DL_PAGE_ACTIONS) return false;
            if (token.op == DL_GLYPH)
            {
                if (token.value < 32 || token.value > 126 || token.style > 5 || column >= dlg_columns(node)) return false;
                column++; content=true;
            }
            else
            {
                if (token.style) return false;
                switch (token.op)
                {
                    case DL_NEWLINE: if (token.value || ++row >= DL_ROWS) return false; column=0; break;
                    case DL_PAGE:
                        if (token.value || !content) return false;
                        row=column=0; actions=0; content=false; break;
                    case DL_SPEED: if (token.value > 1000) return false; break;
                    case DL_PAUSE: if (!token.value || token.value > 5000) return false; break;
                    case DL_FINISH: if (token.value || !content || t+1 != node->token_count) return false; break;
                    default: return false;
                }
            }
        }
        if (node->tokens[node->token_count-1].op != DL_FINISH) return false;
    }
    return true;
}

static void clear_pane(DlgPane *pane)
{
    for (u16 i=0; i<DL_CELLS; i++) pane->cells[i]=(DlgCell){0,0};
}

static void enter(Dialogue *dialogue, u16 id)
{
    const DlgNode *node=&dialogue->program->nodes[id];
    dialogue->node=id; dialogue->token=0; dialogue->wait_ms=0; dialogue->speed_ms=30;
    dialogue->row=dialogue->column=dialogue->selected=dialogue->visible_count=0;
    dialogue->state=DL_TYPING;
    clear_pane(&dialogue->panes[node->pane]);
    dialogue->panes[node->pane].node=node;
    if (!(node->flags & DL_SPLIT)) dialogue->panes[1-node->pane].node=0;
    dialogue->revision++; dialogue->events |= DL_ENTER_EVENT;
}

bool dlg_start(Dialogue *dialogue, const DlgProgram *program, u16 start, u16 flags)
{
    if (!dialogue) return false;
    *dialogue=(Dialogue){0};
    dialogue->state=DL_ERROR;
    if (!dlg_validate(program) || start >= program->count || (flags & ~program->flag_mask)) return false;
    dialogue->program=program; dialogue->flags=flags; enter(dialogue,start);
    return true;
}

static void finish(Dialogue *dialogue)
{
    const DlgNode *node=&dialogue->program->nodes[dialogue->node];
    dialogue->visible_count=0;
    for (u8 i=0; i<node->choice_count; i++)
    {
        const DlgChoice *choice=&node->choices[i];
        if ((dialogue->flags & choice->require) == choice->require && !(dialogue->flags & choice->forbid))
            dialogue->visible[dialogue->visible_count++]=i;
    }
    dialogue->state=dialogue->visible_count ? DL_CHOOSE : DL_NODE_WAIT;
}

static void reveal(Dialogue *dialogue, u16 elapsed, bool instant)
{
    const DlgNode *node=&dialogue->program->nodes[dialogue->node];
    for (u16 work=0; work<DL_PAGE_ACTIONS; work++)
    {
        if (!instant)
        {
            if (dialogue->wait_ms > elapsed) { dialogue->wait_ms-=elapsed; return; }
            elapsed-=dialogue->wait_ms;
        }
        dialogue->wait_ms=0;
        const DlgToken token=node->tokens[dialogue->token++];
        switch (token.op)
        {
            case DL_GLYPH:
                dialogue->panes[node->pane].cells[dialogue->row*DL_COLUMNS+dialogue->column++]=
                    (DlgCell){(u8)token.value,token.style};
                dialogue->wait_ms=dialogue->speed_ms;
                dialogue->events |= DL_REVEAL_EVENT;
                dialogue->revision++; break;
            case DL_NEWLINE: dialogue->column=0; dialogue->row++; break;
            case DL_SPEED: dialogue->speed_ms=token.value; break;
            case DL_PAUSE: dialogue->wait_ms=token.value; break;
            case DL_PAGE: dialogue->state=DL_PAGE_WAIT; dialogue->revision++; return;
            case DL_FINISH: finish(dialogue); dialogue->revision++; return;
            default: dialogue->state=DL_ERROR; return;
        }
    }
    dialogue->state=DL_ERROR;
}

void dlg_step(Dialogue *dialogue, u16 elapsed_ms, u16 pressed)
{
    if (!dialogue) return;
    dialogue->events=0;
    if (dialogue->state == DL_ERROR || dialogue->state == DL_DONE) return;
    if (elapsed_ms > 1000) elapsed_ms=1000;
    if (dialogue->state == DL_TYPING)
    {
        reveal(dialogue,elapsed_ms,(pressed & DL_ACCEPT) != 0);
        return;
    }
    if (dialogue->state == DL_CHOOSE)
    {
        if (pressed & DL_UP) dialogue->selected=(dialogue->selected+dialogue->visible_count-1)%dialogue->visible_count;
        else if (pressed & DL_DOWN) dialogue->selected=(dialogue->selected+1)%dialogue->visible_count;
        if (pressed & (DL_UP|DL_DOWN)) dialogue->revision++;
        if (pressed & DL_ACCEPT)
        {
            const DlgNode *node=&dialogue->program->nodes[dialogue->node];
            const DlgChoice *choice=&node->choices[dialogue->visible[dialogue->selected]];
            dialogue->flags |= choice->set;
            dialogue->events |= DL_CHOICE_EVENT;
            enter(dialogue,choice->target);
        }
        return;
    }
    if (!(pressed & DL_ACCEPT)) return;
    dialogue->events |= DL_PAGE_EVENT;
    if (dialogue->state == DL_PAGE_WAIT)
    {
        const DlgNode *node=&dialogue->program->nodes[dialogue->node];
        clear_pane(&dialogue->panes[node->pane]);
        dialogue->row=dialogue->column=0; dialogue->wait_ms=0;
        dialogue->state=DL_TYPING; dialogue->revision++;
    }
    else
    {
        const u16 next=dialogue->program->nodes[dialogue->node].next;
        if (next == DL_END) { dialogue->state=DL_DONE; dialogue->revision++; dialogue->events |= DL_DONE_EVENT; }
        else enter(dialogue,next);
    }
}

bool dlg_has_more(const Dialogue *dialogue)
{
    return dialogue && (dialogue->state == DL_PAGE_WAIT ||
        (dialogue->state == DL_NODE_WAIT && dialogue->program->nodes[dialogue->node].next != DL_END));
}
