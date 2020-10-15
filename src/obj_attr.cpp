#include "obj_attr.h"

u8 obj_attr::size()
{
    u8 size = 0;
    size |= attr_0.attr.s;
    size <<= 2;
    size |= attr_1.attr.s;
    return size;
}

void obj_attr::print()
{
    std::cout << "attr_0: " << std::hex << attr_0._zero << "\n";
    std::cout << "attr_1: " << std::hex << attr_1._one << "\n";
    std::cout << "attr_2: " << std::hex << attr_2._two << "\n";
}