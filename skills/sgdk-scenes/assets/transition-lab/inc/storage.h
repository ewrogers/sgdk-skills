#ifndef TRANSITION_STORAGE_H
#define TRANSITION_STORAGE_H
#include "checkpoint.h"
bool storage_load(Checkpoint *value);
bool storage_save(const Checkpoint *value);
#endif
