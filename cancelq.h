#ifndef __CANCEL_H__
#define __CANCEL_H__

#include "ntddk.h"


//
//	Device extension for CSQ(Cancel Safe Queue).
//
typedef struct tagDEVICE_EXTENSION
{
	KSPIN_LOCK Lock;
	IO_CSQ CsQueue;
	LIST_ENTRY PendingIrpQueue;

} DEVICE_EXTENTION;


typedef struct tagFILE_CONTEXT
{
	//
	// Lock to rundown threads that are dispatching I/Os on a file handle 
	// while the cleanup for that handle is in progress.
	//
	IO_REMOVE_LOCK FileRundownLock;

} FILE_CONTEXT;

//
//	Insert routine.
//
VOID
CsqInsertIrp(
	PIO_CSQ pCsq,
	PIRP	pIrp
	);

//
//	Remove IRP.
//
VOID
CsqRemoveIrp(
	PIO_CSQ pCsq,
	PIRP	pIrp
	);

//
//	Get next IRP.
//
PIRP
CsqPeekNextIrp(
	PIO_CSQ pCsq,
	PIRP	pIrp,
	PVOID	pPeekContext
	);

//
//	Aquire lock.
//
VOID
CsqAquireLock(
	PIO_CSQ pCsq,
	PKIRQL Irql
	);

//
//	Release already aquired lock.
//
VOID
CsqReleaseLock(
	PIO_CSQ pCsq,
	KIRQL Irql
	);

//
//	Complete canceled IRP.
//
VOID
CsqCompleteCancelIrp(
	PIO_CSQ pCsq,
	PIRP	pIrp
	);


NTSTATUS
DispatchCreateClose(
	PDEVICE_OBJECT DevObject,
	PIRP Irp
	);

NTSTATUS
DispatchDeviceControl(
	PDEVICE_OBJECT DevObject,
	PIRP Irp
	);

VOID
DriverUnload(
	PDRIVER_OBJECT DrvObject
	);


#endif	//	__CANCEL_H__