#pragma once

#include "common.h"
#include <queue>
#include <functional>
#include <vector>

class Scheduler
{
public:
    Scheduler() { cycles = 0; }

    u64 cycles;
    void add(int, std::function<void(void)>);
    void advance(int);

private:
    struct Event
    {
        u64 timestamp;
        std::function<void(void)> handler;

        inline bool operator > (Event const &rhs) const { return timestamp > rhs.timestamp; }
    };

    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> events;
};