#include "hlSrioConfig.h"

#ifdef SRIO_HR

HL_STATUS hlShow578Port()
{
	UINT32 data;
	
	DB(printf("--------------------------show cur 578 switch port status-------------------\n"));
	
	
	hlMaintRead(0xff,0,0x158,&data);
	DB(printf("(C chip) port = 0,  status = 0x%x\n",data));
	
	hlMaintRead(0xff,0,0x158+4*0x20,&data);
	DB(printf("(Output) port = 4,  status = 0x%x\n",data));
	
	hlMaintRead(0xff,0,0x158+6*0x20,&data);
	DB(printf("(B chip) port = 6,  status = 0x%x\n",data));
	
	hlMaintRead(0xff,0,0x158+12*0x20,&data);
	DB(printf("(D chip) port = 12, status = 0x%x\n",data));
	
	hlMaintRead(0xff,0,0x158+14*0x20,&data);
	DB(printf("(A chip) port = 14, status = 0x%x\n",data));
}

HL_STATUS hlShowLocalSrio(UINT16 dstid)
{
	UINT32 data;	
	
	hlMaintRead(dstid,0,0x158,&data);
	DB(printf("dstid =  %d, status = 0x%x\n",dstid,data));	
}

ROUTE_TABLE table1[50] = {
								{3, 0, 0},																	   	
								{2,	6, 0},
								{4, 12,0},                                       		
                                {0xFFFF,0xFFFF,0xFFFF}
						  };
void hlSetLoaclRouteTable()
{
	SetRouteTable(1,table1);	
}

void hlRead578(UINT32 offset)
{
	UINT32 data;

	hlMaintRead(0xff,0,offset,&data);
	printf("offset = 0x%x, val = 0x%x\n",offset,data);
}

void hlWrite578(UINT32 offset,UINT32 data)
{	
	hlMaintWrite(0xff,0,offset,data);	
}

void hlReadLocal(UINT16 dstid,UINT32 offset)
{
	UINT32 data;

	hlMaintRead(dstid,1,offset,&data);
	printf("offset = 0x%x, val = 0x%x\n",offset,data);
}

void hlWriteLocal(UINT16 dstid,UINT32 offset,UINT32 data)
{	
	hlMaintWrite(dstid,1,offset,data);	
}

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
void hl578PortStatisticSet(int portSel,int counterSel,int portDirect,int packetType)
{
	UINT32 offset,_val;

	if(portSel<0||portSel>16)
	{
		printf("param portSel error!\n");
		return;
	}
	
	if(counterSel<0||counterSel>5)
	{
		printf("param counterSel error!\n");
		return;
	}
	
	if(portDirect<0||portDirect>1)
	{
		printf("param portDirect error!\n");
		return;
	}
	
	if(packetType<0||packetType>7)
	{
		printf("param packetType error!\n");
		return;
	}
	
	if( counterSel==0 || counterSel == 1)
	{
		offset = 0x13020+portSel*0x100;
	}
	else if( counterSel==2 || counterSel == 3)
	{
		offset = 0x13024+portSel*0x100;
	}
	else if( counterSel==4 || counterSel == 5)
	{
		offset = 0x13028+portSel*0x100;
	}
	
	hlMaintRead(0xff,0,offset,&_val);
	if( counterSel%2==0)
	{
		_val = _val&0xfef8ffff;
		_val = _val+(portDirect<<24)+(packetType<<15);
		_val = _val|0xf0000000;
	}
	else
	{
		_val = _val&0xfffffef8;
		_val = _val+(portDirect<<8)+packetType;
		_val = _val|0xf000;				
	}
	
	hlMaintWrite(0xff,0,offset,_val);	
}

/*
 * 578对每个port提供了6种计数器，用于记录578端口的包统计个数 
 * portSel:0-16
 * counterSel:0-5  
 */
void hl578PortStatisticShow(int portSel,int counterSel)
{
	UINT32 offset,_val;

	if(portSel<0||portSel>16)
	{
		printf("param portSel error!\n");
		return;
	}
	
	if(counterSel<0||counterSel>5)
	{
		printf("param counterSel error!\n");
		return;
	}
	
	offset = 0x13040+counterSel*4+portSel*0x100;
	hlRead578(offset);
}

#endif



