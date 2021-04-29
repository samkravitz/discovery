#include "Scheduler.h"

void Scheduler::add(int until, std::function<void(void)> handler)
{
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