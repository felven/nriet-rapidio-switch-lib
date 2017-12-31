#include "hlSrioConfig.h"

#define Host_Port		14

                                     		/*dstID		dstPort     hopcount */
ROUTE_TABLE table[50] = 	{
							{ 2,			0,               	0     },
							{ 1,       		Host_Port,   	0     },									   	
                                       		{ 3,       		6,               	0     },
                                       		{ 4,       		12,			0     },
                                       		{ 5,       		4,             	1     },
                                       		{ 6,       		4,             	1     },
                                       		{ 7,       		4,             	1     },
                                       		{ 8,       		4,             	1     },
                                       		{ 0xFFFF,   	0xFFFF,		0xFFFF}
						};

/*
void appUser()
{
	SetRouteTable(1,table1);
}*/

void SetRouteTable(int hostID,ROUTE_TABLE entry[])
{
    unsigned int tempport;
    int port;
    unsigned int status,mtRdata;
    int masterEn = 0;
    int id=0,mid=0;
    long returnValue=0;
    int idx = 0;
    unsigned int hopCount = 1;
    
    M_Enable_Local_Master(hostID);

    do{
        SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, hostID);	    
        SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, Host_Port);		
        SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, hostID);
        tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
    }while(tempport != Host_Port);	
    
    /*lock the switch direct connect to the host*/
    returnValue=hlSrioHostConfigLock(hostID);
    if(returnValue != HL_LOCK)
    {
        printf("the local host already been locked!\n");
        return;
    }

    /*discover*/
    SRIO_LOCAL_REG_WRITE(SRIO_PGCCSR_OFFSET, ((SRIO_LOCAL_REG_READ(SRIO_PGCCSR_OFFSET))|(PGCCSR_DISCOVERED)));                       
    
    while( entry[idx].dstID != 0xFFFF ) 
    {     
        if( entry[idx].hopCount == 0 )
        {
            if( entry[idx].dstID == hostID )
            {
            	   idx++;
                continue;
            }
		port = entry[idx].portNo;
		status = SRIO_LOCAL_REG_READ(SRIO_PORTX_STATUS_OFFSET(port));
		if((status&PORT_OK)==PORT_OK)
		{			
			do{
				SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, 0xff);                                    
				SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, port);
				SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, 0xff); 
				tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
			}while(tempport!=port);
			/*try to lock*/
			returnValue=hlSrioRemoteConfigLock(hostID,hopCount);

                 	/*enable the master enable bit. */
                    if (HL_OK == hlMaintRead(0xff, hopCount, SRIO_PGCCSR_OFFSET, &mtRdata))			
                        masterEn = mtRdata;                
			
                    masterEn |= MASTER_ENABLE;
			hlMaintWrite(0xff,hopCount,SRIO_PGCCSR_OFFSET,masterEn);	
    
                    hlSrioConfigSetDiscover(hostID, 0xff, hopCount);
                    port = entry[idx].portNo;
                    id=entry[idx].dstID;
                    mid = id<<16;
                    /*set id*/
                    hlMaintWrite(0xff, hopCount, SRIO_BDIDCSR_OFFSET,mid);
                    /*updata switch route table*/
                    do{
                        SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, id);                                    
                        SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, port);
                        SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, id); 
                        tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
                    }while(tempport!=port);                                        
		
                    printf("local id:id= %d    port= %d \n",id,port);
            }
            else
            {
                printf("port%d is error!!!\n",port);
             }
        }
        else
        {
            port = entry[idx].portNo;
            id=entry[idx].dstID;
            do{
                SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, id);	    
                SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_PORT, port);		
                SRIO_LOCAL_REG_WRITE(SRIO_ROUTE_CFG_DESTID, id);
                tempport = SRIO_LOCAL_REG_READ(SRIO_ROUTE_CFG_PORT);
            }while(tempport != port);	                
	     printf("remote id:id= %d    port= %d \n",id,port);
        }                
        idx++; 
    }     

    SRIO_LOCAL_REG_READ(0);
}

