set(HEADERS
    APU.h
    Arm7Tdmi.h
    common.h
    config.h
    Discovery.h
    Gamepad.h
    IRQ.h
    LcdStat.h
    log.h
    Memory.h
    mmio.h
    PPU.h
    Scheduler.h
    Timer.h
    util.h

    backup/Backup.h
    backup/Flash.h
    backup/None.h
    backup/SRAM.h
)

PREPEND(HEADERS)

set(HEADERS ${HEADERS} PARENT_SCOPE)
