#include "Scheduler.h"
#include "log.h"
#include <cassert>
#include <vector>

void Scheduler::add(int until, std::function<void(void)> handler, int id)
{
    auto iter = events.begin();
    while (iter != events.end() && (iter->timestamp < cycles + until))
        iter++;
    
    events.insert(iter, { cycles + until, handler, id });
}

void Scheduler::advance(int amount)
{
    cycles += amount;   
    while (!events.empty() && events.front().timestamp < cycles)
    {
        auto event = events.front();
        
        event.handler();
        events.pop_front();
    }
}

void Scheduler::remove(int id)
{   
    for (auto iter = events.begin(); iter != events.end(); iter++)
    {
        if (iter->id == id)
        {
            events.erase(iter);
            break;
        }
    }
}