#ifndef ROUTE_VIEW_H
#define ROUTE_VIEW_H
#include "route.h"
/* Caller-owned column stays alive until the frame's DMA has drained. */
bool rt_column(const Route *r,u16 column,u16 words[24]);
#endif
