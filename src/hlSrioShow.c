#include "hlSrioConfig.h"

void hlSrioDevIDShow()
{
	int j,k,srioDevId;
	SWITCH_ROUTE_TBL *pSw=NULL;
	if( pDrvCtrl->pAllSwTbl == NULL )
		return;

	printf("-----------SRIO Device Id Show----------\n");
	printf("total srio device num=%d \n",pDrvCtrl->pAllSwTbl->srioDevNum);
	for(j=0;j<pDrvCtrl->pAllSwTbl->switchNum;j++)
    {	
		pSw = pDrvCtrl->pAllSwTbl->hostAllSwInfo[j];
		if( pSw == NULL )
			continue;
				
		for(k=0;k<SWITCH_LOCAL_DEV_NUM;k++)
      	{              
	    	if(pSw->hostCfgRouteTbl[k].deviceStyle==0)
        		continue;
          
          	srioDevId=pSw->hostCfgRouteTbl[k].nodeIndex;
			printf("srio id = %d\n",srioDevId);
		}
	}
}

/****************************************************************************************************
*       hlSrioNetTopoShow       show the information of the srio network.
*
*****************************************************************************************************/
void hlSrioTopoShow()
{
	int i,j,k;
	int m,temp;
	UINT32 distance;
	hostAllSwTbl *pHostAllSwTbl=NULL;
	SWITCH_ROUTE_TBL *pSw=NULL;				

	if( pDrvCtrl == NULL )
		return;
					
	pHostAllSwTbl=pDrvCtrl->pAllSwTbl;
	if(pHostAllSwTbl==NULL)
	{
		printf("SRIO NET TOPO not initialized!\n");
		return ;
	}
	printf("***************************SRIO NET TOPO  SHOW******************************\n");
	printf("total switch num=%d \n",pHostAllSwTbl->switchNum);
	printf("total srio device num=%d \n",pHostAllSwTbl->srioDevNum);
	printf("total processor num=%d \n",pHostAllSwTbl->procNum);
		
	if(pHostAllSwTbl->switchNum>1)
	{       
		printf("topo between switch directly connect is shown below:\n");
		printf("\n");
		printf("switch0    switch1  portNum(switch0-switch1)\n");
		for(i=0;i<pHostAllSwTbl->switchNum;i++)
		{
			for(j=0;j<pHostAllSwTbl->switchNum;j++)
			{
				if(pHostAllSwTbl->switchInterConnectionInfo[i][j].distance==1)
				{       
					printf("    %d         %d        ",i,j);
					for(m=0;m<pHostAllSwTbl->switchInterConnectionInfo[i][j].totalPortNum;m++)
					{
						printf("%d - %d ",(pHostAllSwTbl->switchInterConnectionInfo[i][j].portPtr[m]),(pHostAllSwTbl->switchInterConnectionInfo[i][j].portBrotherPtr[m]));
					}					
					printf("\n");
				}
			}                
		}
		printf("\n");
		printf("\n");
		/*show the road between srio devices that are not directly connected*/
		for(i=0;i<pHostAllSwTbl->switchNum;i++)
		{                        
			for(j=i+1;j<pHostAllSwTbl->switchNum;j++)
			{                                
				if((distance=pHostAllSwTbl->switchInterConnectionInfo[i][j].distance)>1)
				{
					printf("switch %d ->",i);
					m=i;
					while(distance!=1)
					{
						for(k=0;k<pHostAllSwTbl->switchNum;k++)
						{
							if( pHostAllSwTbl->switchInterConnectionInfo[m][k].portPtr == NULL ||
								pHostAllSwTbl->switchInterConnectionInfo[m][j].portPtr == NULL )
								continue;
							temp = memcmp(pHostAllSwTbl->switchInterConnectionInfo[m][k].portPtr,\
								pHostAllSwTbl->switchInterConnectionInfo[m][j].portPtr,\
								pHostAllSwTbl->switchInterConnectionInfo[m][j].maxNum*sizeof(UINT32));						                                                      
							if((temp==0)&&(1==pHostAllSwTbl->switchInterConnectionInfo[m][k].distance))
							{
								m=k;
								printf("%d -> ",k);
								distance--;
								break;
							}
						}
					}
					printf("%d \n",j);
				}
			}                        
		}                
	}/*more than one swith*/

	printf("\n");
	printf("\n");
	/*show all the srio devices id and interconnect relationship */
	for(i=0;i<pHostAllSwTbl->switchNum;i++)
	{
		pSw=pHostAllSwTbl->hostAllSwInfo[i];
		printf("srio devices directly connect to switch %d   total num=%d :  \n",i,pSw->srioDeviceNum);
		for(j=0;j<pSw->srioDeviceNum;j++)
		{
			printf("ID=%d--0x%x,  port= %d,  ",pSw->hostCfgRouteTbl[j].nodeIndex,\
					pSw->hostCfgRouteTbl[j].nodeIndex,pSw->hostCfgRouteTbl[j].portNum);
			switch(pSw->hostCfgRouteTbl[j].deviceStyle)
			{
				case SRIO_ENDPOINT:
					printf("deviceStyle=SRIO_ENDPOINT \n");
					break;
				case SRIO_PROC:
					printf("deviceStyle=SRIO_PROC \n");
					break;
				case SRIO_HOST:
					printf("deviceStyle=SRIO_HOST \n");
					break;
			}
		}
	}
	printf("****************************************************************************\n");        
}

/*******************************************************************************
*       hlSrioSwInfoGet    return the number of the srio devices  
*
*
*******************************************************************************/
UINT32 hlSrioSwAllInfoGet(SWITCH_ROUTE_TBL *pSwR)
{
	UINT32 num=0;
	int i;
	if(pSwR==NULL)
		return 0;
	for(i=0;i<MAX_NUMBER_RIO_TARGETS;i++)
	{
		if(pSwR->hostCfgRouteTbl[i].distance!=0)
			num++;
	}

	return num;        
}

/*************************************************************************
*       hlSrioGetDeviceId       check whether there is a srio device connect to the target switch
*                                               through the certain port. if find one, return its ID;else, 
*                                               return 0xff which means nothing found.  
*************************************************************************/
UINT32 hlSrioGetDeviceId(SWITCH_ROUTE_TBL *pSw,UINT32 portNum, int *value)
{        
	UINT32 srioDeviceNum=pSw->srioDeviceNum;
	int i;
	for(i=0;i<srioDeviceNum;i++)
	{
		if(portNum==pSw->hostCfgRouteTbl[i].portNum)
		{
			*value=i;
			return(pSw->hostCfgRouteTbl[i].nodeIndex);
		}
	}
	return 0xff;
}

/*************************************************************************
*       hlSrioGetPort       Find the port through the certain deviceID. if find one, return its 
*					port;else return 0xff which means nothing found.    
*					bSearch=0 means broadcast
*					bSearch=1 means multicast
*************************************************************************/
void hlSrioGetPortFromDstID(SWITCH_ROUTE_TBL *pSw,unsigned short dstID[],UINT8 bSearch)
{        
	int i;
	int idx = 0;
	char _bFind = 0;	

	if( bSearch == 0 )
	{
		for(i=0;i<pSw->srioDeviceNum;i++)
		{
			/*printf("hlSrioGetPort find the destID-%d , port-%d in routeTable!\n",pSw->hostCfgRouteTbl[i].nodeIndex,pSw->hostCfgRouteTbl[i].portNum);*/
			globalMCPort[idx]=pSw->hostCfgRouteTbl[i].portNum;			
			idx++;
		}
		globalMCPort[idx]= 0xffff;
		return;
	}

	idx = 1;
	globalMCPort[0]= 0xff;	
	while( dstID[idx] != 0xFFFF )
	{		
		_bFind = 0;
		for(i=0;i<pSw->srioDeviceNum;i++)
		{
			if(dstID[idx]==pSw->hostCfgRouteTbl[i].nodeIndex)
			{
				if( idx >= SRIO_MC_GROUP_PORT_NUM )
				{
					printf("!!!error:: group member are too many,exceed the max num %d\n",SRIO_MC_GROUP_PORT_NUM);
					return;
				}
				printf("hlSrioGetPort find the destID-%d in routeTable!\n",dstID[idx]);
				globalMCPort[idx]=pSw->hostCfgRouteTbl[i].portNum;	
				_bFind = 1;
				idx++;
				break;
			}			
		}

		if( _bFind == 0 )
		{			
			globalMCPort[idx]= 0xff;
			idx++;
		}
		
	}
	
	globalMCPort[idx]= 0xffff;		
	
}

/*************************************************************************
*       hlSrioGetSwitchNum      check whether there is a switch connect to the target switch
*                                         through the certain port. if find one, return its num and the 
*							   brother port;else return 0xff which means nothing found.  
*************************************************************************/
UINT32 hlSrioGetSwitchNum(
	hostAllSwTbl *pHostSw,
	UINT32 portNum,
	UINT32 swNum,
	UINT32* brotherPort)
{        
	int i,j;
	UINT32 swTotalNum=pHostSw->switchNum;
	for(i=0;i<swTotalNum;i++)
	{
		/*check whether it is a switch*/
		if(pHostSw->switchInterConnectionInfo[swNum][i].distance==1)
		{
			for(j=0;j<pHostSw->switchInterConnectionInfo[swNum][i].totalPortNum;j++)
			{
				if(portNum==pHostSw->switchInterConnectionInfo[swNum][i].portPtr[j]) 
				{
					*brotherPort=pHostSw->switchInterConnectionInfo[swNum][i].portBrotherPtr[j];
					return i;
				}
			}
		}
	}
	return 0xff;
}

UINT32 hlSrioLoadPortGet(SWITCH_INFO *switchInterConnectInfo)
{
	SWITCH_INFO *InterConnectInfo = (SWITCH_INFO *)switchInterConnectInfo;
	if(InterConnectInfo->totalPortNum<1)
	{
		printf("error result from no road being found\n");
		return 0xff;
	}
	if((InterConnectInfo->preNum)>=(InterConnectInfo->totalPortNum))
	{
		InterConnectInfo->preNum = 0;
	}

	/*when there are multi loads between switches,loads are assigned in turn*/
	/*printf("preNum = %d,  ",InterConnectInfo->preNum);*/
	if( InterConnectInfo->distance == 1)
		InterConnectInfo->preport = InterConnectInfo->portPtr[InterConnectInfo->preNum];
	else
		InterConnectInfo->preport = InterConnectInfo->shortWayPtr[InterConnectInfo->preNum];
	
	(InterConnectInfo->preNum)++;
	/*printf("preport = %d\n",InterConnectInfo->preport);*/
	return InterConnectInfo->preport; 
}

/****************************************************************************************************
*       hlSrioLUTShow       show the information of the srio lookup table.
*
*****************************************************************************************************/
void hlSrioLUTShow()
{
	int i,j;        
	hostAllSwTbl *pHostAllSwTbl=NULL;
	SWITCH_ROUTE_TBL *pSw=NULL;
                    
	if( pDrvCtrl == NULL )
		return;
                        
	pHostAllSwTbl=pDrvCtrl->pAllSwTbl;
	if(pHostAllSwTbl==NULL)
	{
		printf("SRIO NET TOPO not initialized!\n");
		return ;
	}
	printf("-----------------------SRIO Lookup Table  SHOW---------------------------\n");
	printf("total switch num=%d \n",pHostAllSwTbl->switchNum);
	printf("total srio device num=%d \n",pHostAllSwTbl->srioDevNum);                
	printf("\n\n");
	for(i=0;i<pHostAllSwTbl->switchNum;i++)
	{
		pSw=pHostAllSwTbl->hostAllSwInfo[i];
        
		printf("switch %d lookup Table \n",i);
		printf("  ID             Port    \n");
            
		for(j=0;j<pSw->srioDeviceNum;j++)
		{
			printf("%.3d--0x%.2x         %.2d\n",pSw->hostCfgRouteTbl[j].nodeIndex,\
					pSw->hostCfgRouteTbl[j].nodeIndex,pSw->hostCfgRouteTbl[j].portNum);
		}
		printf("------------------------\n");
		for(j=0;j<pSw->remoteSrioDeviceNum;j++)
		{
			printf("%.3d--0x%.2x         %.2d\n",pSw->hostCfgRouteTbl[j+SWITCH_LOCAL_DEV_NUM].nodeIndex,\
					pSw->hostCfgRouteTbl[j+SWITCH_LOCAL_DEV_NUM].nodeIndex,pSw->hostCfgRouteTbl[j+SWITCH_LOCAL_DEV_NUM].portNum);
		}
		printf("\n");        
	}

	printf("---------------------------------------------------------------------\n");            
}

/*******************************************************************************
*       hlSrioSwSpanningTreeForm    form a spanning tree according to the switchinterconnect table
*                                                switch 0 will be the root.
********************************************************************************/
HL_STATUS hlSrioSwSpanningTreeForm(
	swConcedeTree *pSwCTree,
	hostAllSwTbl *pHostAllSw,
	int *array, 
	int swNum)
{
	int swIndex,i;
    int status;
    swConcedeTree *pSwConTree=NULL;
    swIndex=pSwCTree->switchIndex;
    for(i=0;i<swNum;i++)
    {
    	if(array[i]==1)             /*1 means switch i is in the spanning tree*/
        	continue;

		if(pHostAllSw->switchInterConnectionInfo[swIndex][i].distance==1)
        {
        	pSwConTree=(swConcedeTree *)malloc(sizeof(swConcedeTree));
            if(pSwConTree==NULL)
            	return HL_ERROR;

			memset(pSwConTree,0,sizeof(swConcedeTree));
            pSwConTree->switchIndex=i;
            pSwCTree->pSwConcedeChildTree[pSwCTree->childSwitchNum++]=pSwConTree;
            array[i]=1;
        }
    }
    if(pSwCTree->childSwitchNum>0)
    {
    	for(i=0;i<pSwCTree->childSwitchNum;i++)
        {
        	status=hlSrioSwSpanningTreeForm(pSwCTree->pSwConcedeChildTree[i], pHostAllSw, array, swNum);
            if(status==HL_ERROR)
            	return HL_ERROR;
        }
     }

	 return HL_OK;
}

void hlSrioSpanTree()
{
	hostAllSwTbl *pHostAllSwTbl=NULL;
	swConcedeTree *pSwRootTree=NULL;
	int i;
	int switchTopoTag[MAX_NUMBER_RIO_SW];	  /*1 means the switch is inserted to the spaning tree,0 means not*/

	if( pDrvCtrl == NULL )
		return;

	pHostAllSwTbl = pDrvCtrl->pAllSwTbl;
	if(pHostAllSwTbl->switchNum == 0)
		return;
			
	memset(switchTopoTag,0,sizeof(int)*MAX_NUMBER_RIO_SW);
	pSwRootTree=(swConcedeTree *)malloc(sizeof(swConcedeTree));
	if(pSwRootTree==NULL)
	{
		printf("error while malloc mem when conceding the srio net \n");
		return;
	}
	memset(pSwRootTree,0,sizeof(swConcedeTree));
	switchTopoTag[0]=1; 

	hlSrioSwSpanningTreeForm(pSwRootTree, pHostAllSwTbl, \
                                          switchTopoTag, pHostAllSwTbl->switchNum);

	for(i=0;i<pSwRootTree->childSwitchNum;i++)
	{

	}
}


