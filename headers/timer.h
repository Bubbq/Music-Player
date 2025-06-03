#include <stdbool.h>

typedef struct
{
	double startTime;
	double lifeTime;
} Timer;

bool is_timer_done(Timer timer);
void start_timer(Timer *timer, double lifetime); 