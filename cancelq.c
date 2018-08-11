#include "cancelq.h"
#include "cancelioctl.h"


typedef unsigned int DWORD;

VOID
DriverUnload(
	PDRIVER_OBJECT DrvObject
	)
{
	UNICODE_STRING SymLink;

	RtlInitUnicodeString(&SymLink, CANCELQ_DOS_DEVICE_NAME_W);
	IoDeleteSymbolicLink(&SymLink);

	IoDeleteDevice(DrvObject->DeviceObject);

	return;
}


NTSTATUS
DispatchCreateClose(
	PDEVICE_OBJECT DevObject,
	PIRP Irp
	)
{
	if (NULL == DevObject || NULL == Irp)
	{
		return STATUS_INVALID_PARAMETER;
	}

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


NTSTATUS
DispatchDeviceControl(
	PDEVICE_OBJECT DevObject,
	PIRP Irp
	)
{
	BOOLEAN bIsPending;
	ULONG ulControlCode;
	NTSTATUS Status = STATUS_SUCCESS;
	DEVICE_EXTENTION *pDevExt = NULL;
	PIO_STACK_LOCATION pStackLoc = NULL;

	if (NULL == DevObject || NULL == Irp)
	{
		return STATUS_INVALID_PARAMETER;
	}

	bIsPending = FALSE;
	pStackLoc = IoGetCurrentIrpStackLocation(Irp);


	pDevExt = (DEVICE_EXTENTION *)DevObject->DeviceExtension;
	ulControlCode = pStackLoc->Parameters.DeviceIoControl.IoControlCode;
	switch (ulControlCode)
	{
		case IOCTL_READ_DATA:
		{
			CsqInsertIrp(&pDevExt->CsQueue, Irp);
			Status = STATUS_PENDING;
			bIsPending = TRUE;
		}
		break;
	}

	if (STATUS_PENDING != Status)
	{
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	return Status;
}


NTSTATUS
DriverEntry(
	PDRIVER_OBJECT DriverObj,
	PUNICODE_STRING RegPath
	)
{
	DWORD dwIter;
	UNICODE_STRING DevName;
	UNICODE_STRING SymLinkName;
	UNICODE_STRING SymbolicLinkName;
	PDEVICE_OBJECT DevObject = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	DEVICE_EXTENTION *pDevExt = NULL;

	DriverObj->MajorFunction[IRP_MJ_CREATE] = 
	DriverObj->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
	DriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
	DriverObj->DriverUnload = DriverUnload;

	RtlInitUnicodeString(&DevName, CANCELQ_DEVICE_NAME_W);

	Status = IoCreateDevice(
						DriverObj,
						sizeof(DEVICE_EXTENTION),
						&DevName,
						FILE_DEVICE_UNKNOWN,
						0,
						FALSE,
						&DevObject);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("IoCreateDevice (Status: 0x%x)...FAILED.\n", Status);
		return Status;
	}

	RtlInitUnicodeString(&SymLinkName, CANCELQ_DOS_DEVICE_NAME_W);
	Status = IoCreateSymbolicLink(&SymLinkName, &DevName);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("IoCreateSymbolicLink (Status: 0x%x)...FAILED.\n", Status);

		IoDeleteDevice(DevObject);
		return Status;
	}

	pDevExt = (DEVICE_EXTENTION *)DevObject->DeviceExtension;

	KeInitializeSpinLock(&pDevExt->Lock);

	InitializeListHead(&pDevExt->PendingIrpQueue);

	Status = IoCsqInitialize(
							&pDevExt->CsQueue,
							CsqInsertIrp,
							CsqRemoveIrp,
							CsqPeekNextIrp,
							CsqAquireLock,
							CsqReleaseLock,
							CsqCompleteCancelIrp);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("IoCreateSymbolicLink (Status: 0x%x)...FAILED.\n", Status);

		IoDeleteSymbolicLink(&SymLinkName);
		IoDeleteDevice(DevObject);
		return Status;
	}

	return Status;
}

VOID
CsqInsertIrp(
	PIO_CSQ pCsq,
	PIRP	pIrp
	)
{
	DEVICE_EXTENTION *pDevExt;

	pDevExt = CONTAINING_RECORD(pCsq, DEVICE_EXTENTION, CsQueue);

	InsertTailList(&pDevExt->PendingIrpQueue, &pIrp->Tail.Overlay.ListEntry);
}


//
//	Remove IRP.
//
VOID
CsqRemoveIrp(
	PIO_CSQ pCsq,
	PIRP	pIrp
	)
{
	//UNREFERENCED_PARAMETER(pCsq)

	RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
}


//
//	Get next IRP.
//
PIRP
CsqPeekNextIrp(
	PIO_CSQ pCsq,
	PIRP	pIrp,
	PVOID	pPeekContext
	)
{
	PIRP Irp;
	PLIST_ENTRY listHead;
	PLIST_ENTRY nextList;
	DEVICE_EXTENTION *pDevExt;
	PIO_STACK_LOCATION pStackLoc;

	pDevExt = CONTAINING_RECORD(pCsq, DEVICE_EXTENTION, CsQueue);

	listHead = &pDevExt->PendingIrpQueue;

	if (NULL == pIrp)
	{
		nextList = &listHead->Flink;
	}
	else
	{
		nextList = &pIrp->Tail.Overlay.ListEntry;
	}

	while (nextList != listHead)
	{
		Irp = CONTAINING_RECORD(nextList, IRP, Tail.Overlay.ListEntry);
		pStackLoc = IoGetCurrentIrpStackLocation(Irp);

		if (pPeekContext)
		{
			if ((PFILE_OBJECT)pPeekContext == pStackLoc->FileObject)
			{
				break;
			}
		}
		else
		{
			break;
		}

		Irp = NULL;
		nextList = nextList->Flink;
	}


	return Irp;
}


VOID
CsqAquireLock(
	PIO_CSQ pCsq,
	PKIRQL Irql
	)
{
	DEVICE_EXTENTION *pDevExt = CONTAINING_RECORD(pCsq, DEVICE_EXTENTION, CsQueue);
	KeAcquireSpinLock(&pDevExt->Lock, Irql);
}


VOID
CsqReleaseLock(
	PIO_CSQ pCsq,
	KIRQL Irql
	)
{
	DEVICE_EXTENTION *pDevExt = CONTAINING_RECORD(pCsq, DEVICE_EXTENTION, CsQueue);
	KeReleaseSpinLock(&pDevExt->Lock, Irql);
}


VOID
CsqCompleteCancelIrp(
	PIO_CSQ pCsq,
	PIRP	pIrp
	)
{
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_CANCELLED;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
}
