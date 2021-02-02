format binary as 'gba'

include 'lib/header.asm'

align 4

color = 0

main:
    ; enable interrupts in cpsr
    mrs r0, cpsr
    bic r0, r0, #0x80
    msr cpsr_c, r0

	mov r0, #0x4000000
	mov r1, #0x400
	add r1, r1, #3
	str r1, [r0]

    ; enable vblank interrupt in dispstat
    mov r0, #0x4000000
    add r0, r0, 4
    mov r1, #1 shl 3
    strh r1, [r0]

    ; enable interrupts
    mov r0, #0x4000000
    add r0, r0, 0x200
    add r0, r0, 8
    mov r1, #1
    strb r1, [r0]

    ; enable V Blank interrupt
    mov r0, #0x4000000
    add r0, r0, 0x200
    mov r1, #1
    strh r1,[r0]

    ; move handler code to ISR addr
    mov r0, #0x3000000
    add r0, r0, #0x8000
    sub r0, r0, 4
    mov r1, handler
    str r1, [r0]

    mov r0, #0x6000000
	mov r1, #0xFF
	mov r2, #0x9600
    ;add r1, r1, #1

    loop2:
        strh r1, [r0], #2
        subs r2, r2, #1
        bne loop2


infin:
	b infin

handler:
    ; disable interrupts
    mov r0, #0x4000000
    add r0, r0, 0x200
    add r0, r0, 8
    mov r1, 0
    strh r1, [r0]

    mov r0, #0x6000000
	mov r1, color
	mov r2, #0x9600
    add r1, r1, #1

    loop:
        strh r1, [r0], #2
        subs r2, r2, #1
        bne loop
    
    ; enable interrupts
    mov r0, #0x4000000
    add r0, r0, 0x200
    add r0, r0, 8
    mov r1, 1
    str r1, [r0]

    ; acknowledge interrupt has been serviced
    mov r0, #0x4000000
    add r0, r0, 4
    mov r1, #1 shl 3
    strh r1, [r0]

    ; return control to system
    bx lr


