    .h8300h
    .section .text
    .global _start
    .type _start,@function
_start:
    mov.l   #0xffff00, sp   #自動変数が配置されるスタック領域をRAM上にしていする。RAM上であれば、読み書き可能。
    jsr     @_main

1:
    bra 1b
