    .h8300h
    .section .text
    .global _start
    .type _start,@function
_start:
    mov.l   #_stack, sp   #自動変数が配置されるスタック領域をRAM上にしていする。RAM上であれば、読み書き可能。
    jsr     @_main

1:
    bra 1b  #戻り先アドレスをスタックに保存せずにジャンプする。ジャンプ先は「1:」。そのため、無限ループする。(main関数を抜けた後、変なコードを実行することを防ぐため)
