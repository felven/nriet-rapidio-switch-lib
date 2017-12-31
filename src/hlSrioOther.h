#ifndef _HLSRIO_OTHER_H
#define  _HLSRIO_OTHER_H

void hlSrioFreeMemory();

HL_STATUS hlSrioNetConcede(RIO_DRV_CTRL *pDrvCtrl, UINT32 hostTargetId);

HL_STATUS hlSrioRemoteConfigLockCheck(
	UINT32 hostTargetID,
	UINT16 hopcount);

UINT32 hlSrioSwAllInfoGet(SWITCH_ROUTE_TBL *pSwR);

long hlSrioSwitchCheck(hostAllSwTbl *pHostAllSwTbl);

void hlSrioSwitchTblModify(
	hostAllSwTbl *pHostAllSwTbl,
	int switchIndex);

HL_STATUS hlSrioConfigClrDiscover(  
	UINT32 hostTargetID,
	UINT32 dstID,
	UINT16 hopCount);

HL_STATUS hlSrioConfigUnlock(
	UINT32 hostTargetID,
	UINT32 dstID,
	UINT16 hopCount);

HL_STATUS hlSrioConfigTargetId(
	UINT16 hopcount,
	UINT32 hostTargetID,
	UINT32 dstId,
	UINT32 targetId);

HL_STATUS hlSrioSwitchIDSet(
	UINT32 hostTargetID,
	UINT32 swVirtID,
	UINT16 hopCount);

HL_STATUS hlSrioHostConfigWaitConcede(UINT32 hostTargetID);

HL_STATUS hlSrioRemoteConfigWaitConcede(
	UINT32 hostTargetID,
	UINT16 hopcount);

HL_STATUS hlSrioConfigSetDiscover(
	UINT32 hostTargetID,
	UINT32 dstId,
	UINT16 hopcount);

HL_STATUS hlSrioSwitchConcede(
	RIO_DRV_CTRL *pDrvCtrl,
	UINT32 parentSwIndex,
	UINT32 hostTargetID,
	UINT16 hopCount,
	swConcedeTree *pSwCTree);


void hlSrioEasyLdAssign(
	UINT32 sorId,
	UINT32 dstId,
	UINT32 Count,
	UINT32 switchIndex0,
	UINT32 switchIndex1,
	UINT32 switchIndex2);

HL_STATUS hlSrioSwSpanningTreeForm(
	swConcedeTree *pSwCTree,
	hostAllSwTbl *pHostAllSw,
	int *array, 
	int swNum);

HL_STATUS hlSrioLoadAssign(
	UINT32 sorId,
	UINT32 dstId,
	UINT32 Count,
	UINT32 *switchIndex);

#endif

