#ifndef _HLSRIOCONFIG_H
#define _HLSRIOCONFIG_H

#ifdef _cplusplus
extern "C" {
#endif
 
#undef 	SRIO_HR
#define SRIO_ZYNQ

#ifdef SRIO_HR
#include "memLib.h"
#include "fioLib.h"
#include "logLib.h"
#include "taskLib.h"
#include "cacheLib.h"
#endif

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "hlType.h"
#include "hlSrio.h"


#define HLSRIOCONFIG_DEBUG
#define HLSRIO_DEBUG_INTERFACE
/*
*定义是否根据槽位号对srio节点的ID进行绑定
*/
//#define ID_SLOT_BIND

/*!!!如果是由处理板上的srio节点作为主节点进行枚举,处理板必须首先枚举到
**!!!交换板,否则需要修改程序.因此这和机箱的底板Topo结构相关!
*/
/*
*在槽位号绑定的前提下,定义是否由处理板上的srio节点作为主节点进行枚举
*/
#undef ENUM_BY_PROCESS

/*
*在处理板上的srio节点作为主节点的前提下,定义该处理板所连交换芯片1的端口号
*对应的交换芯片1的端口号包括2,4,5,8,9;对应的索引为1,2,3,4,5
*/
#define PROCESS_PORT_INDEX		4

#define SRIO_ZYNQ_BASEADDR			0x40000000
#define SRIO_ZYNQ_NODE_BASEADDR		0x10100
/*status*/
#define HL_OK   0
#define HL_LOCK 0
#define HL_ERROR -1
#define HL_UNLOCK -1
#define HL_CONCEDE -2
#define HL_WAIT_OTHER_CONCEDE -3
#define HL_DISCOVERED -5
#define HL_NOT_DISCOVERED -6
#define HL_LINK_RESET 	-7
#define HL_ALREADY_LOCK  -8
#define HL_SW_LOST -9
#define HL_MAINTREAD_FAIL  -10

#define RIO_SW_ID_MASK 0xff

#define MAX_NUMBER_RIO_SW    18					/*15*/
#define MAX_SRIO_SWITCH_PORT_NUM 19
#define MAX_NUMBER_RIO_TARGETS   220 
#define SWITCH_LOCAL_DEV_NUM  35                /*leave 6 id used to config srio device directly connect to the local swith  - 35 - */
#define RIO_SW_FIRST_VIRTUAL_TGTID 0xa0			/*0xe6*/
#define RIO_SW_GROUP_ID			0xf0

#define RIO_EXPAND_ID_1				0xd0
#define RIO_EXPAND_ID_2				0xc0
/*SRIO*/
#define SRIO_REG_BASE_ADDR 0x88000000

#define SRIO_BDIDCSR_OFFSET 0x60
#define SRIO_HBDIDLCSR_OFFSET 0x68
#define SRIO_COMP_TAG 0x6c
#define SRIO_PGCCSR_OFFSET 0x13c
#define SRIO_ROUTE_CFG_DESTID 0x00070
#define SRIO_ROUTE_CFG_PORT 0x00074
#define SRIO_SW_PORT_INFO 0x00014
#define SRIO_SW_PORT_NUM_MASK 0xff00
#define SRIO_ROUTE_DEFAULT_PORT 0x00078
#define SRIO_PEFCAR_OFFSET 0x0010

#define SRIO_MC_MASK_PORT		0x00080
#define SRIO_MC_ASSOC_SEL		0x00084
#define SRIO_MC_ASSOC_OP		0x00088

#define SRIO_ADD_MC_MASK_PORT	0x10
#define SRIO_DEL_MC_MASK_PORT	0x20
#define SRIO_ADD_ALL_MC_MASK_PORT	0x50

#define SRIO_BROADCAST_ID		0xf8
#define SRIO_BROADCAST_MASK	7

/*The association is for a small transport destination IDs*/
#define SRIO_MC_ASSOC_ADD_S		0x60
/*The association is for a large transport destination IDs*/
#define SRIO_MC_ASSOC_ADD_L		0xE0

#define SRIO_MC_GROUP_NUM		7
#define SRIO_MC_GROUP_PORT_NUM	20

#define SRIO_PORTX_STATUS_OFFSET(portNum)  (0x158+portNum*0x20)
#define SRIO_LINK_REQUEST_OFFSET(portNum)  (0x140+portNum*0x20)
#define SRIO_LINK_RESPONSE_OFFSET(portNum)  (0x144+portNum*0x20)
#define SRIO_ACKID_STATUS_OFFSET(portNum)  (0x148+portNum*0x20)
#define SRIO_PORT_SELFLOCK_OFFSET(portNum)  (0x15c+portNum*0x20)

#define PGCCSR_DISCOVERED 0x20000000						
#define PORT_OK  0x00000002
#define PORT_ERR  0x00000004
#define PORT_ERROR1  0x306
#define PORT_ERROR2  0x206
#define PORT_ERROR3  0x106

#define PORT_ERROR4  0x20202
#define PORT_ERROR5  0x202

#define PORT_ERR_CLR	0x07120214

/*FPGA的基地址*/
#define FPGA_BASE_ADDR		0x90000e00

/*	读写SRIO节点配置寄存器基地址*/
#define FPGA_SRIO_NODE_BASEADDR		0x22000000

/*	读写交换芯片配置寄存器基地址*/
#define FPGA_SWITCH_BASEADDR			0x22800000

/*	读写交换芯片24位寄存器时的配置寄存器地址*/
#define FPGA_SWITCH_24OFFSET_ADDR			0x23000560

#ifdef SRIO_HR
#define HOST_PORT                                       14
#endif

#ifdef SRIO_ZYNQ
#define HOST_PORT                                       9
#endif


/*SRIO做主的功能寄存器*/
#ifdef SRIO_HR
#define SRIO_NODE_MASTERENABLE		0x138
#endif

#ifdef SRIO_ZYNQ
#define SRIO_NODE_MASTERENABLE		0x13c
#endif

/*当前的zynq上给我使用的hopcount最多是4位,所以最大是63,在hlMainRead和hlMainWrite函数中
*  会将hopcount+1,和华睿板有区别,所以这里最大定义为62
*/
#define SRIO_ZYNQ_MAX_HOPCOUNT		62

/*SRIO的ID*/
#define SRIO_NODE_DEST_ID                        0x64

/*SRIO的ID节点锁寄存器*/
#define SRIO_NODE_ID_LOCK                       0x6c

#define SRIO_MAINT_DEST_ID				0x23000530
#define SRIO_MAINT_HOPCOUNT			0x23000538
#define SRIO_MAINT_PRIORITY			0x23000540

//zz add for compute more same short way
#define SRIO_EXTEND_PORT_FOR_SHORTWAY		10

#define SRIO_ZYQN_DEST_ID
/*********************************
  5.1
  A------port 14
  B------port 6
  C------port 0
  D------port 12
*********************************/  

/******************************************************************************************************************
下表的含义是，需要先通过配置SRIO_MAINT_DEST_ID 和SRIO_MAINT_HOPCOUNT寄存器来设置
destID 和hopcount 的值，然后再根据读写基地址完成对SRIO节点或交换芯片的操作

												maint_dest_id					maint_hop_count			读写基地址
								
读写本SRIO节点							0xFF							0						0x2200_0000

读写本板交换芯片(TSI578)			0xFF							0						0x2280_0000

读写其余SRIO节点						相应节点device id			0xFF(匹配值)		0x2200_0000

读写其余交换芯片(TSI578)			0xFF	(对应的虚id)		匹配值				0x2280_0000

*******************************************************************************************************************/

#define SRIO_LOCAL_REG_BIT_CLEAR(offset, bit)  \
                                (*(volatile unsigned int *)(SRIO_REG_BASE_ADDR + (UINT32)(offset)) = ((SRIO_LOCAL_REG_READ(offset))&(~(bit))))

#define	DEVICE_ID_1848		0x3740038
#define	DEVICE_ID_578		0x578000d

#define MASTER_ENABLE 0x40000000

#define SRIO_MASTERENABLE		0xE0000000

#define SRIO_MSG_PAYLOAD_MAX_SIZE 62

#define TRANSMIT_RECEIVE_ENABLE 0x00600000

#ifdef HLSRIOCONFIG_DEBUG
#define DB(x) x
#else
#define DB(x)
#endif

#ifndef NULL
#define NULL 0
#endif

typedef union pefcar_desc 
{
    struct
    {
        UINT32 eaf:3;
        UINT32 ef:1;		
        UINT32 ctls:1;        
        UINT32 reserved2:14;
        UINT32 doorbell:1;
        UINT32 mailbox:4;
        UINT32 reserved1:4;
        UINT32 sw:1;
        UINT32 proc:1;
	UINT32 memory:1;
	UINT32 bridge:1;
    } desc;
    UINT32 word;
}PEFCAR_DESC;


/*  zz modify,由于高低字节相反
	UINT32 bridge:1;
      UINT32 memory:1;
      UINT32 proc:1;
      UINT32 sw:1;
      UINT32 reserved1:4;
      UINT32 mailbox:4;
      UINT32 doorbell:1;
      UINT32 reserved2:14;
      UINT32 ctls:1;
      UINT32 ef:1;
      UINT32 eaf:3;
        */


typedef enum _riocmd
{
    READ_INITIATOR_PORT, /*arquire the parents' port*/
    SET_HIERARCHY,
    SET_DEFAULT_PORT,
    SET_SWITCH_ID,
    GET_SWITCH_ID,
    GET_PORT_TO_SW
}RIO_CMD;

typedef enum _srioDeviceStyle
{
        SRIO_PROC=1,  					/*processor, in our system is the 8641d*/
        SRIO_ENDPOINT,					/*endpoint, such as fpga srio device*/
        SRIO_HOST     					/*host of the system*/
}srioDeviceStyle;

typedef struct _switchRouteNodeInfo
{
    UINT32 nodeIndex;
    UINT32 distance; /*the distance is 1 for nodes that are directly connected to the switch
                                  *the distance is 0 for nodes that are infinitely far away*/
    UINT32 portNum;
    srioDeviceStyle deviceStyle;
}SWITCH_ROUTE_NODE_INFO;

typedef struct _switchRouteTbl
{
	SWITCH_ROUTE_NODE_INFO  hostCfgRouteTbl[MAX_NUMBER_RIO_TARGETS];  
	UINT32  srioDeviceNum;             /*the num of srio devices directly connect to the local switch*/ 
	UINT32	remoteSrioDeviceNum;		/*the num of remote srio devices not directly connect to the local switch*/
}SWITCH_ROUTE_TBL;

typedef struct _switchInfo
{
	UINT32 distance;/*1 means connected, 0  means disconnected, n means there are n-1 switchs between*/
	UINT32 totalPortNum;
	UINT32 *portPtr;		/*if switch directly connect to other switch, use this value to config the shortest way*/
	UINT32 *portBrotherPtr;
	UINT32 *shortWayPtr;	/*zz add, if switch not directly connect to other switch,use this value to config the shortest way*/
	UINT32 preport;
	UINT32 preNum;
	UINT32 maxNum;
}SWITCH_INFO;

 /*contain the information of the srio net topo.*/
typedef struct _hostAllSwTbl
{
	SWITCH_ROUTE_TBL *hostAllSwInfo[MAX_NUMBER_RIO_SW];             /* recording the switch info*/
	SWITCH_INFO switchInterConnectionInfo[MAX_NUMBER_RIO_SW][MAX_NUMBER_RIO_SW];     /*recording the specific info of between switch and node.*/
	UINT32 switchIndex[MAX_NUMBER_RIO_SW];     /*hash table point from switchindex to the switch route tbl*/
	UINT32 switchNum;                    /*total numnber of switch in the system*/
	UINT32 procNum;                         /*number of processor in the system*/
	UINT32 srioDevNum;                      /*total num of srio device in the system ,except switch*/
}hostAllSwTbl;

typedef struct _RioDrvCtrl
{
	UINT32      host;
	UINT32      bridgeStatus;   /* status of host bridge */
	UINT32      lawbar;         /* LAWBAR allocated to RapidIO */
	UINT32      lawar;          /* LAWAR allocated to RapidIO */
	int         lawbarIndex;   /* LAWBAR index */ 
	UINT32      deviceSize;
	UINT32      rioBusSize;
	UINT32      pHcfDevice;
	UINT32      tgtIf;
	hostAllSwTbl  *pAllSwTbl;  /*hold the information of the srio network*/            
	UINT32 		portToSw;
	UINT32 		portToSwDeviceId;
}RIO_DRV_CTRL;

typedef struct _switchMethod
{
 	/*load LUT to the all ports*/
    HL_STATUS (*switchLoadLUTtoAll)
    (
    	RIO_DRV_CTRL*pDrvCtrl,
        UINT32 hostTargetID,
        UINT32 dstId,
        UINT16 hopcount,
        UINT32 portNum,
        UINT32 destID
	);

    /*control on switch,
      including:
      setting up port LUT mode,
      reading the initiator port
      .....*/
    HL_STATUS (*swtichControl)
    (
    	RIO_DRV_CTRL*pDrvCtrl,
        UINT32 hostTargetID,
        UINT32 dstId,
        UINT16 hopcount,
        RIO_CMD rioCmd,
        void* pReturnValue   
	);     
}SWITCH_METHOD;

/*used in concede the srio net by the host which have lower priority*/
typedef struct _switchConcedeTree
{
        UINT32 switchIndex;
        UINT32 childSwitchNum;           
        struct _switchConcedeTree *pSwConcedeChildTree[MAX_NUMBER_RIO_SW];
}swConcedeTree;

HL_STATUS hlSrioNetInit(UINT32 hostTargetID);

HL_STATUS hlSrioNetSearch(UINT32 hostTargetID);

HL_STATUS hlMasterEndPoint(UINT32 hostTargetID);

void hlSrioGetInfoFromMainSwitch(
	RIO_DRV_CTRL *pDrvCtrl,
	UINT16  hopCount,
	UINT32 	hostTargetID,
	UINT32  localSwithIndex,
	int 	portCnt);

HL_STATUS hlSrioSwitchEnum(
	RIO_DRV_CTRL *pDrvCtrl,	
	SWITCH_ROUTE_TBL *pSwTbl,
	UINT16  hopCount,
	UINT32 hostTargetID,
	UINT32  localSwithIndex,
	UINT32  parentSwithIndex);

HL_STATUS hlSrioSwitchInfoExchange(hostAllSwTbl *pHostSw);

void hlSrioAddRoute(RIO_DRV_CTRL *pDrvCtrl,	int hostTargetID);

HL_STATUS hlSrioReNetSearch(UINT32 hostTargetID);

HL_STATUS hlSrioReNetInit(UINT32 hostTargetID);

HL_STATUS hlSrioReSwitchEnum(
	RIO_DRV_CTRL *pDrvCtrl,
	SWITCH_ROUTE_TBL *pSwTbl,
	UINT16 hopCount,
	UINT32 hostTargetID,
	UINT32 localSwithIndex,
	UINT32 parentSwithIndex);

void hlSrioReConfigRoute(RIO_DRV_CTRL *pDrvCtrl,int hostTargetID);

long hlSrioSwitchReconfig(
	RIO_DRV_CTRL *pDrvCtrl, 
	UINT16  hopCount,
	UINT32 switchIndex,
	UINT32 parentSwitchIndex);

HL_STATUS hlMasterResetEndpoint(UINT32 hostTargetID);

UINT32 hlSrioGetDeviceId(
	SWITCH_ROUTE_TBL *pSw,
	UINT32 portNum, 
	int *value);

UINT32 hlSrioGetSwitchNum(
	hostAllSwTbl *pHostSw,
	UINT32 portNum,
	UINT32 swNum,
	UINT32* brotherPort);

void hlSrioDeviceEnableSet(UINT32 hostTargetId,UINT32 dstId);

HL_STATUS hlSrioCheckLink(
	UINT32  status,
	int 	port,
	UINT16  truehopcount,
	UINT32  localSwithIndex,
	UINT32  hostTargetID);

void hlSrioResetAckID(	
	int 	port,
	int 	linkpartnerport,
	UINT16  truehopcount,
	UINT32  localSwithIndex);

UINT32 hlSrioLoadPortGet(SWITCH_INFO *switchInterConnectInfo);

void hlSrioGetPortFromDstID(SWITCH_ROUTE_TBL *pSw,unsigned short dstID[],UINT8 bSearch);

void hlSrioSetMutiCast(unsigned short group[],UINT8 bSearch);

void hlSrioGroup();

void hlSrioSetAllSWDefaultPort();

#include "hlSrioOther.h"
#include "hlSrioBase.h"
#include "hlSrioDebug.h"

extern RIO_DRV_CTRL *pDrvCtrl;
extern UINT32 globalSrioNetStatus;
extern UINT32 globalSrioNetConcede;
extern SWITCH_METHOD srioSwitchMethod;
extern UINT32 globalSwitchStatus[MAX_NUMBER_RIO_SW];
extern UINT32 globalSwitchResetNum;
extern UINT32 globalNewSrioDevIndex;
extern UINT32 globalSysHostID;
extern UINT32 globalSysHostID_1;
extern UINT32 globalSysHostID_2;

extern unsigned char globalMCMask;
extern UINT16 globalMCPort[SRIO_MC_GROUP_PORT_NUM];
extern unsigned short globalSrioGroup[SRIO_MC_GROUP_NUM][SRIO_MC_GROUP_PORT_NUM];
extern unsigned char  globalSrioGroupNum;
extern unsigned char  globalBridge;
extern unsigned char  globalMaxSWPort;
#ifdef _cplusplus
#endif
#endif


