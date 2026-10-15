#include "scene_switch.h"
void scene_switch_init(SceneSwitch *flow)
{
    *flow = (SceneSwitch){SCENE_TITLE, SCENE_TITLE, SWITCH_IDLE, 0, false};
}
bool scene_request(SceneSwitch *flow, SceneId next)
{
    if (!flow || flow->phase != SWITCH_IDLE || (unsigned)next >= SCENE_COUNT) return false;
    flow->target = next; flow->phase = SWITCH_BEGIN;
    return true;
}
void scene_switch_step(SceneSwitch *flow, const ScenePort *port)
{
    if (flow->phase == SWITCH_BEGIN) {
        port->fade_out(); flow->phase = SWITCH_OUT;
    } else if (flow->phase == SWITCH_OUT) {
        if (!port->fade_done()) return;
        port->leave();
        if (!port->enter(flow->target)) {
            /* Clean a partially constructed scene before trying the safe title. */
            port->leave(); flow->target = SCENE_TITLE; flow->recovered = true;
            if (!port->enter(flow->target)) { flow->phase = SWITCH_FAILED; return; }
        }
        flow->current = flow->target; ++flow->completed;
        port->fade_in(); flow->phase = SWITCH_IN;
    } else if (flow->phase == SWITCH_IN && port->fade_done()) flow->phase = SWITCH_IDLE;
}
