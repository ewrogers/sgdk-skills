#ifndef TRANSITION_SCENE_SWITCH_H
#define TRANSITION_SCENE_SWITCH_H
#include "lab.h"
typedef enum { SWITCH_IDLE, SWITCH_BEGIN, SWITCH_OUT, SWITCH_IN, SWITCH_FAILED } SwitchPhase;
typedef struct { SceneId current, target; SwitchPhase phase; u32 completed; bool recovered; } SceneSwitch;
/* These are this example's C functions, not an SGDK scene API. */
typedef struct {
    void (*fade_out)(void);
    void (*fade_in)(void);
    bool (*fade_done)(void);
    void (*leave)(void);
    bool (*enter)(SceneId scene);
} ScenePort;
void scene_switch_init(SceneSwitch *flow);
bool scene_request(SceneSwitch *flow, SceneId next);
void scene_switch_step(SceneSwitch *flow, const ScenePort *port);
#endif
