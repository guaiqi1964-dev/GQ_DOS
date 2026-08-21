; GQ_DOS 中断桩函数 (NASM, 64-bit)
bits 64

section .text

extern exception_handler
extern irq_handler

%macro pushall 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popall 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

; 带错误码的异常（CPU 已压入 err_code）
%macro ISR_ERR 1
global isr%1
isr%1:
    push %1
    jmp isr_common
%endmacro

; 无错误码的异常（压入 dummy 0）
%macro ISR_NOERR 1
global isr%1
isr%1:
    push 0
    push %1
    jmp isr_common
%endmacro

isr_common:
    pushall
    mov rdi, rsp
    call exception_handler
    popall
    add rsp, 16
    iretq

; IRQ 桩
%macro IRQ 1
global irq%1
irq%1:
    push 0
    push (0x20 + %1)
    jmp irq_common
%endmacro

irq_common:
    pushall
    mov rdi, rsp
    call irq_handler
    popall
    add rsp, 16
    iretq

; 异常 0-31（有错误码的：8,10,11,12,13,14,17,21,29,30）
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_ERR 29
ISR_ERR 30
ISR_NOERR 31

IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15

; APIC 定时器桩 (向量 0x30)
global apic_timer_stub
apic_timer_stub:
    push 0
    push 0x30
    jmp irq_common

; 桩函数表
section .data
global isr_stub_table
isr_stub_table:
    %assign i 0
    %rep 32
    dq isr%+i
    %assign i i+1
    %endrep

global irq_stub_table
irq_stub_table:
    %assign i 0
    %rep 16
    dq irq%+i
    %assign i i+1
    %endrep
