#ifndef TIMER_H
#define TIMER_H

#include <inttypes.h>

//extern void initTimers();

extern void setRTC(uint32_t unixTime);
extern uint32_t rtcNow();

#endif