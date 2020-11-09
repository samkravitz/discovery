format binary as 'gba'

include './lib/macros.inc'

header:
    include './lib/header.asm'

main:
    m_text_init
    m_text_color 0xFFFF, 1
    m_vsync

    m_text_pos 72, 60

    m_text_char 's'
    m_text_char 'w'
    m_text_char 'i'
    m_text_char ' '
    m_text_char 'd'
    m_text_char 'i'
    m_text_char 'v'
    m_text_char 'i'
    m_text_char 's'
    m_text_char 'i'
    m_text_char 'o'
    m_text_char 'n'

    mov r0, 200
    mov r1, 10

    swi 0x60000
    mov r6, r0

    m_text_pos 50, 76
    m_text_char '2'
    m_text_char '0'
    m_text_char '0'
    m_text_char '/'
    m_text_char '1'
    m_text_char '0'
    m_text_char ' '
    m_text_char '='
    m_text_char ' '
    
    m_print_reg r6

idle:
    b idle

include './lib/text.asm'