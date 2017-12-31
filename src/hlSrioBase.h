#ifndef _HLSRIO_BASE_H
#define  _HLSRIO_BASE_H

HL_STATUS hlMaintWrite(	
	UINT32 dstId,
	UINT16 hopcount, 
	UINT32 offset, 
	UINT32 writedata);

HL_STATUS hlMaintRead(
	UINT32 dstId,
	UINT16 hopcount, 
	UINT32 offset, 
	void *mrdataAdr);

HL_STATUS hlMtRdFailReport(
	int dstId, 
	UINT16 hopcount, 
	int offset, 
	hostAllSwTbl *pHostAllSwTbl);

unsigned int sysARD32(unsigned int addrBase,unsigned int addrOffset);

void sysAWR32(unsigned int addrBase,unsigned int addrOffset,unsigned int value);

void M_Enable_Local_Master(UINT32 hostID);

void M_SetHostID(UINT32 hostID);

unsigned int SRIO_LOCAL_REG_READ(UINT32 offset);

void SRIO_LOCAL_REG_WRITE(UINT32 offset, UINT32 writedata);

HL_STATUS hlSrioHostConfigLock(UINT32 hostTargetID);

HL_STATUS hlSrioRemoteConfigLock(
	UINT32 hostTargetID,
    UINT16 hopcount);

HL_STATUS hlSrioRemoteConfigLockDrect(
	UINT32 hostTargetID,
	UINT16 hopcount,
	UINT32 dstId);

HL_STATUS hlSwitchSetLUT(
	RIO_DRV_CTRL*pDrvCtrl,
	UINT32 hostTargetID,
	UINT32 dstId,
	UINT16 hopcount,
	UINT32 portNum,
    UINT32 destID);

HL_STATUS hlSwtichControl(
	RIO_DRV_CTRL*pDrvCtrl,
	UINT32 hostTargetID,
	UINT32 dstId,
	UINT16 hopcount,
	RIO_CMD rioCmd,
	void* pReturnValue);


#endif

