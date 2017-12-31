#ifndef _HLSRIO_H
#define _HLSRIO_H

#include "hlType.h"

/*
 * A--578(port 14)
 * B--578(port 6)
 * C--578(port 0)
 * D--578(port 12)
 * O--578(port 4)
 */


/*
 * 578状态显示函数，显示578的5个端口的状态，该函数必须在交换板枚举完成后使用
 * 如果没有交换板，或交换板没有枚举到该板，需要先运行hlSetLoaclRouteTable函数
 * 再使用
 */
HL_STATUS hlShow578Port();

/*
 * 手动路由配置函数，自动将A片id设置为1，B片id设置为2，C片id设置成3，D片id设置为4
 * 该函数是在交换板枚举不成功的状态下看578端口状态时需要先运行的函数
 */
void hlSetLoaclRouteTable();

/*
 * 读取578的寄存器，该函数必须在交换板枚举完成后使用
 * 如果没有交换板，或交换板没有枚举到该板，需要先运行hlSetLoaclRouteTable函数
 * 再使用
 */
void hlRead578(UINT32 offset);

/*
 * 写578的寄存器，该函数必须在交换板枚举完成后使用
 * 如果没有交换板，或交换板没有枚举到该板，需要先运行hlSetLoaclRouteTable函数
 * 再使用
 */
void hlWrite578(UINT32 offset,UINT32 data);

/*
 * 读取本地A/B/C/D节点寄存器，该函数必须在交换板枚举完成后使用
 * 如果没有交换板，或交换板没有枚举到该板，需要先运行hlSetLoaclRouteTable函数
 * 再使用,dstid是要读取节点的id，offset是偏移量
 */
void hlReadLocal(UINT16 dstid,UINT32 offset);

/*
 * 写本地A/B/C/D节点寄存器，该函数必须在交换板枚举完成后使用
 * 如果没有交换板，或交换板没有枚举到该板，需要先运行hlSetLoaclRouteTable函数
 * 再使用,dstid是要读取节点的id，offset是偏移量,data是写入值
 */
void hlWriteLocal(UINT16 dstid,UINT32 offset,UINT32 data);

/*
 * 578对每个port提供了6种计数器，计数器有属性可设置，包括计数发送包个数，还是接收包个数；计数的包类型；优先级
 * 设置完计数器的属性后就可对端口进行计数.
 * portSel:0-16
 * counterSel:0-5
 * portDirect:0-接收；1-发送
 * packetType:0-7
 * 000 = Count all unicast request packets only. The response packet
	maintenance packets, and maintenance packets with hop 
	count of 0 are excluded from this counter. 
	001 = Count all unicast packet types. This counter includes all 
	request, response, maintenance packets (including the 
	maintenance packets with hop count 0). 
	010 = Count all retry control symbols only.
	011 = Count all control symbols (excluding retry control symbols). 
	100 = Count every 32-bits of unicast data. This counter counts all 
	accepted unicast packets data (including header). 
	101 = Count all multicast packets only.
	110 = Count all multicast control symbols.
	111 = Count every 32-bits of multicast data. This counter includes 
	counting the entire multicast packet (including header). 
 */
void hl578PortStatisticSet(int portSel,int counterSel,int portDirect,int packetType);


/*
 * 578对每个port提供了6种计数器，用于记录578端口的包统计个数 
 * portSel:0-16
 * counterSel:0-5  
 */
void hl578PortStatisticShow(int portSel,int counterSel);


/*
 * 枚举函数，自动完成枚举及路由配置功能
 */
HL_STATUS hlSrioInit();

/*
 * 重枚举函数，若某块处理板在枚举时，未被找到，可以使用该函数对处理板进行重枚举。该函数需要在hlSrioInit完成后再使用，否则可能会有问题。
 */
HL_STATUS hlSrioReInit();

/*
 * 重构函数，若某块处理板重启后，可以使用该函数对重启的处理板进行id及路由的重新配置。
 * 该函数需要在hlSrioInit完成后再使用，并且需要在处理板重启完成后再使用
 * 使用前,网络中的其它节点不能再向该处理板上的节点发送数据,否则有问题。
 */
HL_STATUS hlSrioReconfig();

/*
 * 显示Srio网络的拓扑结构，在完成hlSrioInit后，可以通过该函数显示所有扫描到的节点及交换芯片，并显示他们之间的连接关系。
 */
void hlSrioTopoShow();

/*
 * 显示Srio网络中所有的设备节点，在完成hlSrioInit后，可以通过该函数对Srio网络中所有的设备节点进行显示。
 */
void hlSrioDevIDShow();

/*
 * 显示Srio网络中每个交换芯片上配置的路由表，在完成hlSrioInit后，可以通过该函数显示每块交换芯片上配置的路由，可以通过它查看负载均衡的状态。
 */
void hlSrioLUTShow();

/*
 * 设置Srio网络中的组播发送模式，在完成hlSrioInit后，可以通过该函数设置组播id。
 * 该函数最多只能被调用7次,也就是说最多只能设置7个组,组播的id范围是f1-f7
 * 组中的值为设备节点id,第一个节点id为该组播id,即f1-f7中的任意一个;
 * 组中节点id用0xffff 表示结束.目前最多可以同时发送19个节点,该值可以根据需求修改
 * 需要注意的是,如果组中的节点是发送端,它自己是收不到数据的!对于广播也同样.
 * example:
 * unsigned short group0[10]={0xf1,10,11,12,0xffff};
 * hlSrioSetGroup(group0);
 * 表示节点10,11,12都在f1这个组中,当发送id为0xf1时,数据同时会发送给节点10,11,12
 */
void hlSrioSetGroup(unsigned short group[]);


/*
 * 设置Srio网络的广播模式，在完成hlSrioInit后，可以通过该函数设置广播模式
 * 网络中的任意一个节点向0xf8的id发送数据,所有的节点都能收到
 * 需要注意的是发送方自己收不到数据,和组播模式一样
 */
void hlSrioSetBroadCast();
/*--------------------------------------------------------------------------------------------------*/
	
/*
 * 自己配置路由表的数据结构 
 * dstID:目的id
 * portNo:目的id出去的端口
 * hopCount:区分是本板id,还是外板id
 */
typedef struct _ROUTE_TABLE_
{
        unsigned int dstID;
        unsigned int portNo;
        unsigned int hopCount;
}ROUTE_TABLE;

/*
 *华睿板自己配置 本板的路由表,设置本板的设备id(deviceID)接口函数
 *该函数都是将A片作为主节点,端口号为14
 *参数hostID为主节点的设备ID
 *参数entry为需要配置的路由表,格式参考如下,结束以0xFFFF为标记
 *
						dstID		portNo	hopCount
 ROUTE_TABLE table[30] = {{ 1,       	       0,            0     },									   	
                          { 2,       		6,            0     },
                          { 3,       		12,           0     },
                          { 4,       		0,             1     },
                          { 23,       		6,             1     },
                          { 0xFFFF,   0xFFFF,      0xFFFF}};
 *hopCount为0的项目是本板的路由;hopCount非0的项目是本板发送给相应的deviceID
 *需要配置出去的端口
 *
 */
void SetRouteTable(int hostID,ROUTE_TABLE entry[]);


#endif 
