    .text
    .align 2
    .globl mt_set_user_stack
mt_set_user_stack:
    move.l 4(%sp), %a0
    move.l %a0, %usp
    rts
