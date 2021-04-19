#include "None.h"

#include <cassert>

None::None(int size) :
    Backup(size)
{
    assert(size == 0x8000);
}

void None::write(u32 index, u8 value) { }
u8 None::read(u32 index) { return 0xFF; }
