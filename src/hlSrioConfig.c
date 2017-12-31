#include "hlSrioConfig.h"

/*for easy transplant, if a different switch is needed.*/
SWITCH_METHOD srioSwitchMethod = 
{
    hlSwitchSetLUT,
    hlSwtichControl
};

RIO_DRV_CTRL *pDrvCtrl = NULL;
UINT32 globalSrioNetStatus = 0;
UINT32 globalSrioNetConcede = 0;

/*used in srio reconfig, because ring may exist in the net */
/*
bit31::0-means not found;		1-means has been found;
bit30::0-means not reset;		1-means has been reset;
*/
UINT32 globalSwitchStatus[MAX_NUMBER_RIO_SW];

/*used in srio reconfig,record the reset switch nums*/
UINT32 globalSwitchResetNum = 0;

UINT32 globalNewSrioDevIndex=0;
UINT32 globalSysHostID = 1;

unsigned char globalMCMask=0;
UINT16 globalMCPort[SRIO_MC_GROUP_PORT_NUM];


unsigned short globalSrioGroup[SRIO_MC_GROUP_NUM][SRIO_MC_GROUP_PORT_NUM];
unsigned char  globalSrioGroupNum = 0;
unsigned char  globalBridge = 0;

/*交换板对外只出17路，如果有交换芯片不直接和交换板上的
*交换芯片相连，如果需要进行id绑定，则需要从第18路开始往
*后累加，进行id的绑定。
*/
unsigned char  globalMaxSWPort = 18;

HL_STATUS hlSrioNetInit(UINT32 hostTargetID)
{
	UINT32 returnValue = 0;
	UINT32 mtRdata,tempport = 0xff;
	if(globalSrioNetStatus == 1)
    	{
    		printf("The SrioNet has been initiated\n");
		return HL_ERROR;
    	}
	else
	{
		DB(printf("----------------SrioLib Version::17-12-28.\n"));
		DB(printf("The SrioNet Enum begin.\n"));
	}

	pDrvCtrl = (RIO_DRV_CTRL *)malloc(sizeof(RIO_DRV_CTRL));
	if(pDrvCtrl==NULL)
		return HL_ERROR;

	pDrvCtrl->portToSw = 0xff;
	pDrvCtrl->portToSwDeviceId = 0;
	printf("Enable local master.\n");
	M_Enable_Local_Master(hostTargetID);	
	printf("Config local master route table.\n");

	do{
		SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, hostTargetID);	    
        SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, HOST_PORT);
        SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, hostTargetID);
        tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
	}while(tempport != HOST_PORT);		

	/*对于1848  Output port transmit enable 和input port receive enable都是没有使能的
	*所以需要将端口的transmit和receive状态使能,对于578这两个状态已经使能了*/

	mtRdata = SRIO_LOCAL_REG_READ(0);
	if( mtRdata == DEVICE_ID_1848 )
	{
		mtRdata = SRIO_LOCAL_REG_READ(SRIO_PORT_SELFLOCK_OFFSET(HOST_PORT));
		mtRdata |= TRANSMIT_RECEIVE_ENABLE;	
		SRIO_LOCAL_REG_WRITE(SRIO_PORT_SELFLOCK_OFFSET(HOST_PORT), mtRdata);
	}
	
	returnValue = hlSrioNetSearch(hostTargetID);

	if(HL_OK == returnValue)
	{
		globalSrioNetStatus = 1;
		printf("The SrioNet Enum is over.\n");
		return HL_OK;
	}
	else if(HL_CONCEDE == returnValue)
	{
	     globalSrioNetConcede = 1;
	     return HL_ERROR;
	}
	else if (HL_MAINTREAD_FAIL == returnValue)
	{
		printf ("check system link status and restart to try again\n");
		return HL_ERROR; 
	}
	else
	{
		printf("error happen during hlSrioNetSearch\n");
		return HL_ERROR;
	}
}

HL_STATUS hlSrioNetSearch(UINT32 hostTargetID)
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
	returnValue=hlSrioHostConfigLock(hostTargetID);/*lock the switch direct connect to the host*/
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
              
	returnValue=hlSrioSwitchEnum(pDrvCtrl,pSwTbl,hopcount,hostTargetID,0,0);
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
	hlSrioFreeMemory();
	return returnValue;
}


/********************************************************************
*       hlSrioSwitchEnum   	enum the switch                                                   
********************************************************************/

HL_STATUS hlSrioSwitchEnum(
	RIO_DRV_CTRL *pDrvCtrl,
	SWITCH_ROUTE_TBL *pSwTbl,
	UINT16  hopCount,
	UINT32  hostTargetID,
	UINT32  localSwithIndex,
	UINT32  parentSwithIndex)
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
	int portCnt = 0;
	UINT32 tempport;
	UINT32 localSwitchId;
	UINT32 childSwitchIndex=localSwithIndex;
	unsigned int status;
	unsigned int parentPortNum;     
	PEFCAR_DESC pefcar;
	hostAllSwTbl *pHostAllSwTbl=NULL;
	SWITCH_ROUTE_TBL *childpSwTbl = NULL;
	UINT32 mtRdata = 0;
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
		pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][i].maxNum = portCnt+SRIO_EXTEND_PORT_FOR_SHORTWAY;         

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
			DB(printf("--------------localSwithIndex = %d,  mainSWPort = %d .\n",localSwithIndex,mainSWPort));
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
				mtRdata = SRIO_LOCAL_REG_READ(0);
				if( mtRdata == DEVICE_ID_1848 )
				{
					mtRdata = SRIO_LOCAL_REG_READ(SRIO_PORT_SELFLOCK_OFFSET(port));
					mtRdata |= TRANSMIT_RECEIVE_ENABLE;	
					SRIO_LOCAL_REG_WRITE(SRIO_PORT_SELFLOCK_OFFSET(port), mtRdata);					
				}
				continue;
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
				/*one switch connect to another switch*/				
				if( (pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].totalPortNum) == 0)
				{
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].maxNum = portCnt+SRIO_EXTEND_PORT_FOR_SHORTWAY;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].portPtr   \
                                        = (UINT32 *)malloc(sizeof(UINT32)*portCnt);					
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].portBrotherPtr\
                                        = (UINT32 *)malloc(sizeof(UINT32)*portCnt);					
					if( (pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].portPtr==NULL)||
						(pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].portBrotherPtr==NULL))
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
					(srioSwitchMethod.swtichControl)(pDrvCtrl,hostTargetID,0xff,hopCount, READ_INITIATOR_PORT, &portBrother);
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][parentSwithIndex].distance=1;
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
			DB(printf("switch %d-port %d status is ok.\n",localSwithIndex,port));
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
			returnValue=hlSrioRemoteConfigLock(hostTargetID,hopCount);
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
						pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].maxNum = portCnt+SRIO_EXTEND_PORT_FOR_SHORTWAY;
						pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].portPtr   \
                                             = (UINT32 *)malloc(sizeof(UINT32)*portCnt);
						pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].portBrotherPtr\
                                             = (UINT32 *)malloc(sizeof(UINT32)*portCnt);
						if( (pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].portPtr==NULL)||
							(pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][tempValue].portBrotherPtr==NULL))
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
			if (HL_OK == hlMaintRead(0xff,hopCount,SRIO_PEFCAR_OFFSET,&mtRdata))            
				pefcar.word = mtRdata;							                
			else             
				hlMtRdFailReport (0xff, hopCount, SRIO_PEFCAR_OFFSET, pHostAllSwTbl);
            						
			if((pefcar.desc.memory==1) || (pefcar.desc.proc==1)||(pefcar.desc.bridge==1))
			{      				
				DB(printf("it's a end point connect to port %d !!\n",port));	
				/*disable the master enable bit.  after enum is over, enable all end point*/
				if (HL_OK == hlMaintRead(0xff, hopCount, SRIO_PGCCSR_OFFSET, &mtRdata))			
					masterEn = mtRdata;                
				else            
					hlMtRdFailReport (0xff, hopCount, SRIO_PGCCSR_OFFSET, pHostAllSwTbl);            
	                                
				if (masterEn & MASTER_ENABLE)            
					hlMaintWrite(0xff, hopCount, SRIO_PGCCSR_OFFSET,masterEn&(~MASTER_ENABLE));                          
            
				/*use port num to reflect the srio device id for x4 mode which will be convenient for user to use,
				* but in x1mode this may be a problem because a switch has 16 ports, we only leave 10 id
				* numbers to be used in one board.      at that time we must change the way to define the id.
				*such as:
				id=localSwithIndex*20+localSrioDevCount;
				*/                                                                                                         
				hlSrioConfigSetDiscover(hostTargetID, 0xff, hopCount);
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
					else
					{
						DB(printf("local switch %d,  portToSw = %d!!!!!!!\n",localSwithIndex,pDrvCtrl->portToSw));
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
				else if( mainSWPort == 12 )
				{
					id = 0xf9-localSWDeviceId;					
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
				/*如果是交换后插的bridge设备，需要将它扩展16个endpoint*/
				if(pefcar.desc.bridge==1&&pefcar.desc.proc!=1)
				{
					DB(printf("it's a bridge connect to port %d, i need to expand to 16 endpoint !!\n",port));
					if( globalBridge == 0 )
						id = RIO_EXPAND_ID_1;
					else if( globalBridge == 1 )
						id = RIO_EXPAND_ID_2;
					else
					{
						DB(printf("!!!!!error, system has too many bridge!!!\n"));
						continue;
					}
					globalBridge++;
					for(i=0; i<16;i++)
					{
						if( i != 0)
							id++;

						localSrioDevCount++;						
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
						pSwTbl->hostCfgRouteTbl[routeTblIndex].deviceStyle=SRIO_ENDPOINT;
					}
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
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].maxNum = portCnt+SRIO_EXTEND_PORT_FOR_SHORTWAY;
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].portPtr   \
                                            = (UINT32 *)malloc(sizeof(UINT32)*portCnt);
					pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].portBrotherPtr\
                                            = (UINT32 *)malloc(sizeof(UINT32)*portCnt);					
					
					if( (pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].portPtr==NULL) ||
						(pHostAllSwTbl->switchInterConnectionInfo[localSwithIndex][childSwitchIndex].portBrotherPtr==NULL))
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
				returnValue=hlSrioSwitchEnum(pDrvCtrl,childpSwTbl,childHopCount,hostTargetID,childSwitchIndex,localSwithIndex);
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


/********************************************************************************
*       hlM8641dSrioSwitchInfoExchange		exchagne information between switch to get the shortest
*                                                      		way to connect to other switch       
*********************************************************************************/
HL_STATUS hlSrioSwitchInfoExchange(hostAllSwTbl *pHostSw)
{
	hostAllSwTbl  *pHostAllSw=pHostSw;
	UINT32 index1=0,  index2=0, index3=0;           
	UINT32 curPortNum = 0;
	
	if(pHostAllSw==NULL)
		return HL_ERROR;

	for(index1=0;index1<pHostAllSw->switchNum;index1++)
	{
		for(index2=0;index2<pHostAllSw->switchNum;index2++)
		{
			for(index3=0;index3<pHostAllSw->switchNum;index3++)
			{
				if(index3==index2||index3==index1)
					continue;
				/*0 means infinite*/
				if(pHostAllSw->switchInterConnectionInfo[index1][index2].distance==0||  \
                            pHostAllSw->switchInterConnectionInfo[index2][index3].distance==0)
					continue;
                                                                
				if((pHostAllSw->switchInterConnectionInfo[index1][index2].distance+  \
                            pHostAllSw->switchInterConnectionInfo[index2][index3].distance< \
                            pHostAllSw->switchInterConnectionInfo[index1][index3].distance)||  \
                            (pHostAllSw->switchInterConnectionInfo[index1][index3].distance==0))
				{
					pHostAllSw->switchInterConnectionInfo[index1][index3].distance=  \
                    		pHostAllSw->switchInterConnectionInfo[index1][index2].distance+  \
                            	pHostAllSw->switchInterConnectionInfo[index2][index3].distance;
					/*change the port in use for switch route configure*/
					
					if(pHostAllSw->switchInterConnectionInfo[index1][index3].portPtr==NULL)
					{
						pHostAllSw->switchInterConnectionInfo[index1][index3].portPtr = 
								(UINT32*)malloc((sizeof(UINT32))*(pHostAllSw->switchInterConnectionInfo[index1][index3].maxNum));
						if( pHostAllSw->switchInterConnectionInfo[index1][index3].portPtr ==NULL)
						{
							printf("switchInterConnectionInfo between switch %d and switch %d malloc fail.\n",index1,index3);
							return HL_ERROR;
						}
					}
					if(pHostAllSw->switchInterConnectionInfo[index1][index3].shortWayPtr==NULL)
					{
						pHostAllSw->switchInterConnectionInfo[index1][index3].shortWayPtr= 
								(UINT32*)malloc((sizeof(UINT32))*(pHostAllSw->switchInterConnectionInfo[index1][index3].maxNum));
						if( pHostAllSw->switchInterConnectionInfo[index1][index3].shortWayPtr==NULL)
						{
							printf("switchInterConnectionInfo between switch %d and switch %d malloc fail.\n",index1,index3);
							return HL_ERROR;
						}
					}
					
					memcpy(pHostAllSw->switchInterConnectionInfo[index1][index3].portPtr,  \
							pHostAllSw->switchInterConnectionInfo[index1][index2].portPtr,    \
							sizeof(UINT32)*(pHostAllSw->switchInterConnectionInfo[index1][index2].maxNum));
					memcpy(pHostAllSw->switchInterConnectionInfo[index1][index3].shortWayPtr,  \
							pHostAllSw->switchInterConnectionInfo[index1][index2].portPtr,    \
							sizeof(UINT32)*(pHostAllSw->switchInterConnectionInfo[index1][index2].maxNum));
					pHostAllSw->switchInterConnectionInfo[index1][index3].totalPortNum = \
							pHostAllSw->switchInterConnectionInfo[index1][index2].totalPortNum;
				}
				else if((pHostAllSw->switchInterConnectionInfo[index1][index2].distance+  \
				pHostAllSw->switchInterConnectionInfo[index2][index3].distance == \
				pHostAllSw->switchInterConnectionInfo[index1][index3].distance) )
				{
					if( (curPortNum+pHostAllSw->switchInterConnectionInfo[index1][index2].totalPortNum)<= \
						pHostAllSw->switchInterConnectionInfo[index1][index3].maxNum )
					{
						curPortNum = pHostAllSw->switchInterConnectionInfo[index1][index3].totalPortNum;
						memcpy(&pHostAllSw->switchInterConnectionInfo[index1][index3].shortWayPtr[curPortNum],  \
							pHostAllSw->switchInterConnectionInfo[index1][index2].portPtr,    \
							sizeof(UINT32)*(pHostAllSw->switchInterConnectionInfo[index1][index2].totalPortNum));
						pHostAllSw->switchInterConnectionInfo[index1][index3].totalPortNum = curPortNum+\
							pHostAllSw->switchInterConnectionInfo[index1][index2].totalPortNum;	
					}
					else
					{
						printf("!!!error,too many the same short way between switch %d and switch %d.\n",index1,index3);
						continue;
					}
					
				}
			}
		}
	}
	return HL_OK;        
}

#if 0
HL_STATUS hlSrioSwitchInfoExchange(hostAllSwTbl *pHostSw)
{
	hostAllSwTbl  *pHostAllSw=pHostSw;
	UINT32 index1=0,  index2=0, index3=0;           
	UINT32 curPortNum = 0;
	UINT32 _curPort,i,j;
	UINT8 _bFind;
	
	if(pHostAllSw==NULL)
		return HL_ERROR;

	for(index1=0;index1<pHostAllSw->switchNum;index1++)
	{
		for(index2=0;index2<pHostAllSw->switchNum;index2++)
		{			
			for(index3=0;index3<pHostAllSw->switchNum;index3++)
			{
				if(index3==index2||index3==index1)
					continue;
				/*0 means infinite*/
				if(pHostAllSw->switchInterConnectionInfo[index1][index2].distance==0||  \
                            pHostAllSw->switchInterConnectionInfo[index2][index3].distance==0)
					continue;
                                                                
				if((pHostAllSw->switchInterConnectionInfo[index1][index2].distance+  \
                            pHostAllSw->switchInterConnectionInfo[index2][index3].distance< \
                            pHostAllSw->switchInterConnectionInfo[index1][index3].distance)||  \
                            (pHostAllSw->switchInterConnectionInfo[index1][index3].distance==0))
				{
					pHostAllSw->switchInterConnectionInfo[index1][index3].distance=  \
                    		pHostAllSw->switchInterConnectionInfo[index1][index2].distance+  \
                            	pHostAllSw->switchInterConnectionInfo[index2][index3].distance;
					/*change the port in use for switch route configure*/
					
					if(pHostAllSw->switchInterConnectionInfo[index1][index3].portPtr==NULL)
					{
						pHostAllSw->switchInterConnectionInfo[index1][index3].portPtr = 
								(UINT32*)malloc((sizeof(UINT32))*(pHostAllSw->switchInterConnectionInfo[index1][index3].maxNum));
						if( pHostAllSw->switchInterConnectionInfo[index1][index3].portPtr ==NULL)
						{
							printf("switchInterConnectionInfo between switch %d and switch %d malloc fail.\n",index1,index3);
							return HL_ERROR;
						}
					}
					if(pHostAllSw->switchInterConnectionInfo[index1][index3].shortWayPtr==NULL)
					{
						pHostAllSw->switchInterConnectionInfo[index1][index3].shortWayPtr= 
								(UINT32*)malloc((sizeof(UINT32))*(pHostAllSw->switchInterConnectionInfo[index1][index3].maxNum));
						if( pHostAllSw->switchInterConnectionInfo[index1][index3].shortWayPtr==NULL)
						{
							printf("switchInterConnectionInfo between switch %d and switch %d malloc fail.\n",index1,index3);
							return HL_ERROR;
						}
					}
					
					memcpy(pHostAllSw->switchInterConnectionInfo[index1][index3].portPtr,  \
							pHostAllSw->switchInterConnectionInfo[index1][index2].portPtr,    \
							sizeof(UINT32)*(pHostAllSw->switchInterConnectionInfo[index1][index2].maxNum));
					memcpy(pHostAllSw->switchInterConnectionInfo[index1][index3].shortWayPtr,  \
							pHostAllSw->switchInterConnectionInfo[index1][index2].portPtr,    \
							sizeof(UINT32)*(pHostAllSw->switchInterConnectionInfo[index1][index2].maxNum));
					pHostAllSw->switchInterConnectionInfo[index1][index3].totalPortNum = \
							pHostAllSw->switchInterConnectionInfo[index1][index2].totalPortNum;
				}
				else if((pHostAllSw->switchInterConnectionInfo[index1][index2].distance+  \
				pHostAllSw->switchInterConnectionInfo[index2][index3].distance == \
				pHostAllSw->switchInterConnectionInfo[index1][index3].distance) )
				{
					curPortNum = pHostAllSw->switchInterConnectionInfo[index1][index3].totalPortNum;
					if( (curPortNum+pHostAllSw->switchInterConnectionInfo[index1][index2].totalPortNum)<= \
						pHostAllSw->switchInterConnectionInfo[index1][index3].maxNum )
					{

						if( pHostAllSw->switchInterConnectionInfo[index1][index2].totalPortNum > 1 )
						{
							for( i=0;i<pHostAllSw->switchInterConnectionInfo[index1][index2].totalPortNum;i++)
							{
								_bFind = 0;
								_curPort = pHostAllSw->switchInterConnectionInfo[index1][index2].shortWayPtr[i];
								for( j=0;j<pHostAllSw->switchInterConnectionInfo[index1][index3].totalPortNum;j++)
								{
									if( _curPort == pHostAllSw->switchInterConnectionInfo[index1][index3].shortWayPtr[j])
									{
										_bFind = 1;
										break;
									}
								}

								if( _bFind == 0 )
								{							
									pHostAllSw->switchInterConnectionInfo[index1][index3].shortWayPtr[curPortNum] = _curPort;									
									pHostAllSw->switchInterConnectionInfo[index1][index3].totalPortNum++;
									curPortNum++;
								}								
							}
							
						}
						else
						{
							_curPort = pHostAllSw->switchInterConnectionInfo[index1][index2].portPtr[0];
							_bFind = 0;
							for( i=0;i<curPortNum;i++)
							{
								if( _curPort == pHostAllSw->switchInterConnectionInfo[index1][index3].shortWayPtr[i])
								{
									_bFind = 1;
									break;
								}
							}

							if( _bFind == 0 )
							{							
								pHostAllSw->switchInterConnectionInfo[index1][index3].shortWayPtr[curPortNum] = _curPort;								
								pHostAllSw->switchInterConnectionInfo[index1][index3].totalPortNum++;
							}
						}																			
						
					}
					else
					{
						printf("!!!error,too many the same short way between switch %d and switch %d.\n",index1,index3);
						printf("curret way is %d, max short way is %d\n",curPortNum+pHostAllSw->switchInterConnectionInfo[index1][index2].totalPortNum,pHostAllSw->switchInterConnectionInfo[index1][index3].maxNum);
						continue;
					}
					
				}
			}
		}
	}
	return HL_OK;        
}
#endif
HL_STATUS hlSrioInit()
{	
	UINT32 returnValue = 0;
	globalSysHostID = globalSysHostID_1;

	returnValue = hlSrioNetInit(globalSysHostID);
	
	if( returnValue == HL_OK)
	{
		hlSrioSetAllSWDefaultPort();
		returnValue = hlMasterEndPoint(globalSysHostID);		
	}
	else
		return HL_ERROR;
	
	SRIO_LOCAL_REG_READ(0);
	
	DB(printf("OK,The Srio Init complete.\n"));
	return returnValue;
}


HL_STATUS hlMasterEndPoint(UINT32 hostTargetID)
{
#if 0
	int msgLen, sysPointNum;
	int *sysIDArray = NULL, *sysIDArrayHead = NULL;
	int srioDevId;
	int i;
	UINT32 returnValue = 0;
	if((globalSrioNetConcede == 0)&&(globalSrioNetStatus ==1))
	{
		/*hostTargetID = SRIO_LOCAL_REG_READ(SRIO_HBDIDLCSR_OFFSET)&(0x0000FFFF);
		globalSysHostID = hostTargetID;zz delete*/

		sysIDArray = hlSrioEndIDGet(&msgLen, &sysPointNum);
		if(sysPointNum == 0)
				return HL_OK;
	
		sysIDArrayHead = sysIDArray;
				
		DB(printf ("master enable bit setting...\n"));		
		for(i=0; i<sysPointNum; i++)
		{
				srioDevId = *sysIDArray;
				printf("srioDevId:%d\n", dstID);
			  
				hlSrioDeviceEnableSet(hostTargetID, srioDevId);
			   
				sysIDArray++;
		}
		printf ("master enable bit set!\n");
		free(sysIDArrayHead);
	else
		return returnValue;		
#endif
	/*zz modify,origin is above*/

	int j,k,srioDevId;
	SWITCH_ROUTE_TBL *pSw=NULL;
	UINT32 returnValue = 0;
	if((globalSrioNetConcede == 0)&&(globalSrioNetStatus ==1))
	{		
		if( pDrvCtrl->pAllSwTbl == NULL )
			return HL_ERROR;
					
		for(j=0;j<pDrvCtrl->pAllSwTbl->switchNum;j++)
		{	
			pSw = pDrvCtrl->pAllSwTbl->hostAllSwInfo[j];
			if( pSw == NULL )
				continue;
						
			for(k=0;k<SWITCH_LOCAL_DEV_NUM;k++)
			{			   
				if(pSw->hostCfgRouteTbl[k].deviceStyle==0||pSw->hostCfgRouteTbl[k].deviceStyle==SRIO_HOST)
					continue;
				  
				srioDevId=pSw->hostCfgRouteTbl[k].nodeIndex;
				/*如果是虚拟节点跳过*/
				if( srioDevId >=0xc0 && srioDevId <=0xdf)
					continue;
				
				DB(printf("srioDevId id = %d--0x%x\n",srioDevId,srioDevId));
				hlSrioDeviceEnableSet(hostTargetID, srioDevId);
			}
		}
		return HL_OK;
	}
	else
		return returnValue;
}

/********************************************************************************
*       hlSrioAddRoute		reconifg the switch route table for srio devices that no directly 
*							connect to the target switch.
********************************************************************************/
void hlSrioAddRoute(RIO_DRV_CTRL *pDrvCtrl,int hostTargetID)
{        
	int i,j,k;
	UINT16 hopcount=0;
	int distance=0;
	int tgtId=0xff, srioDevId, srioDevNum;
	int port;
	hostAllSwTbl *pHostAllSwTbl=NULL;
	SWITCH_ROUTE_TBL *pSw=NULL;
	SWITCH_ROUTE_TBL *pSw1=NULL;
	int switchCount = 0;
	int hopDistance = 0; 
	UINT16 truehopcount = 0;
	UINT32 tempport = 0xff;
	pHostAllSwTbl=pDrvCtrl->pAllSwTbl;
		
	/*config each switch virtual id into all the other switches, used for switch 0 */
	DB(printf("Conifg the switch route table !\n"));
                        
	while(switchCount<(pHostAllSwTbl->switchNum))
	{
		for(i=0;i<(pHostAllSwTbl->switchNum);i++)
		{			
			while((pHostAllSwTbl->switchInterConnectionInfo[0][i].distance) == hopDistance)
			{
				hopcount= (UINT16)pHostAllSwTbl->switchInterConnectionInfo[0][i].distance;  
				truehopcount=hopcount;				
				pSw1=pHostAllSwTbl->hostAllSwInfo[i];
				tgtId = i+RIO_SW_FIRST_VIRTUAL_TGTID;
				srioDevNum=SWITCH_LOCAL_DEV_NUM;
				for(j=0;j<pHostAllSwTbl->switchNum;j++)
				{
					if(i==j)
						continue;                                   
					pSw=pHostAllSwTbl->hostAllSwInfo[j];
					port=hlSrioLoadPortGet(&pHostAllSwTbl->switchInterConnectionInfo[i][j]);
					distance=pHostAllSwTbl->switchInterConnectionInfo[i][j].distance+1;
					srioDevId=j+RIO_SW_FIRST_VIRTUAL_TGTID;
					if(hopcount == 0)   
					{
						do{
							SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, srioDevId);                                    
							SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, port);
							SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, srioDevId); 
							tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
						}while(tempport!=port);
					}
					else
					{
						(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,hostTargetID,tgtId,truehopcount,port, srioDevId);
					}
					/*DB(printf("local switch %d switch id=%d ,port=%d  hopcount = %d!\n",j,srioDevId,port,hopcount));*/
					for(k=0;k<pSw->srioDeviceNum;k++)
					{                                
						srioDevId=pSw->hostCfgRouteTbl[k].nodeIndex;
						port=hlSrioLoadPortGet(&pHostAllSwTbl->switchInterConnectionInfo[i][j]);												
						/*update the switch route table*/
						if(hopcount == 0)       
						{
							do{
								SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, srioDevId);                                    
								SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, port);
								SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, srioDevId); 
								tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
							}while(tempport!=port);
						}
						else
						{
							(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,hostTargetID,tgtId,truehopcount,port, srioDevId);
						}
						/*updata the hostAllSwInfo*/
						pSw1->hostCfgRouteTbl[srioDevNum].portNum=port;
						pSw1->hostCfgRouteTbl[srioDevNum].distance=distance;
						pSw1->hostCfgRouteTbl[srioDevNum].nodeIndex=srioDevId;
						/*DB(printf("pSwHostCfgTbl[%d]-->%d\n",srioDevNum, srioDevId));*/
						pSw1->hostCfgRouteTbl[srioDevNum].deviceStyle=pSw->hostCfgRouteTbl[k].deviceStyle;
						pSw1->remoteSrioDeviceNum++;
						srioDevNum++;
						/*DB(printf("sw %d port %d id %d distance %d!\n",j,port,srioDevId,distance));*/
					}
				}       
				switchCount++;
				break;
			}
		}
		hopDistance++;
	}        
}


/*************************************************************************
根据当前switch的 索引，判断它与主交换芯片之间的位置
关系，然后确定它连接在哪个主交换芯片上，以及主交
换的端口号，来锁定槽位号，这样就可以绑定id了。
*************************************************************************/
void hlSrioGetInfoFromMainSwitch(
	RIO_DRV_CTRL *pDrvCtrl,
	UINT16  hopCount,
	UINT32 	hostTargetID,
	UINT32  localSwithIndex,
	int 	portCnt)
{
	long returnValue;
	UINT16 truehopcount = 0;	
	UINT32	tempValue=0;
	int port;
	unsigned int status = 0;
	PEFCAR_DESC pefcar;
	UINT32 SwPortToLocal = 0xff;
	UINT32 mtRdata = 0;		
	unsigned char processSwitch = 0;
#ifdef ENUM_BY_PROCESS
	processSwitch = 1;
#endif

	if(hopCount>=1 )
		truehopcount = hopCount-1;			

	/*main switch num=0,1,2;but if the process node as the main node,main switch num = 1,2,3*/
#ifdef ENUM_BY_PROCESS
	if((localSwithIndex>2+processSwitch)||(localSwithIndex==0))
#else
	if(localSwithIndex>2+processSwitch)
#endif
	{
		/*to get the port num of host switch to this switch*/	
		for (port=0; port<portCnt; port++)
		{
			/*to get the link status of this switch*/
			if (HL_OK == hlMaintRead(0xff, truehopcount, SRIO_PORTX_STATUS_OFFSET(port), &mtRdata))
				status = mtRdata;						 
			
			if((status&PORT_OK) == PORT_OK)
			{	
				/*the port with number port is link on*/
				/*configure the route*/
				/*DB(printf("configure 0xFF to switch %d port %d\n", localSwithIndex, port));*/
				(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,hostTargetID,0xff,truehopcount,port, 0xff);	
				/*try to lock*/
				returnValue = hlSrioRemoteConfigLockCheck(hostTargetID,hopCount);	
				
				if(returnValue == HL_MAINTREAD_FAIL)
				{	
					printf("switch %d remoteconfig port %d device, maintread fail when lock check\n", localSwithIndex, port);				
					continue;
				}
				if(returnValue == HL_ALREADY_LOCK)
				{
					/*check if the host switch*/
					/*ring exist, locked before */
					tempValue=0;	
					if (HL_OK == hlMaintRead(0xff, hopCount, SRIO_PEFCAR_OFFSET, &mtRdata))
						pefcar.word = mtRdata;					

				
					if(pefcar.desc.sw == 1)
					{	 
						/*get the switch virtual id*/
						if(HL_OK == hlMaintRead(0xff, hopCount, SRIO_COMP_TAG, &mtRdata)) 				
							tempValue = mtRdata;					
							
						tempValue -= RIO_SW_FIRST_VIRTUAL_TGTID;						
						if (tempValue == (0+processSwitch)||tempValue == (1+processSwitch)||tempValue == (2+processSwitch))
						{
							/*means find the host switch*/
							/*configure the route*/
							(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,hostTargetID,0xff,truehopcount,port, 0xff);
							(srioSwitchMethod.swtichControl)(pDrvCtrl,hostTargetID,0xff,hopCount,GET_PORT_TO_SW,&SwPortToLocal);

							/*non-host switch those directly connectted to host switch must step to here, get the 
							host switch port connectted this switch, those indirectly connectted to host switch will not 
							step here ,and retain the port number recorded by the upper and directly-connectted to
							hostsw switch*/
							if (tempValue == (0+processSwitch))
							{
								if (SwPortToLocal == 2) 							
									pDrvCtrl->portToSw= 1;
								else if(SwPortToLocal == 4)
									pDrvCtrl->portToSw= 2;
								else if(SwPortToLocal == 5) 							
									pDrvCtrl->portToSw= 3;
								else if(SwPortToLocal == 8) 							
									pDrvCtrl->portToSw= 4;
								else if(SwPortToLocal == 9) 							
									pDrvCtrl->portToSw= 5;
								
								pDrvCtrl->portToSwDeviceId= 0;
								break;
							}
							else if(tempValue == (1+processSwitch))
							{
								if (SwPortToLocal == 1) 							
									pDrvCtrl->portToSw= 6;
								else if(SwPortToLocal == 2) 							
									pDrvCtrl->portToSw= 7;
								else if(SwPortToLocal == 4) 							
									pDrvCtrl->portToSw= 8;
								else if(SwPortToLocal == 5) 							
									pDrvCtrl->portToSw= 9;
								else if(SwPortToLocal == 8) 							
									pDrvCtrl->portToSw= 10;
								else if(SwPortToLocal == 9) 							
									pDrvCtrl->portToSw= 11;
									
								pDrvCtrl->portToSwDeviceId= 0;
								break;
							}
							else if(tempValue == (2+processSwitch))
							{
								if (SwPortToLocal == 0) 							
									pDrvCtrl->portToSw= 12;
								else if(SwPortToLocal == 1) 							
									pDrvCtrl->portToSw= 13;
								else if(SwPortToLocal == 4) 							
									pDrvCtrl->portToSw= 14;
								else if(SwPortToLocal == 5) 							
									pDrvCtrl->portToSw= 15;
								else if(SwPortToLocal == 8) 							
									pDrvCtrl->portToSw= 16;
								else if(SwPortToLocal == 9) 							
									pDrvCtrl->portToSw= 17;
								
								pDrvCtrl->portToSwDeviceId= 0;
								break;
							}
						}
					}
				}	
			}
		}
	}	
	
}

/********************************************************************************
*       hlSrioSetMutiCast		Set the switch multicast
*						
*						
********************************************************************************/
void hlSrioSetMutiCast(unsigned short group[],UINT8 bSearch)
{
	int k=0;
	UINT16 hopcount=0;
	int tgtId=0xff;
	int value;
	UINT32 portNum,portBrotherNum;
	hostAllSwTbl *pHostAllSwTbl=NULL;
	SWITCH_ROUTE_TBL *pSw=NULL;
	UINT16 switchCount = 0;
	UINT8 swEnumCur = 0;
	UINT8 swEnumNext = 1;
	UINT8 swMCMask = 0;
	
	if( pDrvCtrl == NULL )
	{
		printf("!!!error, hlSrio not initialize!\n");
		return;
	}
	
	pHostAllSwTbl=pDrvCtrl->pAllSwTbl;

	if( bSearch == 0 )		
		swMCMask = SRIO_BROADCAST_MASK;		
	else		
		swMCMask = globalMCMask;
		
	if( (pHostAllSwTbl->switchNum) < 2 )
	{				
		pSw=pHostAllSwTbl->hostAllSwInfo[0];
		hlSrioGetPortFromDstID(pSw, group,bSearch);
		printf("Set switch %d\n",swEnumCur);
		value = (group[k]<<16)|(swMCMask);
		SRIO_LOCAL_REG_WRITE(SRIO_MC_ASSOC_SEL,value);
		/*printf("0x84--0x%x\n",value);*/
		SRIO_LOCAL_REG_WRITE(SRIO_MC_ASSOC_OP,SRIO_MC_ASSOC_ADD_S);
		/*printf("0x88--0x%x\n",SRIO_MC_ASSOC_ADD_S);*/

		k=0;
		while( globalMCPort[k] != 0xFFFF )
		{     					
			if( globalMCPort[k] != 0xff )
			{
				value = (swMCMask<<16)|((globalMCPort[k]<<8)|SRIO_ADD_MC_MASK_PORT);
				SRIO_LOCAL_REG_WRITE(SRIO_MC_MASK_PORT,value);
				/*printf("0x80--add port %d\--val = 0x%x\n",globalMCPort[k],value);*/
			}							
			k++;
		}
	}
	else
	{
		/*当switch的个数大于2时,根据枚举好的顺序去除环形结构*/
		while(switchCount<(pHostAllSwTbl->switchNum))
		{
			if( ((pHostAllSwTbl->switchInterConnectionInfo[swEnumCur][swEnumNext].distance) == 1))
			{
				switchCount++;
				portNum = (pHostAllSwTbl->switchInterConnectionInfo[swEnumCur][swEnumNext].portPtr[0]);
				portBrotherNum = (pHostAllSwTbl->switchInterConnectionInfo[swEnumCur][swEnumNext].portBrotherPtr[0]);
				
				if( swEnumCur == 0 )
				{	
					if( swEnumNext == 1 )
					{
						switchCount++;
						/*printf("Set switch %d\n",swEnumCur);*/
						value = (group[k]<<16)|(swMCMask);
						SRIO_LOCAL_REG_WRITE(SRIO_MC_ASSOC_SEL,value);
						/*printf("0x84--0x%x\n",value);*/
						SRIO_LOCAL_REG_WRITE(SRIO_MC_ASSOC_OP,SRIO_MC_ASSOC_ADD_S);
						/*printf("0x88--0x%x\n",SRIO_MC_ASSOC_ADD_S);*/
						value = (swMCMask<<16)|((portNum<<8)|SRIO_ADD_MC_MASK_PORT);
						SRIO_LOCAL_REG_WRITE(SRIO_MC_MASK_PORT,value);
						/*printf("0x80--add port %d\--val = 0x%x\n",portNum,value);*/

						pSw=pHostAllSwTbl->hostAllSwInfo[swEnumCur];
						tgtId = swEnumCur+RIO_SW_FIRST_VIRTUAL_TGTID;
						hlSrioGetPortFromDstID(pSw, group,bSearch);
						
						k=0;
						while( globalMCPort[k] != 0xFFFF )
						{     					
							if( globalMCPort[k] != 0xff )
							{
								value = (swMCMask<<16)|((globalMCPort[k]<<8)|SRIO_ADD_MC_MASK_PORT);
								SRIO_LOCAL_REG_WRITE(SRIO_MC_MASK_PORT,value);
								/*printf("0x80--add port %d\--val = 0x%x\n",globalMCPort[k],value);*/
							}							
							k++;
						}
					}
					else
					{
						/*printf("Set switch %d\n",swEnumCur);*/
						value = (swMCMask<<16)|((portNum<<8)|SRIO_ADD_MC_MASK_PORT);
						SRIO_LOCAL_REG_WRITE(SRIO_MC_MASK_PORT,value);
						/*printf("0x80--add port %d\--val = 0x%x\n",portNum,value);*/
					}
				}
				else
				{
					hopcount=(UINT16)pHostAllSwTbl->switchInterConnectionInfo[0][swEnumCur].distance; 	
					tgtId = swEnumCur+RIO_SW_FIRST_VIRTUAL_TGTID;
					/*printf("Set switch %d, hopcount = %d\n",swEnumCur,hopcount);*/
					value = (swMCMask<<16)|((portNum<<8)|SRIO_ADD_MC_MASK_PORT);
					hlMaintWrite(tgtId, hopcount, SRIO_MC_MASK_PORT,value);
					/*printf("0x80--add port %d\--val = 0x%x\n",portNum,value);*/
				}

				/*设置swEnumNext的端口*/				
				hopcount=(UINT16)pHostAllSwTbl->switchInterConnectionInfo[0][swEnumNext].distance; 
				pSw=pHostAllSwTbl->hostAllSwInfo[swEnumNext];
				tgtId = swEnumNext+RIO_SW_FIRST_VIRTUAL_TGTID;
				k=0;
				/*printf("Set connected switch %d, hopcount = %d\n",swEnumNext,hopcount);*/
				value = (group[k]<<16)|(swMCMask);
				hlMaintWrite(tgtId, hopcount, SRIO_MC_ASSOC_SEL,value);
				/*printf("0x84--0x%x\n",value);*/
				hlMaintWrite(tgtId, hopcount, SRIO_MC_ASSOC_OP,SRIO_MC_ASSOC_ADD_S);
				/*printf("0x88--0x%x\n",SRIO_MC_ASSOC_ADD_S);*/
				value = (swMCMask<<16)|((portBrotherNum<<8)|SRIO_ADD_MC_MASK_PORT);
				hlMaintWrite(tgtId, hopcount, SRIO_MC_MASK_PORT,value);
				/*printf("0x80--add brother port %d\--val = 0x%x\n",portBrotherNum,value);*/

				hlSrioGetPortFromDstID(pSw, group,bSearch);
				k=0;
				while( globalMCPort[k] != 0xFFFF )
				{     					
					if( globalMCPort[k] != 0xff )
					{
						value = (swMCMask<<16)|((globalMCPort[k]<<8)|SRIO_ADD_MC_MASK_PORT);
						hlMaintWrite(tgtId, hopcount, SRIO_MC_MASK_PORT,value);
						/*printf("0x80--add port %d\--val = 0x%x\n",globalMCPort[k],value);*/
					}							
					k++;
				}
				
				swEnumCur = swEnumNext;
				swEnumNext++;								
			}
			else
			{
				/*printf("switch %d--and switch %d\ not connected!\n",swEnumCur,swEnumNext);*/
				swEnumCur--;				
			}		
		}		
	}			        

	SRIO_LOCAL_REG_READ(0);
}


/********************************************************************************
*       hlSrioSetGroup		Set the switch multicast, 
*						
*						group id={0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7}
*	example:
unsigned short entry0[]={0xf1,7,8,12,0xffff};
void hlgroup()
{
	hlSrioSetGroup(entry0);	
}
********************************************************************************/
unsigned short entry0[]={0xf1,7,8,0xffff};
void hlSrioGroup()
{
	int i=0;
	for(i=0; i<globalSrioGroupNum; i++ )
	{
		hlSrioSetGroup(globalSrioGroup[i]);
	}
}

void hlSrioSetGroup(unsigned short group[])
{

	if( 	globalMCMask >=SRIO_MC_GROUP_NUM )
	{
		printf("!!!error::group numbers too many, exceed max num %d.\n",SRIO_MC_GROUP_NUM);
		return;
	}

	if( group[0] < 0xf1 || group[0] > 0xf7 )
	{
		printf("!!!error::group id not correct.\n");
		return;
	}
	
	DB(printf("Conifg the switch group table[%d] start!\n",globalMCMask));
	
	hlSrioSetMutiCast(group,1);						        

	DB(printf("Conifg the switch group table[%d] end!\n",globalMCMask));
	
	globalMCMask++;
	
}

void hlSrioSetBroadCast()
{
	UINT16 group[2];
	group[0] = SRIO_BROADCAST_ID;

	DB(printf("Conifg the switch broadcast start!\n"));
	hlSrioSetMutiCast(group,0);
	DB(printf("Conifg the switch broadcast end!\n"));	
}

void hlSrioSetAllSWDefaultPort()
{
	int tgtId=0xff;
	UINT16 hopcount=0;
	hostAllSwTbl *pHostAllSwTbl=NULL;
	int _swIndex;
	
	if( pDrvCtrl == NULL )
		return;
					
	pHostAllSwTbl=pDrvCtrl->pAllSwTbl;
	if(pHostAllSwTbl==NULL)
	{
		printf("SRIO NET TOPO not initialized!\n");
		return ;
	}

	for( _swIndex=0; _swIndex<pHostAllSwTbl->switchNum; _swIndex++)
	{
		tgtId = _swIndex+RIO_SW_FIRST_VIRTUAL_TGTID;
		hopcount=(UINT16)pHostAllSwTbl->switchInterConnectionInfo[0][_swIndex].distance;
		hlMaintWrite(tgtId,hopcount,SRIO_ROUTE_DEFAULT_PORT,0xff);
	}	
	
}

