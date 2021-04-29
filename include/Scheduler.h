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
    void add(int, std::function<void(void)>, int id = -1);
    void advance(int);
    void remove(int);

private:
    struct Event
    {
        u64 timestamp;
        std::function<void(void)> handler;
        int id;

        inline bool operator > (Event const &rhs) const { return timestamp > rhs.timestamp; }
    };

    template<class pq>
    struct Events : public pq
    {
        pq::container_type &container() { return this->c; }
    };

    Events<std::priority_queue<Event, std::vector<Event>, std::greater<Event>>> events;
};