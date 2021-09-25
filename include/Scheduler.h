#pragma once

#include "common.h"
#include <functional>
#include <list>

class Scheduler
{
public:
    Scheduler() { cycles = 0; }

    u64 cycles;

    /**
     * @param until n cycles until the callback function is called
     * @param handler callback function
     * @param id event id
     */
    void add(int, std::function<void(void)>, int id = -1);
    void advance(int);
    void remove(int);

private:
    struct Event
    {
        u64 timestamp;
        std::function<void(void)> handler;
        int id;
    };

    // TODO: use a more efficient DS than a list
    std::list<Event> events;
};