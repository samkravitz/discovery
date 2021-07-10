set(SOURCES
    APU.cpp
    Arm7Tdmi.cpp
    ArmISA.cpp
    config.cpp
    Discovery.cpp
    Gamepad.cpp
    IRQ.cpp
    main.cpp
    Memory.cpp
    PPU.cpp
    Scheduler.cpp
    swi.cpp
    ThumbISA.cpp
    Timer.cpp
    util.cpp

    backup/Flash.cpp
    backup/None.cpp
    backup/SRAM.cpp
)

PREPEND(SOURCES)

set(SOURCES ${SOURCES} PARENT_SCOPE)
