#include "Scheduler.h"
#include "log.h"
#include <cassert>
#include <vector>

void Scheduler::add(int until, std::function<void(void)> handler, int id)
{
    events.push({ cycles + until, handler, id });
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

void Scheduler::remove(int id)
{   
    for (auto iter = events.container().begin(); iter != events.container().end(); iter++)
    {
        if (iter->id == id)
        {
            events.container().erase(iter);
            break;
        }
    }
}