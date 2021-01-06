include 'constants.inc'

macro m_half reg, value {
        mov     reg, value and 0xFF
        orr     reg, value and 0xFF00
}

macro m_word reg, value {
        mov     reg, value and 0xFF
        orr     reg, value and 0xFF00
        orr     reg, value and 0xFF0000
        orr     reg, value and 0xFF000000
}

macro m_vsync {
local .vblank_end
local .vblank_beg

        stmfd   sp!, \{r0-r1\}
        mov     r0, MEM_IO

.vblank_end:
        ldr     r1, [r0, REG_DISPSTAT]
        tst     r1, STAT_VBLANK_FLG
        bne     .vblank_end             ; Loop until not in vblank

.vblank_beg:
        ldr     r1, [r0, REG_DISPSTAT]
        tst     r1, STAT_VBLANK_FLG
        beq     .vblank_beg             ; Loop until in vblank
        ldmfd   sp!, \{r0-r1\}
}

macro m_text_init {
        bl      text_init
}

macro m_text_color color, index {
        m_half  r0, color
        mov     r1, index
        bl      text_color
}

macro m_text_pos x, y {
        mov     r0, x
        mov     r1, y
}

macro m_text_char char {
        mov     r2, char
        bl      text_char
}

; prints a register's value to the screen
macro m_print_reg val {
        mov r8, val

        mov r2, '0'
        bl text_char
        mov r2, 'x'
        bl text_char
        
        ; bits 31-28
        mov r2, r8, lsr 28
        and r2, 0xF
        add r2, 48
        bl text_char

        ; bits 27-24
        mov r2, r8, lsr 24
        and r2, 0xF
        add r2, 48
        bl text_char

        ; bits 23-20
        mov r2, r8, lsr 20
        and r2, 0xF
        add r2, 48
        bl text_char

        ; bits 19-16
        mov r2, r8, lsr 16
        and r2, 0xF
        add r2, 48
        bl text_char

        ; bits 15-12
        mov r2, r8, lsr 12
        and r2, 0xF
        add r2, 48
        bl text_char

        ; bits 11-8
        mov r2, r8, lsr 8
        and r2, 0xF
        add r2, 48
        bl text_char

        ; bits 7-4
        mov r2, r8, lsr 4
        and r2, 0xF
        add r2, 48
        bl text_char

        ; bits 3-0
        mov r2, r8, lsr 0
        and r2, 0xF
        add r2, 48
        bl text_char
}

