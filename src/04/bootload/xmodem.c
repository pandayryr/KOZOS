#include "defines.h"
#include "serial.h"
#include "lib.h"
#include "xmodem.h"

//制御コード定義
#define XMODEM_SOH 0x01 //
#define XMODEM_STX 0x02 //
#define XMODEM_EOT 0x04 //データの終わりに送信される
#define XMODEM_ACK 0x06 //受信成功時の応答
#define XMODEM_NAK 0x15 //受信エラー時の応答。受信準備完了時も送信される。
#define XMODEM_CAN 0x18 //送受信の中断時に送信する。
#define XMODEM_EOF 0x1a //Ctrl_z

#define XMODEM_BLOCK_SIZE (128)
/*
    アドレス(?) サイズ      内容
    0           1byte      SOH
    1           1byte      ブロック番号。0~255。1からの連番。
    2           1byte      ブロック番号をビット反転したもの(チェック用)
    3           128byte    データ。データが128バイトに満たない場合は、EOFで空きを埋める
    131         1byte      データ部のチェックサム。
*/

static int xmodem_wait(void)
{
    long cnt = 0;

    while(!serial_is_recv_enable(SERIAL_DEFAULT_DEVICE))
    {
        if(++cnt >= 2000000)
        {
            cnt = 0;
            serial_send_byte(SERIAL_DEFAULT_DEVICE, XMODEM_NAK);
        }
    }
    return 0;
}

static int xmodem_read_block(unsigned char block_number, char* buf)
{
    unsigned char c, block_num, check_sum;
    int i;

    block_num = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
    if(block_num != block_number)
    {
        return -1;
    }

    block_num ^= serial_recv_byte(SERIAL_DEFAULT_DEVICE);
    if(block_num != 0xff)
    {
        return -1;
    }

    check_sum = 0;
    for(i = 0; i < XMODEM_BLOCK_SIZE; i++)
    {
        c = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
        *(buf++) = c;
        check_sum += c;
    }

    check_sum ^= serial_recv_byte(SERIAL_DEFAULT_DEVICE);
    if(check_sum)
    {
        return -1;
    }

    return i;
}

long xmodem_recv(char* buf)
{
    int r, receiving = 0;
    long size = 0;
    unsigned char c, block_number = 1;

    while(1)
    {
        if(!receiving)
        {
            xmodem_wait();
        }

        c = serial_recv_byte(SERIAL_DEFAULT_DEVICE);

        if(c == XMODEM_EOT)
        {
            serial_send_byte(SERIAL_DEFAULT_DEVICE, XMODEM_ACK);
            break;
        }
        else if(c == XMODEM_CAN)
        {
            return -1;
        }
        else if(c == XMODEM_SOH)
        {
            receiving++;
            r = xmodem_read_block(block_number, buf);
            if(r < 0)
            {
                serial_send_byte(SERIAL_DEFAULT_DEVICE, XMODEM_NAK);
            }
            else
            {
                block_number++;
                size += r;
                buf += r;
                serial_send_byte(SERIAL_DEFAULT_DEVICE, XMODEM_ACK);
            }
        }
        else
        {
            if(receiving)
            {
                return -1;
            }
        }
    }

    return size;
}
