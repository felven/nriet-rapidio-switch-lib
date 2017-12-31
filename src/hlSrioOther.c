#include "hlSrioConfig.h"

/***********************************************
		This file define not important function!
***********************************************/


/********************************************************************
*       hlSrioSwitchIDSet   set the switch virtual id in the component tag register.
********************************************************************/
HL_STATUS hlSrioSwitchIDSet(UINT32 hostTargetID,UINT32 swVirtID,UINT16 hopCount)
{
	UINT32 tempVal=0;
	UINT16 truehopcount =0;
	UINT32 tgtDefaultID = 0xff;
	UINT32 mtRdata=0;
	if(hopCount>0)
    		truehopcount = hopCount-1;

	if(hopCount == 0)
	{
		SRIO_LOCAL_REG_BIT_CLEAR(SRIO_COMP_TAG, RIO_SW_ID_MASK);
		SRIO_LOCAL_REG_WRITE(SRIO_PGCCSR_OFFSET, ((SRIO_LOCAL_REG_READ(SRIO_PGCCSR_OFFSET))|(PGCCSR_DISCOVERED)));
	}
	else
	{
		if (HL_OK == hlMaintRead(tgtDefaultID, truehopcount, SRIO_COMP_TAG, &mtRdata))        
        		tempVal = mtRdata;            
		else        
			hlMtRdFailReport (tgtDefaultID, truehopcount, SRIO_COMP_TAG, NULL);
                            
		tempVal &=RIO_SW_ID_MASK;
		tempVal|=swVirtID;
		hlMaintWrite(tgtDefaultID, truehopcount, SRIO_COMP_TAG, tempVal);
	}
	return HL_OK;
}



HL_STATUS hlMtRdFailReport(int dstId, UINT16 hopcount, int offset, hostAllSwTbl *pHostAllSwTbl)
{
	printf("maintRead from 0x%08x with hopcount %d reg 0x%04x FAIL!\n", dstId, hopcount, offset);

	if (NULL != pHostAllSwTbl)
	{
		free(pHostAllSwTbl);
		pHostAllSwTbl = NULL;
		pDrvCtrl->pAllSwTbl = NULL;
   	}
   
	return HL_MAINTREAD_FAIL;
}

/*******************************************************************
* RioConfigUnlock -unlock the device
* before unlock devices, the host must cfg a maintenance window for the operation.
*******************************************************************/
HL_STATUS hlSrioConfigUnlock(
    UINT32 hostTargetID,
    UINT32 dstID,
    UINT16 hopCount)
{
	UINT16 truehopCount = 0;
	if(hopCount > 0)
		truehopCount = hopCount-1;

	if(hopCount == 0)
		SRIO_LOCAL_REG_WRITE(SRIO_HBDIDLCSR_OFFSET, hostTargetID);
	else
		hlMaintWrite(dstID, truehopCount, SRIO_HBDIDLCSR_OFFSET, hostTargetID);
	
	return HL_OK;    
}

/***********************************
RioConfigTargetId - config device id 
***********************************/
HL_STATUS hlSrioConfigTargetId(
	UINT16 hopcount,
	UINT32 hostTargetID,
	UINT32 dstId,
	UINT32 targetId)
{
	UINT32 tempVal=0;  
	UINT16 truehopcount=0;
	UINT32 mtRdata=0;
	if(hopcount>1)
		truehopcount = hopcount-1;

	if(hopcount == 0)
		tempVal = SRIO_LOCAL_REG_READ(SRIO_BDIDCSR_OFFSET);
	else
	{
		if (HL_OK == hlMaintRead(dstId,truehopcount, SRIO_BDIDCSR_OFFSET, &mtRdata))        
			tempVal = mtRdata;
		else
			hlMtRdFailReport (dstId, truehopcount, SRIO_BDIDCSR_OFFSET, NULL);
	}
            
	if(tempVal==(0xfe << 16))
	{
		printf("skip boot device!\n");
		return HL_OK;
	}

	tempVal = targetId << 16;
	if(hopcount == 0)
		SRIO_LOCAL_REG_WRITE(SRIO_BDIDCSR_OFFSET, tempVal);
	else
		hlMaintWrite(dstId,truehopcount, SRIO_BDIDCSR_OFFSET, tempVal);
        
	if(hopcount == 0)
		tempVal = SRIO_LOCAL_REG_READ(SRIO_BDIDCSR_OFFSET);
	else
	{
		if (HL_OK == hlMaintRead(dstId,truehopcount, SRIO_BDIDCSR_OFFSET, &mtRdata))
			tempVal = mtRdata;
		else
			hlMtRdFailReport (dstId, truehopcount, SRIO_BDIDCSR_OFFSET, NULL);
	}
            
	if(tempVal==( targetId << 16))
		return HL_OK;
	else
		return HL_ERROR;
} 

/****************************************************************************
	RioConfigSetDiscover - clear  the device's discover bit
*****************************************************************************/
HL_STATUS hlSrioConfigClrDiscover(  
	UINT32 hostTargetID,
	UINT32 dstID,
	UINT16 hopCount)
{
	UINT32 tempVal=0;
	UINT16 truehopcount = 0;
	UINT32 mtRdata=0;
	if(hopCount>0)
		truehopcount = hopCount-1;
     
	if(hopCount == 0)
		tempVal = SRIO_LOCAL_REG_BIT_CLEAR(SRIO_PGCCSR_OFFSET,PGCCSR_DISCOVERED);
	else
	{
		if (HL_OK == hlMaintRead(dstID, truehopcount, SRIO_PGCCSR_OFFSET, &mtRdata))
			tempVal = mtRdata;
		else        
			hlMtRdFailReport (dstID, truehopcount, SRIO_PGCCSR_OFFSET, NULL);
                
		tempVal&=(~PGCCSR_DISCOVERED);
		hlMaintWrite(dstID, truehopcount, SRIO_PGCCSR_OFFSET,tempVal);
	}

	return HL_OK;
} 

HL_STATUS hlSrioRemoteConfigWaitConcede(
	UINT32 hostTargetID,
	UINT16 hopcount)
{
    long returnValue;
    /*int i=10;   
    taskDelay(i);*/
    while((returnValue=hlSrioRemoteConfigLock(hostTargetID,hopcount))!=HL_LOCK)
    {
        if(returnValue==HL_CONCEDE)        
            return returnValue;		
        
         DB(printf("wait for others conceding!\n"));
         /*taskDelay(i*10);*/
    }
    return HL_OK;
}

HL_STATUS hlSrioHostConfigWaitConcede(UINT32 hostTargetID)
{
    HL_STATUS returnValue;
    /*int i=10;   
    taskDelay(i);*/
    while((returnValue=hlSrioHostConfigLock(hostTargetID))!=HL_LOCK)
    {
        if(returnValue==HL_CONCEDE)
        {
            return returnValue;
         }
        
         DB(printf("lock host,wait for others conceding!\n"));
         /*taskDelay(i*10);*/
    }
    return HL_OK;
}

HL_STATUS hlSrioRemoteConfigLockDrect(UINT32 hostTargetID,UINT16 hopcount,UINT32 dstId)
{
	UINT32 tempVal=0;
	UINT32 mtRdata=0;
	if (HL_OK == hlMaintRead(dstId,hopcount, SRIO_HBDIDLCSR_OFFSET, &mtRdata))    
		tempVal = mtRdata;        
	else
		hlMtRdFailReport (dstId, hopcount, SRIO_HBDIDLCSR_OFFSET, NULL);
        
        
    if(tempVal==hostTargetID)    
        return HL_ALREADY_LOCK;

    hlMaintWrite(dstId, hopcount, SRIO_HBDIDLCSR_OFFSET, hostTargetID);      
        
    if(HL_OK == hlMaintRead(dstId,hopcount, SRIO_HBDIDLCSR_OFFSET, &mtRdata))    
    	tempVal = mtRdata&(0x0000FFFF);       
    else       
        hlMtRdFailReport (dstId, hopcount, SRIO_HBDIDLCSR_OFFSET, NULL);
                     
    if(tempVal>hostTargetID)    
    	return HL_CONCEDE;         
    else if(tempVal<hostTargetID)     
        return HL_WAIT_OTHER_CONCEDE;    

    return HL_LOCK;
}

int* hlSrioEndIDGet(void *msgLenAddr, void *sysPointNumAddr)
{
	int sysPointNum = pDrvCtrl->pAllSwTbl->srioDevNum;
	int *sysEndID = NULL, *sysIDArrayHead = NULL;
	int i, msgLen, idCnt = 0;
	SWITCH_ROUTE_TBL *pSw = pDrvCtrl->pAllSwTbl->hostAllSwInfo[0];
		
	if (!sysPointNum) 
		msgLen = 0;
	else
	{
		msgLen = ((sysPointNum)%SRIO_MSG_PAYLOAD_MAX_SIZE)?  \
				((sysPointNum)/SRIO_MSG_PAYLOAD_MAX_SIZE):  \
				(((sysPointNum)/SRIO_MSG_PAYLOAD_MAX_SIZE)-1);
	}
		
	*(int*)msgLenAddr = msgLen;						
	sysEndID = (int*)malloc(sysPointNum*(sizeof(int)));
	if(sysEndID == NULL)
	{
		printf("hlSrioEndIDGet:sysEndID malloc fail.\n");
		return NULL;
	}
	sysIDArrayHead = sysEndID;
	for (i=0; i<MAX_NUMBER_RIO_TARGETS; i++)
	{
		if ((pSw->hostCfgRouteTbl[i].deviceStyle == SRIO_ENDPOINT ||	\
				pSw->hostCfgRouteTbl[i].deviceStyle == SRIO_PROC) && \
				(pSw->hostCfgRouteTbl[i].nodeIndex != globalSysHostID) && \
				(pSw->hostCfgRouteTbl[i].distance > 0))
		{
			*(int*)sysEndID = pSw->hostCfgRouteTbl[i].nodeIndex;
			/*DB(printf("sysEndID%d:%d\n",i,*(int*)sysEndID));*/
			sysEndID++;
			idCnt++;
		}
	}
	DB(printf("sysEndpoint %d  idCnt%d\n", sysPointNum, idCnt));
	if (idCnt != (sysPointNum - 1))
	{
		printf ("something wrong happened in hostAllSwTbl\n");
		return NULL;
	}
	*(int*)sysPointNumAddr = idCnt;
	if(idCnt == 0)
	{
		free(sysEndID);
		return NULL;
	}
	return sysIDArrayHead;
}
	
	/*hlSrioLoadAssign-user assign the load*/
	HL_STATUS hlSrioLoadAssign(UINT32 sorId,UINT32 dstId,UINT32 Count,UINT32 *switchIndex)
	{
		int i=0;
		int port,switchId;
		int dstport = 0;
		int sorport = 0;
		UINT32 distance;
		UINT16 hopcount;
		UINT32 tempport = 0xff;
		hostAllSwTbl *pSwTbl = pDrvCtrl->pAllSwTbl;
		SWITCH_ROUTE_TBL *pSwInfo = NULL;
		SWITCH_INFO pSwInterConnectInfo;
			
		/*adjust whether the assigned load is available*/
		/*step 1:whether the sorId is connected to the switchIndex0 directly*/
		pSwInfo = pSwTbl->hostAllSwInfo[*(UINT32 *)switchIndex];
		for(i=0;i<MAX_NUMBER_RIO_TARGETS;i++)
		{
			if((pSwInfo->hostCfgRouteTbl[i].nodeIndex==sorId)&&(pSwInfo->hostCfgRouteTbl[i].distance==1))
			{
				sorport = pSwInfo->hostCfgRouteTbl[i].portNum;
				break;
			}
			else
				continue;
		}
		if(i==(MAX_NUMBER_RIO_TARGETS-1))
		{
			printf("the assigned load is not available\n");
			printf("the sorId %d is not connected directly to the switchIndex %d\n",sorId,*(UINT32 *)switchIndex);
			return HL_ERROR;
		}
		/*step 2:whether the inter switch is connected directly*/
		for(i=0;i<(Count-1);i++)
		{
			pSwInterConnectInfo = pSwTbl->switchInterConnectionInfo[*(UINT32 *)(switchIndex+i)][*(UINT32 *)(switchIndex+i+1)];
			if(pSwInterConnectInfo.distance==1)
				continue;
			else
			{
				printf("the assigned load is not available\n");
				printf("the link between switchIndex %d and switchIndex %d is not exist.\n",*(UINT32 *)switchIndex,*(UINT32 *)(switchIndex+1));
				return HL_ERROR;
			}
		}
		/*step 3:whether the dstId is connected to the switchIndex0 directly*/
		pSwInfo = pSwTbl->hostAllSwInfo[*(UINT32 *)(switchIndex+Count-1)];
		for(i=0;i<MAX_NUMBER_RIO_TARGETS;i++)
		{
			if((pSwInfo->hostCfgRouteTbl[i].nodeIndex==dstId)&&(pSwInfo->hostCfgRouteTbl[i].distance==1))
			{
				dstport = pSwInfo->hostCfgRouteTbl[i].portNum;
				break;
			}
			else
				continue;
		}
		if(i==(MAX_NUMBER_RIO_TARGETS-1))
		{
			printf("the assigned load is not available\n");
			printf("the dstId %d is not connected directly to the switchIndex %d\n",sorId,*(UINT32 *)(switchIndex+Count-1));
			return HL_ERROR;
		}
		DB(printf("switchIndex0 = %d\n",*(UINT32 *)(switchIndex)));
		DB(printf("switchIndex1 = %d\n",*(UINT32 *)(switchIndex+1)));
		DB(printf("switchIndex2 = %d\n",*(UINT32 *)(switchIndex+2)));
		/*Update the assigned load by user*/
		for(i=0;i<Count;i++)
		{
			if(i<(Count-1))
				DB(printf("switch %d -->",*(UINT32 *)(switchIndex+i)));
			else
				DB(printf("switch %d\n",*(UINT32 *)(switchIndex+i)));
	
			switchId = (*(UINT32 *)(switchIndex+i))+RIO_SW_FIRST_VIRTUAL_TGTID;
			distance = (UINT16)pSwTbl->switchInterConnectionInfo[0][*(UINT32 *)(switchIndex+i)].distance;
			hopcount = distance-1;				  
			if(*(UINT32 *)(switchIndex+i)==0)
			{
				if(i==0)
				{
					/*dstId->sorId*/
					do{
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, sorId); 								   
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, sorport);
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, sorId); 
						tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
					}while(tempport!=sorport);
					/*sorId->dstId*/
					port = hlSrioLoadPortGet(&(pSwTbl->switchInterConnectionInfo[*(UINT32 *)(switchIndex+i)][*(UINT32 *)(switchIndex+i+1)]));
					do{
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, dstId); 								   
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, port);
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, dstId); 
						tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
					}while(tempport!=port);
				}
				else if((i>0)&&(i<(Count-1)))
				{
					/*dstId->sorId*/
					port = hlSrioLoadPortGet(&(pSwTbl->switchInterConnectionInfo[*(UINT32 *)(switchIndex+i)][*(UINT32 *)(switchIndex+i-1)]));
					do{
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, sorId); 								   
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, port);
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, sorId); 
						tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
					}while(tempport!=port); 							   
					/*sorId->dstId*/						
					port = hlSrioLoadPortGet(&(pSwTbl->switchInterConnectionInfo[*(UINT32 *)(switchIndex+i)][*(UINT32 *)(switchIndex+i+1)]));
					do{
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, dstId); 								   
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, port);
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, dstId); 
						tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
					}while(tempport!=port);
				}
				else if(i==(Count-1))
				{
					/*dstId->sorId*/
					port = hlSrioLoadPortGet(&(pSwTbl->switchInterConnectionInfo[*(UINT32 *)(switchIndex+i)][*(UINT32 *)(switchIndex+i-1)]));
					do{
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, sorId); 								   
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, port);
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, sorId); 
						tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
					}while(tempport!=port);
					/*sorId->dstId*/
					do{
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, dstId); 								   
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, dstport);
						SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, dstId); 
						tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
					}while(tempport!=dstport);
				}																						
			}
			else	
			{		
				if(i==0)
				{
					/*dstId->sorId*/
					(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,1,switchId,hopcount,sorport, sorId);
					/*sorId->dstId*/
					port = hlSrioLoadPortGet(&(pSwTbl->switchInterConnectionInfo[*(UINT32 *)(switchIndex+i)][*(UINT32 *)(switchIndex+i+1)]));
					(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,1,switchId,hopcount,port, dstId);
				}
				else if((i>0)&&(i<(Count-1)))
				{
					/*dstId->sorId*/
					port = hlSrioLoadPortGet(&(pSwTbl->switchInterConnectionInfo[*(UINT32 *)(switchIndex+i)][*(UINT32 *)(switchIndex+i-1)]));
					(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,1,switchId,hopcount,port, sorId);
					/*sorId->dstId*/						
					port = hlSrioLoadPortGet(&(pSwTbl->switchInterConnectionInfo[*(UINT32 *)(switchIndex+i)][*(UINT32 *)(switchIndex+i+1)]));
					(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,1,switchId,hopcount,port, dstId);
				}
				else if(i==(Count-1))
				{
					/*dstId->sorId*/
					port = hlSrioLoadPortGet(&(pSwTbl->switchInterConnectionInfo[*(UINT32 *)(switchIndex+i)][*(UINT32 *)(switchIndex+i-1)]));
					(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,1,switchId,hopcount,port, sorId);
					/*sorId->dstId*/
					(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,1,switchId,hopcount,dstport, dstId);
				}
			}
		}
	
		return HL_OK;
}

void hlSrioEasyLdAssign(
	UINT32 sorId,
	UINT32 dstId,
	UINT32 Count,
	UINT32 switchIndex0,
	UINT32 switchIndex1,
	UINT32 switchIndex2)
{
	int i=0;
	UINT32 switchIndex[MAX_NUMBER_RIO_SW];
	while(i<Count)
	{
		if(i==0)
			switchIndex[0]=switchIndex0;			
		else if(i==1)
			switchIndex[1]=switchIndex1;			
		else if(i==2)
			switchIndex[2]=switchIndex2;
					  
		i++;
	}
		
	hlSrioLoadAssign(sorId,dstId,Count,switchIndex);		
}

/*******************************************************************************
*       hlSrioSwitchConcedeRouteAdd   add route information of endpoints connect to switch that
*                                                   the pointer pSwCTree point to local switch.
********************************************************************************/
HL_STATUS hlSrioSwitchConcedeRouteAdd(RIO_DRV_CTRL *pDrvCtrl,
                                                       UINT32 localSwitchIndex,
                                                       UINT32  port,
                                                       UINT16 hopcount,
                                                       UINT32 hostTargetId,
                                                       UINT32 dstId,
                                                       swConcedeTree * pSwCTree)
{
	UINT32 childSwIDOffset,srioDevNum,srioSwNum;
	UINT32 tgtID,portNum,localSwIDOffset;
	UINT16 truehopcount = 0;
	UINT32 tempport = 0xff;
	int i,j;
	swConcedeTree *pChildSwCTree=NULL;
	SWITCH_ROUTE_TBL *pChildSwRTbl=NULL;
	hostAllSwTbl *pHostAllSw=NULL;

	if(hopcount>0)
		truehopcount = hopcount-1;

	pHostAllSw=pDrvCtrl->pAllSwTbl;
	srioSwNum=pSwCTree->childSwitchNum;

	if(srioSwNum==0)
		return HL_OK;

	localSwIDOffset=pHostAllSw->switchIndex[localSwitchIndex];
	for(i=0;i<srioSwNum;i++)
	{
		pChildSwCTree=pSwCTree->pSwConcedeChildTree[i];
		if(pChildSwCTree==NULL)
       		return HL_ERROR;

		childSwIDOffset=pHostAllSw->switchIndex[pChildSwCTree->switchIndex];
		pChildSwRTbl=pHostAllSw->hostAllSwInfo[childSwIDOffset];

		if(pChildSwRTbl==NULL)
			return HL_ERROR;

		/*switches directly connnect to the local switch*/
		if(pHostAllSw->switchInterConnectionInfo[localSwIDOffset][childSwIDOffset].distance==1)                	
			portNum = hlSrioLoadPortGet(&(pHostAllSw->switchInterConnectionInfo[localSwIDOffset][childSwIDOffset]));                                     
		else     /*there exist switches between the local switch and the target switch */        
			portNum=port;

       	srioDevNum=pChildSwRTbl->srioDeviceNum;
		for(j=0;j<srioDevNum;j++)
        	{
        		/*add route information*/
			tgtID=pChildSwRTbl->hostCfgRouteTbl[j].nodeIndex;
			if(hopcount == 0)
			{
				do{
	            			SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, tgtID);	    
		                    SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, portNum);
            			      SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, tgtID);
		                    tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
             			}while(tempport != portNum);
             		}
             		else
                 		(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl,hostTargetId,dstId,truehopcount,portNum, tgtID);
         	}
                
		if(HL_OK!=hlSrioSwitchConcedeRouteAdd(pDrvCtrl, localSwitchIndex,  portNum,hopcount,hostTargetId,dstId,pChildSwCTree))         
         		return HL_ERROR;         
                
	}

    return HL_OK;        
}

/****************************************************************************
*       hlSrioSwitchConcede     concede the srio endpoints directly connect to the switch and 
*                                         add route information which is necessory to visit other far away
*                                         endpoints according to the spanning tree.
*
*****************************************************************************/
HL_STATUS hlSrioSwitchConcede(RIO_DRV_CTRL *pDrvCtrl,
                                         UINT32 parentSwIndex,
                                         UINT32 hostTargetID,
                                         UINT16 hopCount,
                                         swConcedeTree *pSwCTree)
{
	int swIDOffset1;
    UINT32 tgtDefaultID=0xff,tgtID;
    UINT32 localSwitchID,childSwIDOffset;
    UINT32 swIndex;
    UINT16 truehopcount=0;
    UINT32 tempport = 0xff;
    int childHopCount,port,i;
    hostAllSwTbl *pHostAllSwTbl=NULL;
    SWITCH_ROUTE_TBL *pSw=NULL;
    pHostAllSwTbl=pDrvCtrl->pAllSwTbl;

    if(hopCount>0)
        truehopcount = hopCount-1;
       
    if(pSwCTree==NULL)    
        return HL_ERROR;
      

    childHopCount=hopCount+1;       
    swIndex=pSwCTree->switchIndex;
    swIDOffset1=pHostAllSwTbl->switchIndex[swIndex];
    pSw=pHostAllSwTbl->hostAllSwInfo[swIDOffset1];

    if(hopCount == 0)
	    localSwitchID = SRIO_LOCAL_REG_READ(SRIO_COMP_TAG);
    else
        (srioSwitchMethod.swtichControl)(pDrvCtrl,hostTargetID,tgtDefaultID,truehopcount,GET_SWITCH_ID,&(localSwitchID));
        
     localSwitchID-=RIO_SW_FIRST_VIRTUAL_TGTID;
       
     if(localSwitchID!=swIDOffset1)
     {
		printf("wrong switch found %d, but it should be %d  \n",localSwitchID,swIDOffset1);
		return HL_ERROR;  
     }
     /*first add route information to far away endpoint that connect the host and the endpoint as the shortest road*/
     /* Also route path to the host must be added.*/
     if(hopCount==0)
     	port = (0xff)&SRIO_LOCAL_REG_READ(SRIO_SW_PORT_INFO);
     else
        (srioSwitchMethod.swtichControl)(pDrvCtrl,hostTargetID,tgtDefaultID,truehopcount,READ_INITIATOR_PORT,&(port));

	 /*updata the switch LUT, it is the host 8641d   */
     if(hopCount == 0)
     {
     		do{
	        	SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, hostTargetID);	    
               	SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, port);
               	SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, hostTargetID);
               	tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
            }while(tempport != port);
      }
      else
            (srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl, hostTargetID,tgtDefaultID,truehopcount,port, hostTargetID);
       
      hlSrioSwitchConcedeRouteAdd(pDrvCtrl,swIndex,0,hopCount,hostTargetID,tgtDefaultID,pSwCTree) ;

      for(i=0;i<pSwCTree->childSwitchNum;i++)
      {  
      		childSwIDOffset=pSwCTree->pSwConcedeChildTree[i]->switchIndex;
            port = hlSrioLoadPortGet(&(pHostAllSwTbl->switchInterConnectionInfo[localSwitchID][childSwIDOffset]));
            if(hopCount == 0)
            {
            	do{
                	SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, tgtDefaultID);
                  	SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, port);
                  	SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, hostTargetID);
                  	tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
                }while(tempport != port);
            }
            else
            	(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl, hostTargetID,tgtDefaultID,truehopcount,port, tgtDefaultID);

			hlSrioSwitchConcede(pDrvCtrl,swIndex,hostTargetID, childHopCount,pSwCTree->pSwConcedeChildTree[i]);
     	}
       	/* concede all the endpoint */
       	for(i=0;i<pSw->srioDeviceNum;i++)
       	{
        	port=pSw->hostCfgRouteTbl[i].portNum;
            tgtID=pSw->hostCfgRouteTbl[i].nodeIndex;
			
			/*concede the host at last */
            if(tgtID==hostTargetID)                        	
                continue;
               
            if(hopCount>0)
            {
            	(srioSwitchMethod.switchLoadLUTtoAll)(pDrvCtrl, hostTargetID,tgtDefaultID,truehopcount,port, tgtID);
                hlSrioConfigClrDiscover(hostTargetID,tgtID,childHopCount);
                hlSrioConfigUnlock(hostTargetID,tgtID,childHopCount);
                hlSrioConfigTargetId(childHopCount,hostTargetID,tgtID,tgtDefaultID);
             }
       	}
       	/*concede the switch */
       	free(pSw);
       	pSw=NULL;
       	pHostAllSwTbl->switchNum--;
        
       	hlSrioSwitchIDSet(hostTargetID,0,hopCount);
       	hlSrioConfigClrDiscover(hostTargetID,tgtDefaultID,hopCount);
       	hlSrioConfigUnlock(hostTargetID,tgtDefaultID,hopCount);       
       	return HL_OK;        
}

HL_STATUS hlSrioNetConcede(RIO_DRV_CTRL *pDrvCtrl, UINT32 hostTargetId)
{
    UINT32 status;
    int i;
    hostAllSwTbl *pHostAllSwTbl=NULL;
    swConcedeTree *pSwRootTree=NULL;
    int switchTopoTag[MAX_NUMBER_RIO_SW];     /*1 means the switch is inserted to the spaning tree,0 means not*/
    pHostAllSwTbl = pDrvCtrl->pAllSwTbl;
    if(pHostAllSwTbl->switchNum == 0)
    	return HL_ERROR;
        
    memset(switchTopoTag,0,sizeof(int)*MAX_NUMBER_RIO_SW);
    pSwRootTree=(swConcedeTree *)malloc(sizeof(swConcedeTree));
    if(pSwRootTree==NULL)
    {
    	printf("error while malloc mem when conceding the srio net \n");
        return HL_ERROR;
    }
    memset(pSwRootTree,0,sizeof(swConcedeTree));
    switchTopoTag[0]=1;        
    status = hlSrioSwSpanningTreeForm(pSwRootTree, pHostAllSwTbl, \
                                          switchTopoTag, pHostAllSwTbl->switchNum);
    if(status==HL_ERROR)
    {
    	printf("error while execute  hlSrioSwSpanningTreeForm !!!\n");
        return HL_ERROR;
    }

    for(i=0;i<pHostAllSwTbl->switchNum;i++)
    {
    	/*check if all the switches are in the spanning tree */
        if(switchTopoTag[i]!=1)
        {
        	printf(" switch index %d is not in the spanning tree !!!\n",i);
            return HL_ERROR;
        }
    }

    if(HL_OK!=hlSrioSwitchConcede(pDrvCtrl, 0,hostTargetId, 0, pSwRootTree))
    {     
    	printf(" hlSrioSwitchConcede failed !!!\n");
        return HL_ERROR;
    }
       
    return HL_OK;
}

/**********************************************************************************
*  hlM8641dSrioSwitchTblModify      delete the lost switch information in the switch interConnect  tbl.  
**********************************************************************************/
void hlSrioSwitchTblModify(hostAllSwTbl *pHostAllSwTbl,int switchIndex)
{
	int i;
	for(i=0;i<MAX_NUMBER_RIO_SW;i++)
	{
		if(pHostAllSwTbl->switchInterConnectionInfo[switchIndex][i].portPtr!=NULL)
		free(pHostAllSwTbl->switchInterConnectionInfo[switchIndex][i].portPtr);
		if(pHostAllSwTbl->switchInterConnectionInfo[i][switchIndex].portPtr!=NULL)
		free(pHostAllSwTbl->switchInterConnectionInfo[i][switchIndex].portPtr);
		memset(&pHostAllSwTbl->switchInterConnectionInfo[switchIndex][i],1,sizeof(SWITCH_INFO));
		memset(&pHostAllSwTbl->switchInterConnectionInfo[i][switchIndex],1,sizeof(SWITCH_INFO));
	}
	return;
} 

/************************************************************************
*       hlSrioSwitchCheck      check that if there is any switches enumed by host
*                                                 missed, if any switches lost, the route table of each
*                                                 switch must be modified
************************************************************************/
long hlSrioSwitchCheck(hostAllSwTbl *pHostAllSwTbl)
{
	int i,switchMissedCount;
	int swIndex0;
	int switchMissedIndex[MAX_NUMBER_RIO_SW];
	int *switchIndexTbl;
	SWITCH_ROUTE_TBL *pSw=NULL;
	switchMissedCount=0;

	memset(switchMissedIndex,0,sizeof(int)*MAX_NUMBER_RIO_SW);
	switchIndexTbl=(int *)(pHostAllSwTbl->switchIndex);

	for(i=0;i<pHostAllSwTbl->switchNum;i++)
	{
		swIndex0=switchIndexTbl[i];
		/*when globalSwitchStatus[i] is not equal to 1, 
		it means that switch i is not found by the reconfig 
		function, so this switch is considered to be missed  */                  
		if((globalSwitchStatus[swIndex0]&0x1)!=1)
		{
			switchMissedIndex[switchMissedCount]=swIndex0;
			switchMissedCount++;
		}        
	}
	if(switchMissedCount==0)
	{
		/*none of the switches enumed by host lost*/
		return HL_OK;
	}
	/*switch lost,modify the switch route table ,delete the lost switch information */
	for(i=0;i<switchMissedCount;i++)
	{
		swIndex0=switchMissedIndex[i];
		pSw=pHostAllSwTbl->hostAllSwInfo[swIndex0];
		pHostAllSwTbl->srioDevNum-=pSw->srioDeviceNum;
		/*how to deal with srio devices directly connect to the lost switch ?????*/
		free(pSw);
		pSw=NULL;
		hlSrioSwitchTblModify(pHostAllSwTbl,swIndex0);
	}
	pHostAllSwTbl->switchNum-=switchMissedCount;
	return HL_SW_LOST;                
}


