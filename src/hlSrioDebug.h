#ifndef _HLSRIO_DEBUG_H
#define  _HLSRIO_DEBUG_H

#ifdef HLSRIO_DEBUG_INTERFACE

#include "hlType.h"

/*
 * 将1848端口的统计功能打开,这样可以统一端口收到和转发的包数.
 * 使能hlSetPortCounter(1)
 * 不使能hlSetPortCounter(0)
 */
void hlSetPortCounter(char bEnable);

/*
 * 显示指定交换芯片的所有寄存器的值，在完成hlSrioInit后，可以通过该函数对指定的
 * 交换芯片中所有寄存器值进行显示。swIndex为指定交换芯片的索引号。
 */
void hlReadSwitchAllReg(UINT8 swIndex);

/*
 * 完成对指定交换芯片指定offset的寄存器进行写操作，在完成hlSrioInit后，可以通过该函
 * 数对指定的交换芯片中的指定寄存器值进行写操作。
 * swIndex为指定的交换芯片的索引,offset为交换芯片内部的寄存器偏移量,data为写入的值。
 */
void hlWriteReg(UINT8 swIndex,UINT32 offset,UINT32 data);


/*
 * 完成对指定交换芯片指定offset的寄存器进行读操作，在完成hlSrioInit后，可以通过该函
 * 数对指定的交换芯片中的指定寄存器值进行读操作。
 * swIndex为指定的交换芯片的索引,offset为交换芯片内部的寄存器偏移量。
 */
UINT32 hlReadReg(UINT8 swIndex,UINT32 offset);

/*
 * 完成对指定EndPoint指定offset的寄存器进行读操作，在完成hlSrioInit后，可以通过该函
 * 数对指定的EndPoint中的指定寄存器值进行读操作。
 * dstId为指定的EndPoint Id,offset为EndPoint内部的寄存器偏移量。
 */
UINT32 hlReadEP(UINT32 dstId,UINT32 offset);

/*
 * 完成对指定EP指定offset的寄存器进行写操作，在完成hlSrioInit后，可以通过该函
 * 数对指定的EP中的指定寄存器值进行写操作。
 * dstId为指定的EP的索引,offset为EP内部的寄存器偏移量,writedata为写入的值。
 */
void hlWriteEP(UINT32 dstId,UINT32 offset, UINT32 writedata);

/*
 * 完成交换板上交换芯片端口的统计功能显示
 */
void hlSrioPortStaticShow();

#endif
#endif

