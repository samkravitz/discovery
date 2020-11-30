#include <queue>
#include <iostream>

int main()
{
    std::queue<int> *q = new std::queue<int>;

    std::cout << q->empty() << "\n";

    q->push(43);

    std::cout << q->empty() << "\n";
}