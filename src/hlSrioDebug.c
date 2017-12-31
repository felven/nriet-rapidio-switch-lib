#include "hlSrioConfig.h"

#ifdef HLSRIO_DEBUG_INTERFACE


#ifdef SRIO_ZYNQ
void hlWriteEP(UINT32 dstId,UINT32 offset, UINT32 writedata)
{
	UINT32 reg_addr;
	sysAWR32(SRIO_ZYNQ_BASEADDR,SRIO_ZYNQ_NODE_BASEADDR,dstId);	
	reg_addr = (((SRIO_ZYNQ_MAX_HOPCOUNT+1)<<24)|offset);	

	sysAWR32(SRIO_ZYNQ_BASEADDR,reg_addr,writedata);	
}

UINT32 hlReadEP(UINT32 dstId,UINT32 offset)
{	
	UINT32 data;
	UINT32 reg_addr;
	sysAWR32(SRIO_ZYNQ_BASEADDR,SRIO_ZYNQ_NODE_BASEADDR,dstId);	
	reg_addr = (((SRIO_ZYNQ_MAX_HOPCOUNT+1)<<24)|offset);
	data = sysARD32(SRIO_ZYNQ_BASEADDR,reg_addr);			

	printf("M_SRIO_EP_READ: dstId = %d, offset = 0x%x, value = 0x%x\n",dstId,offset,data);
	return data;
}
#else
void hlWriteEP(UINT32 dstId,UINT32 offset, UINT32 writedata)
{
	sysAWR32(FPGA_BASE_ADDR,SRIO_MAINT_DEST_ID,dstId);	
	sysAWR32(FPGA_BASE_ADDR,SRIO_MAINT_HOPCOUNT,0xff);	
	sysAWR32(FPGA_BASE_ADDR,FPGA_SRIO_NODE_BASEADDR+offset,writedata);				
}

void hlReadEP(UINT32 dstId,UINT32 offset)
{	
	UINT32 data;
	sysAWR32(FPGA_BASE_ADDR,SRIO_MAINT_DEST_ID,dstId);	
	sysAWR32(FPGA_BASE_ADDR,SRIO_MAINT_HOPCOUNT,0xff);	
	data = sysARD32(FPGA_BASE_ADDR,FPGA_SRIO_NODE_BASEADDR+offset);

	printf("M_SRIO_REG_READ: offset = 0x%x, value = 0x%x\n",offset,data);
}
#endif

void hlWriteReg(UINT8 swIndex,UINT32 offset,UINT32 data)
{
	int tgtId=0xff;
	UINT16 hopcount=0;
	hostAllSwTbl *pHostAllSwTbl=NULL;
	
	if( pDrvCtrl == NULL )
		return;
					
	pHostAllSwTbl=pDrvCtrl->pAllSwTbl;
	if(pHostAllSwTbl==NULL)
	{
		printf("SRIO NET TOPO not initialized!\n");
		return ;
	}

	if( swIndex >= pHostAllSwTbl->switchNum )
	{
		printf("SWITCH %d NOT FOUND!\n",swIndex);
		return;
	}

	tgtId = swIndex+RIO_SW_FIRST_VIRTUAL_TGTID;
	hopcount=(UINT16)pHostAllSwTbl->switchInterConnectionInfo[0][swIndex].distance; 

	hlMaintWrite(tgtId,hopcount,offset,data);
	
}


UINT32 hlReadReg(UINT8 swIndex,UINT32 offset)
{
	int tgtId=0xff;
	UINT16 hopcount=0;
	hostAllSwTbl *pHostAllSwTbl=NULL;
	UINT32 mtRdata = 0;
	
	if( pDrvCtrl == NULL )
		return 0;
					
	pHostAllSwTbl=pDrvCtrl->pAllSwTbl;
	if(pHostAllSwTbl==NULL)
	{
		printf("SRIO NET TOPO not initialized!\n");
		return 0;
	}

	if( swIndex >= pHostAllSwTbl->switchNum )
	{
		printf("SWITCH %d NOT FOUND!\n",swIndex);
		return 0;
	}

	tgtId = swIndex+RIO_SW_FIRST_VIRTUAL_TGTID;
	hopcount= (UINT16)pHostAllSwTbl->switchInterConnectionInfo[0][swIndex].distance; 	
	printf("dstid = 0x%x,hopcount = %d\n",tgtId,hopcount);
	
	hlMaintRead(tgtId,hopcount,offset,&mtRdata);
	printf("hlReadReg::offset = 0x%x, value = 0x%x\n",offset,mtRdata);
	return mtRdata;
}

/*默认1848端口统计功能不使能,需要通过该函数将统计功能打开*/
void hlSetPortCounter(char bEnable)
{
	int tgtId=0xff;
	UINT16 hopcount=0;
	hostAllSwTbl *pHostAllSwTbl=NULL;
	UINT32 mtRdata = 0;
	UINT32 offset = 0;
	UINT8 portCnt = 18;
	int i,j;
	
	if( pDrvCtrl == NULL )
		return;
					
	pHostAllSwTbl=pDrvCtrl->pAllSwTbl;
	if(pHostAllSwTbl==NULL)
	{
		printf("SRIO NET TOPO not initialized!\n");
		return ;
	}	
	
	for( i=0; i<4; i++ )
	{
		tgtId = i+RIO_SW_FIRST_VIRTUAL_TGTID;
		hopcount=(UINT16)pHostAllSwTbl->switchInterConnectionInfo[0][i].distance; 		

		offset = 0;
		hlMaintRead(tgtId,hopcount,offset,&mtRdata);
		if( mtRdata == DEVICE_ID_1848 )
		{			
			for( j=0; j<portCnt; j++)
			{
				offset = 0xf40004+j*0x100;
				if( bEnable == 1 )
					hlMaintWrite(tgtId,hopcount,offset,0x6400000);
				else if( bEnable == 0 )
					hlMaintWrite(tgtId,hopcount,offset,0x2400000);
#if 0
				/*trace 0-----value=type5*/
				offset = 0xe40000+j*0x100;
				hlMaintWrite(tgtId,hopcount,offset,0x50000);
				offset = 0xe40014+j*0x100;
				hlMaintWrite(tgtId,hopcount,offset,0xf0000);
				/*trace 1-----value=type8*/
				offset = 0xe40028+j*0x100;
				hlMaintWrite(tgtId,hopcount,offset,0x80000);
				offset = 0xe4003c+j*0x100;
				hlMaintWrite(tgtId,hopcount,offset,0xf0000);
				/*trace 2-----value=type10*/
				offset = 0xe40050+j*0x100;
				hlMaintWrite(tgtId,hopcount,offset,0xa0000);
				offset = 0xe40064+j*0x100;
				hlMaintWrite(tgtId,hopcount,offset,0xf0000);
				/*trace 3-----value=type13*/
				offset = 0xe40078+j*0x100;
				hlMaintWrite(tgtId,hopcount,offset,0xd0000);
				offset = 0xe4008c+j*0x100;
				hlMaintWrite(tgtId,hopcount,offset,0xf0000);
#endif
			}
		}
	}

}

void hlReadSwitchAllReg(UINT8 swIndex)
{
	int tgtId=0xff;
	UINT16 hopcount=0;
	hostAllSwTbl *pHostAllSwTbl=NULL;
	UINT32 mtRdata = 0;
	UINT32 offset = 0;
	UINT8 portCnt = 0;
	int i;
	
	if( pDrvCtrl == NULL )
		return;
					
	pHostAllSwTbl=pDrvCtrl->pAllSwTbl;
	if(pHostAllSwTbl==NULL)
	{
		printf("SRIO NET TOPO not initialized!\n");
		return ;
	}

	if( swIndex >= pHostAllSwTbl->switchNum )
	{
		printf("SWITCH %d NOT FOUND!\n",swIndex);
		return;
	}
	printf("***************************SWITCH-%d REGISTER  SHOW******************************\n",swIndex);

	tgtId = swIndex+RIO_SW_FIRST_VIRTUAL_TGTID;
	hopcount=(UINT16)pHostAllSwTbl->switchInterConnectionInfo[0][swIndex].distance; 

	offset = 0;
	hlMaintRead(tgtId,hopcount,offset,&mtRdata);
	printf("offset = 0x%x, value = 0x%x -- RIO_DEV_ID\n",offset,mtRdata);
	offset = 0x10;
	hlMaintRead(tgtId,hopcount,offset,&mtRdata);
	printf("offset = 0x%x, value = 0x%x -- RIO_PE_FEAT\n",offset,mtRdata);

	offset = 0x14;
	hlMaintRead(tgtId,hopcount,offset,&mtRdata);
	portCnt	= (mtRdata&SRIO_SW_PORT_NUM_MASK)>>8;
	printf("offset = 0x%x, value = 0x%x -- RIO_SW_PORT\n",offset,mtRdata);

	offset = 0x18;
	hlMaintRead(tgtId,hopcount,offset,&mtRdata);
	printf("offset = 0x%x, value = 0x%x -- RIO_SRC_OP\n",offset,mtRdata);
	
	offset = 0x38;
	hlMaintRead(tgtId,hopcount,offset,&mtRdata);
	printf("offset = 0x%x, value = 0x%x -- RIO_SW_MC_INFO\n",offset,mtRdata);

	offset = 0x68;
	hlMaintRead(tgtId,hopcount,offset,&mtRdata);
	printf("offset = 0x%x, value = 0x%x -- RIO_HOST_BASE_ID_LOCK\n",offset,mtRdata);

	offset = 0x6c;
	hlMaintRead(tgtId,hopcount,offset,&mtRdata);
	printf("offset = 0x%x, value = 0x%x -- SRIO_COMP_TAG\n",offset,mtRdata);

	offset = 0x78;
	hlMaintRead(tgtId,hopcount,offset,&mtRdata);
	printf("offset = 0x%x, value = 0x%x -- RIO_LUT_ATTR\n",offset,mtRdata);

	offset = 0x120;
	hlMaintRead(tgtId,hopcount,offset,&mtRdata);
	printf("offset = 0x%x, value = 0x%x -- RIO_SW_LT_CTL\n",offset,mtRdata);
	
	offset = 0x13c;
	hlMaintRead(tgtId,hopcount,offset,&mtRdata);
	printf("offset = 0x%x, value = 0x%x -- RIO_SW_GEN_CTL\n",offset,mtRdata);

	printf("\n------------------------------------\n");
	for( i=0; i<portCnt; i++)
	{
		hlMaintRead(0xff, hopcount, SRIO_PORTX_STATUS_OFFSET(i), &mtRdata);
		if((mtRdata&PORT_OK)==PORT_OK)
		{
			offset = 0x144+i*0x20;
			hlMaintRead(tgtId,hopcount,offset,&mtRdata);
			printf("offset = 0x%x, value = 0x%x -- PORT[%d]_LM_RESP\n",offset,mtRdata,i);		
			
			offset = 0x148+i*0x20;
			hlMaintRead(tgtId,hopcount,offset,&mtRdata);
			printf("offset = 0x%x, value = 0x%x -- PORT[%d]_ACKID_STAT\n",offset,mtRdata,i);		

			offset = 0x158+i*0x20;
			hlMaintRead(tgtId,hopcount,offset,&mtRdata);
			printf("offset = 0x%x, value = 0x%x -- PORT[%d]_ERR_STATUS\n",offset,mtRdata,i);	

			offset = 0x15c+i*0x20;
			hlMaintRead(tgtId,hopcount,offset,&mtRdata);
			printf("offset = 0x%x, value = 0x%x -- PORT[%d]_CTL\n",offset,mtRdata,i);				
		}		
	}
	printf("\n------------------------------------\n");

	offset = 0;
	hlMaintRead(tgtId,hopcount,offset,&mtRdata);
	if( mtRdata == DEVICE_ID_1848 )
	{
		for( i=0; i<portCnt; i++)
		{
			hlMaintRead(0xff, hopcount, SRIO_PORTX_STATUS_OFFSET(i), &mtRdata);
			if((mtRdata&PORT_OK)==PORT_OK)
			{
				offset = 0xf40004+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&mtRdata);
				printf("offset = 0x%x, value = 0x%x -- PORT[%d]_OPS\n",offset,mtRdata,i);
										
				offset = 0xf40040+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&mtRdata);
				printf("offset = 0x%x, value = 0x%x -- PORT[%d]_ACK_RX_COUNTER\n",offset,mtRdata,i);
				offset = 0xf4001c+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&mtRdata);
				printf("offset = 0x%x, value = 0x%x -- PORT[%d]_PACKET_TX_COUNTER\n",offset,mtRdata,i);
				
				offset = 0xf40014+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&mtRdata);
				printf("offset = 0x%x, value = 0x%x -- PORT[%d]_NACK_TX_COUNTER\n",offset,mtRdata,i);
				offset = 0xf40044+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&mtRdata);
				printf("offset = 0x%x, value = 0x%x -- PORT[%d]_NACK_RX_COUNTER\n",offset,mtRdata,i);
				
				offset = 0xf40018+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&mtRdata);
				printf("offset = 0x%x, value = 0x%x -- PORT[%d]_RTRY_TX_COUNTER\n",offset,mtRdata,i);
				offset = 0xf40048+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&mtRdata);
				printf("offset = 0x%x, value = 0x%x -- PORT[%d]_RTRY_RX_COUNTER\n",offset,mtRdata,i);
				
				offset = 0xf40068+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&mtRdata);
				printf("offset = 0x%x, value = 0x%x -- PORT[%d]_PACKET_DROP_TX_COUNTER\n",offset,mtRdata,i);
				offset = 0xf40064+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&mtRdata);
				printf("offset = 0x%x, value = 0x%x -- PORT[%d]_PACKET_DROP_RX_COUNTER\n",offset,mtRdata,i);
				
				offset = 0xf40050+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&mtRdata);
				printf("offset = 0x%x, value = 0x%x -- PORT[%d]_PACKET_RX_COUNTER\n",offset,mtRdata,i);	
				offset = 0xf40010+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&mtRdata);
				printf("offset = 0x%x, value = 0x%x -- PORT[%d]_ACK_TX_COUNTER\n",offset,mtRdata,i);
			}
		}		
		printf("\n------------------------------------\n");
/*
		for( i=0; i<47; i++)
		{
			offset = 0x2010+i*0x20;
			hlMaintRead(tgtId,hopcount,offset,&mtRdata);
			printf("offset = 0x%x, value = 0x%x -- LANE[%d]_STATUS\n",offset,mtRdata,i);				
		}
		printf("\n------------------------------------\n");
*/
	}

}

#if 0
void hlSrioPortStaticShow()
{
	int tgtId=0xff;
	UINT16 hopcount=0;
	hostAllSwTbl *pHostAllSwTbl=NULL;
	UINT32 pdata[6],mtRdata = 0;
	UINT32 offset = 0;
	int i,swIndex;

	if( pDrvCtrl == NULL )
		return;

	pHostAllSwTbl=pDrvCtrl->pAllSwTbl;
	if(pHostAllSwTbl==NULL)
	{
		printf("SRIO NET TOPO not initialized!\n");
		return ;
	}

	for( swIndex=0; swIndex<3; swIndex++ )
	{
		if( swIndex >= pHostAllSwTbl->switchNum )
			break;
	
		printf("***************************SWITCH-%d REGISTER  SHOW******************************\n",swIndex);
		printf("port   pstatus         ACK_RX   NACK_RX   RTRY_RX   PACKET_DROP_RX     PACKET_RX\n");
		tgtId = swIndex+RIO_SW_FIRST_VIRTUAL_TGTID;
		hopcount=(UINT16)pHostAllSwTbl->switchInterConnectionInfo[0][swIndex].distance;

		offset = 0;
		hlMaintRead(tgtId,hopcount,offset,&mtRdata);
		if( mtRdata == DEVICE_ID_1848 )
		{			
			for( i=0; i<18; i++)
			{				
				offset = 0x158+i*0x20;
				hlMaintRead(tgtId,hopcount,offset,&pdata[0]);
			
				offset = 0xf40040+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[1]);
				
				offset = 0xf40044+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[2]);

				offset = 0xf40048+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[3]);

				offset = 0xf40064+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[4]);

				offset = 0xf40050+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[5]);
				printf("0x%x      0x%x      0x%x       0x%x       0x%x         0x%x               0x%x     \n",i,pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5]);
			}
		}
		printf("\n");
	}
}
#endif

void hlSrioPortStaticShow()
{
	int tgtId=0xff;
	UINT16 hopcount=0;
	hostAllSwTbl *pHostAllSwTbl=NULL;
	UINT32 pdata[8],mtRdata = 0;
	UINT32 offset = 0;
	int i,swIndex;

	if( pDrvCtrl == NULL )
		return;

	pHostAllSwTbl=pDrvCtrl->pAllSwTbl;
	if(pHostAllSwTbl==NULL)
	{
		printf("SRIO NET TOPO not initialized!\n");
		return ;
	}

	for( swIndex=0; swIndex<3; swIndex++ )
	{
		if( swIndex >= pHostAllSwTbl->switchNum )
			break;
	
		printf("**************************************************SWITCH-%d TX  REGISTER  SHOW**************************************************\n",swIndex);
		printf("port      pstatus         pa               pna            retrysymbol        alltx            droptx\n");
		tgtId = swIndex+RIO_SW_FIRST_VIRTUAL_TGTID;
		hopcount=(UINT16)pHostAllSwTbl->switchInterConnectionInfo[0][swIndex].distance;

		offset = 0;
		hlMaintRead(tgtId,hopcount,offset,&mtRdata);
		if( mtRdata == DEVICE_ID_1848 )
		{			
			for( i=0; i<18; i++)
			{				
				offset = 0x158+i*0x20;
				hlMaintRead(tgtId,hopcount,offset,&pdata[0]);
			
				offset = 0xf40010+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[1]);
				
				offset = 0xf40014+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[2]);

				offset = 0xf40018+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[3]);

				offset = 0xf4001c+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[4]);

				offset = 0xf40068+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[5]);
				printf("0x%.2x     0x%.8x      0x%.8x      0x%.8x       0x%.8x         0x%.8x       0x%.8x\n",i,pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5]);

			}
		}
		printf("\n");
	}

	printf("-----------------------------------------------------------------------------------------------------------------\n");
	printf("\n");
	
	for( swIndex=0; swIndex<3; swIndex++ )
	{
		if( swIndex >= pHostAllSwTbl->switchNum )
			break;
	
		printf("**************************************************SWITCH-%d RX  REGISTER  SHOW**************************************************\n",swIndex);
		printf("port      pstatus         pa               pna            retrysymbol        allrx            droprx\n");
		tgtId = swIndex+RIO_SW_FIRST_VIRTUAL_TGTID;
		hopcount=(UINT16)pHostAllSwTbl->switchInterConnectionInfo[0][swIndex].distance;

		offset = 0;
		hlMaintRead(tgtId,hopcount,offset,&mtRdata);
		if( mtRdata == DEVICE_ID_1848 )
		{			
			for( i=0; i<18; i++)
			{				
				offset = 0x158+i*0x20;
				hlMaintRead(tgtId,hopcount,offset,&pdata[0]);
			
				offset = 0xf40040+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[1]);
				
				offset = 0xf40044+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[2]);

				offset = 0xf40048+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[3]);

				offset = 0xf40050+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[4]);

				offset = 0xf40064+i*0x100;
				hlMaintRead(tgtId,hopcount,offset,&pdata[5]);
				printf("0x%.2x     0x%.8x      0x%.8x      0x%.8x       0x%.8x         0x%.8x       0x%.8x\n",i,pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5]);
			}
		}
		printf("\n");
	}
	
}

#endif
