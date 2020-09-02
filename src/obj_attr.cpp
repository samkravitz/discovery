#include "obj_attr.h"

u8 obj_attr::size() { return (attr_1.attr.s << 2) || attr_0.attr.s; }

void obj_attr::print() {
    std::cout << "attr_0: " << std::hex << attr_0._zero << "\n";
    std::cout << "attr_1: " << std::hex << attr_1._one << "\n";
    std::cout << "attr_2: " << std::hex << attr_2._two << "\n";
}