#include "Scheduler.h"
#include "log.h"
#include <cassert>
#include <vector>

void Scheduler::add(int until, std::function<void(void)> handler, int id)
{
    events.push({ cycles + until, handler, id });
    //LOG("{}\n", events.size());
    
    assert(events.size() < 100);
}

void Scheduler::advance(int amount)
{
    LOG("start\n");
    
    cycles += amount;

    
        while (!events.empty() && events.top().timestamp < cycles) {
        auto event = events.top();
        LOG("{} {} {} {}\n", event.id, event.timestamp, amount, cycles);
        
        event.handler();
        events.pop();
        }
    LOG("end\n");
    
        
}

void Scheduler::remove(int id)
{   
    for (auto iter = events.container().begin(); iter != events.container().end(); iter++)
    {   
        //LOG("{} {}\n", (u32) iter->timestamp, iter->id);
        
        if (iter->id == id)
        {
            events.container().erase(iter);
            break;
        }
    }
}