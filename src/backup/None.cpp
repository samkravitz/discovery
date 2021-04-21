#include "None.h"

#include <cassert>

None::None(int size) :
    Backup(size)
{
    assert(size == 0x8000);
}

void None::write(int index, u8 value) { }
u8 None::read(int index) { return 0xFF; }
