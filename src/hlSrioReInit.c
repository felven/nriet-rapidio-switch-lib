#include "hlSrioConfig.h"

UINT32 globalSysHostID_1 = 1;
UINT32 globalSysHostID_2 = 2;

/*释放上次枚举时的内存资源*/
void hlSrioFreeMemory()
{
	int i,j;
	DB(printf("Free the alloced memory of pDrvCtrl.\n"));
	if( pDrvCtrl != NULL )
	{
		for(i=0;i<MAX_NUMBER_RIO_SW;i++)
		{
			free(pDrvCtrl->pAllSwTbl->hostAllSwInfo[i]);
			pDrvCtrl->pAllSwTbl->hostAllSwInfo[i] = NULL;
		}	

		for(i=0; i<MAX_NUMBER_RIO_SW;i++)			
		{
			for(j=0; j<MAX_NUMBER_RIO_SW;j++)
			{
				free(pDrvCtrl->pAllSwTbl->switchInterConnectionInfo[i][j].portPtr);
				free(pDrvCtrl->pAllSwTbl->switchInterConnectionInfo[i][j].portBrotherPtr);
				free(pDrvCtrl->pAllSwTbl->switchInterConnectionInfo[i][j].shortWayPtr);
			}
		}
		
		free(pDrvCtrl->pAllSwTbl);
		pDrvCtrl->pAllSwTbl = NULL;
		
		free( pDrvCtrl );
		pDrvCtrl = NULL;
	}			
	
}

HL_STATUS hlSrioReInit()
{		
	UINT32 returnValue = 0;
	
	if( globalSysHostID == globalSysHostID_1 )	
		globalSysHostID = globalSysHostID_2;
	else
		globalSysHostID = globalSysHostID_1;

	if(globalSrioNetStatus == 1)
		hlSrioFreeMemory();

	DB(printf("The SrioNet ReEnum again, now begin.\n"));
	returnValue = hlSrioReNetInit(globalSysHostID);	

	/*zz modify
	if( returnValue == HL_OK)
		returnValue = hlMasterEndPoint(globalSysHostID);		*/

	/*目的是将destid设置为0xff，将hopcount设置为0*/
	SRIO_LOCAL_REG_READ(0);
	DB(printf("OK,The SrioNet ReInit complete.\n"));
    return returnValue;
}

HL_STATUS hlSrioReNetInit(UINT32 hostTargetID)
{
	UINT32 returnValue = 0;
	UINT32 tempport = 0xff;    
	
	pDrvCtrl = (RIO_DRV_CTRL *)malloc(sizeof(RIO_DRV_CTRL));
	if(pDrvCtrl == NULL)
		return HL_ERROR;

	/*zz modify*/
	M_SetHostID(hostTargetID);	

	do{
		SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, hostTargetID);	    
		SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, HOST_PORT);		
		SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, hostTargetID);
		tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
	}while(tempport != HOST_PORT);		
	
	returnValue = hlSrioReNetSearch(hostTargetID);

	if(HL_OK == returnValue)
	{
		globalSrioNetStatus = 1;
		printf("The SrioNet ReEnum is over.\n");
		return HL_OK;
	}
	else if(HL_CONCEDE == returnValue)
	{
	     globalSrioNetConcede = 1;
	     return HL_ERROR;
	}
	else if (HL_MAINTREAD_FAIL == returnValue)
	{
		printf("check system link status and restart to try again\n");
		return HL_ERROR; 
	}
	else
	{
		printf("error happen during hlSrioReNetSearch\n");
		return HL_ERROR;
	}
}

HL_STATUS hlSrioHostReConfigLock(UINT32 hostTargetID)
{
	UINT32 tempVal=0; 
	/*check if the target device has already been locked by other host or by self */
	tempVal = SRIO_LOCAL_REG_READ(SRIO_HBDIDLCSR_OFFSET)&(0x0000FFFF);
	if(tempVal==hostTargetID)     
		return HL_ALREADY_LOCK;
        
	SRIO_LOCAL_REG_WRITE(SRIO_HBDIDLCSR_OFFSET,hostTargetID);
	tempVal = SRIO_LOCAL_REG_READ(SRIO_HBDIDLCSR_OFFSET)&(0x0000FFFF);         
	
	/*if not equal the lock id,need unlock with the last id*/
	if( tempVal != globalSysHostID_1 && tempVal != globalSysHostID_2 )
		SRIO_LOCAL_REG_WRITE(SRIO_HBDIDLCSR_OFFSET, hostTargetID);/*lock with host id*/
	else
	{
		SRIO_LOCAL_REG_WRITE(SRIO_HBDIDLCSR_OFFSET,tempVal);/*unlock with last id	*/
		SRIO_LOCAL_REG_WRITE(SRIO_HBDIDLCSR_OFFSET, hostTargetID);/*lock with host id*/
		tempVal = SRIO_LOCAL_REG_READ(SRIO_HBDIDLCSR_OFFSET)&(0x0000FFFF);
		if(tempVal>hostTargetID)     
			return HL_CONCEDE;
   		else if(tempVal<hostTargetID)     
			return HL_WAIT_OTHER_CONCEDE;
	}

	return HL_LOCK;
} 

HL_STATUS hlSrioRemoteReConfigLock(UINT32 hostTargetID,UINT16 hopcount)
{
	UINT32 tempVal=0;
	UINT32 mtRdata=0;

	if (HL_OK == hlMaintRead(0xff,hopcount, SRIO_HBDIDLCSR_OFFSET, &mtRdata))    
		tempVal = mtRdata&(0x0000FFFF);        
	else
		hlMtRdFailReport (0xff, hopcount, SRIO_HBDIDLCSR_OFFSET, NULL);        

	if(tempVal == hostTargetID)
		return HL_ALREADY_LOCK;

	/*if not equal the lock id,need unlock with the last id*/
	if( tempVal != globalSysHostID_1 && tempVal != globalSysHostID_2 )
	{
		DB(printf("hlSrioRemoteReConfigLock--lock the switch !\n"));
		hlMaintWrite(0xff, hopcount, SRIO_HBDIDLCSR_OFFSET,hostTargetID);/*lock with host id	*/
	}
	else
	{
		hlMaintWrite(0xff, hopcount, SRIO_HBDIDLCSR_OFFSET,tempVal);/*unlock with last id*/
/*		
		if (HL_OK == hlMaintRead(0xff,hopcount, SRIO_HBDIDLCSR_OFFSET, &mtRdata))    
			tempVal = mtRdata&(0x0000FFFF); 	   
		else
			hlMtRdFailReport (0xff, hopcount, SRIO_HBDIDLCSR_OFFSET, NULL); 	   
*/	
		hlMaintWrite(0xff, hopcount, SRIO_HBDIDLCSR_OFFSET,hostTargetID);
	
		if (HL_OK == hlMaintRead(0xff,hopcount, SRIO_HBDIDLCSR_OFFSET, &mtRdata))
    			tempVal = mtRdata&(0x0000FFFF);        
		else
        		hlMtRdFailReport (0xff, hopcount, SRIO_HBDIDLCSR_OFFSET, NULL);
	
		if(tempVal>hostTargetID)				
			return HL_CONCEDE;		
   		else if(tempVal<hostTargetID)     
			return HL_WAIT_OTHER_CONCEDE;
	}

	return HL_LOCK;
}

HL_STATUS hlSrioReNetSearch(UINT32 hostTargetID)
{        
	int index;
	int hostPort = HOST_PORT;      /*zz add for host id port*/
	UINT16 hopcount=1;         		/*zz modify,origin is 0*/
	long returnValue=0;
	SWITCH_ROUTE_TBL *pSwTbl=NULL;
	hostAllSwTbl *pHostAllSwTbl=NULL;

	/*init the  struct used to store the information of the srio network*/	
	pHostAllSwTbl=(hostAllSwTbl*)malloc(sizeof(hostAllSwTbl));
	if(pHostAllSwTbl==NULL)
		return HL_ERROR;
        
	pHostAllSwTbl->switchNum=0;
	pHostAllSwTbl->srioDevNum=0;            
	pHostAllSwTbl->procNum=0;
        
	for(index=0;index<MAX_NUMBER_RIO_SW;index++)
		pHostAllSwTbl->hostAllSwInfo[index]=NULL;
                
	memset(pHostAllSwTbl->switchInterConnectionInfo,0,sizeof(SWITCH_INFO)*MAX_NUMBER_RIO_SW*MAX_NUMBER_RIO_SW);
	pDrvCtrl->pAllSwTbl=pHostAllSwTbl;	
	
	/*begin to topo the net */
	/*try to lock, if failed, then concede the host*/
	returnValue=hlSrioHostReConfigLock(hostTargetID);/*lock the switch direct connect to the host*/
	if(returnValue == HL_CONCEDE)
	{
		printf("the local host'priority is lower, it has to concede !\n");
		goto exit;
	}
	if(returnValue == HL_WAIT_OTHER_CONCEDE)
	{
		printf("the local host has been locked, it has to concede !\n");
		returnValue = HL_CONCEDE; 
		goto exit;
	}
	if(returnValue == HL_ALREADY_LOCK)
	{
		DB(printf("the local host already been locked by self !\n"));
		goto exit;
 	}

	pSwTbl=(SWITCH_ROUTE_TBL *)malloc(sizeof(SWITCH_ROUTE_TBL));
	if(pSwTbl == NULL)
	{
		printf("childpSwTbl switch 0 malloc failed.\n");
		returnValue = HL_ERROR;
		goto exit;
	}
	/*DB(printf("address of pSwtbl is 0x%08x sizeof(pSwTbl)=%d  \n",(UINT32)pSwTbl,sizeof(pSwTbl)));*/
	bzero((char*)pSwTbl,sizeof(SWITCH_ROUTE_TBL));
	pSwTbl->srioDeviceNum=0;
	pSwTbl->remoteSrioDeviceNum=0;
	pSwTbl->hostCfgRouteTbl[0].portNum=HOST_PORT;
	pSwTbl->hostCfgRouteTbl[0].distance=1;
	pSwTbl->hostCfgRouteTbl[0].nodeIndex=hostTargetID;
	/*DB(printf("pSwHostCfgRouTbl[0]-->%d\n",hostTargetID));*/
	pSwTbl->hostCfgRouteTbl[0].deviceStyle=SRIO_HOST;
	DB(printf("host of the system:id=0x%x, port= %d\n",hostTargetID,hostPort));
	pSwTbl->srioDeviceNum += 1;
              
	returnValue=hlSrioReSwitchEnum(pDrvCtrl,pSwTbl,hopcount,hostTargetID,0,0);
	if(returnValue==HL_CONCEDE)
	{
    		printf("one switch has been locked by other host, local host has to concede !\n");
		globalSrioNetConcede = 1;

		returnValue = hlSrioNetConcede(pDrvCtrl, hostTargetID);
		if( HL_OK !=returnValue)
       		printf("The local virtual host concede failed.\n");

		goto exit;
	}
                        
	if(pHostAllSwTbl->switchNum>2)
	{
    		/*a complex net,first change switch information to get a shortest inter-connect between switches */
	        hlSrioSwitchInfoExchange(pHostAllSwTbl);
	}
	/*configure the shortest way for each srio device according to the switch information*/
	hlSrioAddRoute(pDrvCtrl,hostTargetID);         
	return HL_OK;     

exit:

	free(pHostAllSwTbl);
	pHostAllSwTbl = NULL;
	pDrvCtrl->pAllSwTbl = NULL;
	return returnValue;
}

HL_STATUS hlSrioReSwitchEnum(
	RIO_DRV_CTRL *pDrvCtrl,
	SWITCH_ROUTE_TBL *pSwTbl,
	UINT16 hopCount,
	UINT32 hostTargetID,
	UINT32 localSwithIndex,
	UINT32 parentSwithIndex)
{
	long returnValue;
	int i;
	int masterEn;
	int id=0,mid=0;
	/*the num of srio device connect to local switch other than switch  */
	int localSrioDevCount=pSwTbl->srioDeviceNum;        
	int childHopCount=hopCount+1;
	UINT16 truehopcount = 0;
	int routeTblIndex=0;
	UINT32	tempValue=0;
	int port,portBrother;
	int portCnt=0;
	UINT32 mtRdata=0;
	UINT32 tempport;
	UINT32 localSwitchId;
	UINT32 childSwitchIndex=localSwithIndex;
	unsigned int status;
	unsigned int parentPortNum;     
	PEFCAR_DESC pefcar;
	hostAllSwTbl *pHostAllSwTbl=NULL;
	SWITCH_ROUTE_TBL *childpSwTbl = NULL;		
	UINT32 mainSWPort = 0;
	
#ifdef ID_SLOT_BIND	
	unsigned char processSwitch = 0;
	UINT32 localSWDeviceId = 0;
	#ifdef ENUM_BY_PROCESS
		processSwitch = 1;
	#endif
#endif
	if(hopCount>=1 )
		truehopcount = hopCount-1;
        
	pHostAllSwTbl=pDrvCtrl->pAllSwTbl;
	pHostAllSwTbl->hostAllSwInfo[pHostAllSwTbl->switchNum]=pSwTbl;
	pHostAllSwTbl->switchNum++;
	localSwitchId=RIO_SW_FIRST_VIRTUAL_TGTID+localSwithIndex;

	if(localSwithIndex==0)
	{
		/*discover*/
		SRIO_LOCAL_REG_WRITE(SRIO_PGCCSR_OFFSET, ((SRIO_LOCAL_REG_READ(SRIO_PGCCSR_OFFSET))|(PGCCSR_DISCOVERED)));                        
		/*set local switch virtual id*/
		SRIO_LOCAL_REG_WRITE(SRIO_COMP_TAG, localSwitchId);
		parentPortNum = HOST_PORT;		
	}
	else
	{
		hlSrioConfigSetDiscover(hostTargetID, 0xff, truehopcount);                      
		/*set local switch virtual id*/
		(srioSwitchMethod.swtichControl)(pDrvCtrl,hostTargetID,0xff,truehopcount,SET_SWITCH_ID, &localSwitchId);                        
		/*get the mother port,*/
		(srioSwitchMethod.swtichControl)(pDrvCtrl,hostTargetID,0xff,truehopcount, READ_INITIATOR_PORT, &parentPortNum);                         
		/*updata the LUT,  */
		(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,hostTargetID,0xff,truehopcount,parentPortNum, hostTargetID);                        
		/*set the default port to be the parent port num*/
		(srioSwitchMethod.swtichControl)(pDrvCtrl, hostTargetID,0xff,truehopcount,SET_DEFAULT_PORT,&parentPortNum );
		DB(printf("parentport Num=%d  switch =%d  \n",parentPortNum,(pHostAllSwTbl->switchNum)-1));
	}
                
	/*search each port of the switch and try to find if there are any srio devices */
	if(localSwithIndex == 0) 
		portCnt = ((SRIO_LOCAL_REG_READ(SRIO_SW_PORT_INFO))&SRIO_SW_PORT_NUM_MASK)>>8;
	else
	{
		if(HL_OK == hlMaintRead(0xff, truehopcount, SRIO_SW_PORT_INFO, &mtRdata))
			portCnt	= (mtRdata&SRIO_SW_PORT_NUM_MASK)>>8;
		else        
			hlMtRdFailReport(0xff, truehopcount, SRIO_SW_PORT_INFO, pHostAllSwTbl);        
	}                
	DB(printf("local switch %d portCnt %d.\n",localSwithIndex,portCnt));
	for(i=0;i<MAX_NUMBER_RIO_SW;i++)         
		pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][i].maxNum = portCnt;         

#ifdef ID_SLOT_BIND
	#ifdef ENUM_BY_PROCESS
		if(localSwithIndex == 0 )
		{
			mainSWPort = PROCESS_PORT_INDEX;
			localSWDeviceId = 0;
		}
		else
		{
			pDrvCtrl->portToSw= 0xff;
			hlSrioGetInfoFromMainSwitch(pDrvCtrl,hopCount,hostTargetID,localSwithIndex,portCnt);
			mainSWPort = pDrvCtrl->portToSw;
			localSWDeviceId = 0;
			/*DB(printf("--------------localSwithIndex = %d,  mainSWPort = %d .\n",localSwithIndex,mainSWPort));*/
		}
	#else
		pDrvCtrl->portToSw= 0xff;
		hlSrioGetInfoFromMainSwitch(pDrvCtrl,hopCount,hostTargetID,localSwithIndex,portCnt);
		mainSWPort = pDrvCtrl->portToSw;
		localSWDeviceId = 0;
		DB(printf("--------------localSwithIndex = %d,  mainSWPort = %d .\n",localSwithIndex,mainSWPort));
	#endif
#endif

	for(port=0;port<portCnt;port++)
  	{
		if(port == parentPortNum)
		{
			if(localSwithIndex == 0)
			{
				continue;
			}
			else
			{
				/*one switch connect to another switch*/
				if(pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].totalPortNum == 0)
				{
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].maxNum = portCnt;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].portPtr   \
                                        = (UINT32 *)malloc(sizeof(UINT32)*portCnt);
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].portBrotherPtr\
                                        = (UINT32 *)malloc(sizeof(UINT32)*portCnt);
					if( pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].portPtr==NULL||
						pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].portBrotherPtr==NULL)
					{
						printf("switchInterConnectionInfo between switch %d and switch %d malloc fail.\n",localSwithIndex,parentSwithIndex);
						return HL_ERROR;
					}
					(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,hostTargetID,0xff,truehopcount,port, 0xff);
					(srioSwitchMethod.swtichControl)(pDrvCtrl,hostTargetID,0xff,hopCount, READ_INITIATOR_PORT, &portBrother);							
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].distance=1;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].totalPortNum++;                                     
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].portPtr \
                                                      [pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].totalPortNum-1]=port;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].portBrotherPtr\
                                                      [pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].totalPortNum-1]=portBrother;
					DB(printf("switch %d connect to switch %d through port %d to port %d\n",localSwithIndex,parentSwithIndex,port,portBrother));
				}
				else
				{					
					(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,hostTargetID,0xff,truehopcount,port, 0xff);
					(srioSwitchMethod.swtichControl)(pDrvCtrl,hostTargetID,0xff, hopCount, READ_INITIATOR_PORT, &portBrother);					
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].totalPortNum++;                                     
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].portPtr \
                                 [pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].totalPortNum-1]=port;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].portBrotherPtr\
                                                      [pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].totalPortNum-1]=portBrother;
					DB(printf("switch %d connect to switch %d through port %d to port %d\n",localSwithIndex,parentSwithIndex,port,portBrother));
				}
			}
			continue;
		}

		if(localSwithIndex == 0) 
			status = SRIO_LOCAL_REG_READ(SRIO_PORTX_STATUS_OFFSET(port));
		else
		{
			if (HL_OK == hlMaintRead(0xff, truehopcount, SRIO_PORTX_STATUS_OFFSET(port), &mtRdata))			
				status = mtRdata;
			else            
				hlMtRdFailReport(0xff, truehopcount, SRIO_PORTX_STATUS_OFFSET(port), pHostAllSwTbl);             
		}
   
		if((status&PORT_OK)==PORT_OK)
		{
			DB(printf("switch %d - port %d status is ok.hopcount = %d\n",localSwithIndex,port,hopCount));						
			/*use 0xff as default id to visit this srio device*/
			if(localSwithIndex == 0)
			{
				do{
					SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, 0xff);                                    
					SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, port);
					SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, 0xff); 
					tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
				}while(tempport!=port);
			}
			else
			{
				(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,hostTargetID,0xff,truehopcount,port, 0xff);
			}
			
			/*对于1848  Output port transmit enable 和input port receive enable都是没有使能的
			*所以需要将端口的transmit和receive状态使能,对于578这两个状态已经使能了*/
			if(localSwithIndex == 0)
			{
				mtRdata = SRIO_LOCAL_REG_READ(0);
				if( mtRdata == DEVICE_ID_1848 )
				{
					mtRdata = SRIO_LOCAL_REG_READ(SRIO_PORT_SELFLOCK_OFFSET(port));
					mtRdata |= TRANSMIT_RECEIVE_ENABLE;	
					SRIO_LOCAL_REG_WRITE(SRIO_PORT_SELFLOCK_OFFSET(port), mtRdata);
				}
			}
			else
			{
			
				hlMaintRead(0xff, truehopcount, 0, &mtRdata);
				if( mtRdata == DEVICE_ID_1848 )
				{
					mtRdata = hlMaintRead(0xff, truehopcount, SRIO_PORT_SELFLOCK_OFFSET(port), &mtRdata);
					mtRdata |= TRANSMIT_RECEIVE_ENABLE;	
					hlMaintWrite(0xff, truehopcount, SRIO_PORT_SELFLOCK_OFFSET(port),mtRdata);					
				}
			}

			/*try to lock*/
			returnValue=hlSrioRemoteReConfigLock(hostTargetID,hopCount);
	
			if(returnValue == HL_CONCEDE)
				return returnValue;                                
			else if(returnValue == HL_WAIT_OTHER_CONCEDE)
			{
				returnValue=hlSrioRemoteConfigWaitConcede(hostTargetID,hopCount);  
				if(returnValue == HL_CONCEDE)
					return returnValue;
			}                        
			else if(returnValue == HL_ALREADY_LOCK)
			{                               
				/*ring exist, locked before */				 
                	if (HL_OK == hlMaintRead(0xff, hopCount, SRIO_PEFCAR_OFFSET, &mtRdata))
				pefcar.word = mtRdata;                    
			else                 
				hlMtRdFailReport (0xff, hopCount, SRIO_PEFCAR_OFFSET, pHostAllSwTbl);                
						
			if(pefcar.desc.sw == 1)
			{    
				/*get the switch virtual id*/
				if(HL_OK == hlMaintRead(0xff, hopCount, SRIO_COMP_TAG, &mtRdata))                                     
					tempValue = mtRdata;                                     
				else                                     
					hlMtRdFailReport (0xff, hopCount, SRIO_COMP_TAG, pHostAllSwTbl);                                     
                                
				tempValue-=RIO_SW_FIRST_VIRTUAL_TGTID;
				/*one switch connect to another switch*/
				if(pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].totalPortNum == 0)
				{
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].maxNum = portCnt;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].portPtr   \
                                            = (UINT32 *)malloc(sizeof(UINT32)*portCnt);
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].portBrotherPtr\
                                            = (UINT32 *)malloc(sizeof(UINT32)*portCnt);						
					if( pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].portPtr==NULL||
						pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].portBrotherPtr==NULL)
					{
						printf("switchInterConnectionInfo between switch %d and switch %d malloc fail.\n",localSwithIndex,tempValue);
						return HL_ERROR;
					}
					(srioSwitchMethod.swtichControl)(pDrvCtrl,hostTargetID,0xff,hopCount, READ_INITIATOR_PORT, &portBrother);
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].distance=1;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].totalPortNum++;                                     
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].portPtr \
                                                       [pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].totalPortNum-1]=port;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].portBrotherPtr\
                                                      [pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].totalPortNum-1]=portBrother;
				}
				else
				{
					(srioSwitchMethod.swtichControl)(pDrvCtrl,hostTargetID,0xff,hopCount, READ_INITIATOR_PORT, &portBrother);
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].totalPortNum++;                                     
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].portPtr \
                                                       [pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].totalPortNum-1]=port;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].portBrotherPtr\
                                                       [pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].totalPortNum-1]=portBrother;
				}                         
				DB(printf(" ring exist between switch %d and switch %d through port %d to port %d!!\n",localSwithIndex,tempValue,port,portBrother));
				continue;
			}
		}

		/*判断是哪种设备*/
		/*采用以下方式是因为86板子上的节点有时会读不出来*/
		do{
			if (HL_OK == hlMaintRead(0xff, hopCount, SRIO_PEFCAR_OFFSET, &mtRdata))            
				pefcar.word = mtRdata;							                
			else             
				hlMtRdFailReport (0xff, hopCount, SRIO_PEFCAR_OFFSET, pHostAllSwTbl);
			
		}while(pefcar.word == 0);
		/*
		if (HL_OK == hlMaintRead(0xff, hopCount, SRIO_PEFCAR_OFFSET, &mtRdata))            
			pefcar.word = mtRdata;							                
		else             
			hlMtRdFailReport (0xff, hopCount, SRIO_PEFCAR_OFFSET, pHostAllSwTbl);
		*/			
		if((pefcar.desc.memory==1) || (pefcar.desc.proc==1)||(pefcar.desc.bridge==1))
		{                        
			DB(printf("it's a end point connect to port %d !!\n",port));
			/*重枚举时，如果该节点已经使能了，不能将它disable掉
			因为这时很可能它正在发送数据，但如果发现的节点
			没有使能，反而需要让它使能，因为后面不会再使能
			节点了。这种情况是在处理板重启时,如果节点默认不
			使能。zz modify*/				
			if (HL_OK == hlMaintRead(0xff, hopCount, SRIO_PGCCSR_OFFSET, &mtRdata))			
				masterEn = mtRdata;                
			else            
				hlMtRdFailReport (0xff, hopCount, SRIO_PGCCSR_OFFSET, pHostAllSwTbl);            
	                                
			if (!(masterEn & MASTER_ENABLE)) 
				hlMaintWrite(0xff, hopCount, SRIO_PGCCSR_OFFSET,masterEn|MASTER_ENABLE);                          
			
			#if 0
			/*disable the master enable bit.  after enum is over, enable all end point*/
			if (HL_OK == hlMaintRead(0xff, hopCount, SRIO_PGCCSR_OFFSET, &mtRdata))			
				masterEn = mtRdata;                
			else            
				hlMtRdFailReport (0xff, hopCount, SRIO_PGCCSR_OFFSET, pHostAllSwTbl);            
	                                
			if (masterEn & MASTER_ENABLE)            
				hlMaintWrite(0xff, hopCount, SRIO_PGCCSR_OFFSET,masterEn&(~MASTER_ENABLE));                          
			#endif            
			/*use port num to reflect the srio device id for x4 mode which will be convenient for user to use,
			* but in x1mode this may be a problem because a switch has 16 ports, we only leave 10 id
			* numbers to be used in one board.      at that time we must change the way to define the id.
			*such as:
			id=localSwithIndex*20+localSrioDevCount;
			*/                                                                                
			/*zz modify,enum again not config the local switch */
			/*hlSrioConfigSetDiscover(hostTargetID, 0xff, hopCount);*/ 
#ifdef ID_SLOT_BIND
			/*说明是该endpoint是直接连接在交换板的交换芯片上*
			  *需要找出是交换板上的哪块交换芯片，哪个端口*/			
			if (localSwithIndex == (0+processSwitch))
			{
				if (port == 2) 							
					mainSWPort= 1;
				else if(port == 4)
					mainSWPort= 2;
				else if(port == 5) 							
					mainSWPort= 3;
				else if(port == 8) 							
					mainSWPort= 4;
				else if(port == 9) 							
					mainSWPort= 5;		

				localSWDeviceId = 0;
			}
			else if(localSwithIndex == (1+processSwitch))
			{
				if (port == 1) 							
					mainSWPort= 6;
				else if(port== 2) 							
					mainSWPort= 7;
				else if(port == 4) 							
					mainSWPort= 8;
				else if(port == 5) 							
					mainSWPort= 9;
				else if(port == 8) 							
					mainSWPort= 10;
				else if(port == 9) 							
					mainSWPort= 11;

				localSWDeviceId = 0;
			}
			else if(localSwithIndex == (2+processSwitch))
			{
				if (port == 0) 							
					mainSWPort= 12;
				else if(port == 1)
					mainSWPort= 13;
				else if(port== 4) 							
					mainSWPort= 14;
				else if(port == 5) 							
					mainSWPort= 15;
				else if(port == 8) 							
					mainSWPort= 16;
				else if(port == 9) 							
					mainSWPort= 17;								

				localSWDeviceId = 0;
			}
			else
			{
				//if( pDrvCtrl->portToSw == 0xff )
				if( mainSWPort == 0xff )
				{
					mainSWPort = globalMaxSWPort+1;
					pDrvCtrl->portToSw = globalMaxSWPort;
					//globalMaxSWPort++;
					localSWDeviceId = 0;
					DB(printf("local switch %d not direct connect to the switch board, set mainSWPort = %d!!!!!!!\n",localSwithIndex,mainSWPort));
				}
			}
		
			if( mainSWPort == 6 )
			{
				id = 0x80+localSWDeviceId;
			}
			else if( mainSWPort == 1 )
			{
				id = 0xe0+localSWDeviceId;
			}
			else if( mainSWPort == 3 )
			{
				id = 0x10+localSWDeviceId;
			}
			else if( mainSWPort == 5 )
			{
				id = 0x20+localSWDeviceId;
			}
			else if( mainSWPort == 2 )
			{
				id = 0x30+localSWDeviceId;
			}
			else if( mainSWPort == 4 )
			{
				id = 0x40+localSWDeviceId;
			}
			else if( mainSWPort == 7 )
			{
				id = 0x50+localSWDeviceId;
			}
			else if( mainSWPort == 9 )
			{
				id = 0x60+localSWDeviceId;
			}
			else if( mainSWPort == 11 )
			{
				id = 0x70+localSWDeviceId;
			}
			else if( mainSWPort == 8 )
			{
				id = 0x90+localSWDeviceId;
			}
			else if( mainSWPort == 17 )
			{
				id = 0xf6;
			}
			else if( mainSWPort == 13 )
			{
				id = 0xf7;
			}
			else
			{
				id = ((mainSWPort<<3)|localSWDeviceId);
			}

			localSWDeviceId++;		
#else	
			id=(localSwithIndex+1)*SWITCH_LOCAL_DEV_NUM+localSrioDevCount;				
#endif
			mid = id<<16;
			localSrioDevCount++;
			/*set id*/
			hlMaintWrite(0xff, hopCount, SRIO_BDIDCSR_OFFSET,mid);
			/*updata switch route table*/ 								
			if(hopCount == 0 || localSwithIndex == 0 )	/*zz modify, origin is if(hopCount == 0)*/
			{
				do{
					SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, id);                                    
					SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, port);
					SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, id); 
					tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
				}while(tempport!=port);                                        
			}
			else
				(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,hostTargetID,0xff,truehopcount,port,id);

			DB(printf("id= %d    mainSWPort = %d    localSwithIndex = %d   port= %d \n",id,mainSWPort,localSwithIndex,port));
			routeTblIndex=localSrioDevCount-1;
			pSwTbl->hostCfgRouteTbl[routeTblIndex].portNum=port;
			pSwTbl->hostCfgRouteTbl[routeTblIndex].distance=1;
			pSwTbl->hostCfgRouteTbl[routeTblIndex].nodeIndex=id;                                
			/*DB(printf("pSwHostCfgRouTbl[%d]-->%d\n",routeTblIndex,id));*/
			if(pefcar.desc.bridge==1&&pefcar.desc.proc==1)
			{
				/*it must be 8641d*/
				pHostAllSwTbl->procNum++;
				pSwTbl->hostCfgRouteTbl[routeTblIndex].deviceStyle=SRIO_PROC;
			}
			else
			{
				pSwTbl->hostCfgRouteTbl[routeTblIndex].deviceStyle=SRIO_ENDPOINT;
			}               
			continue;
		}

		if(pefcar.desc.sw==1)
			{
				DB(printf("it's a child switch connect to port %d !!\n",port));
				childSwitchIndex=pHostAllSwTbl->switchNum;
				/*one switch connect to another switch*/
				if(pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].totalPortNum == 0)
				{
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].maxNum = portCnt;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].portPtr   \
                                            = (UINT32 *)malloc(sizeof(UINT32)*portCnt);
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].portBrotherPtr\
                                            = (UINT32 *)malloc(sizeof(UINT32)*portCnt);
					if( pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].portPtr==NULL ||
						pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].portBrotherPtr==NULL)
					{
						DB(printf("switchInterConnectionInfo between switch %d and switch %d malloc fail.\n",localSwithIndex,childSwitchIndex));
						return HL_ERROR;
					}
					(srioSwitchMethod.swtichControl)(pDrvCtrl,hostTargetID,0xff,hopCount, READ_INITIATOR_PORT, &portBrother);
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].distance=1;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].totalPortNum++;                                     
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].portPtr \
                                                        [pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].totalPortNum-1]=port;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].portBrotherPtr\
                                                        [pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].totalPortNum-1]=portBrother;
					DB(printf("switch %d connect to switch %d through port %d to port %d\n",localSwithIndex,childSwitchIndex,port,portBrother));
				}
				else
				{
					(srioSwitchMethod.swtichControl)(pDrvCtrl,hostTargetID,0xff,hopCount, READ_INITIATOR_PORT, &portBrother);
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].totalPortNum++;                                     
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].portPtr \
                                                        [pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].totalPortNum-1]=port;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].portBrotherPtr\
                                                        [pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].totalPortNum-1]=portBrother;
					DB(printf("switch %d connect to switch %d through port %d to port %d\n",localSwithIndex,childSwitchIndex,port,portBrother));
				}
				/*child switch begin enum*/
				childpSwTbl=(SWITCH_ROUTE_TBL *)malloc(sizeof(SWITCH_ROUTE_TBL));
				if(childpSwTbl == NULL)
				{
					printf("childpSwTbl switch %d malloc failed.\n",childSwitchIndex);
					return HL_ERROR;
				}                                         
				/*DB(printf("address of pSwtbl is 0x%08x sizeof(pSwTbl)=%d  \n",(UINT32)childpSwTbl,sizeof(childpSwTbl)));*/
				bzero((char*)childpSwTbl,sizeof(SWITCH_ROUTE_TBL));
				childpSwTbl->srioDeviceNum=0;
				childpSwTbl->remoteSrioDeviceNum=0;
				returnValue=hlSrioReSwitchEnum(pDrvCtrl,childpSwTbl,childHopCount,hostTargetID,childSwitchIndex,localSwithIndex);
				if(returnValue==HL_CONCEDE)
				{
					DB(printf("one switch has been locked by other host with higher priority, local 8641d has to concede !\n"));
					return  returnValue;
				}
				continue;                                                               
			}
		}	        
	} 
		
	if(localSwithIndex==0)
		pSwTbl->srioDeviceNum+=(localSrioDevCount-1);
	else
		pSwTbl->srioDeviceNum+=localSrioDevCount;
        
	pHostAllSwTbl->srioDevNum+=pSwTbl->srioDeviceNum;		
	DB(printf("After the enum of switch %d, the total srioDevNum is %d\n",localSwithIndex,pHostAllSwTbl->srioDevNum));

	return HL_OK;
}



