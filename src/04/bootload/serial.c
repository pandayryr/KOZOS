#include "defines.h"
#include "serial.h"

//---------------------------------------------
#define SERIAL_SCI_NUM 3

/*
    H8マイコンのCPUでのシリアル・コントローラのアドレス
    H8マイコンのシリアル・コネクタにはSCI1が繋がっている
*/
#define H8_3069F_SCI0 ((volatile struct h8_3069f_sci *)0xffffb0)
#define H8_3069F_SCI1 ((volatile struct h8_3069f_sci *)0xffffb8)
#define H8_3069F_SCI2 ((volatile struct h8_3069f_sci *)0xffffc0)

/*
    シリアル・コントローラのレジスタのマッピング
*/
struct h8_3069f_sci
{
    volatile uint8 smr;     //シリアル通信モードの設定。シリアルモードレジスタ。
    volatile uint8 brr;     //ボーレート(転送速度)の設定。ビットレートレジスタ。
    volatile uint8 scr;     //送受信の有効/無効など。シリアルコントロールレジスタ。
    volatile uint8 tdr;     //送信したい1byteの情報を書き込む
    volatile uint8 ssr;     //送信完了/受信官僚などを表す。シリアルステータスレジスタ。
    volatile uint8 rdr;     //受信した1byteの情報
    volatile uint8 scmr;    //？
};

//SMR bit define
/*
    bit位置     意味
    0,1         クロックセレクト。共にならクロックをそのまま利用する。
    2           マルチプロセッサ機能を選択する。0でマルチプロセッサ機能を禁止する。調歩同期式モードのときのみ有効。
    3           ストップビット長。0で1bit、1で2bit。調歩同期式モードのときのみ有効。
    4           パリティの種類。0で偶数パリティ。1で奇数パリティ。
    5           0でパリティ無効。1でパリティ無効。
    6           調歩同期式モードのデータ長。0で8bit、1で7bit。
    7           0で調歩同期式モード。1でクロック同期式モード。

    現在は　データ長：8bit、ストップビット長：1bit、パリティ：無効　の設定が主流。
    よって、基本的にSMRレジスタの値は0でよい。
*/
#define H8_3069F_SCI_SMR_CKS_PER1   (0<<0)
#define H8_3069F_SCI_SMR_CKS_PER4   (1<<0)
#define H8_3069F_SCI_SMR_CKS_PER16  (2<<0)
#define H8_3069F_SCI_SMR_CKS_PER64  (3<<0)
#define H8_3069F_SCI_SMR_MP         (1<<2)
#define H8_3069F_SCI_SMR_STOP       (1<<3)
#define H8_3069F_SCI_SMR_OE         (1<<4)
#define H8_3069F_SCI_SMR_PE         (1<<5)
#define H8_3069F_SCI_SMR_CHR        (1<<6)
#define H8_3069F_SCI_SMR_CA         (1<<7)

//SCR bit define
/*
    bit位置     意味
    0,1         clock enable。ひとまず、0でよい。
    2           送信終了割込み要求(TEI)を許可/禁止する。1で有効。
    3           マルチプロセッサ割り込みを許可/禁止する。1で有効。
    4           受信可能判定。1で受信開始。
    5           送信可能判定。1で送信開始。
    6           受信データフル割込み(RXI)可能判定。1で受信割込み有効。
    7           送信データエンプティ割込み(TXI)可能判定。1で送信割込み有効。

    初期化時は0に設定。コントローラの設定変更はコントローラを無効化した状態で行うのが一般的。
    その後、必要に応じてbitを立てる。
*/
#define H8_3069F_SCI_SCR_CKE0   (1<<0)
#define H8_3069F_SCI_SCR_CKE1   (1<<1)
#define H8_3069F_SCI_SCR_TEIE   (1<<2)
#define H8_3069F_SCI_SCR_MPIE   (1<<3)
#define H8_3069F_SCI_SCR_RE     (1<<4)
#define H8_3069F_SCI_SCR_TE     (1<<5)
#define H8_3069F_SCI_SCR_RIE    (1<<6)
#define H8_3069F_SCI_SCR_TIE    (1<<7)

//SSR bit define
/*
    bit位置     意味
    0           送信時のマルチプロセッサビットの値を設定する。
    1           受信したマルチプロセッサビットを格納する領域。
    2           送信終了を示すステータスフラグ。0:送信中、1:送信完了。
    3           受信時のパリティエラーの検出の有無を示す。0:受信中、または正常に受信完了。1:受信時にパリティエラー検出。
    4           受信フレーミングエラーの検出の有無を示す。0:受信中、または正常に受信完了。1:受信時にフレーミングエラーが発生。
    5           受信時にオーバランエラーの検出の有無を示す。0:受信中、または正常に受信完了。1:受信時にオーバランエラー発生。
    6           受信を完了し、RDRにデータが格納されたことを示す。0:未受信。RDRにデータなし。1:受信完了。RDRにデータあり。
    7           送信完了し、TDRにデータを書き込めることを示す。0:送信中。TDRに送信データあり。1:送信完了。TDRにデータなし。
*/
#define H8_3069F_SCI_SSR_MPBT   (1<<0)
#define H8_3069F_SCI_SSR_MPB    (1<<1)
#define H8_3069F_SCI_SSR_TEND   (1<<2)
#define H8_3069F_SCI_SSR_PER    (1<<3)
#define H8_3069F_SCI_SSR_FERERS (1<<4)
#define H8_3069F_SCI_SSR_ORER   (1<<5)
#define H8_3069F_SCI_SSR_RDRF   (1<<6)
#define H8_3069F_SCI_SSR_TDRE   (1<<7)

//---------------------------------------------
static struct
{
    volatile struct h8_3069f_sci *sci;
} regs[SERIAL_SCI_NUM] = 
{
    {H8_3069F_SCI0},
    {H8_3069F_SCI1},
    {H8_3069F_SCI2},
};

//---------------------------------------------
int serial_init(int index)
{
    volatile struct h8_3069f_sci *sci = regs[index].sci;

    sci->scr = 0;                                           //コントローラのモード設定のため、一度無効化する。
    sci->smr = 0;
    sci->brr = 64;                                          //ビットレート9600、20MHzで行う。
    sci->scr = H8_3069F_SCI_SCR_RE | H8_3069F_SCI_SCR_TE;   //送受信可能状態にする
    sci->ssr = 0;

    return 0;
}

int serial_is_send_enable(int index)
{
    volatile struct h8_3069f_sci* sci = regs[index].sci;

    //SSRレジスタのTDREビットが立っているかを確認。立っていれば送信可能。
    return (sci->ssr & H8_3069F_SCI_SSR_TDRE);
}

int serial_send_byte(int index, unsigned char c)
{
    volatile struct h8_3069f_sci* sci = regs[index].sci;

    while(!serial_is_send_enable(index));
    sci->tdr = c;                           //シリアルに送信するデータを格納。
    sci->ssr &= ~H8_3069F_SCI_SSR_TDRE;     //送信完了ビットを落とし、送信を開始する。送信完了後は、SCIコントローラが自動でビットを立てる。

    return 0;
}

int serial_is_recv_enable(int index)
{
    volatile struct h8_3069f_sci* sci = regs[index].sci;

    //SSRレジスタのRDRFビットが立っているかを確認。立っていれば受信したデータが存在する。受信完了時にSCIコントローラが自動でビットを立てる。
    return (sci->ssr & H8_3069F_SCI_SSR_RDRF);
}

unsigned char serial_recv_byte(int index)
{
    volatile struct h8_3069f_sci* sci = regs[index].sci;
    unsigned char c;

    while(!serial_is_send_enable(index));
    c = sci->rdr;                           //シリアルに送信するデータを格納。
    sci->ssr &= ~H8_3069F_SCI_SSR_RDRF;     //受信完了ビットを落とし、次のデータを受信可能にする。

    return c;
}
