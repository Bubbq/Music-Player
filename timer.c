#include "headers/raylib.h"
#include "headers/timer.h"

#include <stdbool.h>

void start_timer(Timer *timer, double lifetime) 
{
	timer->startTime = GetTime();
	timer->lifeTime = lifetime;
}

bool is_timer_done(Timer timer)
{ 
	return (GetTime() - timer.startTime) >= timer.lifeTime; 
} 