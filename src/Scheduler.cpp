#include "Scheduler.h"
#include "log.h"

void Scheduler::add(int until, std::function<void(void)> handler)
{
    if (events.size() > 100)
        LOG("TOO BIG\n");
        
    events.push({ cycles + until, handler });
}

void Scheduler::advance(int amount)
{
    cycles += amount;

    while (!events.empty() && events.top().timestamp < cycles)
    {
        auto event = events.top();
        event.handler();
        events.pop();
    }
}