#include "defines.h"
#include "serial.h"
#include "lib.h"

static int init(void)
{
    /* リンカ・スクリプトで定義したシンボル */
    extern int erodata, data_start, edata, bss_start, ebss;

    /* データ領域とBSS領域を初期化する */
    //&data_start: RAMの.dataセクションの先頭
    //&erodata： ROMの.dataセクションの先頭 = .rodataの終端(.rodata → AT>.dataという配置になっているから)
    memcpy(&data_start, &erodata, (long)&edata - (long)&data_start);
    memset(&bss_start, 0, (long)&ebss - (long)&bss_start);

    serial_init(SERIAL_DEFAULT_DEVICE);
}

int main(void)
{
    puts("Hello world!\n");

    putxval(0x10, 0);   puts("\n");
    putxval(0xffff, 0); puts("\n");

    while(1);

    return 0;
}
