#include "hlSrioConfig.h"

#ifdef SRIO_ZYNQ

#include "xil_io.h"

void M_Enable_Local_Master(UINT32 hostID)
{	
	int mid;
	mid = hostID<<16;
	sysAWR32(SRIO_ZYNQ_BASEADDR,SRIO_NODE_MASTERENABLE,SRIO_MASTERENABLE);
	/*zynq的二代srio节点是小端模式,而华睿的srio节点是大端模式60/68---64/6c*/
	sysAWR32(SRIO_ZYNQ_BASEADDR,SRIO_NODE_DEST_ID-0x4,mid);			
	sysAWR32(SRIO_ZYNQ_BASEADDR,SRIO_NODE_ID_LOCK-0x4,hostID);
	/*set the maintance packet highest priority
	mid = sysARD32(SRIO_ZYNQ_BASEADDR,SRIO_MAINT_PRIORITY);	
	sysAWR32(SRIO_ZYNQ_BASEADDR,SRIO_MAINT_PRIORITY,mid|3);*/
}	

void M_SetHostID(UINT32 hostID)
{	
	int mid;
	mid = hostID<<16;
	sysAWR32(SRIO_ZYNQ_BASEADDR,SRIO_NODE_DEST_ID-0x4,mid);			
	sysAWR32(SRIO_ZYNQ_BASEADDR,SRIO_NODE_ID_LOCK-0x4,hostID);	
}

void sysAWR32(unsigned int addrBase,unsigned int addrOffset,unsigned int value)
{
	UINT32 reg_addr;
	reg_addr=addrBase+addrOffset;
	Xil_Out32(reg_addr,value);
}

unsigned int sysARD32(unsigned int addrBase,unsigned int addrOffset)
{
	UINT32 data;
	data=addrBase+addrOffset;
	return(Xil_In32(data));
}

void SRIO_LOCAL_REG_WRITE(UINT32 offset, UINT32 writedata)
{
	UINT32 reg_addr;
	UINT8 hopcount = 1;
	sysAWR32(SRIO_ZYNQ_BASEADDR,SRIO_ZYNQ_NODE_BASEADDR,0xff);
	reg_addr = ((hopcount<<24)|offset);
	sysAWR32(SRIO_ZYNQ_BASEADDR,reg_addr,writedata);
}

unsigned int SRIO_LOCAL_REG_READ(UINT32 offset)
{
	UINT32 reg_addr,data;
	UINT8 hopcount = 1;

	sysAWR32(SRIO_ZYNQ_BASEADDR,SRIO_ZYNQ_NODE_BASEADDR,0xff);
	reg_addr = ((hopcount<<24)|offset);
	data = sysARD32(SRIO_ZYNQ_BASEADDR,reg_addr);
	return data;
}

HL_STATUS hlMaintWrite(UINT32 dstId,UINT16 hopcount, UINT32 offset, UINT32 writedata)
{
	UINT32 reg_addr;
	if( hopcount > SRIO_ZYNQ_MAX_HOPCOUNT )
	{
		printf("!!!error, hopcount = %d,  > %d\n",hopcount,SRIO_ZYNQ_MAX_HOPCOUNT);
		return HL_ERROR;
	}
	
	sysAWR32(SRIO_ZYNQ_BASEADDR,SRIO_ZYNQ_NODE_BASEADDR,dstId);	
	reg_addr = (((hopcount+1)<<24)|offset);
	sysAWR32(SRIO_ZYNQ_BASEADDR,reg_addr,writedata);		
	
	return HL_OK;  
}

HL_STATUS hlMaintRead(UINT32 dstId,UINT16 hopcount, UINT32 offset, void *mrdataAdr)
{	
	UINT32 reg_addr;
	if( hopcount > SRIO_ZYNQ_MAX_HOPCOUNT )
	{
		printf("!!!error, hopcount = %d,  > %d\n",hopcount,SRIO_ZYNQ_MAX_HOPCOUNT);
		return HL_ERROR;
	}
	
	sysAWR32(SRIO_ZYNQ_BASEADDR,SRIO_ZYNQ_NODE_BASEADDR,dstId);	
	reg_addr = (((hopcount+1)<<24)|offset);
	*(UINT32 *)mrdataAdr = sysARD32(SRIO_ZYNQ_BASEADDR,reg_addr);		

	/*printf("M_SRIO_MAINT_REG_READ: hopcount = %d, offset = 0x%x, value = 0x%x\n",hopcount,offset,*(UINT32 *)mrdataAdr);*/
	return HL_OK;  
}

#endif

#ifdef SRIO_HR

void M_Enable_Local_Master(UINT32 hostID)
{	
	int mid;
	mid = hostID<<16;
	sysAWR32(FPGA_BASE_ADDR,FPGA_SRIO_NODE_BASEADDR+SRIO_NODE_MASTERENABLE,SRIO_MASTERENABLE);
	sysAWR32(FPGA_BASE_ADDR,FPGA_SRIO_NODE_BASEADDR+SRIO_NODE_DEST_ID,mid);
	sysAWR32(FPGA_BASE_ADDR,FPGA_SRIO_NODE_BASEADDR+SRIO_NODE_ID_LOCK,hostID);	
	/*set the maintance packet highest priority*/
	mid = sysARD32(FPGA_BASE_ADDR,SRIO_MAINT_PRIORITY);	
	sysAWR32(FPGA_BASE_ADDR,SRIO_MAINT_PRIORITY,mid|3);
}	

void M_SetHostID(UINT32 hostID)
{	
	int mid;
	mid = hostID<<16;
	sysAWR32(FPGA_BASE_ADDR,FPGA_SRIO_NODE_BASEADDR+SRIO_NODE_DEST_ID,mid);
	sysAWR32(FPGA_BASE_ADDR,FPGA_SRIO_NODE_BASEADDR+SRIO_NODE_ID_LOCK,hostID);	
}

void SRIO_LOCAL_REG_WRITE(UINT32 offset, UINT32 writedata)
{	
	sysAWR32(FPGA_BASE_ADDR,SRIO_MAINT_DEST_ID,0xff);	
	sysAWR32(FPGA_BASE_ADDR,SRIO_MAINT_HOPCOUNT,0);	
	sysAWR32(FPGA_BASE_ADDR,FPGA_SWITCH_24OFFSET_ADDR,(offset&0xf00000)>>20);
	sysAWR32(FPGA_BASE_ADDR,FPGA_SWITCH_BASEADDR+(offset&0xfffff),writedata);	
	sysAWR32(FPGA_BASE_ADDR,FPGA_SWITCH_24OFFSET_ADDR,0);
}

unsigned int SRIO_LOCAL_REG_READ(UINT32 offset)
{
	unsigned int _val;	
	
	sysAWR32(FPGA_BASE_ADDR,SRIO_MAINT_DEST_ID,0xff);	
	sysAWR32(FPGA_BASE_ADDR,SRIO_MAINT_HOPCOUNT,0);	
	sysAWR32(FPGA_BASE_ADDR,FPGA_SWITCH_24OFFSET_ADDR,(offset&0xf00000)>>20);
	_val = sysARD32(FPGA_BASE_ADDR,FPGA_SWITCH_BASEADDR+(offset&0xfffff));		
/*	printf("M_SRIO_LOCAL_REG_READ: offset = 0x%x, value = %d\n",offset,_val);	*/
	sysAWR32(FPGA_BASE_ADDR,FPGA_SWITCH_24OFFSET_ADDR,0);
	return _val;
}

HL_STATUS hlMaintWrite(UINT32 dstId,UINT16 hopcount, UINT32 offset, UINT32 writedata)
{
	sysAWR32(FPGA_BASE_ADDR,SRIO_MAINT_DEST_ID,dstId);	
	sysAWR32(FPGA_BASE_ADDR,SRIO_MAINT_HOPCOUNT,hopcount);	
	sysAWR32(FPGA_BASE_ADDR,FPGA_SWITCH_24OFFSET_ADDR,(offset&0xf00000)>>20);
	sysAWR32(FPGA_BASE_ADDR,FPGA_SWITCH_BASEADDR+(offset&0xfffff),writedata);		
	sysAWR32(FPGA_BASE_ADDR,FPGA_SWITCH_24OFFSET_ADDR,0);
	return HL_OK;  
}

HL_STATUS hlMaintRead(UINT32 dstId,UINT16 hopcount, UINT32 offset, void *mrdataAdr)
{	
	sysAWR32(FPGA_BASE_ADDR,SRIO_MAINT_DEST_ID,dstId);	
	sysAWR32(FPGA_BASE_ADDR,SRIO_MAINT_HOPCOUNT,hopcount);
	sysAWR32(FPGA_BASE_ADDR,FPGA_SWITCH_24OFFSET_ADDR,(offset&0xf00000)>>20);
	
	*(UINT32 *)mrdataAdr = sysARD32(FPGA_BASE_ADDR,FPGA_SWITCH_BASEADDR+(offset&0xfffff));		
/*
	_val = *(UINT32 *)mrdataAdr;
	printf("M_SRIO_MAINT_REG_READ: offset = 0x%x, value = %d\n",offset,_val);*/	
	sysAWR32(FPGA_BASE_ADDR,FPGA_SWITCH_24OFFSET_ADDR,0);
	return HL_OK;  
}

#endif

/*************************************************************
*          give the srio device a ID and updata the LUT
*          in the switch
*************************************************************/
HL_STATUS hlSwitchSetLUT(
	RIO_DRV_CTRL*pDrvCtrl,
	UINT32 hostTargetID,
	UINT32 dstId,
	UINT16 hopcount,
	UINT32 portNum,
	UINT32 destID)
{
	UINT32 writedata;
	UINT32 tempport = 0xff;
	UINT32 mtRdata=0;

	do{
		writedata = (destID&(0x000000FF));
		hlMaintWrite(dstId,hopcount,SRIO_ROUTE_CFG_DESTID,writedata);
                
		writedata = portNum;
		hlMaintWrite(dstId,hopcount,SRIO_ROUTE_CFG_PORT,writedata);

		writedata = (destID&(0x000000FF));
		hlMaintWrite(dstId,hopcount,SRIO_ROUTE_CFG_DESTID,writedata);

		if (HL_OK == hlMaintRead(dstId, hopcount, SRIO_ROUTE_CFG_PORT, &mtRdata))        
			tempport = mtRdata;            
		else        
			hlMtRdFailReport (dstId, hopcount, SRIO_ROUTE_CFG_PORT, NULL);

	}while (tempport != portNum);

	return HL_OK;
}

/*****************************************************************************
* SwtichControl - 	control the switch
* 					Guranteeing the validity of paramters(rioCmd, pReturnValue)
*					is users' responsibility. 
******************************************************************************/
HL_STATUS hlSwtichControl(
	RIO_DRV_CTRL*pDrvCtrl,
	UINT32 hostTargetID,
	UINT32 dstId,
	UINT16 hopcount,
	RIO_CMD rioCmd,
	void* pReturnValue)
{
	UINT32 dataVal;
	UINT32 mtRdata=0;
	switch(rioCmd)
	{
		case READ_INITIATOR_PORT:
		{
			if (HL_OK == hlMaintRead(dstId,hopcount,SRIO_SW_PORT_INFO, &mtRdata))
				dataVal = mtRdata;               
			else            
				hlMtRdFailReport (dstId, hopcount, SRIO_SW_PORT_INFO, NULL);            
             
			*((UINT32*)pReturnValue) = dataVal&(0x000000ff);
		}
			break;
		case SET_DEFAULT_PORT:
			dataVal = *((UINT32*)pReturnValue)&0x000000ff;
			hlMaintWrite(dstId,hopcount,SRIO_ROUTE_DEFAULT_PORT,dataVal);
			break;
		case SET_SWITCH_ID:
			hlMaintWrite(dstId,hopcount,SRIO_COMP_TAG,*(UINT32 *)pReturnValue);
			break;
		case GET_SWITCH_ID:
		{
			if (HL_OK == hlMaintRead(dstId,hopcount,SRIO_COMP_TAG, &mtRdata))            
				dataVal = mtRdata;            
			else            
				hlMtRdFailReport (dstId, hopcount, SRIO_COMP_TAG, NULL);            
             
			*((UINT32*)pReturnValue) = dataVal&(0x000000ff);
		}
			break;
		case GET_PORT_TO_SW:
		{
			if (HL_OK == hlMaintRead(dstId,hopcount,SRIO_SW_PORT_INFO, &mtRdata))			
				dataVal	= mtRdata;				
			else
			{
				hlMtRdFailReport (dstId, hopcount, SRIO_SW_PORT_INFO, NULL);
				dataVal = 0xff;
			}
			*((UINT32*)pReturnValue) = dataVal&(0x000000ff);
		}
			break;
		default:
			return HL_ERROR;
	}

	return HL_OK;
}


HL_STATUS hlSrioRemoteConfigLockCheck(UINT32 hostTargetID,UINT16 hopcount)
{
	volatile UINT32 tempVal=0;
	UINT32 mtRdata=0;
	/*DB(printf("hlSrioRemoteConfigLock: try to read device !\n"));*/
	if (HL_OK == hlMaintRead(0xff,hopcount, SRIO_HBDIDLCSR_OFFSET, &mtRdata))	
		tempVal = mtRdata&(0x0000FFFF);
	else	
		return hlMtRdFailReport (0xff, hopcount, SRIO_HBDIDLCSR_OFFSET, NULL);				 
 
	if(tempVal == hostTargetID)	
		return HL_ALREADY_LOCK;	     		 
	else if (tempVal == 0xFFFF)	
		return HL_NOT_DISCOVERED; 

	/*zz add for reEnum*/
	return HL_LOCK;
 }

 HL_STATUS hlSrioConfigSetDiscover(
	 UINT32 hostTargetID,
	 UINT32 dstId,
	 UINT16 hopcount)
 {
	 UINT32 tempVal=0;
	 UINT32 mtRdata=0;
	 if (HL_OK == hlMaintRead(dstId, hopcount, SRIO_PGCCSR_OFFSET, &mtRdata))    
		 tempVal = mtRdata; 	   
	 else	 
		 hlMtRdFailReport (dstId, hopcount, SRIO_PGCCSR_OFFSET, NULL);
			
	 tempVal|=PGCCSR_DISCOVERED;	  
	 hlMaintWrite( dstId, hopcount, SRIO_PGCCSR_OFFSET, tempVal);
	 return HL_OK;
 } 

 HL_STATUS hlSrioHostConfigLock(UINT32 hostTargetID)
 {
	  UINT32 tempVal=0; 
	  /*check if the target device has already been locked by other host or by self */
	  tempVal = SRIO_LOCAL_REG_READ(SRIO_HBDIDLCSR_OFFSET)&(0x0000FFFF);
	  if(tempVal==hostTargetID) 	
		   return HL_ALREADY_LOCK;
		 
	  SRIO_LOCAL_REG_WRITE(SRIO_HBDIDLCSR_OFFSET,hostTargetID); 		
	  tempVal = SRIO_LOCAL_REG_READ(SRIO_HBDIDLCSR_OFFSET)&(0x0000FFFF);		 
	  if(tempVal>hostTargetID)	   
		  return HL_CONCEDE;		 
	  else if(tempVal<hostTargetID) 	
		  return HL_WAIT_OTHER_CONCEDE;
		 
	  return HL_LOCK;
 }

 HL_STATUS hlSrioRemoteConfigLock(UINT32 hostTargetID,UINT16 hopcount)
 {
	 UINT32 tempVal=0;
	 UINT32 mtRdata=0;
	 if (HL_OK == hlMaintRead(0xff,hopcount, SRIO_HBDIDLCSR_OFFSET, &mtRdata))	
		 tempVal = mtRdata&(0x0000FFFF);		
	 else
		 hlMtRdFailReport (0xff, hopcount, SRIO_HBDIDLCSR_OFFSET, NULL);		
 
	 if(tempVal == hostTargetID)
		 return HL_ALREADY_LOCK;
 
	 hlMaintWrite(0xff, hopcount, SRIO_HBDIDLCSR_OFFSET, hostTargetID);
 
	 if (HL_OK == hlMaintRead(0xff,hopcount, SRIO_HBDIDLCSR_OFFSET, &mtRdata))
		 tempVal = mtRdata&(0x0000FFFF);		
	 else
		 hlMtRdFailReport (0xff, hopcount, SRIO_HBDIDLCSR_OFFSET, NULL);
		 
	 if(tempVal>hostTargetID)
		 return HL_CONCEDE;    
	 else if(tempVal<hostTargetID)
		 return HL_WAIT_OTHER_CONCEDE;
 
	 return HL_LOCK;
 }



